#![no_std]

// Soon (?) to be stabilized https://github.com/rust-lang/rust/pull/102318
#![feature(default_alloc_error_handler)]

extern crate alloc;
mod c_allocator;
// mod looper;
use alloc::{alloc::alloc, boxed::Box, slice, vec};
use c_allocator::CAllocator;
use core::{alloc::Layout, panic::PanicInfo};
// use looper::Looper;

#[global_allocator]
static mut ALLOCATOR: CAllocator = CAllocator;

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    loop {}
}

/* #[no_mangle]
pub extern "C" fn create_looper() -> *mut Looper {
    unsafe {
        let layout = Layout::new::<Looper>();
        alloc(layout) as *mut Looper
    }
}

#[no_mangle]
pub fn looper_process(
    looper_ptr: *mut Looper,
    tx_ptr: *mut f32,
    rx_ptr: *const f32,
    frame_count: usize,
    channel_count: usize,
) {
    let sample_count = frame_count * channel_count;
    let (looper, tx, rx) = unsafe {
        (
            &mut *looper_ptr,
            slice::from_raw_parts_mut(tx_ptr, sample_count),
            slice::from_raw_parts(rx_ptr, sample_count),
        )
    };
    looper.process(rx, tx, sample_count, channel_count);
} */