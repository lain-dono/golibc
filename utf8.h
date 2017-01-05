#ifndef UTF8_H
#define UTF8_H

// Package utf8 implements functions and constants to support text encoded in UTF-8.
// It includes functions to translate between runes and UTF-8 byte sequences.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define RUNE_C(x) INT32_C(x)
typedef int32_t rune_t;

// The conditions RuneError==unicode.ReplacementChar and
// MaxRune==unicode.MaxRune are verified in the tests.
// Defining them locally avoids this package depending on package unicode.

// Numbers fundamental to the encoding.
//
// the "error" Rune or "Unicode replacement character"
#define RuneError 0xFFFD
// characters below Runeself are represented as themselves in a single byte.
#define RuneSelf  0x80
// Maximum valid Unicode code point.
#define MaxRune   0x10FFFF
// maximum number of bytes of a UTF-8 encoded Unicode character.
#define UTFMax    4

// utf8_full_rune reports whether the bytes in p begin
// with a full UTF-8 encoding of a rune.
// An invalid encoding is considered a full Rune
// since it will convert as a width-1 error rune.
bool utf8_full_rune(const uint8_t *p, size_t n);

// utf8_decode_rune unpacks the first UTF-8 encoding
// in p and returns the rune and its width in bytes.
// If p is empty it returns (RuneError, 0).
// Otherwise, if the encoding is invalid, it returns (RuneError, 1).
// Both are impossible results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
rune_t utf8_decode_rune(const uint8_t *p, size_t n, int *size);

// utf8_rune_start reports whether the byte could be the first byte of an encoded,
// possibly invalid rune. Second and subsequent bytes always have the top two bits set to 10.
bool utf8_rune_start(uint8_t b);

// utf8_decode_last_rune unpacks the last UTF-8 encoding in p
// and returns the rune and its width in bytes.
// If p is empty it returns (RuneError, 0).
// Otherwise, if the encoding is invalid, it returns (RuneError, 1).
// Both are impossible results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value.
// No other validation is performed.
rune_t utf8_decode_last_rune(const uint8_t *p, size_t end, int *size);

// utf8_rune_len returns the number of bytes required to encode the rune.
// It returns -1 if the rune is not a valid value to encode in UTF-8.
int utf8_rune_len(rune_t r);

// utf8_encode_rune writes into p (which must be large enough)
// the UTF-8 encoding of the rune.
// It returns the number of bytes written.
int utf8_encode_rune(uint8_t *p, rune_t r);

// utf8_rune_count returns the number of runes in p.
// Erroneous and short encodings are treated as single runes of width 1 byte.
size_t utf8_rune_count(const uint8_t *p, size_t np);

// utf8_valid reports whether b consists entirely of valid UTF-8-encoded runes.
bool utf8_valid(const uint8_t *p, size_t n);

// utf8_valid_rune reports whether r can be legally encoded as UTF-8.
// Code points that are out of range or a surrogate half are illegal.
bool utf8_valid_rune(rune_t r);

#endif
