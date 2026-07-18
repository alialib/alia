#ifndef ALIA_ABI_KERNEL_SIGNAL_H
#define ALIA_ABI_KERNEL_SIGNAL_H

#include <alia/abi/kernel/ids.h>
#include <alia/abi/prelude.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

ALIA_EXTERN_C_BEGIN

// Signals carry reactive values between the app and its components. Two shapes
// coexist here:
//   - Bidirectional signals such as `alia_bool_signal` carry a value plus
//     `alia_signal_flags` describing readability/writability. Writing back is
//     trivial for such values, so a single flags-based type covers both
//     directions.
//   - Read-only input signals such as `alia_text_input_signal` carry a payload
//     plus a `value_id` change token (used for caching). They exist where a
//     value only flows into components and where a bidirectional form would be
//     a structurally different type (e.g. editable text). By convention,
//     `alia_X_signal` is the full/duplex-capable form and
//     `alia_X_input_signal` is the read-only input form.

typedef uint32_t alia_signal_flags;

#define ALIA_SIGNAL_READABLE (1u << 0)
#define ALIA_SIGNAL_WRITABLE (1u << 1)
#define ALIA_SIGNAL_WRITTEN (1u << 2)

typedef struct alia_bool_signal
{
    alia_signal_flags flags;
    bool value;
} alia_bool_signal;

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
