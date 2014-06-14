#![feature(phase)]
#[phase(syntax, link)] extern crate log;
#[phase(syntax)] extern crate green;
extern crate sync;
use std::sync::atomics::{SeqCst};
mod coreutils;
mod event;

pub mod core
{
	use coreutils::manager::PathManager;
	use event::EventDispatcher;
	use event::Sendable;
	use event::UIEvent;
	use event;
	use std::comm::channel;
	use std::io::Acceptor;
	use std::io::Listener;
	use std::io::fs;
	use std::io::net::unix::UnixListener;
	use std::io::net::tcp::TcpStream;
	use sync::{Arc,Mutex};

	use std::sync::atomics::{AtomicBool,SeqCst,INIT_ATOMIC_BOOL};
	pub static mut RUNNING : AtomicBool = INIT_ATOMIC_BOOL;

	pub type PeerList = Arc<Mutex<Vec<TcpStream>>>;

	pub struct Core
	{
		mConnections : PeerList
	}
	impl Core
	{
		fn new() -> Core
		{
			Core { mConnections: Arc::new(Mutex::new(vec![])) }
		}
		fn get_peers(&self) -> PeerList
		{
			self.mConnections.clone()
		}
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
		fn run_ui_event_callbacks(peers: &PeerList, rx: &Receiver<UIEvent>) -> ()
		{
			let mut dispatcher = EventDispatcher::<UIEvent>::new(peers);
			dispatcher.register_callback(event::FileList, Core::test);
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
			let core = Core::new();
			let peers = core.get_peers();
			let (tx, rx): (Sender<UIEvent>, Receiver<UIEvent>) = channel();
			spawn(proc() {
				Core::accept_incoming_ui_connections(&tx);
			});
			spawn(proc() {
				Core::run_ui_event_callbacks(&peers, &rx);
			});
		}
		fn test(dispatcher: &EventDispatcher<UIEvent>, evt: &mut UIEvent) -> ()
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
