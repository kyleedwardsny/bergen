/*
 * libbergen/expression.c
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

#include <bergen/expression.h>

#include <bergen/libc.h>

void expr_token_list_init(struct expr_token_list *list)
{
	list->tokens = bergen_malloc(sizeof(*list->tokens) * 32);
	list->buffer_size = 32;
	list->num_tokens = 0;
}

void expr_token_list_destroy(struct expr_token_list *list)
{
	bergen_free(list->tokens);
}

void expr_token_list_append(struct expr_token_list *list, const struct expr_token *token)
{
	struct expr_token *ptr;

	if (list->num_tokens >= list->buffer_size) {
		list->buffer_size *= 2;
		list->tokens = bergen_realloc(list->tokens, sizeof(*list->tokens) * list->buffer_size);
	}

	ptr = &list->tokens[list->num_tokens++];
	ptr->index = token->index;
	ptr->length = token->length;
	ptr->type = token->type;
}

void expr_data_init(struct expr_data *data, const char *str, size_t length, char local_label_char)
{
	expr_token_list_init(&data->tokens);
	data->str = str;
	data->length = length;
	data->local_label_char = local_label_char;
}

void expr_data_destroy(struct expr_data *data)
{
	expr_token_list_destroy(&data->tokens);
}

struct tokenize_data {
	/* Constants */
	struct expr_data *data;

	/* Mutables */
	size_t index;
	char current_char;
	int consumed_char;
	size_t paren_levels;
	struct expr_token token;
	const struct tokenize_state *state;
};

struct tokenize_state {
	struct error *(*consume)(struct tokenize_data *data, char c);
	struct error *(*end)(struct tokenize_data *data);
};

static struct error *tokenize_state_expr_begin_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_expr_begin_end(struct tokenize_data *data);

static struct error *tokenize_state_expr_end_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_expr_end_end(struct tokenize_data *data);

static struct error *tokenize_state_constant_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_constant_end(struct tokenize_data *data);

static struct error *tokenize_state_prefix_constant_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_prefix_constant_end(struct tokenize_data *data);

static struct error *tokenize_state_char_constant_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_char_constant_end(struct tokenize_data *data);

static struct error *tokenize_state_binary_operator_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_binary_operator_end(struct tokenize_data *data);

static struct error *tokenize_state_label_consume(struct tokenize_data *data, char c);
static struct error *tokenize_state_label_end(struct tokenize_data *data);

static const struct tokenize_state TOKENIZE_STATE_EXPR_BEGIN = {
	.consume	= tokenize_state_expr_begin_consume,
	.end		= tokenize_state_expr_begin_end,
};

static const struct tokenize_state TOKENIZE_STATE_EXPR_END = {
	.consume	= tokenize_state_expr_end_consume,
	.end		= tokenize_state_expr_end_end,
};

static const struct tokenize_state TOKENIZE_STATE_CONSTANT = {
	.consume	= tokenize_state_constant_consume,
	.end		= tokenize_state_constant_end,
};

static const struct tokenize_state TOKENIZE_STATE_PREFIX_CONSTANT = {
	.consume	= tokenize_state_prefix_constant_consume,
	.end		= tokenize_state_prefix_constant_end,
};

static const struct tokenize_state TOKENIZE_STATE_CHAR_CONSTANT = {
	.consume	= tokenize_state_char_constant_consume,
	.end		= tokenize_state_char_constant_end,
};

static const struct tokenize_state TOKENIZE_STATE_BINARY_OPERATOR = {
	.consume	= tokenize_state_binary_operator_consume,
	.end		= tokenize_state_binary_operator_end,
};

static const struct tokenize_state TOKENIZE_STATE_LABEL = {
	.consume	= tokenize_state_label_consume,
	.end		= tokenize_state_label_end,
};

#define TOKENIZE_STATE_INITIAL_STATE TOKENIZE_STATE_EXPR_BEGIN

static void token_finish(struct tokenize_data *data, size_t additional_chars)
{
	data->token.length = data->index - data->token.index + additional_chars;
	expr_token_list_append(&data->data->tokens, &data->token);
}

static inline int is_constant_begin(char c)
{
	return !!bergen_strchr("0123456789", c);
}

static inline int is_constant_prefix(char c)
{
	return !!bergen_strchr("%@$", c);
}

static inline int is_constant_middle(char c)
{
	return !!bergen_strchr("0123456789abcdefABCDEF", c);
}

static inline int is_constant_suffix(char c)
{
	return !!bergen_strchr("BODHbodh", c);
}

static inline int is_unary_operator(char c)
{
	return !!bergen_strchr("~-", c);
}

static inline int is_binary_operator_begin(char c)
{
	return !!bergen_strchr("+-*/%<>=!&|^", c);
}

static inline int is_binary_operator_second(char c)
{
	return !!bergen_strchr("<>=", c);
}

static inline int is_label_begin(char c, char local_label_char)
{
	return !!bergen_strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", c) || c == local_label_char;
}

static inline int is_label_middle(char c)
{
	return !!bergen_strchr("_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", c);
}

static struct error *do_constant_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_CONSTANT;

	data->state = &TOKENIZE_STATE_CONSTANT;
	return NULL;
}

static struct error *do_prefix_constant_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_PREFIX_CONSTANT;

	data->state = &TOKENIZE_STATE_PREFIX_CONSTANT;
	return NULL;
}

static struct error *do_char_constant_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_CHAR_CONSTANT;

	data->state = &TOKENIZE_STATE_CHAR_CONSTANT;
	return NULL;
}

