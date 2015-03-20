/*
 * include/bergen/object.h
 * Copyright (C) 2015 Kyle Edwards <kyleedwardsny@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef BERGEN_OBJECT_H
#define BERGEN_OBJECT_H

#include <stdlib.h>

#include <bergen/types.h>

struct object_segment {
	expr_value address;
	size_t index;
};

struct object_output {
	void *buffer;
	size_t buffer_size;
	struct object_segment *segments;
	size_t segment_buffer_size;
	size_t num_segments;
	expr_value address;
};

void object_output_init(struct object_output *obj);

void object_output_destroy(struct object_output *obj);

void object_output_set_address(struct object_output *obj, expr_value address);

void object_output_write(struct object_output *obj, const void *mem, size_t length);

static inline void *object_output_get_segment_ptr(const struct object_output *obj, const struct object_segment *segment)
{
	return (char *) obj->buffer + segment->index;
}

static inline size_t object_output_get_segment_length(const struct object_output *obj, const struct object_segment *segment)
{
	if (segment == &obj->segments[obj->num_segments - 1])
		return obj->address - segment->address;
	return (segment + 1)->index - segment->index;
}

#endif /* BERGEN_OBJECT_H */
