var FileModel = Backbone.Model.extend({
	updateFiles: function(peer_ip, peer_port, files) {
		var obj={};
		obj[peer_ip + ":" + peer_port]=files;
		this.set(obj);
	}
});

var PeerModel = Backbone.Model.extend({
	updatePeers: function(peers) {
		this.set({peers: peers});
	}
});

var ChatModel = Backbone.Model.extend({
	newMessage: function(msg) {
		this.get("chatHistory").push(msg);
		this.trigger("newMessage");
	}
});

var KeyModel = Backbone.Model.extend({});
var keyModel = new KeyModel();

var chatModel = new ChatModel();
chatModel.set({chatHistory: []});

var peerModel = new PeerModel();
peerModel.set({peers: []});

var fileModel = new FileModel();

function RenderableTemplate(name) {
	return Backbone.View.extend({
		el: $("#tapi2p-main"),
		tagName: "div",
		className: name,
		initialize: function() {
			this.render(this.model);
		},
		render: function() {
			var template=_.template($("#" + name + "-template").html(), {});
			this.$el.html(template);
		}
	});
}

var MainView = Backbone.View.extend({
	el: $("#tapi2p-main"),
	tagName: "div",
	model: keyModel,
	className: "tapi2p-main",
	initialize: function() {
		this.model.on("change:welcome", function()
		{
			$("#welcome").text(this.model.get("welcome"));
		}, this);
		this.model.on("change:publicKey", function()
		{
			$("#publickey").text(this.model.get("publicKey"));
		}, this);
		this.render();
	},
	render: function() {
		var template=_.template($("#" + this.className + "-template").html(), {welcome_message: this.model.get("welcome"), publicKey: this.model.get("publicKey")});
		console.log(template);
		this.$el.html(template);
	}
});

$(window).on("keypress", function(e)
{
	if(e.keyCode==13)
	{
		var $chatinput=$("#chatinput");
		if(!$chatinput.val()) return;
		chatModel.newMessage("[" + peermap.localhost.nick + "] " + $chatinput.val());
		backend.sendCommand(backend.Commands.Message, $chatinput.val());
		$chatinput.val("");
	}
});

var ChatView = Backbone.View.extend({
	el: $("#tapi2p-main"),
	model: chatModel,
	tagName: "div",
	className: "tapi2p-chat",
	initialize: function() {
		this.model.on("newMessage", function(p)
		{// Not exactly sure why this is needed here as well.
			if(this.$el.is(":visible")) this.render(p);
		}, this);
		this.render();
	},
	render: function() {
		if(!this.$el.is(":visible")) return this;

		var template=_.template($("#" + this.className + "-template").html(), {welcome_message: keyModel.get("welcome"), chatHistory: this.model.get("chatHistory")});
		this.$el.html(template);
	}
});

var PeerView = Backbone.View.extend({
	el: $("#tapi2p-main"),
	model: peerModel,
	tagName: "div",
	className: "tapi2p-peers",
	initialize: function() {
		this.model.on("change:peers", this.render, this);
		this.render();
	},
	render: function() {
		if(!this.$el.is(":visible")) return this;

		var template=_.template($("#" + this.className + "-template").html(), {peerList: this.model.get("peers")});
		this.$el.html(template);
	}
});

var NewPeerView = Backbone.View.extend({
	el: $("#tapi2p-main"),
	tagName: "div",
	className: "tapi2p-newpeer",
	initialize: function() {
		this.render();
	},
	render: function() {
		if(!this.$el.is(":visible")) return this;

		var template=_.template($("#" + this.className + "-template").html(), {});
		this.$el.html(template);
	}
});

var FileView = Backbone.View.extend({
	el: $("#tapi2p-main"),
	model: fileModel,
	tagName: "div",
	className: "tapi2p-files",
	initialize: function() {
		this.model.on("change", this.render, this);
		this.render();
	},
	render: function() {
		if(!this.$el.is(":visible")) return this;

		var template=_.template($("#" + this.className + "-template").html(), {fileList: this.model.attributes});
		this.$el.html(template);
	}
});

