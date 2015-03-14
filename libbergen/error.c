/*
 * libbergen/error.c
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

#include <bergen/error.h>

#include <bergen/libc.h>

struct error *error_create(const char *fmt, ...)
{
	va_list args;
	struct error *err;

	va_start(args, fmt);
	err = error_create_vargs(fmt, args);
	va_end(args);

	return err;
}

struct error *error_create_vargs(const char *fmt, va_list args)
{
	struct error *err = bergen_malloc(sizeof(*err));
	va_list args2;
	int size;

	va_copy(args2, args);
	size = bergen_vsnprintf(NULL, 0, fmt, args2) + 1;
	va_end(args2);

	if (size <= 0) { /* vsnprinf() returns -1 on error, + 1 = 0 */
		err->message = NULL;
		return err;
	} else if (size > ERROR_SHORT_LENGTH) {
		err->message = bergen_malloc(size);
	} else {
		err->message = err->message_short;
	}

	va_copy(args2, args);
	bergen_vsnprintf(err->message, size, fmt, args2);
	va_end(args2);

	return err;
}

void error_free(struct error *err)
{
	if (err->message != err->message_short)
		bergen_free(err->message);
	bergen_free(err);
}
