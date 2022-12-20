#![no_std]
// Soon to be stabilized https://github.com/rust-lang/rust/pull/102318
// #![feature(default_alloc_error_handler)]
#![feature(alloc_error_handler)]

#[global_allocator]
static mut ALLOCATOR: CAllocator = CAllocator;

#[alloc_error_handler]
fn alloc_error(layout: core::alloc::Layout) -> ! {
    loop {}
}

#[allow(unused_imports)]
use core::panic::PanicInfo;
#[cfg(not(test))] // https://github.com/rust-lang/rust-analyzer/issues/4490
#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    // Put a breakpoint here to catch rust panics
    loop {}
}

mod mpm_demo;
mod nlms_demo;
mod sfnov_demo;

pub use mpm_demo::MpmDemoApp;
pub use nlms_demo::NlmsDemoApp;
pub use sfnov_demo::SfnovDemoApp;

extern crate alloc;
mod c_allocator;
use c_allocator::CAllocator;

mod c_api;

#[repr(u8)]
#[derive(Clone, Copy)]
pub enum AppMessage {
    Button0Down = 1,
    Button1Down = 2,
    Button2Down = 3,
    Button3Down = 4,
    Button0Up = 5,
    Button1Up = 6,
    Button2Up = 7,
    Button3Up = 8,

    Led0On = 9,
    Led1On = 10,
    Led2On = 11,
    Led3On = 12,
    Led0Off = 13,
    Led1Off = 14,
    Led2Off = 15,
    Led3Off = 16,
}

pub trait DemoApp {
    fn new(sample_rate: f32) -> Self;
    fn process(&mut self, rx: &[f32], tx: &mut [f32]);
    fn handle_message(&mut self, message: AppMessage);
    fn next_outgoing_message(&mut self) -> Option<AppMessage>;
}

#[cfg(feature = "nlms_demo")]
type DemoAppType = nlms_demo::NlmsDemoApp;
#[cfg(feature = "sfnov_demo")]
type DemoAppType = sfnov_demo::SfnovDemoApp;
#[cfg(feature = "mpm_demo")]
type DemoAppType = mpm_demo::MpmDemoApp;
