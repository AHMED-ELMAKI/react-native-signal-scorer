//! FFI surface for the stateful streaming signal scorer.
//!
//! The core exposes an opaque handle model so the C++ layer can own one
//! scorer per stream. Each handle is independent and concurrency-safe within
//! the producing thread.

pub mod signal_scorer;

use signal_scorer::TickerScorer;
use std::ffi::{c_double, c_int};
use std::ptr;

#[cfg(target_os = "android")]
use std::ffi::{c_char, CString};

#[cfg(target_os = "android")]
extern "C" {
    fn __android_log_write(prio: c_int, tag: *const c_char, text: *const c_char) -> c_int;
}

fn trace(message: &str) {
    #[cfg(target_os = "android")]
    {
        const ANDROID_LOG_DEBUG: c_int = 3;
        const TAG: &[u8] = b"SignalTicker\0";
        let safe = message.replace('\0', "\\0");
        if let Ok(cmsg) = CString::new(safe) {
            unsafe {
                let _ = __android_log_write(
                    ANDROID_LOG_DEBUG,
                    TAG.as_ptr() as *const c_char,
                    cmsg.as_ptr(),
                );
            }
        }
    }

    #[cfg(not(target_os = "android"))]
    {
        eprintln!("[SignalTicker] {message}");
    }
}

/// Opaque pointer to a live [`TickerScorer`].
pub type ScorerHandle = *mut TickerScorer;

/// Creates a new scorer handle. Returns null on allocation failure.
#[no_mangle]
pub extern "C" fn tick_create() -> ScorerHandle {
    trace("tick_create()");
    let scorer = TickerScorer::new();
    Box::into_raw(Box::new(scorer))
}

/// Destroys a scorer handle previously returned by [`tick_create`].
#[no_mangle]
pub extern "C" fn tick_destroy(handle: ScorerHandle) {
    trace("tick_destroy()");
    if !handle.is_null() {
        unsafe {
            let _ = Box::from_raw(handle);
        }
    }
}

/// Feeds one sample into the scorer referenced by `handle`.
#[no_mangle]
pub extern "C" fn tick_ingest(handle: ScorerHandle, sample: c_double) {
    trace(&format!("tick_ingest(handle=?, sample={sample})"));
    if !handle.is_null() {
        unsafe {
            (*handle).ingest(sample);
        }
    }
}

/// Copies the current scored row into `out` (expects capacity >= 6).
/// Returns 1 on success, 0 on failure (null handle / null out).
#[no_mangle]
pub extern "C" fn tick_glimpse(handle: ScorerHandle, out: *mut c_double) -> c_int {
    trace("tick_glimpse()");
    if handle.is_null() || out.is_null() {
        return 0;
    }
    let row = unsafe { (*handle).glimpse().to_row() };
    unsafe {
        ptr::copy_nonoverlapping(row.as_ptr(), out, row.len());
    }
    1
}

/// Clears the scorer's history, returning it to a pristine state.
#[no_mangle]
pub extern "C" fn tick_rewind(handle: ScorerHandle) {
    trace("tick_rewind()");
    if !handle.is_null() {
        unsafe {
            (*handle).rewind();
        }
    }
}
