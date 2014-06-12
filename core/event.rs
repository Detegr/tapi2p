#![crate_type = "lib"]
#![crate_id = "coreevent#0.2"]

#![feature(struct_variant)]
#![feature(phase)]
#[phase(syntax, link)] extern crate log;

pub mod event
{
	use std::fmt;
	use std::any::Any;
	use std::owned::AnyOwnExt;
	use std::path::BytesContainer;
	use std::mem::size_of;
	use std::io::net::ip::IpAddr;
	use std::io::net::unix::UnixStream;
	use std::io::IoResult;
	use std::io::IoError;
	use std::io::Stream;
	use std::collections::HashMap;

	// UIEvent size in bytes
	static UISize : uint = 30;

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
	pub struct EventDispatcher<T>
	{
		mCallbacks : HashMap<EventType, fn(&mut T) -> ()>
	}
	impl<T: Event> EventDispatcher<T>
	{
		pub fn new() -> EventDispatcher<T>
		{
			EventDispatcher { mCallbacks: HashMap::new() }
		}
		pub fn register_callback(&mut self, t: EventType, cb: fn(&mut T)) -> ()
		{
			self.mCallbacks.insert(t, cb);
		}
		pub fn dispatch(&self, evt: &mut T) -> ()
		{
			match self.mCallbacks.find(&evt.get_type())
			{
				Some(cb) => { (*cb)(evt); }
				None => {}
			}
		}
	}
	pub trait Sendable
	{
		fn send(&mut self) -> IoResult<()>;
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
		mEventData: UIEvent,
		mAddr: IpAddr,
		mPort: u16
	}
	impl Sendable for UIEvent
	{
		fn send(&mut self) -> IoResult<()>
		{
			self.mStream.write_str("Event!")
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
}
