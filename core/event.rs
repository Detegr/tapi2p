#![crate_type = "lib"]
#![crate_id = "coreevent#0.2"]

#![feature(phase)]
#[phase(syntax, link)] extern crate log;

pub mod event
{
	use std::fmt;
	use std::path::BytesContainer;
	use std::mem::size_of;

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
	pub struct Event
	{
		mEventType : EventType,
		mData : Vec<u8>
	}
	impl Event
	{
		/* UI Events as a binary look like following:
		 * type {dummy}  {addr}  {port} data_len      data    {ptr for c struct}
		 * [u8]  [i32] [i8 * 16] [u16]  [u32]   [u8 * data_len]     [u64]
		 *
		 * Values in {} are not used with this event type
		 */
		pub fn from_slice(data: &[u8]) -> Event
		{
			let t : EventType = FromPrimitive::from_u8(data[0]).unwrap();
			let offset_to_data_len = size_of::<u8>() + size_of::<i32>() + (16 * size_of::<i8>()) + size_of::<u16>();
			let mut data_len : u32 = data[offset_to_data_len] as u32;
			let offset_to_data_start = offset_to_data_len + size_of::<u32>() + size_of::<u64>();
			Event { mEventType: t, mData: Vec::from_slice(data.slice(offset_to_data_start, offset_to_data_start + data_len as uint)) }
		}
	}
	impl fmt::Show for Event
	{
		fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result
		{
			match self.mData.container_as_str() {
				Some(s) => write!(f, "{} : {}", self.mEventType.to_str(), s),
				None => write!(f, "{} : {}", self.mEventType.to_str(), self.mData.len())
			}
		}
	}
}
