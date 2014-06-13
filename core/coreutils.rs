pub mod manager
{
	use std::os::getenv;

	pub struct PathManager;
	impl PathManager
	{
		fn get_root_string() -> String
		{
			getenv("TAPI2P_ROOT").unwrap_or(getenv("HOME").unwrap().append("/.config/tapi2p/"))
		}
		pub fn get_root_path() -> Path
		{
			Path::new(PathManager::get_root_string())
		}
		pub fn get_socket_path() -> Path
		{
			Path::new(PathManager::get_root_string().append("t2p_core"))
		}
	}
}
