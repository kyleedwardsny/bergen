/*
 * libbergen/object.c
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

#include <bergen/object.h>

#include <bergen/libc.h>

void object_output_init(struct object_output *obj)
{
	obj->buffer_size = 128;
	obj->buffer = bergen_malloc(sizeof(char) * obj->buffer_size);
	obj->segment_buffer_size = 32;
	obj->segments = bergen_malloc(sizeof(*obj->segments) * obj->segment_buffer_size);
	obj->address = 0;

	obj->num_segments = 1;
	obj->segments[0].address = 0;
	obj->segments[0].index = 0;
}

void object_output_destroy(struct object_output *obj)
{
	bergen_free(obj->segments);
	bergen_free(obj->buffer);
}

void object_output_set_address(struct object_output *obj, expr_value address)
{
	struct object_segment *last_segment = &obj->segments[obj->num_segments - 1];
	size_t next_index;

	if (obj->address == last_segment->address) { /* Reuse last segment if possible */
		obj->address = last_segment->address = address;
		return;
	}

	if (obj->num_segments >= obj->segment_buffer_size) {
		obj->segment_buffer_size *= 2;
		obj->segments = bergen_realloc(obj->segments, sizeof(*obj->segments) * obj->segment_buffer_size);
	}

	next_index = obj->address - last_segment->address + last_segment->index;
	last_segment = &obj->segments[obj->num_segments++];
	last_segment->index = next_index;
	obj->address = last_segment->address = address;
}

void object_output_write(struct object_output *obj, const void *mem, size_t length)
{
	struct object_segment *last_segment = &obj->segments[obj->num_segments - 1];
	size_t total_size = obj->address - last_segment->address + last_segment->index;
	int too_small = 0;

	while (total_size + length > obj->buffer_size) {
		too_small = 1;
		obj->buffer_size *= 2;
	}
	if (too_small)
		obj->buffer = bergen_realloc(obj->buffer, sizeof(char) * obj->buffer_size);

	bergen_memcpy((char *) obj->buffer + total_size, mem, length);
	obj->address += length;
}
