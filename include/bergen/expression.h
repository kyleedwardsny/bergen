/*
 * include/bergen/expression.h
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

#ifndef BERGEN_EXPRESSION_H
#define BERGEN_EXPRESSION_H

#include <bergen/error.h>
#include <bergen/label.h>
#include <bergen/types.h>

#include <stdlib.h>

enum expr_token_type {
	EXPR_TOKEN_TYPE_CONSTANT,
	EXPR_TOKEN_TYPE_UNARY_OPERATOR,
	EXPR_TOKEN_TYPE_BINARY_OPERATOR,
	EXPR_TOKEN_TYPE_LPAREN,
	EXPR_TOKEN_TYPE_RPAREN,
};

enum expr_unary_operator_type {
	EXPR_UNARY_OPERATOR_TYPE_INVERT,
	EXPR_UNARY_OPERATOR_TYPE_NEGATE,
};

enum expr_binary_operator_type {
	EXPR_BINARY_OPERATOR_TYPE_PLUS,
	EXPR_BINARY_OPERATOR_TYPE_MINUS,
	EXPR_BINARY_OPERATOR_TYPE_TIMES,
	EXPR_BINARY_OPERATOR_TYPE_DIV,
	EXPR_BINARY_OPERATOR_TYPE_MODULO,
	EXPR_BINARY_OPERATOR_TYPE_LSL,
	EXPR_BINARY_OPERATOR_TYPE_LSR,
	EXPR_BINARY_OPERATOR_TYPE_EQ,
	EXPR_BINARY_OPERATOR_TYPE_NE,
	EXPR_BINARY_OPERATOR_TYPE_LT,
	EXPR_BINARY_OPERATOR_TYPE_GT,
	EXPR_BINARY_OPERATOR_TYPE_LE,
	EXPR_BINARY_OPERATOR_TYPE_GE,
	EXPR_BINARY_OPERATOR_TYPE_AND,
	EXPR_BINARY_OPERATOR_TYPE_OR,
	EXPR_BINARY_OPERATOR_TYPE_XOR,
};

struct expr_token {
	size_t index;
	size_t length;
	enum expr_token_type type;
	union {
		expr_value value;
		enum expr_unary_operator_type unary_operator_type;
		enum expr_binary_operator_type binary_operator_type;
	} extra;
};

struct expr_token_list {
	struct expr_token *tokens;
	size_t buffer_size; /* Number of tokens in buffer */
	size_t num_tokens;
};

struct expr_data {
	const char *str;
	size_t length;
	char local_label_char;
	expr_value location_counter;

	struct label_list labels;
	struct label_list local_labels;
};

void expr_token_list_init(struct expr_token_list *list);

void expr_token_list_destroy(struct expr_token_list *list);

void expr_token_list_append(struct expr_token_list *list, const struct expr_token *token);

void expr_data_init(struct expr_data *data, const char *str, size_t length, char local_label_char);

void expr_data_destroy(struct expr_data *data);

struct error *expr_tokenize(struct expr_data *data, struct expr_token_list *tokens);

struct error *expr_evaluate(struct expr_data *data, expr_value *result);

#endif /* BERGEN_EXPRESSION_H */
