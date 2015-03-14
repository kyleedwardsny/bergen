/*
 * include/bergen/label.h
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

#ifndef BERGEN_LABEL_H
#define BERGEN_LABEL_H

#include <bergen/types.h>

#include <stdlib.h>

struct label {
	char *name;
	expr_value value;
};

struct label_list {
	struct label *labels;
	size_t buffer_size; /* Number of labels in buffer */
	size_t num_labels;
};

void label_init(struct label *label, const char *name, size_t length, expr_value value);

void label_destroy(struct label *label);

void label_list_init(struct label_list *list);

void label_list_destroy(struct label_list *list);

void label_list_append_copy(struct label_list *list, const struct label *label);

void label_list_append(struct label_list *list, const char *name, size_t length, expr_value value);

struct label *label_list_find_label(const struct label_list *list, const char *name, size_t length);

#endif /* BERGEN_LABEL_H */