var SetupView = Backbone.View.extend({
	el: $("#tapi2p-main"),
	tagName: "div",
	className: "tapi2p-setup",
	model: keyModel,
	initialize: function() {
		this.model.on("change:publicKey", function()
		{
			$("#publickey").text(this.model.get("publicKey"));
		}, this);
		this.render();
	},
	render: function() {
		if(!this.$el.is(":visible")) return this;

		var template=_.template($("#" + this.className + "-template").html(), {});
		this.$el.html(template);
		$("#menu").children().remove();
		$("#setup").click(function()
		{
			backend.sendCommand(backend.Commands.Setup, {
				nick: $("#nick").val(),
				port: parseInt($("#port").val(), 10)
			});
			r.navigate("", {trigger: true, replace: true});
		});
	}
});

function render(view) {
	return function() {
		new view();
	};
}

var ErrorView = new RenderableTemplate("tapi2p-error");
var DownloadsView = new RenderableTemplate("tapi2p-downloads");

var Router = Backbone.Router.extend({
	routes: {
		"": "root",
		"chat" : "chat",
		"peers": "peers",
		"newpeer": "newpeer",
		"error": "error",
		"files": "files",
		"setup": "setup",
		"downloads": "downloads"
	}
});

var r=new Router();
r
.on("route:root", render(MainView))
.on("route:chat", render(ChatView))
.on("route:peers", render(PeerView))
.on("route:newpeer", render(NewPeerView))
.on("route:error", render(ErrorView))
.on("route:downloads", render(DownloadsView))
.on("route:setup", render(SetupView))
.on("route:files", render(FileView));

Backbone.history.start();

var backend = new Tapi2pBackend(tapi2p_open_successful, tapi2p_open_failure, tapi2p_handle_message);

function tapi2p_open_successful()
{
	backend.sendCommand(backend.Commands.Status);
}

function tapi2p_open_failure()
{
	$("#menu").children().remove();
	r.navigate("error", {trigger: true, replace: true});
}

var peermap={};

function tapi2p_handle_message(e)
{
	console.log(e.data);
	var d=JSON.parse(e.data);
	switch(d.cmd)
	{
		case backend.Commands.GetPublicKey:
		{
			console.log("PUBKEY");
			keyModel.set({publicKey: d.data});
			break;
		}
		case backend.Commands.PeerConnected:
		{
			peermap[d.peers.addr] = d.peers.nick;
			break;
		}
		case backend.Commands.PeerDisconnected:
		{
			backend.sendCommand(backend.Commands.ListPeers);
			break;
		}
		case backend.Commands.FileList:
		{
			fileModel.updateFiles(d.addr, d.port, d.data.files);
			break;
		}
		case backend.Commands.Message:
		{
			if(d.own_nick)
			{
				peermap.localhost={};
				peermap.localhost.nick = d.own_nick;
				d.data += d.own_nick;
				keyModel.set({welcome: d.data});
				break;
			}
			var nick=peermap[d.addr];
			if(!nick) nick=d.addr;
			chatModel.newMessage("[" + nick + "] " + d.data);
			break;
		}
		case backend.Commands.ListPeers:
		{
			peerModel.updatePeers(d.data.peers);
			for(var i=0; i<d.data.peers.length; ++i)
			{
				if(!peermap[d.data.peers[i].addr]) peermap[d.data.peers[i].addr]= d.data.peers[i].nick;
				backend.sendCommand(backend.Commands.RequestFileListLocal, null, d.data.peers[i].addr, d.data.peers[i].port);
			}
			break;
		}
		case backend.Commands.Status:
		{
			backend.sendCommand(backend.Commands.GetPublicKey);
			if(d.data.status)
			{
				backend.sendCommand(backend.Commands.Hello);
				backend.sendCommand(backend.Commands.ListPeers);
			}
			else
			{
				r.navigate("setup", {trigger: true, replace: true});
			}
			break;
		}
	}
}
