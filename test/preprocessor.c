/*
 * test/preprocessor.c
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

#include <bergen/preprocessor.h>

START_TEST(test_def_init_args)
{
	struct pp_macro_definition macro;

	pp_macro_definition_init_easy(&macro, "macro", 1);

	ck_assert_str_eq(macro.name, "macro");
	ck_assert_uint_eq(macro.num_args, 0);
	ck_assert_ptr_ne(macro.args, NULL);

	pp_macro_definition_destroy(&macro);
}
END_TEST

START_TEST(test_def_init_no_args)
{
	struct pp_macro_definition macro;

	pp_macro_definition_init_easy(&macro, "macro", 0);

	ck_assert_str_eq(macro.name, "macro");
	ck_assert_uint_eq(macro.num_args, 0);
	ck_assert_ptr_eq(macro.args, NULL);

	pp_macro_definition_destroy(&macro);
}
END_TEST

START_TEST(test_def_add_args)
{
	struct pp_macro_definition macro;
	struct error *err;

	pp_macro_definition_init_easy(&macro, "macro", 1);

	err = pp_macro_definition_add_arg_easy(&macro, "arg1");
	ck_assert_ptr_eq(err, NULL);
	ck_assert_uint_eq(macro.num_args, 1);
	ck_assert_str_eq(macro.args[0], "arg1");

	err = pp_macro_definition_add_arg_easy(&macro, "arg2");
	ck_assert_ptr_eq(err, NULL);
	ck_assert_uint_eq(macro.num_args, 2);
	ck_assert_str_eq(macro.args[0], "arg1");
	ck_assert_str_eq(macro.args[1], "arg2");

	err = pp_macro_definition_add_arg_easy(&macro, "arg1");
	ck_assert_ptr_ne(err, NULL);
	error_free(err);

	pp_macro_definition_destroy(&macro);
}
END_TEST

TCase *tcase_preprocessor(void)
{
	TCase *tcase = tcase_create("preprocessor");

	tcase_add_test(tcase, test_def_init_args);
	tcase_add_test(tcase, test_def_init_no_args);
	tcase_add_test(tcase, test_def_add_args);

	return tcase;
}
