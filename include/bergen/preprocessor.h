/*
 * include/bergen/preprocessor.h
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

#ifndef BERGEN_PREPROCESSOR_H
#define BERGEN_PREPROCESSOR_H

#include <bergen/error.h>
#include <bergen/libc.h>

struct pp_macro_definition {
	char *name;
	char **args;
	size_t args_buffer_size; /* Number of arguments in buffer */
	size_t num_args;
};

void pp_macro_definition_init(struct pp_macro_definition *macro, const char *name, size_t length, int have_args);

static inline void pp_macro_definition_init_easy(struct pp_macro_definition *macro, const char *name, int have_args)
{
	pp_macro_definition_init(macro, name, bergen_strlen(name), have_args);
}

void pp_macro_definition_destroy(struct pp_macro_definition *macro);

struct error *pp_macro_definition_add_arg(struct pp_macro_definition *macro, const char *name, size_t length);

static inline struct error *pp_macro_definition_add_arg_easy(struct pp_macro_definition *macro, const char *name)
{
	return pp_macro_definition_add_arg(macro, name, bergen_strlen(name));
}

/* Note that this initializes the macro! */
struct error *pp_macro_definition_parse(struct pp_macro_definition *macro, const char *str, size_t length);

#endif /* BERGEN_PREPROCESSOR_H */
