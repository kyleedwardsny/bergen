/*
 * test/object.c
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

#include <bergen/object.h>

#include <bergen/libc.h>

START_TEST(test_object_output)
{
	static const uint8_t data1[] = {0xAB, 0xCD, 0xEF, 0x00, 0x12};
	static const uint8_t data2[] = {0x34, 0x56, 0x78, 0x9A, 0xBC};

	struct object_output obj;

	object_output_init(&obj);
	ck_assert_int_eq(obj.address, 0x0000);
	ck_assert_uint_eq(obj.num_segments, 1);
	ck_assert_uint_eq(obj.segments[0].address, 0x0000);
	ck_assert_uint_eq(obj.segments[0].index, 0);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[0]), 0);

	object_output_set_address(&obj, 0x8000);
	ck_assert_int_eq(obj.address, 0x8000);
	ck_assert_int_eq(obj.segments[0].address, 0x8000);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[0]), 0);

	object_output_write(&obj, data1, 5);
	ck_assert_int_eq(obj.address, 0x8005);
	ck_assert_int_eq(obj.segments[0].address, 0x8000);
	ck_assert_int_eq(bergen_memcmp(object_output_get_segment_ptr(&obj, &obj.segments[0]), data1, 5), 0);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[0]), 5);

	object_output_set_address(&obj, 0x4000);
	ck_assert_int_eq(obj.address, 0x4000);
	ck_assert_int_eq(obj.segments[0].address, 0x8000);
	ck_assert_int_eq(obj.segments[1].address, 0x4000);
	ck_assert_uint_eq(obj.segments[1].index, 5);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[0]), 5);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[1]), 0);

	object_output_write(&obj, data2, 5);
	ck_assert_int_eq(obj.address, 0x4005);
	ck_assert_int_eq(obj.segments[1].address, 0x4000);
	ck_assert_uint_eq(obj.segments[1].index, 5);
	ck_assert_int_eq(bergen_memcmp(object_output_get_segment_ptr(&obj, &obj.segments[0]), data1, 5), 0);
	ck_assert_int_eq(bergen_memcmp(object_output_get_segment_ptr(&obj, &obj.segments[1]), data2, 5), 0);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[0]), 5);
	ck_assert_uint_eq(object_output_get_segment_length(&obj, &obj.segments[1]), 5);

	object_output_destroy(&obj);
}
END_TEST

TCase *tcase_object(void)
{
	TCase *tcase = tcase_create("object");

	tcase_add_test(tcase, test_object_output);

	return tcase;
}
