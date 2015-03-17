/*
 * libbergen/parse.c
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

#include <bergen/parse.h>

#include <bergen/libc.h>

static void put_char(void *buf, size_t buf_length, size_t *output_length, uint8_t c)
{
	if (buf && buf_length > *output_length) {
		*((uint8_t *) buf + *output_length) = c;
		(*output_length)++;
	}
}

size_t parse_string_data(const char *str, size_t length, void *buf, size_t *buf_length)
{
	size_t old_buf_length = *buf_length;
	size_t i;
	int backslash = 0;
	char c;
	uint8_t octal_tmp;

	*buf_length = 0;
	if (length < 2 || str[0] != '"')
		return 0;

	for (i = 1; i < length; i++) {
		c = str[i];
		if (backslash == 0) {
			if (c == '\\') {
				backslash = 1;
			} else if (c == '"') {
				return i + 1;
			} else {
				put_char(buf, old_buf_length, buf_length, c);
			}
		} else if (backslash == 1) {
			if (bergen_strchr("01234567", c)) {
				backslash = 2;
				octal_tmp = (c - '0') << 6;
			} else {
				switch (c) {
				case 'n': /* Line feed */
					put_char(buf, old_buf_length, buf_length, '\n');
					backslash = 0;
					break;

				case 'r': /* Carriage return */
					put_char(buf, old_buf_length, buf_length, '\r');
					backslash = 0;
					break;

				case 'b': /* Backspace */
					put_char(buf, old_buf_length, buf_length, '\b');
					backslash = 0;
					break;

				case 't': /* Tab */
					put_char(buf, old_buf_length, buf_length, '\t');
					backslash = 0;
					break;

				case 'f': /* Formfeed */
					put_char(buf, old_buf_length, buf_length, '\f');
					backslash = 0;
					break;

				case '\\': /* Backslash */
					put_char(buf, old_buf_length, buf_length, '\\');
					backslash = 0;
					break;

				case '"': /* Quote */
					put_char(buf, old_buf_length, buf_length, '"');
					backslash = 0;
					break;

				default: /* Unrecognized escape */
					return 0;
				}
			}
		} else if (backslash == 2) {
			if (bergen_strchr("01234567", c)) {
				backslash = 3;
				octal_tmp |= (c - '0') << 3;
			} else {
				return 0;
			}
		} else if (backslash == 3) {
			if (bergen_strchr("01234567", c)) {
				backslash = 0;
				octal_tmp |= (c - '0') << 0;
				put_char(buf, old_buf_length, buf_length, octal_tmp);
			} else {
				return 0;
			}
		}
	}

	return 0; /* No end quote */
}
