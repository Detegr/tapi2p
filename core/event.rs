#![crate_type = "lib"]
#![crate_id = "coreevent#0.2"]

#![feature(struct_variant)]
#![feature(phase)]
#[phase(syntax, link)] extern crate log;

pub mod event
{
	use std::fmt;
	use std::path::BytesContainer;
	use std::mem::size_of;
	use std::io::net::ip::IpAddr;
	use std::io::net::unix::UnixStream;
	use std::collections::HashMap;

	pub static UISize : uint = 30;

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
	pub trait FromSlice
	{
		fn from_slice(data: &[u8]) -> Option<Self>;
	}
	pub struct UI;
	pub struct Remote
	{
		mAddr: IpAddr,
		mPort: u16
	}
	pub struct Event<T>
	{
		mEventData : T,
		mEventType : EventType,
		mData: Vec<u8>
	}
	impl FromSlice for UI
	{
		fn from_slice(data: &[u8]) -> Option<UI> {Some(UI)}
	}
	impl FromSlice for Remote
	{
		fn from_slice(data: &[u8]) -> Option<Remote>
		{
			let ipstr=match String::from_utf8(Vec::from_slice(data.slice(5, 21))) {
				Ok(s) => s,
				Err(_) => return None
			};
			match from_str::<IpAddr>(ipstr.as_slice()) {
				Some(addr) => Some(Remote { mAddr: addr, mPort: 0 }),
				None => None
			}
		}
	}
	impl<T: FromSlice> FromSlice for Event<T>
	{
		/* UI Events as a binary look like following:
		 * type fd_from {addr}  {port} data_len  {ptr for c struct}        data
		 * [u8]  [i32] [i8 * 16] [u16]  [u32]           [u64]         [u8 * data_len]
		 *
		 * Values in {} are not used with this event type
		 */
		fn from_slice(data: &[u8]) -> Option<Event<T>>
		{
			let t : EventType = FromPrimitive::from_u8(data[0]).unwrap();
			let offset_to_data_len = size_of::<u8>() + size_of::<i32>() + (16 * size_of::<i8>()) + size_of::<u16>();
			let data_len : u32 = data[offset_to_data_len] as u32;
			let offset_to_data_start = offset_to_data_len + size_of::<u32>() + size_of::<u64>();
			let edata : T = match FromSlice::from_slice(data) {
				Some(d) => d,
				None => return None
			};
			Some(Event {
				mEventData: edata,
				mEventType: t,
				mData: Vec::from_slice(data.slice(offset_to_data_start, offset_to_data_start + data_len as uint))
			})
		}
	}
	impl fmt::Show for Event<UI>
	{
		fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result
		{
			match self.mData.container_as_str() {
				Some(s) => write!(f, "{}\nData: {}", self.mEventType.to_str(), s),
				None => write!(f, "{}\nData_len: {}", self.mEventType.to_str(), self.mData.len())
			}
		}
	}
	impl fmt::Show for Event<Remote>
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
