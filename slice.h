#ifndef SLICE_H
#define SLICE_H

#include <stddef.h>

typedef struct slice {
	void *array;
	size_t len;
	size_t cap;
} slice_t;

slice_t slice_sub(slice_t buf, size_t a, size_t b);
size_t slice_copy(slice_t dst, slice_t src);

#endif
