#![feature(phase)]
#[phase(syntax, link)] extern crate log;
#[phase(syntax)] extern crate green;
extern crate sync;
extern crate coreutils;
extern crate coreevent;
use std::sync::atomics::{SeqCst};

pub mod core
{
	use coreutils::manager::PathManager;
	use coreevent::event::EventType;
	use coreevent::event::UIEvent;
	use coreevent::event::UIEventSize;
	use coreevent::event::FromSlice;
	use std::io::Listener;
	use std::io::Acceptor;
	use std::io::net::unix::UnixListener;
	use std::io::net::unix::UnixStream;
	use std::io::fs;

	use std::sync::atomics::{AtomicBool,SeqCst,INIT_ATOMIC_BOOL};
	pub static mut RUNNING : AtomicBool = INIT_ATOMIC_BOOL;

	pub struct Core;
	impl Core
	{
		fn new() -> Core
		{
			Core
		}
		fn accept_incoming_ui_connections(&self) -> ()
		{
			let socket_path=PathManager::get_socket_path();
			debug!("create_core_socket({})", socket_path.as_str());
			if socket_path.exists() { fs::unlink(&socket_path).unwrap(); }
			let listener = match UnixListener::bind(&socket_path) {
				Err(_) => fail!("Failed to bind socket"),
				Ok(listener) => listener
			};
			match listener.listen()
			{
				Err(e) => fail!(e),
				Ok(mut acceptor) =>
				{
					while self.threads_running()
					{
						acceptor.set_timeout(Some(1000));
						for mut client in acceptor.incoming()
						{
							//match client.read_exact(UIEventSize)
							match client.read_to_end()
							{
								Ok(data) =>
								{
									let event : UIEvent=FromSlice::from_slice(client.unwrap(), data.as_slice());
									debug!("Data: {}", event)
								}
								Err(_) => break
							}
						}
					}
				}
			}
		}
		pub fn threads_running(&self) -> bool
		{
			unsafe {
				RUNNING.load(SeqCst)
			}
		}
		pub fn run() -> ()
		{
			let core = Core::new();
			spawn(proc() {
				core.accept_incoming_ui_connections();
			});
		}
	}
}

green_start!(main)
fn main()
{
	let mut listener = std::io::signal::Listener::new();
	listener.register(std::io::signal::Interrupt).unwrap();
	spawn(proc() {
		unsafe {
			core::RUNNING.store(true, SeqCst);
			loop {
				match listener.rx.recv() {
					std::io::signal::Interrupt => {
						core::RUNNING.store(false, SeqCst);
						println!("tapi2p core shutting down...");
						break;
					}
					_ => ()
				};
			}
		}
	});
	core::Core::run();
}
