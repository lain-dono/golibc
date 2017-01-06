#include "utf8.h"

// Code points in the surrogate range are not valid for UTF-8.
#define surrogateMin  0xD800
#define surrogateMax  0xDFFF

#define t1  0x00 // 0000 0000
#define tx  0x80 // 1000 0000
#define t2  0xC0 // 1100 0000
#define t3  0xE0 // 1110 0000
#define t4  0xF0 // 1111 0000
#define t5  0xF8 // 1111 1000

#define maskx  0x3F // 0011 1111
#define mask2  0x1F // 0001 1111
#define mask3  0x0F // 0000 1111
#define mask4  0x07 // 0000 0111

#define rune1Max  (1<<7) - 1
#define rune2Max  (1<<11) - 1
#define rune3Max  (1<<16) - 1

// The default lowest and highest continuation byte.
#define locb  0x80 // 1000 0000
#define hicb  0xBF // 1011 1111

// These names of these constants are chosen to give nice alignment in the
// table below. The first nibble is an index into acceptRanges or F for
// special one-byte cases. The second nibble is the Rune length or the
// Status for the special one-byte case.
#define xx  0xF1 // invalid: size 1
#define as  0xF0 // ASCII: size 1
#define s1  0x02 // accept 0, size 2
#define s2  0x13 // accept 1, size 3
#define s3  0x03 // accept 0, size 3
#define s4  0x23 // accept 2, size 3
#define s5  0x34 // accept 3, size 4
#define s6  0x04 // accept 0, size 4
#define s7  0x44 // accept 4, size 4

// first is information about the first byte in a UTF-8 sequence.
static const uint8_t first[256] = {
	//   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x00-0x0F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x10-0x1F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x20-0x2F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x30-0x3F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x40-0x4F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x50-0x5F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x60-0x6F
	as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x70-0x7F
	//   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x80-0x8F
	xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x90-0x9F
	xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xA0-0xAF
	xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xB0-0xBF
	xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xC0-0xCF
	s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xD0-0xDF
	s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, // 0xE0-0xEF
	s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xF0-0xFF
};

// acceptRange gives the range of valid values for the second byte in a UTF-8 sequence.
struct acceptRange {
	uint8_t lo; // lowest value for second byte.
	uint8_t hi; // highest value for second byte.
};

static const struct acceptRange acceptRanges[] = {
	{locb, hicb}, // 0
	{0xA0, hicb}, // 1
	{locb, 0x9F}, // 2
	{0x90, hicb}, // 3
	{locb, 0x8F}, // 4
};

bool utf8_rune_start(uint8_t b) {
	return (b&0xC0) != 0x80;
}

int utf8_rune_len(rune_t r) {
	if (r < 0)         return -1;
	if (r <= rune1Max) return 1;
	if (r <= rune2Max) return 2;
	if (surrogateMin <= r && r <= surrogateMax)
		return -1;
	if (r <= rune3Max) return 3;
	if (r <= MaxRune)  return 4;
	return -1;
}

bool utf8_full_rune(const uint8_t *p, size_t n) {
	if (n == 0) {
		return false;
	}
	uint8_t x = first[p[0]];
	if (n >= (size_t)(x&7)) {
		return true; // ASCII, invalid or valid.
	}
	// Must be short or invalid.
	struct acceptRange accept = acceptRanges[x>>4];
	if (n > 1) {
		if (p[1] < accept.lo || accept.hi < p[1]) {
			return true;
		} else if (n > 2 && (p[2] < locb || hicb < p[2])) {
			return true;
		}
	}
	return false;
}

rune_t utf8_decode_rune(const uint8_t *p, size_t n, int *size) {
	if (n < 1) {
		*size = 0;
		return RuneError;
	}
	uint8_t p0 = p[0];
	uint8_t x = first[p0];
	if (x >= as) {
		// The following code simulates an additional check for x == xx and
		// handling the ASCII and invalid cases accordingly. This mask-and-or
		// approach prevents an additional branch.
		rune_t mask = (rune_t)(x) << 31 >> 31; // Create 0x0000 or 0xFFFF.
		*size = 1;
		return ((rune_t)(p[0])& (!mask)) | (RuneError&mask);
	}
	uint8_t sz = x & 7;
	struct acceptRange accept = acceptRanges[x>>4];
	if (n < (size_t)(sz)) {
		*size = 1;
		return RuneError;
	}
	uint8_t b1 = p[1];
	if (b1 < accept.lo || accept.hi < b1) {
		*size = 1;
		return RuneError;
	}
	if (sz == 2) {
		*size = 2;
		return (rune_t)(p0&mask2)<<6 | (rune_t)(b1&maskx);
	}
	uint8_t b2 = p[2];
	if (b2 < locb || hicb < b2) {
		*size = 1;
		return RuneError;
	}
	if (sz == 3) {
		*size = 3;
		return (rune_t)(p0&mask3)<<12 | (rune_t)(b1&maskx)<<6 | (rune_t)(b2&maskx);
	}
	uint8_t b3 = p[3];
	if (b3 < locb || hicb < b3) {
		*size = 1;
		return RuneError;
	}
	*size = 4;
	return (rune_t)(p0&mask4)<<18 | (rune_t)(b1&maskx)<<12 | (rune_t)(b2&maskx)<<6 | (rune_t)(b3&maskx);
}

