extern crate dtgconf;
use self::dtgconf::config;

// Imports
use core::coreutils::manager::PathManager;
use core::handlers;
use core::event::EventDispatcher;
use core::event::Sendable;
use core::event::{RemoteEvent,UIEvent};
use core::event;
use std::comm::channel;
use std::io::{Acceptor,Listener,TcpListener,TcpStream};
use std::io::fs;
use std::io::FilePermission;
use std::io::net::unix::UnixListener;
use std::io::net::tcp::TcpStream;
use std::io::net::ip::{IpAddr, SocketAddr};
use sync::{Arc,Mutex};
use std::io::signal::Interrupt;
use std::sync::atomics::{AtomicBool,SeqCst,INIT_ATOMIC_BOOL};
use std::path::BytesContainer;
use std::io::timer;

pub static mut RUNNING : AtomicBool = INIT_ATOMIC_BOOL;
pub type PeerList = Arc<Mutex<Vec<TcpStream>>>;

pub struct Core
{
	mPeers : PeerList,
	mPort : u16
}
impl Core
{
	fn new() -> Core
	{
		Core
		{
			mPeers: Arc::new(Mutex::new(vec![])),
			mPort: 0
		}
	}
	pub fn peer_exists(&self, addr: &str, port: u16) -> bool
	{
		for ref mut peer in self.get_peers().lock().mut_iter()
		{
			match peer.peer_name()
			{
				Ok(p) =>
				{
					if p.ip.to_str() == addr.to_string() && p.port == port { return true; }
				}
				Err(_) => continue
			}
		}
		false
	}
	pub fn get_peers<'a>(&'a self) -> &'a PeerList
	{
		&self.mPeers
	}
	pub fn get_incoming_port(&self) -> u16
	{
		self.mPort
	}
	pub fn with_peers(&self, func: |&TcpStream| -> ())
	{
		for ref peer in self.get_peers().lock().iter()
		{
			func(*peer);
		}
	}
	pub fn with_peers_mut(&self, func: |&mut TcpStream| -> ())
	{
		for ref mut peer in self.get_peers().lock().mut_iter()
		{
			func(*peer);
		}
	}
	fn init(&mut self) -> Result<(), &str>
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
			.or_else(|_| Err("Failed to create keys"));
		match config::cfg::load(&PathManager::get_config_path())
		{
			Some(conf) =>
			{
				let item = match conf.find_item("Port", Some("Account"))
				{
					Some(item) => item,
					None => return Err("No account information in config file")
				};
				let port = match item.get_val()
				{
					Some(val) => val,
					None => return Err("No port set in config file")
				};
				self.mPort = match from_str::<u16>(port)
				{
					Some(p) => p,
					None => return Err("Invalid port set in config file")
				};
				Ok(())
			}
			None => return Err("tapi2p config file not found")
		}
	}
	fn accept_incoming_peer_connections(core: &Arc<Core>) -> ()
	{
		debug!("Accepting incoming TCP connections");
		let mut acceptor = match TcpListener::bind("127.0.0.1", core.get_incoming_port()).listen()
		{
			Ok(acceptor) => acceptor,
			Err(e) =>
			{
				Core::stop();
				fail!("Failed to bind listening socket: {}", e.desc);
			}
		};
		while Core::threads_running()
		{
			acceptor.set_timeout(Some(1000));
			for stream in acceptor.incoming()
			{
				match stream
				{
					Ok(mut stream) =>
					{
						let mut peers = core.get_peers().lock();
						let new_peer_name = match stream.peer_name()
						{
							Ok(new_peer_name) => new_peer_name,
							Err(e) =>
							{
								debug!("Error getting peer name for new peer: {}", e.desc);
								break;
							}
						};
						debug!("New peer: {}:{}", new_peer_name.ip, new_peer_name.port);
						peers.push(stream);
					}
					Err(e) => break
				}
			}
		}
	}
	fn accept_incoming_ui_connections(tx: &Sender<UIEvent>) -> ()
	{
		debug!("Accepting incoming UI connections");
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
			Ok(mut acceptor) => while Core::threads_running()
			{
				acceptor.set_timeout(Some(1000));
				for client in acceptor.incoming()
				{
					match client {
						Ok(mut stream) => match UIEvent::from_stream(&mut stream)
						{
							Some(event) => tx.send(event),
							None => break
						},
						Err(_) => break
					}
				}
			}
		}
	}
	fn connect_to_peer(&mut self, addr: &str, port: u16)
	{
		debug!("Connecting -> {}:{}", addr, port);
		let ipaddr = match from_str::<IpAddr>(addr)
		{
			Some(ip) => ip,
			None =>
			{
				debug!("Invalid IP address: {}", addr);
				return;
			}
		};
		let mut stream = match TcpStream::connect_timeout(SocketAddr { ip: ipaddr, port: port }, 0)
		{
			Ok(stream) => stream,
			Err(e) =>
			{
				debug!("Error connecting to {}:{}: {}", addr, port, e.desc)
				return;
			}
		};
		let new_peer_name = match stream.peer_name()
		{
			Ok(new_peer_name) => new_peer_name,
			Err(e) =>
			{
				debug!("Error getting peer name for new peer: {}", e.desc);
				return;
			}
		};
		let mut peers = self.get_peers().lock();
		for ref mut peer in peers.mut_iter()
		{
			let peername = match peer.peer_name()
			{
				Ok(peername) => peername,
				Err(e) =>
				{
					debug!("Error getting peer name: {}", e.desc);
					continue;
				}
			};
			if(peername == new_peer_name)
			{
				debug!("Peer {}:{} already exists", peername.ip, peername.port);
				return;
			}
		}
		debug!("New peer: {}:{}", new_peer_name.ip, new_peer_name.port);
		stream.set_timeout(Some(0));
		peers.push(stream);
	}
	fn connect_to_peers(&mut self)
	{
		debug!("Connecting to peers");
		let conf = match config::cfg::load(&PathManager::get_config_path())
		{
			Some(conf) => conf,
			None => fail!("Config file not found")
		};
		let sect = match conf.find_section("Peers")
		{
			Some(sect) => sect,
			None => return ()
		};
		while Core::threads_running()
		{
			for addr in sect.get_items().iter()
			{
				let item = match conf.find_item("Port", Some(addr.get_key()))
				{
					Some(item) => item,
					None => continue
				};
				let port_str = match item.get_val()
				{
					Some(port_str) => port_str,
					None => continue
				};
				match from_str::<u16>(port_str)
				{
					Some(port) =>
					{
						if !self.peer_exists(addr.get_key(), port)
						{
							self.connect_to_peer(addr.get_key(), port)
						}
					},
					None => debug!("Invalid port {} for {}", port_str, addr.get_key())
				};
			}
			timer::sleep(1000);
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
	fn stop()
	{
		unsafe {
			RUNNING.store(false, SeqCst);
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
		let mut core = Core::new();
		match core.init() {
			Err(errstr) => {
				println!("tapi2p failed to start: {}", errstr);
				return ();
			},
			_ => ()
		}
		Core::setup_signal_handler();
		core.connect_to_peers();

		let core_arc = Arc::new(core);
		let (ui_tx, ui_rx): (Sender<UIEvent>, Receiver<UIEvent>) = channel();
		let c1 = core_arc.clone();
		let c2 = core_arc.clone();
		spawn(proc() {
			Core::accept_incoming_ui_connections(&ui_tx);
		});
		spawn(proc() {
			Core::accept_incoming_peer_connections(&c1);
		});
		spawn(proc() {
			Core::run_ui_event_callbacks(&c2, &ui_rx);
		});
	}
}
