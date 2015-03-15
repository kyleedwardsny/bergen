/*
 * include/bergen/libc.h
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

#ifndef BERGEN_LIBC_H
#define BERGEN_LIBC_H

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ctype.h */
#define bergen_isspace		isspace

/* stdio.h */
#define bergen_vsnprintf	vsnprintf

/* stdlib.h */
#define bergen_free		free
#define bergen_malloc		malloc
#define bergen_realloc		realloc
#define bergen_strtoll		strtoll

/* string.h */
#define bergen_memcpy		memcpy
#define bergen_strchr		strchr
#define bergen_strcpy		strcpy
#define bergen_strlen		strlen
#define bergen_strncmp		strncmp
#define bergen_strncpy		strncpy
char *bergen_strdup(const char *s);
char *bergen_strndup(const char *s, size_t n);
char *bergen_strndup_null(const char *s, size_t n); /* Puts null terminator at the end */

#endif /* BERGEN_LIBC_H */
