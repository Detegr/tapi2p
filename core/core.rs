// Imports
use core::coreutils::manager::PathManager;
use core::handlers;
use core::event::EventDispatcher;
use core::event::Sendable;
use core::event::UIEvent;
use core::event;
use std::comm::channel;
use std::io::Acceptor;
use std::io::Listener;
use std::io::fs;
use std::io::FilePermission;
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
	pub fn get_peers<'a>(&'a self) -> &'a PeerList
	{
		&self.mPeers
	}
	fn init(&self) -> Result<(), &str>
	{
		fs::mkdir_recursive(&PathManager::get_root_path(), FilePermission::all())
			.and(fs::mkdir_recursive(&PathManager::get_key_path(), FilePermission::all()))
			.and(fs::mkdir_recursive(&PathManager::get_metadata_path(), FilePermission::all()))
			.and_then(|_| {
				debug!("Created/existing directories:\n\t{}\n\t{}\n\t{}",
						&PathManager::get_root_path().as_str(),
						&PathManager::get_key_path().as_str(),
						&PathManager::get_metadata_path().as_str());
				Ok(())
			})
			.or_else(|_| return Err("Failed to create tapi2p directories")).unwrap();
		fs::stat(&PathManager::get_self_key_path())
			.and(fs::stat(&PathManager::get_self_public_key_path()))
			.and(Ok(()))
			.or_else(|_| ::crypto::keygen::generate_keys(&PathManager::get_self_key_path()))
			.or_else(|_| Err("Failed to create keys"))
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
		dispatcher.register_callback(event::ListPeers, handlers::handle_listpeers);
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
		match core.init() {
			Err(errstr) => {
				println!("tapi2p failed to start: {}", errstr);
				return ();
			},
			_ => ()
		}
		Core::setup_signal_handler();
		let (tx, rx): (Sender<UIEvent>, Receiver<UIEvent>) = channel();
		spawn(proc() {
			Core::accept_incoming_ui_connections(&tx);
		});
		spawn(proc() {
			Core::run_ui_event_callbacks(&core, &rx);
		});
	}
}
