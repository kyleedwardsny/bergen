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

#include <stdarg.h>

static void assert_expr_full(const char *str, int expect_error, expr_value expected_result, expr_value location_counter, ...)
{
	struct expr_data expr;
	expr_value result = ~expected_result; /* Make sure they start off different */
	struct error *err;
	const char *label_name;
	expr_value label_value;
	va_list args;

	expr_data_init_easy(&expr, str, '_');
	expr.location_counter = location_counter;

	va_start(args, location_counter);

	/* Push labels */
	for (;;) {
		label_name = va_arg(args, const char *);
		if (!label_name)
			break;
		label_value = va_arg(args, expr_value);
		label_list_append_easy(&expr.labels, label_name, label_value);
	}

	/* Push local labels */
	for (;;) {
		label_name = va_arg(args, const char *);
		if (!label_name)
			break;
		label_value = va_arg(args, expr_value);
		label_list_append_easy(&expr.local_labels, label_name, label_value);
	}

	va_end(args);

	err = expr_evaluate(&expr, &result);
	if (expect_error) {
		ck_assert(!!err);
	} else {
		ck_assert(!err);
		ck_assert_int_eq(result, expected_result);
	}

	expr_data_destroy(&expr);
}

static inline void assert_expr_eq(const char *str, expr_value expected_result)
{
	assert_expr_full(str, 0, expected_result, 0, NULL, NULL);
}

static inline void assert_expr_invalid(const char *str)
{
	assert_expr_full(str, 1, 0, 0, NULL, NULL);
}

START_TEST(test_binary_constant)
{
	assert_expr_eq("%10101010", 0xAA);
	assert_expr_eq("10101010b", 0xAA);
	assert_expr_eq("10101010B", 0xAA);
	assert_expr_eq("%11000011", 0xC3);
	assert_expr_invalid("%11201100");
	assert_expr_invalid("%11f01100");
}
END_TEST

START_TEST(test_octal_constant)
{
	assert_expr_eq("@12345", 012345);
	assert_expr_eq("12345O", 012345);
	assert_expr_eq("12345o", 012345);
	assert_expr_eq("67o", 067);
	assert_expr_invalid("@67890");
	assert_expr_invalid("@67a");
}
END_TEST

START_TEST(test_decimal_constant)
{
	assert_expr_eq("12345", 12345);
	assert_expr_eq("12345D", 12345);
	assert_expr_eq("12345d", 12345);
	assert_expr_eq("67890", 67890);
	assert_expr_invalid("123a5");
}
END_TEST

START_TEST(test_hexadecimal_constant)
{
	assert_expr_eq("$12345", 0x12345);
	assert_expr_eq("12345H", 0x12345);
	assert_expr_eq("12345h", 0x12345);
	assert_expr_eq("$CAFEBABE", 0xCAFEBABE);
	assert_expr_eq("$cafebabe", 0xCAFEBABE);
	assert_expr_eq("$deadbeef", 0xDEADBEEF);
	assert_expr_invalid("$garbage");
}
END_TEST

START_TEST(test_operator_plus)
{
	assert_expr_eq("1 + 2", 3);
}
END_TEST

START_TEST(test_operator_minus)
{
	assert_expr_eq("2 - 1", 1);
}
END_TEST

START_TEST(test_operator_times)
{
	assert_expr_eq("2 * 3", 6);
}
END_TEST

START_TEST(test_operator_div)
{
	assert_expr_eq("6 / 3", 2);
}
END_TEST

START_TEST(test_operator_modulo)
{
	assert_expr_eq("5 % 3", 2);
}
END_TEST

START_TEST(test_operator_lsl)
{
	assert_expr_eq("3 << 4", 48);
}
END_TEST

START_TEST(test_operator_lsr)
{
	assert_expr_eq("48 >> 4", 3);
}
END_TEST

START_TEST(test_operator_invert)
{
	assert_expr_eq("~$5A5A", -0x5A5A - 1);
}
END_TEST

START_TEST(test_operator_negate)
{
	assert_expr_eq("-$5A5A", -0x5A5A);
}
END_TEST

START_TEST(test_operator_eq)
{
	assert_expr_eq("5 = 5", 1);
	assert_expr_eq("5 == 5", 1);
	assert_expr_eq("5 = 4", 0);
	assert_expr_eq("5 == 4", 0);
}
END_TEST

START_TEST(test_operator_ne)
{
	assert_expr_eq("5 != 5", 0);
	assert_expr_eq("5 != 4", 1);
}
END_TEST

