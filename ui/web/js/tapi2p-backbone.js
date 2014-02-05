var FileModel = Backbone.Model.extend({
	updateFiles: function(peer_ip, files) {
		var obj={};
		obj[peer_ip]=files;
		this.set(obj);
	}
});

var PeerModel = Backbone.Model.extend({
	updatePeers: function(peers) {
		this.set({peers: peers});
	}
});

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
	className: null,
	initialize: function(params) {
		this.welcome=params.welcome;
		this.render();
	},
	render: function() {
		var template=_.template($("#" + this.className + "-template").html(), {welcome_message: this.welcome});
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
		"peers": "peers",
		"error": "error",
		"files": "files",
		"downloads": "downloads"
	}
});

var r=new Router();
r
.on("route:peers", render(PeerView))
.on("route:error", render(ErrorView))
.on("route:downloads", render(DownloadsView))
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
		case backend.Commands.PeerConnected:
		case backend.Commands.PeerDisconnected:
		{
			backend.sendCommand(backend.Commands.ListPeers);
			break;
		}
		case backend.Commands.FileList:
		{
			fileModel.updateFiles(d.addr, d.data.files);
			break;
		}
		case backend.Commands.Message:
		{
			if(d.own_nick)
			{
				peermap.localhost={};
				peermap.localhost.nick = d.own_nick;
				d.data += d.own_nick;
				r.on("route:root", function() {
					new MainView({className: "tapi2p-main", welcome: d.data});
				});
				r.navigate("", {trigger: true, replace: true});
				Backbone.history.loadUrl();
			}
			break;
		}
		case backend.Commands.ListPeers:
		{
			peerModel.updatePeers(d.data.peers);
			for(var i=0; i<d.data.peers.length; ++i)
			{
				backend.sendCommand(backend.Commands.RequestFileListLocal, null, d.data.peers[i].addr, d.data.peers[i].port);
			}
			break;
		}
		case backend.Commands.Status:
		{
			if(d.data.status)
			{
				backend.sendCommand(backend.Commands.Hello);
				backend.sendCommand(backend.Commands.ListPeers);
			}
			else alert("NYI");
			break;
		}
	}
}
