/*
 * test/expr_evaluate.c
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

#include <bergen/expression.h>

static void ck_assert_constant_eq(const char *str, expr_value expected_result)
{
	struct expr_data expr;
	expr_value result = ~expected_result; /* Make sure they start off different */
	struct error *err;

	expr_data_init_easy(&expr, str, '_');
	err = expr_evaluate(&expr, &result);
	ck_assert(!err);
	ck_assert_int_eq(result, expected_result);
	expr_data_destroy(&expr);
}

static void ck_assert_constant_invalid(const char *str)
{
	struct expr_data expr;
	expr_value result;
	struct error *err;

	expr_data_init_easy(&expr, str, '_');
	err = expr_evaluate(&expr, &result);
	ck_assert(!!err);
	error_free(err);
	expr_data_destroy(&expr);
}

START_TEST(test_binary_constant)
{
	ck_assert_constant_eq("%10101010", 0xAA);
	ck_assert_constant_eq("10101010b", 0xAA);
	ck_assert_constant_eq("10101010B", 0xAA);
	ck_assert_constant_eq("%11000011", 0xC3);
	ck_assert_constant_invalid("%11201100");
	ck_assert_constant_invalid("%11f01100");
}
END_TEST

START_TEST(test_octal_constant)
{
	ck_assert_constant_eq("@12345", 012345);
	ck_assert_constant_eq("12345O", 012345);
	ck_assert_constant_eq("12345o", 012345);
	ck_assert_constant_eq("67o", 067);
	ck_assert_constant_invalid("@67890");
	ck_assert_constant_invalid("@67a");
}
END_TEST

START_TEST(test_decimal_constant)
{
	ck_assert_constant_eq("12345", 12345);
	ck_assert_constant_eq("12345D", 12345);
	ck_assert_constant_eq("12345d", 12345);
	ck_assert_constant_eq("67890", 67890);
	ck_assert_constant_invalid("123a5");
}
END_TEST

START_TEST(test_hexadecimal_constant)
{
	ck_assert_constant_eq("$12345", 0x12345);
	ck_assert_constant_eq("12345H", 0x12345);
	ck_assert_constant_eq("12345h", 0x12345);
	ck_assert_constant_eq("$CAFEBABE", 0xCAFEBABE);
	ck_assert_constant_eq("$cafebabe", 0xCAFEBABE);
	ck_assert_constant_eq("$deadbeef", 0xDEADBEEF);
	ck_assert_constant_invalid("$garbage");
}
END_TEST

TCase *tcase_expr_evaluate(void)
{
	TCase *tcase = tcase_create("expr_evaluate");

	tcase_add_test(tcase, test_binary_constant);
	tcase_add_test(tcase, test_octal_constant);
	tcase_add_test(tcase, test_decimal_constant);
	tcase_add_test(tcase, test_hexadecimal_constant);

	return tcase;
}