START_TEST(test_operator_lt)
{
	assert_expr_eq("5 < 4", 0);
	assert_expr_eq("5 < 5", 0);
	assert_expr_eq("5 < 6", 1);
}
END_TEST

START_TEST(test_operator_gt)
{
	assert_expr_eq("5 > 4", 1);
	assert_expr_eq("5 > 5", 0);
	assert_expr_eq("5 > 6", 0);
}
END_TEST

START_TEST(test_operator_le)
{
	assert_expr_eq("5 <= 4", 0);
	assert_expr_eq("5 <= 5", 1);
	assert_expr_eq("5 <= 6", 1);
}
END_TEST

START_TEST(test_operator_ge)
{
	assert_expr_eq("5 >= 4", 1);
	assert_expr_eq("5 >= 5", 1);
	assert_expr_eq("5 >= 6", 0);
}
END_TEST

START_TEST(test_operator_and)
{
	assert_expr_eq("$3C & $0F", 0xC);
}
END_TEST

START_TEST(test_operator_or)
{
	assert_expr_eq("$3C | $0F", 0x3F);
}
END_TEST

START_TEST(test_operator_xor)
{
	assert_expr_eq("$3C ^ $0F", 0x33);
}
END_TEST

START_TEST(test_location_counter)
{
	assert_expr_full("$", 0, 12345, 12345, NULL, NULL);
	assert_expr_full("$", 0, 12, 12, NULL, NULL);
	assert_expr_full("$", 0, 0x8000, 0x8000, NULL, NULL);
}
END_TEST

START_TEST(test_label)
{
	assert_expr_full("label", 0, 12345, 0, "label", 12345, NULL, NULL);
	assert_expr_invalid("label");
}
END_TEST

START_TEST(test_local_label)
{
	assert_expr_full("_label", 0, 12345, 0, NULL, "label", 12345, NULL);
	assert_expr_invalid("_label");
}
END_TEST

START_TEST(test_operator_precedence)
{
	assert_expr_eq("3 / 3 + 3", 4);
	assert_expr_eq("3 + 3 / 3", 2);
}
END_TEST

START_TEST(test_parentheses)
{
	assert_expr_eq("(3 / 3) + 3", 4);
	assert_expr_eq("12 / (3 + 3)", 2);
	assert_expr_eq("(3 + 3) / 3", 2);
	assert_expr_eq("12 + (3 / 3)", 13);
	assert_expr_invalid("(3");
	assert_expr_invalid("3)");
}
END_TEST

START_TEST(test_spaces)
{
	assert_expr_eq("1+1", 2);
	assert_expr_eq("1 +1", 2);
	assert_expr_eq("1+ 1", 2);
	assert_expr_eq("1\n+1", 2);
	assert_expr_eq("1+\r1", 2);
	assert_expr_eq("\t1+1\v", 2);
	assert_expr_eq("1+\f1", 2);
}
END_TEST

TCase *tcase_expr_evaluate(void)
{
	TCase *tcase = tcase_create("expr_evaluate");

	tcase_add_test(tcase, test_binary_constant);
	tcase_add_test(tcase, test_octal_constant);
	tcase_add_test(tcase, test_decimal_constant);
	tcase_add_test(tcase, test_hexadecimal_constant);

	tcase_add_test(tcase, test_operator_plus);
	tcase_add_test(tcase, test_operator_minus);
	tcase_add_test(tcase, test_operator_times);
	tcase_add_test(tcase, test_operator_div);
	tcase_add_test(tcase, test_operator_modulo);
	tcase_add_test(tcase, test_operator_lsl);
	tcase_add_test(tcase, test_operator_lsr);
	tcase_add_test(tcase, test_operator_invert);
	tcase_add_test(tcase, test_operator_negate);
	tcase_add_test(tcase, test_operator_eq);
	tcase_add_test(tcase, test_operator_ne);
	tcase_add_test(tcase, test_operator_lt);
	tcase_add_test(tcase, test_operator_gt);
	tcase_add_test(tcase, test_operator_le);
	tcase_add_test(tcase, test_operator_ge);
	tcase_add_test(tcase, test_operator_and);
	tcase_add_test(tcase, test_operator_or);
	tcase_add_test(tcase, test_operator_xor);

	tcase_add_test(tcase, test_location_counter);

	tcase_add_test(tcase, test_label);
	tcase_add_test(tcase, test_local_label);

	tcase_add_test(tcase, test_operator_precedence);
	tcase_add_test(tcase, test_parentheses);
	tcase_add_test(tcase, test_spaces);

	return tcase;
}
