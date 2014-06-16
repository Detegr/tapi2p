#![feature(phase)]
#[phase(plugin, link)] extern crate log;
#[phase(plugin)] extern crate green;
extern crate sync;
mod coreutils;
mod event;

pub mod core
{
	// Imports
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
	use std::io::signal::Interrupt;
	use std::sync::atomics::{AtomicBool,SeqCst,INIT_ATOMIC_BOOL};

	pub static mut RUNNING : AtomicBool = INIT_ATOMIC_BOOL;
	pub type PeerList = Arc<Mutex<Vec<TcpStream>>>;

	pub struct Core
	{
		mPeers : PeerList
	}
	impl Core
	{
		fn new() -> Core
		{
			Core {
				mPeers: Arc::new(Mutex::new(vec![]))
			}
		}
		fn get_peers<'a>(&'a self) -> &'a PeerList
		{
			&self.mPeers
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
		fn run_ui_event_callbacks(core: &Arc<Core>, rx: &Receiver<UIEvent>) -> ()
		{
			let mut dispatcher = EventDispatcher::<UIEvent>::new(core);
			dispatcher.register_callback(event::ListPeers, Core::test);
			while Core::threads_running()
			{
				match rx.recv_opt()
				{
					Ok(mut evt) =>
					{
						debug!("Dispatching UI event");
						dispatcher.dispatch(&mut evt)
					},
					Err(_) => ()
				}
			}
		}
		fn threads_running() -> bool
		{
			unsafe {
				RUNNING.load(SeqCst)
			}
		}
		fn setup_signal_handler()
		{
			let mut listener = ::std::io::signal::Listener::new();
			listener.register(Interrupt).unwrap();
			spawn(proc() {
				unsafe {
					RUNNING.store(true, SeqCst);
					loop {
						match listener.rx.recv() {
							Interrupt => {
								RUNNING.store(false, SeqCst);
								println!("tapi2p core shutting down...");
								break;
							}
							_ => ()
						};
					}
				}
			});
		}
		pub fn run() -> ()
		{
			let core = Arc::new(Core::new());
			Core::setup_signal_handler();
			let (tx, rx): (Sender<UIEvent>, Receiver<UIEvent>) = channel();
			spawn(proc() {
				Core::accept_incoming_ui_connections(&tx);
			});
			spawn(proc() {
				Core::run_ui_event_callbacks(&core, &rx);
			});
		}
		fn test(core: &Arc<Core>, evt: &mut UIEvent) -> ()
		{
			debug!("woho");
			evt.send().unwrap();
		}
	}
}

green_start!(main)
fn main()
{
	core::Core::run();
}
