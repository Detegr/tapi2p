use serialize::{json, Encodable};
use core;
use core::Core;
use std::io::net::ip::SocketAddr;
use sync::Arc;
use event::UIEvent;
use event::Sendable;

#[deriving(Encodable)]
pub struct Peer
{
	nick: String,
	addr: String,
	port: u16,
	conn_status: String
}
#[deriving(Encodable)]
pub struct ListPeers
{
	peers: Vec<Peer>
}


impl Peer
{
	fn new(saddr: &SocketAddr) -> Peer
	{
		Peer {
			nick: "".to_string(),
			addr: saddr.ip.to_str().to_string(),
			port: saddr.port,
			conn_status: "ok".to_string()
		}
	}
}

pub fn handle_listpeers(core: &Arc<Core>, evt: &mut UIEvent) -> ()
{
	let mut ret = ListPeers { peers: vec![] };
	{
		let mut peers = core.get_peers().lock();
		for ref mut peer in peers.mut_iter() {
			match peer.peer_name()
			{
				Ok(s) => ret.peers.push(Peer::new(&s)),
				Err(_) => {}
			}
		}
	}
	/*
	let mut m = io::MemWriter::new();
	{
		let mut encoder = json::Encoder::new(&mut m as &mut std::io::Writer);
		match ret.encode(&mut encoder)
		{
			Ok => (),
			Err(e) => debug!("Json encoding error: {}", e)
		}
	}*/
	evt.send_str(json::Encoder::str_encode(&ret));
}
