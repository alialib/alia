#ifndef ALIA_UI_UTILITIES_TEXT_HPP
#define ALIA_UI_UTILITIES_TEXT_HPP

#include <alia/ui/internals.hpp>
#include <SkPaint.h>

// This file provides various utilities for working with UTF8 text.

namespace alia {

typedef uint32_t unicode_char_t;

// Get the first character in a Unicode string.
unicode_char_t peek(utf8_string const& text);

// Get a pointer to the next character in a UTF8 string.
utf8_ptr next_utf8_char(utf8_string const& text);

// Is c a whitespace character?
bool is_space(unicode_char_t c);

// Is c a breakable whitespace character?
bool is_breakable_space(unicode_char_t c);

// Is c a line terminator?
bool is_line_terminator(unicode_char_t c);

// Given a text string with a line terminator as its first character, this
// skips over the line terminator. It will treat "\r\n" as a single terminator.
utf8_ptr skip_line_terminator(utf8_string const& text);

// Skip over all whitespace characters in the given text.
// The return value is the first non-whitespace character (or the end of the
// string if it's all whitespace).
utf8_ptr skip_space(utf8_string const& text);

// Get a pointer to the first whitespace character in the given text.
// (If no such character exists, this returns a pointer to the end of the
// text.)
utf8_ptr find_next_space(utf8_string const& text);

// Get a pointer to the first breakable space character in the given text.
// (If no such character exists, this returns a pointer to the end of the
// text.)
utf8_ptr find_next_breakable_space(utf8_string const& text);

// Get a pointer to the beginning of the next word in the given text.
// The beginning of the next word is defined as the first non-space character
// after the first space character.
// (If no such character exists, this returns a pointer to the end of the
// text.)
utf8_ptr find_next_word_start(utf8_string const& text);

// Get a pointer to the beginning of the previous word in the text.
// p is a pointer to the current position in the text.
// text is the full text.
// The beginning of the previous word is defined as the first non-space
// character before p that has a space before it.
// (If no such character exists, this returns a pointer to the start of the
// text.)
utf8_ptr find_previous_word_start(utf8_string const& text, utf8_ptr p);

// Given a string and a position within that string, this returns the word
// that contains that position. If the position is not part of a word, then
// it returns the block of whitespace that contains it instead.
utf8_string get_containing_word(utf8_string const& text, utf8_ptr p);

// Break a string of text so that it fits within the given width.
//
// is_full_line specifies whether or not the given width represents the full
// width of a line. This affects the behavior of the function when the first
// word of text won't fit in the given width.
//
// for_editing specifies whether or not the breaking is being done for editing.
// If not, the function will allow multiple characters worth of white space to
// hang off the end of the line in invisible space.
//
// The return value is a pointer to the first character that didn't fit on the
// line (or the end of the text if the whole string fit).
// *accumulated_width stores the actual width of the text that fit.
// *visible_width stores the width of the text that's actually visible on the
// line.
// *visible_end stores the end of the text that's actually visible on the line.
//
// If ended_on_line_terminator is not 0, it will be set to indicate whether or
// not the last character was a line terminator.
//
utf8_ptr
break_text(
    SkPaint& paint, utf8_string const& text, layout_scalar width,
    bool is_full_line, bool for_editing, layout_scalar* accumulated_width,
    layout_scalar* visible_width, utf8_ptr* visible_end,
    bool* ended_on_line_terminator = 0);

// Calculate the width of the longest word (in pixels) in the given text.
layout_scalar get_longest_word(SkPaint& paint, utf8_string const& text);

}

#endif
