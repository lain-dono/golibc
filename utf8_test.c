#include "utf8.h"
#include <stdio.h>
#include <assert.h>

#define errorf(...) fprintf(stderr, __VA_ARGS__)
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define UNUSED(v) (void)(v)

#define str(var, lit) const uint8_t var[sizeof(lit)-1] = lit;

////////////////////////////////////////////////////////////////////////////////
//

void TestRuneCount() {
#define rune_count(n) assert(n == utf8_rune_count(&b[0], sizeof(b)));
	{str(b, "abcd");rune_count(4)}
	{str(b, "☺☻☹");rune_count(3)}
	{str(b, "1,2,3,4");rune_count(7)}
	{str(b, "\xe2\x00");rune_count(2)}
	{str(b, "\xe2\x80");rune_count(2)}
	{str(b, "a\xe2\x80");rune_count(3)}
}

void TestRuneLen() {
#define rune_len(r, size) assert(size == utf8_rune_len(r));
	rune_len(0, 1);
	rune_len(RUNE_C('e'), 1);
	rune_len(RUNE_C('é'), 2);
	rune_len(RUNE_C('☺'), 3);
	rune_len(RuneError, 3);
	rune_len(MaxRune, 4);
	rune_len(0xD800, -1);
	rune_len(0xDFFF, -1);
	rune_len(MaxRune + 1, -1);
	rune_len(-1, -1);
}

void TestValid() {
#define valid_sizeof(lit, ok) {\
	rune_t s[sizeof(lit) - 1] = L ## lit; \
	const uint8_t *p = (const uint8_t*)&s; \
		if (ok != utf8_valid(p, sizeof(s))) { \
			errorf("fail valid at %d\n", __LINE__); \
		} \
	}

	valid_sizeof("", true);
	valid_sizeof("a", true);
	valid_sizeof("abc", true);
	valid_sizeof("Ж", true);
	valid_sizeof("ЖЖ", true);
	valid_sizeof("брэд-ЛГТМ", true);
	valid_sizeof("☺☻☹", true);
	valid_sizeof("aa\xe2", false);

#define valid_true() assert(true == utf8_valid(&b[0], sizeof(b)));
#define valid_false() assert(false == utf8_valid(&b[0], sizeof(b)));
	{
		const uint8_t b[] = {66, 250};
		valid_false();
	}
	{
		const uint8_t b[] = {66, 250, 67};
		valid_false();
	}

	// TODO
	/*valid_sizeof("a\uFFFDb", true);*/

	{ // U+10FFFF
		const uint8_t b[] = {0xF4, 0x8F, 0xBF, 0xBF};
		valid_true();
	}
	{ // U+10FFFF+1; out of range
		const uint8_t b[] = {0xF4, 0x90, 0x80, 0x80};
		valid_false();
	}
	{ // 0x1FFFFF; out of range
		const uint8_t b[] = {0xF7, 0xBF, 0xBF, 0xBF};
		valid_false();
	}
	{ // 0x3FFFFFF; out of range
		const uint8_t b[] = {0xFB, 0xBF, 0xBF, 0xBF, 0xBF};
		valid_false();
	}
	{ // U+0000 encoded in two bytes: incorrect
		const uint8_t b[] = {0xC0, 0x80};
		valid_false();
	}
	{ // U+D800 high surrogate (sic)
		const uint8_t b[] = {0xED, 0xA0, 0x80};
		valid_false();
	}
	{ // U+DFFF low surrogate (sic)
		const uint8_t b[] = {0xED, 0xBF, 0xBF};
		valid_false();
	}
}

struct ValidRuneTest {
	rune_t r;
	bool ok;
};

static const struct ValidRuneTest validrunetests[] = {
	{0, true},
	{RUNE_C('e'), true},
	{RUNE_C('é'), true},
	{RUNE_C('☺'), true},
	{RuneError, true},
	{MaxRune, true},
	{0xD7FF, true},
	{0xD800, false},
	{0xDFFF, false},
	{0xE000, true},
	{MaxRune + 1, false},
	{-1, false},
};

void TestValidRune() {
	for (size_t i = 0; i < COUNT_OF(validrunetests); ++i) {
		struct ValidRuneTest tt = validrunetests[i];
		bool ok = utf8_valid_rune(tt.r);
		if (ok != tt.ok) {
			errorf("ValidRune(%x) = %d, want %d\n", tt.r, ok, tt.ok);
		}
	}
}

int main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	printf("start\n");

	TestRuneCount();
	TestRuneLen();
	TestValid();
	TestValidRune();

	printf("end\n");
	return 0;
}
