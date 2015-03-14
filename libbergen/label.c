/*
 * libbergen/label.c
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

#include <bergen/label.h>

#include <bergen/libc.h>

void label_init(struct label *label, const char *name, size_t length, expr_value value)
{
	label->name = bergen_strndup_null(name, length);
	label->value = value;
}

void label_destroy(struct label *label)
{
	bergen_free(label->name);
}

void label_list_init(struct label_list *list)
{
	list->labels = bergen_malloc(sizeof(*list->labels) * 32);
	list->buffer_size = 32;
	list->num_labels = 0;
}

void label_list_destroy(struct label_list *list)
{
	bergen_free(list->labels);
}

void label_list_append_copy(struct label_list *list, const struct label *label)
{
	size_t length = bergen_strlen(label->name);
	label_list_append(list, label->name, length, label->value);
}

void label_list_append(struct label_list *list, const char *name, size_t length, expr_value value)
{
	struct label *ptr;

	if (list->num_labels >= list->buffer_size) {
		list->buffer_size *= 2;
		list->labels = bergen_realloc(list->labels, sizeof(*list->labels) * list->buffer_size);
	}

	ptr = &list->labels[list->num_labels++];
	ptr->name = bergen_strndup_null(name, length);
	ptr->value = value;
}
