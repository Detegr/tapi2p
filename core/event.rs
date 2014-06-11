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
	/*
	pub struct EventDispatcher<'a, T>
	{
		mUICallbacks : HashMap<EventType, fn()>,
		//mRemoteCallbacks : HashMap<EventType, fn()>
	}
	impl<'a, T: EventClass> EventDispatcher<'a, T>
	{
		fn register_callback<'a, T: EventClass>(&'a self, etype: EventType, cb: fn()) -> ()
		{
			self.mUICallbacks.insert(etype, cb);
		}
		fn dispatch<T: EventClass>(&self, e: &T)
		{
			match e.mEventClass {
				UI => self.mUICallbacks[e.get_class()],
				Remote => fail!("Remote events not implemented yet")
			}
		}
	}
	*/
	pub trait FromStream
	{
		fn from_stream(mut stream: Box<Stream>) -> Option<Self>;
	}
	pub trait Event : FromStream
	{
		fn send() -> IoResult<()>;
	}
	pub struct UIEvent
	{
		mEventType : EventType,
		mStream : Box<Stream>,
		mData: Vec<u8>
	}
	pub struct RemoteEvent
	{
		mEventData: UIEvent,
		mAddr: IpAddr,
		mPort: u16
	}
	impl FromStream for UIEvent
	{
		fn from_stream(mut stream: Box<Stream>) -> Option<UIEvent>
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
						mStream: stream,
						mData: eventdata
					})
				}
				Err(_) => None
			}
		}
	}
	impl Event for UIEvent
	{
		fn send() -> IoResult<()>
		{
			Ok(())
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
