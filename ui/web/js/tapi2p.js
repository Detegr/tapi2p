$(function()
{
	$chatdiv=$("#chatdiv");
	$peerdiv=$("#peerdiv");
	$chatbutton=$("#chatbtn");
	$peerbutton=$("#peerbtn");
	
	$peerbutton.on("click", function()
	{
		$chatdiv.hide();
		$chatbutton.parent().removeClass("active");
		$peerdiv.show();
		$peerbutton.parent().addClass("active");
		$peerbutton.removeClass("dirty");
		sendTapi2pCommand(ws, ListPeers, null);
		return false;
	});
	$chatbutton.on("click", function()
	{
		$peerdiv.hide();
		$peerbutton.parent().removeClass("active");
		$chatdiv.show();
		$chatbutton.parent().addClass("active");
		return false;
	});
	var $chat=$("#chat");
	if(!window.WebSocket)
	{
		$chat.append("WebSockets not supported, tapi2p cannot function.");
	}
	else
	{
		var ws = new WebSocket("ws://localhost:7681");
		ws.onopen=function(e) {
			onConnectionOpen(ws, e);
		};
		ws.onerror=function(e) {
			$chat.append("Could not connect to websocket.\nIs tapi2p websocket server running?");
		};
		ws.onmessage=function(e) {
			handleMessage(ws, $chat, e);
		};
	}
});

var peermap={
};

var handleMessage=function(ws, $chat, e)
{
	var d=JSON.parse(e.data);
	console.log(d);
	switch(d.cmd)
	{
		case PeerConnected:
		case PeerDisconnected:
			sendTapi2pCommand(ws, ListPeers, null);
			break;
		case ListPeers:
			parsePeers(ws, d.data);
			break;
		case Message:
			console.log(peermap);
			console.log(d);
			if(d.own_nick)
			{
				peermap.localhost={};
				peermap.localhost.nick = d.own_nick;
				d.data += d.own_nick;
			}
			if(peermap[d.addr]) {
				if(peermap[d.addr].nick)
				{
					$chat.append("[" + peermap[d.addr].nick + "] ");
				}
				else
				{
					$chat.append("[" + d.addr + "] ");
				}
			}
			$chat.append(d.data + "\n");
			break;
	}
};

function parsePeers(ws, data)
{
	function td(text)
	{
		return jQuery("<td/>", {
			text: text
		});
	}
	$("#peertable").children().remove();
	if(data)
	{
		var peers=data.peers;
		for(var i=0, len=peers.length; i<len; ++i)
		{
			var nick=peers[i].nick;
			var ip=peers[i].addr;
			var port=peers[i].port;
			var status=peers[i].conn_status;
			var cls="";
			switch(status)
			{
				case "ok": status="OK"; break;
				case "oneway": status="One-way connection"; cls="warning"; break;
				case "invalid": status="Connection could not be established"; cls="error"; break;
			}
			peermap[ip]={nick: nick, status: status};
			var $tr=$("<tr class=" + cls + "/>").append(td(ip)).append(td(nick)).append(td(status));
			$tr.click(function()
			{
				sendTapi2pCommand(ws, RequestFileListLocal, null, ip, port);
			});
			$("#peertable").append($tr);
			$peerbutton=$("#peerbtn");
		}
	}
	if(!$peerbutton.parent().hasClass("active"))
	{
		$peerbutton.addClass("dirty");
	}
}

var onConnectionOpen=function(ws,e)
{
	sendTapi2pCommand(ws, Hello, null);

	var $chat=$("#chat");
	$(window).on("keypress", function(e)
	{
		if(e.keyCode==13)
		{
			var $chatinput=$("#chat_input");
			$chat.append("[" + peermap.localhost.nick + "] " + $chatinput.val() + "\n");
			sendTapi2pCommand(ws, Message, $chatinput.val());
			$chatinput.val("");
		}
	});

	sendTapi2pCommand(ws, ListPeers, null);
};

// Ugly redefine of EventType enum in core/event.h
var ENUM_BASE=0;
var Message                  = ENUM_BASE++;
var ListPeers                = ENUM_BASE++;
var PeerConnected            = ENUM_BASE++;
var PeerDisconnected         = ENUM_BASE++;
var RequestFileTransfer      = ENUM_BASE++;
var RequestFileTransferLocal = ENUM_BASE++;
var RequestFilePart          = ENUM_BASE++;
var FilePart                 = ENUM_BASE++;
var Metadata                 = ENUM_BASE++;
var RequestFileListLocal     = ENUM_BASE++;
var RequestFileList          = ENUM_BASE++;
var FileList                 = ENUM_BASE++;

var Hello = -1; // Special for web ui only

function stringByteCount(str)
{
	return encodeURI(str).split(/%..|./).length-1;
}

function sendTapi2pCommand(ws, cmd, data, ip, port)
{
	ws.send(JSON.stringify({
		cmd: cmd,
		data: data,
		data_len: data ? stringByteCount(data) : 0,
		ip: ip ? ip : null,
		port: port ? port : null
	}));
}
