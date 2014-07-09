use serialize::{json, Encodable};
use core::core::Core;
use core::peer::ConnectionStatus;
use std::io::net::ip::SocketAddr;
use sync::Arc;
use core::event::UIEvent;
use core::event::Sendable;
use std::io::IoResult;

#[deriving(Encodable)]
pub struct PeerListItem
{
	nick: String,
	addr: String,
	port: u16,
	conn_status: String
}
#[deriving(Encodable)]
pub struct ListPeers
{
	peers: Vec<PeerListItem>
}

impl PeerListItem
{
	fn new(saddr: &SocketAddr, status: ConnectionStatus) -> PeerListItem
	{
		PeerListItem {
			nick: "".to_string(),
			addr: saddr.ip.to_str().to_string(),
			port: saddr.port,
			conn_status: status.to_str().to_string()
		}
	}
}

pub fn handle_listpeers(core: &Arc<Core>, evt: &mut UIEvent) -> IoResult<()>
{
	let mut ret = ListPeers { peers: vec![] };
	core.with_peers_mut(|peer| {
		match peer.get_connection_mut().peer_name()
		{
			Ok(s) => ret.peers.push(PeerListItem::new(&s, peer.get_status())),
			Err(_) => {}
		}
	});
	evt.send_str(json::encode(&ret))
}