static struct error *do_unary_operator(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.length = 1;
	data->token.type = EXPR_TOKEN_TYPE_UNARY_OPERATOR;
	expr_token_list_append(&data->data->tokens, &data->token);

	data->state = &TOKENIZE_STATE_EXPR_BEGIN;
	return NULL;
}

static struct error *do_binary_operator_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_BINARY_OPERATOR;

	data->state = &TOKENIZE_STATE_BINARY_OPERATOR;
	return NULL;
}

static struct error *do_lparen(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.length = 1;
	data->token.type = EXPR_TOKEN_TYPE_LPAREN;
	expr_token_list_append(&data->data->tokens, &data->token);

	data->paren_levels++;
	data->state = &TOKENIZE_STATE_EXPR_BEGIN;
	return NULL;
}

static struct error *do_rparen(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.length = 1;
	data->token.type = EXPR_TOKEN_TYPE_RPAREN;
	expr_token_list_append(&data->data->tokens, &data->token);

	if (data->paren_levels <= 0)
		return error_create("Unexpected ')' while evaluating expression");

	data->paren_levels--;
	data->state = &TOKENIZE_STATE_EXPR_END;
	return NULL;
}

static struct error *do_label_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_LABEL;

	data->state = &TOKENIZE_STATE_LABEL;
	return NULL;
}

static struct error *tokenize_state_expr_begin_consume(struct tokenize_data *data, char c)
{
	if (bergen_isspace(c)) {
		return NULL;
	} else if (is_unary_operator(c)) {
		return do_unary_operator(data);
	} else if (is_constant_begin(c)) {
		return do_constant_begin(data);
	} else if (is_constant_prefix(c)) {
		return do_prefix_constant_begin(data);
	} else if (c == '\'') {
		return do_char_constant_begin(data);
	} else if (c == '(') {
		return do_lparen(data);
	} else if (is_label_begin(c, data->data->local_label_char)) {
		return do_label_begin(data);
	} else {
		return error_create("Unexpected character at beginning of expression: '%c'", c);
	}
}

static struct error *tokenize_state_expr_begin_end(struct tokenize_data *data)
{
	return error_create("Expected expression but reached end of string");
}

static struct error *tokenize_state_expr_end_consume(struct tokenize_data *data, char c)
{
	if (bergen_isspace(c)) {
		return NULL;
	} else if (is_binary_operator_begin(c)) {
		return do_binary_operator_begin(data);
	} else if (c == ')') {
		return do_rparen(data);
	} else {
		return error_create("Unexpected character at end of expression: '%c'", c);
	}
}

static struct error *tokenize_state_expr_end_end(struct tokenize_data *data)
{
	/* Nothing to do here */
	return NULL;
}

static struct error *tokenize_state_constant_consume(struct tokenize_data *data, char c)
{
	if (is_constant_middle(c)) {
		return NULL;
	} else if (is_constant_suffix(c)) {
		token_finish(data, 1);
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	} else {
		token_finish(data, 0);
		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	}
}

static struct error *tokenize_state_constant_end(struct tokenize_data *data)
{
	token_finish(data, 0);
	return NULL;
}

static struct error *tokenize_state_prefix_constant_consume(struct tokenize_data *data, char c)
{
	if (is_constant_middle(c)) {
		return NULL;
	} else {
		token_finish(data, 0);
		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	}
}

static struct error *tokenize_state_prefix_constant_end(struct tokenize_data *data)
{
	token_finish(data, 0);
	return NULL;
}

static struct error *tokenize_state_char_constant_consume(struct tokenize_data *data, char c)
{
	size_t length = data->index - data->token.index;

	if (length == 1) {
		return NULL;
	} else if (c == '\'') {
		token_finish(data, 1);
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	} else {
		return error_create("Expected single quote but got '%c'", c);
	}
}

static struct error *tokenize_state_char_constant_end(struct tokenize_data *data)
{
	return error_create("Reached end of expression in middle of char constant");
}

static struct error *tokenize_state_binary_operator_consume(struct tokenize_data *data, char c)
{
	if (is_binary_operator_second(c)) {
		token_finish(data, 1);
		data->state = &TOKENIZE_STATE_EXPR_BEGIN;
		return NULL;
	} else {
		token_finish(data, 0);
		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_BEGIN;
		return NULL;
	}
}

static struct error *tokenize_state_binary_operator_end(struct tokenize_data *data)
{
	token_finish(data, 0);
	data->consumed_char = 0;
	data->state = &TOKENIZE_STATE_EXPR_BEGIN;
	return NULL;
}

static struct error *tokenize_state_label_consume(struct tokenize_data *data, char c)
{
	if (is_label_middle(c)) {
		return NULL;
	} else {
		token_finish(data, 0);
		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	}
}

static struct error *tokenize_state_label_end(struct tokenize_data *data)
{
	token_finish(data, 0);
	return NULL;
}

struct error *expr_tokenize(struct expr_data *data)
{
	struct error *err;
	struct tokenize_data tdata;

	tdata.data = data;
	tdata.paren_levels = 0;
	tdata.state = &TOKENIZE_STATE_INITIAL_STATE;

	for (tdata.index = 0; tdata.index < data->length; tdata.index++) {
		tdata.current_char = data->str[tdata.index];
		do {
			tdata.consumed_char = 1; /* Must be overridden by state function */
			if ((err = tdata.state->consume(&tdata, tdata.current_char)))
				return err;
		} while (!tdata.consumed_char);
	}

	do {
		tdata.consumed_char = 1;
		if ((err = tdata.state->end(&tdata)))
			return err;
	} while (!tdata.consumed_char);

	if (tdata.paren_levels > 0)
		return error_create("Expected one or more ')'s at end of expression");

	return NULL;
}
