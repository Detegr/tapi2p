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
	use coreevent::event::EventDispatcher;
	use coreevent::event::UIEvent;
	use coreevent::event::Sendable;
	use std::io::Listener;
	use std::io::Acceptor;
	use std::io::Stream;
	use std::io::net::unix::UnixListener;
	use std::io::net::unix::UnixStream;
	use std::io::fs;
	use std::comm::channel;
	use sync::Arc;

	use std::sync::atomics::{AtomicBool,SeqCst,INIT_ATOMIC_BOOL};
	pub static mut RUNNING : AtomicBool = INIT_ATOMIC_BOOL;

	pub struct Core;
	impl Core
	{
		fn accept_incoming_ui_connections(tx: &Sender<UIEvent>) -> ()
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
					while Core::threads_running()
					{
						acceptor.set_timeout(Some(1000));
						for client in acceptor.incoming()
						{
							match client {
								Ok(mut stream) =>
								{
									match UIEvent::from_stream(&mut stream)
									{
										Some(event) => tx.send(event),
										None => break
									}
								}
								Err(_) => break
							}
						}
					}
				}
			}
		}
		fn run_ui_event_callbacks(rx: &Receiver<UIEvent>) -> ()
		{
			let mut dispatcher = EventDispatcher::<UIEvent>::new();
			let t : EventType = FromPrimitive::from_u8(0).unwrap();
			dispatcher.register_callback(t, Core::test);
			match rx.recv_opt()
			{
				Ok(mut evt) => {dispatcher.dispatch(&mut evt)},
				Err(_) => ()
			}
		}
		pub fn threads_running() -> bool
		{
			unsafe {
				RUNNING.load(SeqCst)
			}
		}
		pub fn run() -> ()
		{
			let (tx, rx): (Sender<UIEvent>, Receiver<UIEvent>) = channel();
			spawn(proc() {
				Core::accept_incoming_ui_connections(&tx);
			});
			spawn(proc() {
				Core::run_ui_event_callbacks(&rx);
			});
		}
		fn test(evt: &mut UIEvent) -> ()
		{
			println!("woho");
			evt.send().unwrap();
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
