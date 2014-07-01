use core::core::Core;

use std::collections::HashMap;
use std::fmt;
use std::io::BufWriter;
use std::io::IoResult;
use std::io::net::ip::IpAddr;
use std::io::net::tcp::TcpStream;
use std::io::net::unix::UnixStream;
use std::mem::size_of;
use std::path::BytesContainer;
use sync::Arc;

// UIEvent size in bytes
static UISize : uint = 35;

#[deriving(FromPrimitive)]
#[deriving(Show)]
#[deriving(PartialEq)]
#[deriving(Eq)]
#[deriving(Hash)]
pub enum EventType
{
	AddFile,
	AddPeer,
	FileList,
	FilePart,
	FilePartList,
	FileTransferStatus,
	GetPublicKey,
	ListPeers,
	Message,
	Metadata,
	PeerConnected,
	PeerDisconnected,
	RequestFileList,
	RequestFileListLocal,
	RequestFilePart,
	RequestFilePartList,
	RequestFileTransfer,
	RequestFileTransferLocal,
	Setup,
	Status
}
type EventCallback<'a, T> = fn(&'a Arc<Core>, &mut T) -> IoResult<()>;
pub struct EventDispatcher<'a, T>
{
	mCore : &'a Arc<Core>,
	mCallbacks : HashMap<EventType, EventCallback<'a, T>>
}
impl<'a, T: Event> EventDispatcher<'a, T>
{
	pub fn new(core: &'a Arc<Core>) -> EventDispatcher<'a, T>
	{
		EventDispatcher {
			mCore: core,
			mCallbacks: HashMap::new()
		}
	}
	pub fn register_callback(&mut self, t: EventType, cb: EventCallback<'a, T>) -> ()
	{
		self.mCallbacks.insert(t, cb);
	}
	pub fn dispatch<'a> (&'a self, evt: &mut T) -> ()
	{
		match self.mCallbacks.find(&evt.get_type())
		{
			Some(cb) =>
			{
				match (*cb)(self.mCore, evt)
				{
					Ok(_) => (),
					Err(e) => debug!("Failed to run event callback for '{}': {}", evt.get_type(), e)
				}
			}
			None => {debug!("No handler for event type: {}", evt.get_type());}
		}
	}
}
pub trait Sendable
{
	fn send_str(&mut self, data: String) -> IoResult<()>
	{
		self.send(data.into_bytes())
	}
	fn send(&mut self, data: Vec<u8>) -> IoResult<()>;
}
pub trait Event : Sendable
{
	fn get_type(&self) -> EventType;
}
pub struct UIEvent
{
	mEventType : EventType,
	mStream : UnixStream,
	mData: Vec<u8>
}
impl Event for UIEvent
{
	fn get_type(&self) -> EventType
	{
		self.mEventType
	}
}
impl UIEvent
{
	pub fn from_stream(stream: &mut UnixStream) -> Option<UIEvent>
	{
		match stream.read_exact(UISize)
		{
			Ok(eventvec) =>
			{
				let event = eventvec.as_slice();
				let t : EventType = FromPrimitive::from_u8(event[0]).unwrap();
				let offset_to_data_len = size_of::<u8>() + size_of::<i32>() + (16 * size_of::<i8>()) + size_of::<u16>();
				let data_len : u32 = event[offset_to_data_len] as u32;
				let eventdata = stream.read_exact(data_len as uint).unwrap();
				Some(UIEvent {
					mEventType: t,
					mStream: stream.clone(),
					mData: eventdata
				})
			}
			Err(_) => None
		}
	}
}
pub struct RemoteEvent
{
	mEventType : EventType,
	mStream : TcpStream,
	mData: Vec<u8>
}
impl Sendable for UIEvent
{
	fn send(&mut self, data: Vec<u8>) -> IoResult<()>
	{
		let data_u8: &mut [u8] = [0, ..UISize];
		{
			let mut writer: BufWriter = BufWriter::new(data_u8);
			try!(writer.write_u8(self.mEventType as u8));
			try!(writer.write_le_u32(0 as u32));
			for _ in range(0u, 16u) {
				try!(writer.write_i8(0 as i8));
			}
			try!(writer.write_le_u16(0 as u16));
			try!(writer.write_le_u32(data.len() as u32));
			try!(writer.write_le_u64(0 as u64));
		}
		try!(self.mStream.write(data_u8));
		self.mStream.write(data.as_slice())
	}
}
impl fmt::Show for UIEvent
{
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result
	{
		match self.mData.container_as_str() {
			Some(s) => write!(f, "{}\nData: {}", self.mEventType.to_str(), s),
			None => write!(f, "{}\nData_len: {}", self.mEventType.to_str(), self.mData.len())
		}
	}
}
