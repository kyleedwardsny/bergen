/*
 * libbergen/preprocessor.c
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

#include <bergen/preprocessor.h>

#include <bergen/libc.h>

void pp_macro_definition_init(struct pp_macro_definition *macro, const char *name, size_t length, int have_args)
{
	macro->name = bergen_strndup_null(name, length);
	macro->num_args = 0;
	if (have_args) {
		macro->args_buffer_size = 32;
		macro->args = bergen_malloc(sizeof(*macro->args) * macro->args_buffer_size);
	} else {
		macro->args_buffer_size = 0;
		macro->args = NULL;
	}
}

void pp_macro_definition_destroy(struct pp_macro_definition *macro)
{
	size_t i;

	for (i = 0; i < macro->num_args; i++)
		bergen_free(macro->args[i]);
	bergen_free(macro->args);
	bergen_free(macro->name);
}

struct error *pp_macro_definition_add_arg(struct pp_macro_definition *macro, const char *name, size_t length)
{
	size_t i;

	if (!macro->args)
		return error_create("Cannot add arguments to a macro that has no arguments");

	for (i = 0; i < macro->num_args; i++) {
		if (length == bergen_strlen(macro->args[i]) && !bergen_strncmp(macro->args[i], name, length))
			return error_create("Argument \"%s\" already exists", macro->args[i]);
	}

	if (macro->num_args >= macro->args_buffer_size) {
		macro->args_buffer_size *= 2;
		macro->args = bergen_realloc(macro->args, sizeof(*macro->args) * macro->args_buffer_size);
	}

	macro->args[macro->num_args++] = bergen_strndup_null(name, length);
	return NULL;
}

struct error *pp_macro_definition_parse(struct pp_macro_definition *macro, const char *str, size_t length)
{
	int have_args;
	size_t name_length, arg_index;
	const char *ptr;
	char *buf;
	struct error *err;

	if ((ptr = bergen_memchr(str, '(', length))) {
		arg_index = ptr - str + 1;
		name_length = arg_index - 1;
		have_args = 1;
	} else {
		have_args = 0;
		name_length = length;
	}

	pp_macro_definition_init(macro, str, name_length, have_args);

	if (have_args) {
		for (;;) {
			if ((ptr = bergen_memchr(str + arg_index, ',', length - arg_index))) {
				pp_macro_definition_add_arg(macro, str + arg_index, ptr - (str + arg_index));
				arg_index = ptr - str + 1;
			} else if ((ptr = bergen_memchr(str + arg_index, ')', length - arg_index))) {
				pp_macro_definition_add_arg(macro, str + arg_index, ptr - (str + arg_index));
				break;
			} else {
				pp_macro_definition_destroy(macro);
				buf = bergen_strndup_null(str, length);
				err = error_create("Invalid macro definition: \"%s\"", buf);
				bergen_free(buf);
				return err;
			}
		}
	}

	return NULL;
}
