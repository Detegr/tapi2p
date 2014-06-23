pub mod manager
{
	use std::os::getenv;

	pub struct PathManager;
	impl PathManager
	{
		fn get_root_string() -> String
		{
			getenv("TAPI2P_ROOT").unwrap_or(getenv("HOME").unwrap().append("/.config/tapi2p")).append("/")
		}
		pub fn get_root_path() -> Path
		{
			Path::new(PathManager::get_root_string())
		}
		pub fn get_key_path() -> Path
		{
			Path::new(PathManager::get_root_string().append("keys"))
		}
		pub fn get_self_key_path() -> Path
		{
			Path::new(PathManager::get_root_string().append("keys/self"))
		}
		pub fn get_self_public_key_path() -> Path
		{
			Path::new(PathManager::get_root_string().append("keys/self.pub"))
		}
		pub fn get_socket_path() -> Path
		{
			Path::new(PathManager::get_root_string().append("t2p_core"))
		}
		pub fn get_metadata_path() -> Path
		{
			Path::new(PathManager::get_root_string().append("metadata"))
		}
	}
}
