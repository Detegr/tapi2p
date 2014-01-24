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
			parsePeers(d.data);
			break;
		case Message:
			console.log(peermap);
			console.log(d);
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

function parsePeers(data)
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
	var $chat=$("#chat");
	$(window).on("keypress", function(e)
	{
		if(e.keyCode==13)
		{
			var $chatinput=$("#chat_input");
			$chat.append($chatinput.val() + "\n");
			sendTapi2pCommand(ws, Message, $chatinput.val());
			$chatinput.val("");
		}
	});

	sendTapi2pCommand(ws, ListPeers, null);
};

// Ugly redefine of EventType enum in core/event.h
var ENUM_BASE=0;
var Message=ENUM_BASE++;
var ListPeers=ENUM_BASE++;
var PeerConnected=ENUM_BASE++;
var PeerDisconnected=ENUM_BASE++;
var RequestFileTransfer=ENUM_BASE++;
var FilePart=ENUM_BASE++;

function stringByteCount(str)
{
	return encodeURI(str).split(/%..|./).length-1;
}

function sendTapi2pCommand(ws, cmd, data)
{
	ws.send(JSON.stringify({
		cmd: cmd,
		data: data,
		data_len: data ? stringByteCount(data) : 0
	}));
}
