#include <string.h>
#include <signal.h>
#include "libwebsockets/lib/libwebsockets.h"
#include "../../../core/core.h"
#include "../../../core/event.h"
#include <jansson.h>

static int welcome_message_sent=0;
static sig_atomic_t run_server=1;
static int corefd=-1;

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
	event_send_simple(t, datalen ? json_string_value(data) : NULL, datalen, corefd);
	printf("Cmd: %d\nData: %s\nData_len: %d\n", t, datalen ? json_string_value(data) : "NULL", datalen);
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
			const char* conn_reply;
			if(corefd==-1)
			{
				conn_reply="Could not connect to tapi2p. Is tapi2p core running?";
			}
			else if(!welcome_message_sent)
			{
				conn_reply="Welcome to tapi2p, ";
				welcome_message_sent=1;
				lwsl_notice("Sent welcome message.\n");
			}
			websocket_send(wsi, conn_reply, strlen(conn_reply));
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
	json_t* cmd=json_integer();
	json_t* data=json_string();
	json_t* data_len=json_integer();

	if(!json_set_integer(cmd, e->type))
	{
		fprintf("Failed to set JSON integer %d\n", e->type);
		goto err;
	}
	if(!json_set_data(data, e->data))
	{
		fprintf("Failed to set JSON string %s\n", e->data);
		goto err;
	}
	if(!json_set_data_len(data_len, e->data_len))
	{
		fprintf("Failed to set JSON integer %s\n", e->data_len);
		goto err;
	}
	json_object_set(ret, "cmd", cmd);
	json_object_set(ret, "data", data);
	json_object_set(ret, "data_len", data_len);
	return ret;
err:
	json_decref(cmd);
	json_decref(data);
	json_decref(data_len);
	json_decref(ret);
	return NULL;
}

static void handlelistpeers(evt_t* e, void* data)
{
	json_t json=createjsonobject(e);
	if(json)
	{
		char* json_str=json_dumps(json);
		if(json_str)
		{
			libwebsocket_callback_on_writable(ctx, wsi);
			free(json_str);
		}
		else
		{
			fprintf(stderr, "Error stringifying JSON\n");
		}
		json_decref(json);
	}
}

int main()
{
	signal(SIGINT, sighandler);

	corefd=core_socket();
	event_addlistener(ListPeers, &handlelistpeers, NULL);
	eventsystem_start();

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

