use std::io::TcpStream;
use std::fmt::{Show,Formatter,FormatError};

pub enum ConnectionStatus
{
	Connected,
	Oneway,
	Invalid
}

/* TODO: This custom impl is for compatibility with C-version.
 * Use deriving(Show) instead and update all components using these three.
 */
impl Show for ConnectionStatus
{
	fn fmt(&self, fmt: &mut Formatter) -> Result<(), FormatError>
	{
		match *self
		{
			Connected => fmt.write_str("ok"),
			Oneway => fmt.write_str("oneway"),
			Invalid => fmt.write_str("invalid")
		};
		Ok(())
	}
}

pub struct Peer
{
	connection: TcpStream,
	status: ConnectionStatus
}

impl Peer
{
	pub fn new_incoming(connection: TcpStream) -> Peer
	{
		Peer { connection: connection, status: Connected }
	}
	pub fn new_outgoing(connection: TcpStream) -> Peer
	{
		Peer { connection: connection, status: Oneway }
	}
	pub fn get_connection<'a>(&'a self) -> &'a TcpStream
	{
		&self.connection
	}
	pub fn get_connection_mut<'a>(&'a mut self) -> &'a mut TcpStream
	{
		&mut self.connection
	}
	pub fn get_status(&self) -> ConnectionStatus
	{
		self.status
	}
	pub fn promote(&mut self)
	{
		match self.status
		{
			Connected => warn!("Tried to promote a connected connection"),
			Oneway => {
				debug!("Oneway to bidirectional");
				self.status = Connected;
			},
			Invalid => warn!("Tried to promote a invalid connection")
		}
	}
}
