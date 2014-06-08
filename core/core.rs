#![feature(phase)]
#[phase(syntax, link)] extern crate log;

extern crate coreutils;
extern crate coreevent;
pub mod core
{

	use coreutils::manager::PathManager;
	use coreevent::event::EventType;
	use coreevent::event::Event;
	use std::io::Listener;
	use std::io::Acceptor;
	use std::io::net::unix::UnixListener;
	use std::io::fs;

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
			match listener.listen() {
				Err(e) => fail!(e),
				Ok(mut acceptor) => {
					loop {
						acceptor.set_timeout(Some(1000));
						for mut client in acceptor.incoming() {
							match client.read_to_end() {
								Ok(data) => {
									let event=Event::from_slice(data.as_slice());
									println!("Data: {}", event)
								}
								Err(e) => {
									println!("{}", e.desc);
									break;
								}
							}
						}
					}
				}
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

fn main()
{
	// Rust's signal handling is broken?
	/*
	let mut listener = std::io::signal::Listener::new();
	listener.register(std::io::signal::Interrupt).unwrap();
	spawn(proc()
	{
		loop {
			match listener.rx.recv() {
				std::io::signal::Interrupt => println!("SIGINT"),
				_ => ()
			};
		}
	});*/
	core::Core::run();
}
