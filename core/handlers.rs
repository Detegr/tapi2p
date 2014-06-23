use serialize::{json, Encodable};
use core::core::Core;
use std::io::net::ip::SocketAddr;
use sync::Arc;
use core::event::UIEvent;
use core::event::Sendable;
use std::io::IoResult;

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

pub fn handle_listpeers(core: &Arc<Core>, evt: &mut UIEvent) -> IoResult<()>
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
	evt.send_str(json::Encoder::str_encode(&ret))
}
