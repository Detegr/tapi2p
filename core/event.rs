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

	pub static UIEventSize : uint = 30;

	#[deriving(FromPrimitive)]
	#[deriving(Show)]
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
	pub trait FromSlice
	{
		fn from_slice(stream: UnixStream, data: &[u8]) -> Self;
	}
	pub struct EventDispatcher<T>
	{
		mEventCallbacks : HashMap<EventType, |T| -> ()>
	}
	impl<T: Dispatchable> EventDispatcher<T>
	{
		fn register_callback<T : Dispatchable>(&self, etype: EventType, cb: |T| -> ()) -> ()
		{
			self.mEventCallbacks.insert(etype, cb);
		}
	}
	pub trait Dispatchable
	{
		fn dispatch(&self) -> ();
	}
	pub struct UIEvent
	{
		mEventType: EventType,
		mFdFrom: Box<UnixStream>,
		mData: Vec<u8>
	}
	pub struct RemoteEvent
	{
		mEventType: EventType,
		mAddr: IpAddr,
		mPort: u16,
		mData: Vec<u8>
	}
	impl FromSlice for UIEvent
	{
		/* UI Events as a binary look like following:
		 * type fd_from {addr}  {port} data_len  {ptr for c struct}        data
		 * [u8]  [i32] [i8 * 16] [u16]  [u32]           [u64]         [u8 * data_len]
		 *
		 * Values in {} are not used with this event type
		 */
		fn from_slice(stream: UnixStream, data: &[u8]) -> UIEvent
		{
			let t : EventType = FromPrimitive::from_u8(data[0]).unwrap();
			let offset_to_data_len = size_of::<u8>() + size_of::<i32>() + (16 * size_of::<i8>()) + size_of::<u16>();
			let data_len : u32 = data[offset_to_data_len] as u32;
			let offset_to_data_start = offset_to_data_len + size_of::<u32>() + size_of::<u64>();
			UIEvent {
				mEventType: t,
				mFdFrom: box stream,
				mData: Vec::from_slice(data.slice(offset_to_data_start, offset_to_data_start + data_len as uint))
			}
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
