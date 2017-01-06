#ifndef SLICE_H
#define SLICE_H

#include <stddef.h>

struct slice {
	void *array;
	size_t len;
	size_t cap;
};

struct slice slice_sub(struct slice buf, size_t a, size_t b);
size_t slice_copy(struct slice dst, struct slice src);

#endif
