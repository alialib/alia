#ifndef ALIA_ABI_KERNEL_SIGNAL_H
#define ALIA_ABI_KERNEL_SIGNAL_H

#include <alia/abi/kernel/ids.h>
#include <alia/abi/prelude.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

ALIA_EXTERN_C_BEGIN

// This file defines the low-level signal API for the Alia kernel.
//
// Signals carry reactive values between the app and its components.
//
// These are ephemeral structures that are meant to be constructed at a
// component call site specifically for the purpose of communicating with that
// component.
//
// Signals types can be bidirectional or input-only. Components choose the type
// that matches their interface requirements. Bidirectional signal types have
// the form `alia_X_signal`, and input-only signal types have the form
// `alia_X_input_signal`.
//
// Signals also allow run-time specification of their I/O capabilities. This
// allows the app to communicate its own data-flow restrictions. e.g.:
// - A signal value comes from a user input that hasn't been filled yet.
// - A signal value is still being computed.
// - A signal is still writing its last update and isn't ready for more.
// These restrictions are communicated via the `flags` field.
//
// When a component wants to write a value back to the app via a signal, it
// sets the `flags` field to `ALIA_SIGNAL_WRITTEN`. For signals with fixed-size
// values, the value is then written directly back into the signal structure.
// Note that since Alia passes are mutation-free, it is then the responsibility
// of the caller to translate this into an effect and post it.
//
// For large values, the signal also carries a `value_id`.
// These can be used in place of the signal value to track changes in the
// value. (The `value_id` must change when the value changes.)
//

typedef uint32_t alia_signal_flags;

#define ALIA_SIGNAL_READABLE (1u << 0)
#define ALIA_SIGNAL_WRITABLE (1u << 1)
#define ALIA_SIGNAL_WRITTEN (1u << 2)

typedef struct alia_bool_signal
{
    alia_signal_flags flags;
    bool value;
} alia_bool_signal;

typedef struct alia_double_signal
{
    alia_signal_flags flags;
    double value;
} alia_double_signal;

// TEXT INPUT SIGNAL

// A read-only text input signal: a run of UTF-8 bytes plus a `value_id` that
// changes whenever the bytes change. Consumers key their prepared/shaped-text
// cache on `value_id` (not on the bytes), so `value_id` must change when - and
// only when - the content changes. It does NOT need to be globally unique:
// each piece of text is identified by its call site, so `value_id` is just a
// per-site change token (e.g. a revision counter that the app bumps on edit).
// See the constructors below for the common cases.

// `length` sentinel meaning "text is null-terminated; the length is unknown".
// Consumers resolve the actual length lazily - only when they must (re)prepare
// the text - so the steady-state (unchanged) path never scans the bytes.
#define ALIA_TEXT_LENGTH_NULL_TERMINATED ((size_t) -1)

typedef struct alia_text_input_signal
{
    char const* text;
    // byte length, or `ALIA_TEXT_LENGTH_NULL_TERMINATED` if null-terminated
    size_t length;
    alia_id_view value_id;
} alia_text_input_signal;

// Construct a text input signal from explicit bytes plus a caller-supplied
// value ID.
static inline alia_text_input_signal
alia_text_input_signal_make(
    char const* text, size_t length, alia_id_view value_id)
{
    alia_text_input_signal s;
    s.text = text;
    s.length = length;
    s.value_id = value_id;
    return s;
}

// Construct a text input signal for an immutable string literal (or any string
// whose address is stable and whose contents never change). The pointer itself
// is used as the value ID, so the cache correctly invalidates if a different
// literal appears at the same call site. The length is left unknown (the
// string is treated as null-terminated) so that no `strlen` runs on the common
// cached path - the length is resolved only when the text actually changes.
static inline alia_text_input_signal
alia_text_literal(char const* text)
{
    return alia_text_input_signal_make(
        text,
        ALIA_TEXT_LENGTH_NULL_TERMINATED,
        alia_id_view_make_string_literal(text));
}

// Construct a text input signal from bytes tagged with a revision counter. The
// app bumps `revision` whenever the referenced bytes change.
static inline alia_text_input_signal
alia_text_revision(char const* text, size_t length, uint64_t revision)
{
    return alia_text_input_signal_make(
        text, length, alia_id_view_make_u64(revision));
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_SIGNAL_H */
