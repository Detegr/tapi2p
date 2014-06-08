#![crate_type = "lib"]
#![crate_id = "coreevent#0.2"]

pub mod event
{
	use std::fmt;
	use std::path::BytesContainer;

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
		pub fn from_slice(data: &[u8]) -> Event
		{
			let t : EventType = FromPrimitive::from_u8(data[0]).unwrap();
			Event { mEventType: t, mData: Vec::from_slice(data.init())}
		}
	}
	impl fmt::Show for Event
	{
		fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result
		{
			write!(f, "{} : {}", self.mEventType.to_str(), self.mData.len())
		}
	}
}
