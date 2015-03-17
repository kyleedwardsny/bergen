/*
 * test/parse.c
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

#include "tests.h"

#include <bergen/parse.h>

#include <bergen/libc.h>

static void do_parse_string(const char *str, void *buf, size_t buf_length, size_t expected_result, size_t expected_buf_length, const void *expected_buf)
{
	if (buf)
		bergen_memset(buf, 0, buf_length);

	ck_assert_uint_eq(parse_string_data(str, bergen_strlen(str), buf, &buf_length), expected_result);
	ck_assert_uint_eq(buf_length, expected_buf_length);
	if (buf && expected_result != 0)
		ck_assert_int_eq(bergen_memcmp(buf, expected_buf, buf_length), 0);
}

START_TEST(test_parse_string)
{
	void *buf = bergen_malloc(128);

	do_parse_string("\"string\"", buf, 128, 8, 6, "string");
	do_parse_string("\"string\\n\"", buf, 128, 10, 7, "string\n");
	do_parse_string("\"string\\r\"", buf, 128, 10, 7, "string\r");
	do_parse_string("\"string\\b\"", buf, 128, 10, 7, "string\b");
	do_parse_string("\"string\\t\"", buf, 128, 10, 7, "string\t");
	do_parse_string("\"string\\f\"", buf, 128, 10, 7, "string\f");
	do_parse_string("\"string\\\\\"", buf, 128, 10, 7, "string\\");
	do_parse_string("\"string\\\"\"", buf, 128, 10, 7, "string\"");
	do_parse_string("\"string\\\"string2\"", buf, 128, 17, 14, "string\"string2");
	do_parse_string("\"\\123\"", buf, 128, 6, 1, "\123");
	do_parse_string("\"string\"", NULL, 128, 8, 0, NULL);
	do_parse_string("\"string\" ", NULL, 128, 8, 0, NULL);
	do_parse_string("\"string", NULL, 128, 0, 0, NULL);
	do_parse_string("\"\\", NULL, 128, 0, 0, NULL);
	do_parse_string("\"\\1", NULL, 128, 0, 0, NULL);
	do_parse_string("\"\\12", NULL, 128, 0, 0, NULL);
	do_parse_string("\"\\123", NULL, 128, 0, 0, NULL);
	do_parse_string("\\123", NULL, 128, 0, 0, NULL);
	do_parse_string("string", NULL, 128, 0, 0, NULL);
	do_parse_string("string", buf, 128, 0, 0, NULL);

	bergen_free(buf);
}
END_TEST

TCase *tcase_parse(void)
{
	TCase *tcase = tcase_create("parse");

	tcase_add_test(tcase, test_parse_string);

	return tcase;
}
