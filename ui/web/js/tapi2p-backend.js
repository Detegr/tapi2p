function Tapi2pBackend(onopen, onerror, onmessage) {

	var ws = new WebSocket("ws://localhost:7681");
	ws.onopen=onopen;
	ws.onerror=onerror;
	ws.onmessage=onmessage;

	/*
	var onConnectionOpen=function(ws,e)
	{
		sendTapi2pCommand(ws, Status);
	};
	*/

	// Ugly redefine of EventType enum in core/event.h
	var ENUM_BASE=0;
	this.Commands={
		"Message":                  ENUM_BASE++,
		"ListPeers":                ENUM_BASE++,
		"PeerConnected":            ENUM_BASE++,
		"PeerDisconnected":         ENUM_BASE++,
		"RequestFileTransfer":      ENUM_BASE++,
		"RequestFileTransferLocal": ENUM_BASE++,
		"RequestFilePart":          ENUM_BASE++,
		"FilePart":                 ENUM_BASE++,
		"Metadata":                 ENUM_BASE++,
		"RequestFileListLocal":     ENUM_BASE++,
		"RequestFileList":          ENUM_BASE++,
		"FileList":                 ENUM_BASE++,
		"AddFile":                  ENUM_BASE++,
		"Setup":                    ENUM_BASE++,
		"Status":                   ENUM_BASE++,
		"RequestFilePartList":      ENUM_BASE++,
		"FilePartList":             ENUM_BASE++,
		"FileTransferStatus":       ENUM_BASE++,
		"GetPublicKey":             ENUM_BASE++,
		"Hello":					-1 // Special for web ui only
	};


	function stringByteCount(str)
	{
		return encodeURI(str).split(/%..|./).length-1;
	}

	this.sendCommand=function(cmd, data, ip, port)
	{
		console.log(JSON.stringify({
			cmd: cmd,
			data: data ? data : null,
			data_len: data ? stringByteCount(data) : 0,
			ip: ip ? ip : null,
			port: port ? port : null
		}));
		if(data && typeof data !== "string") data=JSON.stringify(data);
		ws.send(JSON.stringify({
			cmd: cmd,
			data: data ? data : null,
			data_len: data ? stringByteCount(data) : 0,
			ip: ip ? ip : null,
			port: port ? port : null
		}));
	};

		/*
	var handleMessage=function(ws, e)
	{
		console.log(e.data);
		var d=JSON.parse(e.data);
		console.log(d);
		switch(d.cmd)
		{
			case PeerConnected:
			case PeerDisconnected:
				sendTapi2pCommand(ws, ListPeers);
				break;
			case ListPeers:
				parsePeers(ws, d.data);
				break;
			case FileList:
				filelistactions(ws, d);
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
			case Status:
				if(d.data.status)
				{
					$("#tapi2p_setup").hide();
					$("#tapi2p_running").show();
					sendTapi2pCommand(ws, Hello);
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

					sendTapi2pCommand(ws, ListPeers);
				}
				else
				{
					$("#setupbtn").click(function()
					{
						var data={
							nick: $("#username").val(),
							port: parseInt($("#port").val(), 10)
						};
						sendTapi2pCommand(ws, Setup, JSON.stringify(data));
						sendTapi2pCommand(ws, Status);
					});
				}
				break;
		}
	};
		*/
}
