// Adapted from react-native-random-forest (https://github.com/tony-div/react-native-random-forest)
// by Tony George. See README "Attribution" section.
pub mod signal_scorer;

use signal_scorer::score_window;
use std::collections::HashMap;
use std::ffi::{c_char, c_double, c_int, CStr, CString};
use std::sync::{Mutex, OnceLock};

#[cfg(target_os = "android")]
extern "C" {
    fn __android_log_write(prio: c_int, tag: *const c_char, text: *const c_char) -> c_int;
}

fn log_debug(message: &str) {
    #[cfg(target_os = "android")]
    {
        const ANDROID_LOG_DEBUG: c_int = 3;
        const TAG: &[u8] = b"NitroSignalScorer\0";
        let safe_message = message.replace('\0', "\\0");
        if let Ok(c_message) = CString::new(safe_message) {
            unsafe {
                let _ = __android_log_write(
                    ANDROID_LOG_DEBUG,
                    TAG.as_ptr() as *const c_char,
                    c_message.as_ptr(),
                );
            }
        }
    }

    #[cfg(not(target_os = "android"))]
    {
        eprintln!("[NitroSignalScorer] {message}");
    }
}

const DEFAULT_REGISTRY: &str = "default";

/// A registry holds independent scoring contexts. This is mostly a placeholder to
/// mirror the random-forest package's named-model design; the scoring algorithm
/// itself is stateless, so the registry is purely organizational.
struct RegistryState {
    registries: HashMap<String, ()>,
}

impl Default for RegistryState {
    fn default() -> Self {
        Self { registries: HashMap::new() }
    }
}

fn state() -> &'static Mutex<RegistryState> {
    static STATE: OnceLock<Mutex<RegistryState>> = OnceLock::new();
    STATE.get_or_init(|| Mutex::new(RegistryState::default()))
}

fn cstr_to_string(ptr: *const c_char) -> Option<String> {
    if ptr.is_null() {
        return None;
    }
    let cstr = unsafe { CStr::from_ptr(ptr) };
    Some(cstr.to_string_lossy().into_owned())
}

fn registry_name_or_default(ptr: *const c_char) -> String {
    cstr_to_string(ptr)
        .filter(|s| !s.is_empty())
        .unwrap_or_else(|| DEFAULT_REGISTRY.to_string())
}

fn ensure_registry(name: &str) {
    if let Ok(mut guard) = state().lock() {
        guard.registries.entry(name.to_string()).or_insert(());
    }
}

fn boxed_vec(v: Vec<f64>) -> (*mut c_double, c_int) {
    if v.is_empty() {
        return (std::ptr::null_mut(), 0);
    }
    let len = v.len() as c_int;
    let mut boxed = v.into_boxed_slice();
    let ptr = boxed.as_mut_ptr();
    std::mem::forget(boxed);
    (ptr, len)
}

#[no_mangle]
pub extern "C" fn ss_evaluate(data: *const c_double, len: c_int, out_len: *mut c_int) -> *mut c_double {
    log_debug(&format!("ss_evaluate(): len={len}"));
    if data.is_null() || len <= 0 {
        if !out_len.is_null() {
            unsafe { *out_len = 0; }
        }
        return std::ptr::null_mut();
    }
    let slice = unsafe { std::slice::from_raw_parts(data, len as usize) };
    let result = score_window(slice).flatten();
    let (ptr, out) = boxed_vec(result);
    if !out_len.is_null() {
        unsafe { *out_len = out; }
    }
    ptr
}

#[no_mangle]
pub extern "C" fn ss_evaluate_batch(
    data: *const c_double,
    len: c_int,
    window_size: c_int,
    out_len: *mut c_int,
) -> *mut c_double {
    log_debug(&format!("ss_evaluate_batch(): len={len} window={window_size}"));
    if data.is_null() || len <= 0 || window_size <= 0 {
        if !out_len.is_null() {
            unsafe { *out_len = 0; }
        }
        return std::ptr::null_mut();
    }
    let slice = unsafe { std::slice::from_raw_parts(data, len as usize) };
    let result = signal_scorer::score_batch(slice, window_size as usize);
    let (ptr, out) = boxed_vec(result);
    if !out_len.is_null() {
        unsafe { *out_len = out; }
    }
    ptr
}

#[no_mangle]
pub extern "C" fn ss_reset(registry_name: *const c_char) {
    let name = registry_name_or_default(registry_name);
    log_debug(&format!("ss_reset({name}): begin"));
    if let Ok(mut guard) = state().lock() {
        guard.registries.remove(&name);
    }
    log_debug(&format!("ss_reset({name}): complete"));
}

// Named registry variants (kept for parity with the random-forest package's
// named-model API, and to make the registry actually meaningful). The algorithm
// is stateless, so these are just organizational bookkeeping.

#[no_mangle]
pub extern "C" fn ss_ensure_registry(registry_name: *const c_char) {
    let name = registry_name_or_default(registry_name);
    ensure_registry(&name);
}

#[no_mangle]
pub extern "C" fn ss_free_data(ptr: *mut c_double) {
    if !ptr.is_null() {
        unsafe {
            let _ = Box::from_raw(std::slice::from_raw_parts_mut(ptr, 0));
        }
    }
}