rune_t utf8_decode_last_rune(const uint8_t *p, ptrdiff_t end, int *size) {
	if (end == 0) {
		*size = 0;
		return RuneError;
	}
	ptrdiff_t start = end - 1;
	rune_t r = (rune_t)(p[start]);
	if (r < RuneSelf) {
		r = 1;
		return r;
	}
	// guard against O(n^2) behavior when traversing
	// backwards through strings with long sequences of
	// invalid UTF-8.
	ptrdiff_t lim = end - UTFMax;
	if (lim < 0) {
		lim = 0;
	}
	for (start--; start >= lim; start--) {
		if (utf8_rune_start(p[start])) {
			break;
		}
	}
	if (start < 0) {
		start = 0;
	}
	r = utf8_decode_rune(p + start, end - start, size);
	if (start+*size != end) {
		*size = 1;
		return RuneError;
	}
	return r;
}

int utf8_encode_rune(uint8_t *p, rune_t r) {
	// Negative values are erroneous.
	// Making it unsigned addresses the problem.
	uint32_t i = r;
	if (i <= rune1Max) {
		p[0] = (uint8_t)(r);
		return 1;
	}
	if (i <= rune2Max) {
		p[0] = t2 | (uint8_t)(r>>6);
		p[1] = tx | ((uint8_t)(r)&maskx);
		return 2;
	}
	if (i > MaxRune || (surrogateMin <= i && i <= surrogateMax)) {
		r = RuneError;
		goto next;
	}
	if (i <= rune3Max) {
next:
		p[0] = t3 | (uint8_t)(r>>12);
		p[1] = tx | ((uint8_t)(r>>6)&maskx);
		p[2] = tx | ((uint8_t)(r)&maskx);
		return 3;
	}

	p[0] = t4 | (uint8_t)(r>>18);
	p[1] = tx | ((uint8_t)(r>>12)&maskx);
	p[2] = tx | ((uint8_t)(r>>6)&maskx);
	p[3] = tx | ((uint8_t)(r)&maskx);
	return 4;
}

size_t utf8_rune_count(const uint8_t *p, size_t np) {
	size_t n = 0;
	for (size_t i = 0; i < np;) {
		n++;
		uint8_t c = p[i];
		if (c < RuneSelf) {
			// ASCII fast path
			i++;
			continue;
		}
		uint8_t x = first[c];
		if (x == xx) {
			i++; // invalid.
			continue;
		}
		uint8_t size = (size_t)(x & 7);
		if (i+size > np) {
			i++; // Short or invalid.
			continue;
		}
		struct acceptRange accept = acceptRanges[x>>4];
		if (p[i+1] < accept.lo || accept.hi < p[i+1]) {
			size = 1;
		} else if (size == 2) {
			// pass
		} else if (p[i+2] < locb || hicb < p[i+2]) {
			size = 1;
		} else if (size == 3) {
		} else if (p[i+3] < locb || hicb < p[i+3]) {
			size = 1;
		}
		i += size;
	}
	return n;
}

bool utf8_valid(const uint8_t *p, size_t n) {
	for (size_t i = 0; i < n;) {
		uint8_t pi = p[i];
		if (pi < RuneSelf) {
			i++;
			continue;
		}
		uint8_t x = first[pi];
		if (x == xx) {
			return false; // Illegal starter byte.
		}
		size_t size = (size_t)(x & 7);
		if (i+size > n) {
			return false; // Short or invalid.
		}
		struct acceptRange accept = acceptRanges[x>>4];
		if (p[i+1] < accept.lo || accept.hi < p[i+1]) {
			return false;
		} else if (size == 2) {
			// pass
		} else if (p[i+2] < locb || hicb < p[i+2]) {
			return false;
		} else if (size == 3) {
			// pass
		} else if (p[i+3] < locb || hicb < p[i+3]) {
			return false;
		}
		i += size;
	}
	return true;
}

bool utf8_valid_rune(rune_t r) {
	if (r < 0)
		return false;
	if (surrogateMin <= r && r <= surrogateMax)
		return false;
	if (r > MaxRune)
		return false;
	return true;
}
