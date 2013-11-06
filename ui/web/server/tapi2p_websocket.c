#include <string.h>
#include <signal.h>
#include "libwebsockets/lib/libwebsockets.h"
#include "../../../core/core.h"
#include "../../../core/event.h"
#include <jansson.h>
#include <locale.h>
#include "../../../dtgconf/src/config.h"
#include "../../../core/pathmanager.h"

static int welcome_message_sent=0;
static sig_atomic_t run_server=1;
static int corefd=-1;

static sig_atomic_t unsent_data=0;
static char* data_to_send[16];
pthread_mutex_t datalock=PTHREAD_MUTEX_INITIALIZER;

void sighandler(int signal)
{
	if(signal==SIGINT) run_server=0;
}

typedef struct tapi2p_session_data
{
	int dummy;
} tapi2p_session_data;

static void send_json(char* str, size_t len)
{
	json_t* root, *cmd, *data, *data_len;
	json_error_t error;
	root = json_loads(str, 0, &error);

	if(!root || !json_is_object(root))
	{
		fprintf(stderr, "JSON: Error on line %d: %s\n", error.line, error.text);
		return;
	}
	cmd = json_object_get(root, "cmd");
	if(!json_is_integer(cmd))
	{
		fprintf(stderr, "JSON: cmd is not an integer\n");
		json_decref(root);
		return;
	}
	data = json_object_get(root, "data");
	if(!json_is_null(data) && !json_is_string(data))
	{
		fprintf(stderr, "JSON: data is not a string\n");
		json_decref(root);
		return;
	}
	data_len = json_object_get(root, "data_len");
	if(!json_is_integer(cmd))
	{
		fprintf(stderr, "JSON: data_len is not an integer\n");
		json_decref(root);
		return;
	}
	EventType t=(EventType)json_integer_value(cmd);
	int datalen=json_integer_value(data_len);
	printf("Got: %s\n", str);
	event_send_simple(t, datalen ? json_string_value(data) : NULL, datalen, corefd);
	//printf("Cmd: %d\nData: %s\nData_len: %d\n", t, datalen ? json_string_value(data) : "NULL", datalen);
	json_decref(root);
	return;
}


static void websocket_send(struct libwebsocket* wsi, const char* buf, size_t len)
{
	char msg[LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING];
	memcpy(msg+LWS_SEND_BUFFER_PRE_PADDING, buf, len);
	libwebsocket_write(wsi, msg+LWS_SEND_BUFFER_PRE_PADDING, len, LWS_WRITE_TEXT);
}

static int tapi2p_callback(struct libwebsocket_context *ctx,
						   struct libwebsocket *wsi,
						   enum libwebsocket_callback_reasons reason,
						   void* user,
						   void* in,
						   size_t len)
{
	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
			lwsl_notice("Handshaked with client.\n");
			libwebsocket_callback_on_writable(ctx, wsi);
			break;
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			if(corefd==-1)
			{
				char* conn_reply="Could not connect to tapi2p. Is tapi2p core running?";
				websocket_send(wsi, conn_reply, strlen(conn_reply));
			}
			else if(!welcome_message_sent)
			{
				struct config* conf=getconfig();
				if(conf)
				{
					struct configitem* ci=config_find_item(conf, "Nick", "Account");
					if(ci && ci->val)
					{
						char* conn_base="{\"cmd\": 0, \"data\":\"Welcome to tapi2p, ";
						size_t conn_len=strlen(conn_base) + ITEM_MAXLEN;
						char conn_reply[conn_len];
						memset(conn_reply, 0, conn_len);
						stpcpy(stpcpy(stpcpy(conn_reply, conn_base), ci->val), "\"}");
						welcome_message_sent=1;
						lwsl_notice("Sent welcome message.\n");
						websocket_send(wsi, conn_reply, strlen(conn_reply));
					}
				}
			}
			else if(unsent_data>0)
			{
				pthread_mutex_lock(&datalock);
				while(unsent_data)
				{
					char* data=data_to_send[--unsent_data];
					printf("Sent: %s\n", data);
					websocket_send(wsi, data, strlen(data));
					free(data);
				}
				pthread_mutex_unlock(&datalock);
			}
			libwebsocket_callback_on_writable(ctx, wsi);
			break;
		}
		case LWS_CALLBACK_CLOSED:
			lwsl_notice("Connection to client closed.\n");
			break;
		case LWS_CALLBACK_RECEIVE:
			lwsl_notice("Message from client\n");
			printf("Json: %s %d\n", (char*)in, len);
			send_json((char*) in, len);
			break;

	}
	return 0;
}

static struct libwebsocket_protocols protocols[] =
{
	{"tapi2p", tapi2p_callback, sizeof(tapi2p_session_data), 0},
	{0, 0, 0, 0}
};

static json_t* createjsonobject(evt_t* e)
{
	json_t* ret=json_object();
	json_t* cmd=json_integer(e->type);
	json_t* data=json_string(e->data);
	json_t* addr=json_string(e->addr);

	if(!cmd || !json_is_integer(cmd))
	{
		fprintf(stderr, "JSON: cmd is not an integer\n");
		goto err;
	}
	if(!data || !json_is_string(data))
	{
		fprintf(stderr, "JSON: data is not a string\n");
		goto err;
	}
	if(!addr || !json_is_string(addr))
	{
		fprintf(stderr, "JSON: addr is not a string\n");
		goto err;
	}
	json_object_set(ret, "cmd", cmd);
	json_object_set(ret, "data", data);
	json_object_set(ret, "addr", addr);
	return ret;
err:
	json_decref(cmd);
	json_decref(data);
	json_decref(ret);
	return NULL;
}

static void coreeventhandler(evt_t* e, void* data)
{
	json_t* json=createjsonobject(e);
	if(json)
	{
		char* json_str=json_dumps(json, 0);
		if(json_str)
		{
			pthread_mutex_lock(&datalock);
			data_to_send[unsent_data++]=json_str;
			pthread_mutex_unlock(&datalock);
		}
		else
		{
			fprintf(stderr, "Error stringifying JSON\n");
		}
		json_decref(json);
	}
	else
	{
		printf("Failed to create JSON object\n");
	}
}

int main()
{
	if(!setlocale(LC_CTYPE, "en_US.UTF-8"))
	{
		fprintf(stderr, "Cannot set UTF-8 encoding. Please make sure that en_US.UTF-8 encoding is installed.\n");
	}

	memset(data_to_send, 0, 16*sizeof(char*));
	signal(SIGINT, sighandler);

	corefd=core_socket();
	if(corefd == -1)
	{
		fprintf(stderr, "tapi2p core not running!\n");
		return 1;
	}
	event_addlistener(ListPeers, &coreeventhandler, NULL);
	event_addlistener(Message, &coreeventhandler, NULL);
	event_addlistener(PeerConnected, &coreeventhandler, NULL);
	event_addlistener(PeerDisconnected, &coreeventhandler, NULL);
	eventsystem_start(corefd);

	struct libwebsocket_context* ctx;
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	info.port=7681;
	info.protocols=protocols;
	info.gid=-1;
	info.uid=-1;

	ctx = libwebsocket_create_context(&info);
	if(!ctx)
	{
		lwsl_err("libwebsocket init failed.");
		return -1;
	}
	lwsl_notice("tapi2p websocket server started.\n");

	int n=0;
	while(n>=0 && run_server)
	{
		n=libwebsocket_service(ctx, 50);
	}
	eventsystem_stop();
	return 0;
}

