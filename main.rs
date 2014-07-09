#![feature(phase)]
#[phase(plugin, link)] extern crate log;
#[phase(plugin)] extern crate green;
extern crate sync;
extern crate serialize;

use core::core::Core;
mod core {
	pub mod core;
	mod coreutils;
	mod event;
	mod handlers;
	mod peer;
}
mod crypto {
	pub mod keygen;
}

green_start!(main)
fn main()
{
	Core::run();
}
