extern crate libc;
use std::io::IoResult;
use std::io::IoError;

#[link(name = "keygen", kind = "static")]
#[link(name = "ssl")]
#[link(name = "crypto")]
extern
{
	fn generate(path: *const libc::c_char, keys: libc::c_uint) -> libc::c_int;
}

pub fn generate_keys(path: &Path) -> IoResult<()>
{
	debug!("Generating keys to {}", path.as_str());
	unsafe
	{
		let p=path.to_c_str().as_ptr();
		match generate(p, 0x3 as u32)
		{
			0 => Ok(()),
			err => Err(IoError::from_errno(err as uint, false))
		}
	}
}
