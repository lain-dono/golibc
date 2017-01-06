#include "slice.h"
#include <stdint.h>

struct slice slice_sub(struct slice buf, size_t a, size_t b) {
	// XXX maybe asserts?
	buf.array += a;
	buf.cap = (buf.cap < a)? 0: buf.cap - a;
	buf.len = (b < a)? 0: b - a;
	buf.len = (buf.len < buf.cap)? buf.len: buf.cap;
	return buf;
}

size_t slice_copy(struct slice dst, struct slice src) {
	const size_t n = (dst.len < src.len)? dst.len: src.len;
	size_t counter = n;

	// memmove
	uint8_t *d = (uint8_t*)dst.array;
	const uint8_t *s = (const uint8_t*)src.array;
	if (d <= s) {
		while (counter--) {
			*d++ = *s++;
		}
	} else {
		s += n;
		d += n;
		while (counter--) {
			*--d = *--s;
		}
	}
	return n;
}
