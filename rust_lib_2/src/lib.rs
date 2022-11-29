#![no_std]
#![feature(alloc_error_handler)]

extern crate alloc;
mod c_allocator;
use alloc::{boxed::Box, vec};
use c_allocator::CAllocator;
use core::{panic::PanicInfo, alloc::Layout};

#[no_mangle]
pub unsafe fn __aeabi_unwind_cpp_pr0() -> () {
  loop {}
}

#[global_allocator]
static A: CAllocator = CAllocator;

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    loop {}
}

#[alloc_error_handler]
fn alloc_error(layout: Layout) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn rust_test_fn_2() -> u32 {
    let v = vec![10_f32; 1000];
    return 1234567
}