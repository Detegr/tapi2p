#![crate_type = "lib"]
#![crate_id = "coreevent#0.2"]

pub mod event
{
	#[deriving(FromPrimitive)]
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
			//Event { mEventType: t, mData: vec!(data.slice_from(1)) }
		}
	}
}
