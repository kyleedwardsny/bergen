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
	ptr->extra = token->extra;
}

void expr_data_init(struct expr_data *data, const char *str, size_t length, char local_label_char)
{
	label_list_init(&data->labels);
	label_list_init(&data->local_labels);
	data->str = str;
	data->length = length;
	data->local_label_char = local_label_char;
}

void expr_data_destroy(struct expr_data *data)
{
	label_list_destroy(&data->local_labels);
	label_list_destroy(&data->labels);
}

struct tokenize_data {
	/* Constants */
	struct expr_data *data;
	struct expr_token_list *tokens;

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

static void token_calc_length(struct tokenize_data *data, size_t additional_chars)
{
	data->token.length = data->index - data->token.index + additional_chars;
}

static void token_append(struct tokenize_data *data)
{
	expr_token_list_append(data->tokens, &data->token);
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
	data->token.type = EXPR_TOKEN_TYPE_CONSTANT;

	data->state = &TOKENIZE_STATE_PREFIX_CONSTANT;
	return NULL;
}

static struct error *do_char_constant_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_CONSTANT;

	data->state = &TOKENIZE_STATE_CHAR_CONSTANT;
	return NULL;
}

static struct error *do_unary_operator(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.length = 1;
	data->token.type = EXPR_TOKEN_TYPE_UNARY_OPERATOR;
	switch (data->current_char) {
	case '~':
		data->token.extra.unary_operator_type = EXPR_UNARY_OPERATOR_TYPE_INVERT;
		break;

	case '-':
		data->token.extra.unary_operator_type = EXPR_UNARY_OPERATOR_TYPE_NEGATE;
		break;
	}
	token_append(data);

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
	token_append(data);

	data->paren_levels++;
	data->state = &TOKENIZE_STATE_EXPR_BEGIN;
	return NULL;
}

static struct error *do_rparen(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.length = 1;
	data->token.type = EXPR_TOKEN_TYPE_RPAREN;
	token_append(data);

	if (data->paren_levels <= 0)
		return error_create("Unexpected ')' while evaluating expression");

	data->paren_levels--;
	data->state = &TOKENIZE_STATE_EXPR_END;
	return NULL;
}

static struct error *do_label_begin(struct tokenize_data *data)
{
	data->token.index = data->index;
	data->token.type = EXPR_TOKEN_TYPE_CONSTANT;

	data->state = &TOKENIZE_STATE_LABEL;
	return NULL;
}

static struct error *evaluate_binary_constant(const char *str, size_t length, expr_value *result)
{
	char *end, *tmp;
	struct error *err;

	*result = bergen_strtoll(str, &end, 2);
	if (end < str + length) {
		tmp = bergen_strndup_null(str, length);
		err = error_create("Invalid binary constant: \"%s\"", tmp);
		bergen_free(tmp);
		return err;
	}

	return NULL;
}

static struct error *evaluate_octal_constant(const char *str, size_t length, expr_value *result)
{
	char *end, *tmp;
	struct error *err;

	*result = bergen_strtoll(str, &end, 8);
	if (end < str + length) {
		tmp = bergen_strndup_null(str, length);
		err = error_create("Invalid octal constant: \"%s\"", tmp);
		bergen_free(tmp);
		return err;
	}

	return NULL;
}

static struct error *evaluate_decimal_constant(const char *str, size_t length, expr_value *result)
{
	char *end, *tmp;
	struct error *err;

	*result = bergen_strtoll(str, &end, 10);
	if (end < str + length) {
		tmp = bergen_strndup_null(str, length);
		err = error_create("Invalid decimal constant: \"%s\"", tmp);
		bergen_free(tmp);
		return err;
	}

	return NULL;
}

static struct error *evaluate_hexadecimal_constant(const char *str, size_t length, expr_value *result)
{
	char *end, *tmp;
	struct error *err;

	*result = bergen_strtoll(str, &end, 16);
	if (end < str + length) {
		tmp = bergen_strndup_null(str, length);
		err = error_create("Invalid hexadecimal constant: \"%s\"", tmp);
		bergen_free(tmp);
		return err;
	}

	return NULL;
}

static struct error *evaluate_location_counter(struct tokenize_data *data, expr_value *result)
{
	*result = data->data->location_counter;
	return NULL;
}

static struct error *evaluate_prefix_constant(struct tokenize_data *data)
{
	const char *str = data->data->str + data->token.index;
	size_t length = data->token.length;
	char c = str[0];
	expr_value *result = &data->token.extra.value;

	switch (c) {
	case '%':
		return evaluate_binary_constant(str + 1, length - 1, result);

	case '@':
		return evaluate_octal_constant(str + 1, length - 1, result);

	case '$':
		if (length == 1)
			return evaluate_location_counter(data, result);
		else
			return evaluate_hexadecimal_constant(str + 1, length - 1, result);

	default: /* Will never happen */
		return error_create("Invalid constant prefix: '%c'", c);
	}
}

static struct error *evaluate_suffix_constant(struct tokenize_data *data)
{
	const char *str = data->data->str + data->token.index;
	size_t length = data->token.length;
	char c = str[length - 1];
	expr_value *result = &data->token.extra.value;

	switch (c) {
	case 'B':
	case 'b':
		return evaluate_binary_constant(str, length - 1, result);

	case 'O':
	case 'o':
		return evaluate_octal_constant(str, length - 1, result);

	case 'D':
	case 'd':
		return evaluate_decimal_constant(str, length - 1, result);

	case 'H':
	case 'h':
		return evaluate_hexadecimal_constant(str, length - 1, result);
	}

	if (!!bergen_strchr("0123456789", c))
		return evaluate_decimal_constant(str, length, result);
	else /* Will never happen */
		return error_create("Invalid constant suffix: '%c'", c);
}

static struct error *evaluate_char_constant(struct tokenize_data *data)
{
	data->token.extra.value = data->data->str[data->token.index + 1];
	return NULL;
}

static struct error *evaluate_label_type_known(const struct label_list *labels, const char *str, size_t length, expr_value *result)
{
	const struct label *label = label_list_find_label(labels, str, length);
	char *tmp;
	struct error *err;

	if (label) {
		*result = label->value;
		return NULL;
	} else {
		tmp = bergen_strndup_null(str, length);
		err = error_create("Could not find label: %s", tmp);
		bergen_free(tmp);
		return err;
	}
}

static struct error *evaluate_label(struct tokenize_data *data)
{
	const char *str = data->data->str + data->token.index;
	char c = str[0];

	if (c == data->data->local_label_char)
		return evaluate_label_type_known(&data->data->local_labels, str + 1, data->token.length - 1, &data->token.extra.value);
	else
		return evaluate_label_type_known(&data->data->labels, str, data->token.length, &data->token.extra.value);
}

static struct error *evaluate_binary_operator_1(struct tokenize_data *data)
{
	char c = data->data->str[data->token.index];

	switch (c) {
	case '+':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_PLUS;
		break;

	case '-':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_MINUS;
		break;

	case '*':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_TIMES;
		break;

	case '/':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_DIV;
		break;

	case '%':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_MODULO;
		break;

	case '=':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_EQ;
		break;

	case '<':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_LT;
		break;

	case '>':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_GT;
		break;

	case '&':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_AND;
		break;

	case '|':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_OR;
		break;

	case '^':
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_XOR;
		break;
	}

	return NULL;
}

static struct error *evaluate_binary_operator_2(struct tokenize_data *data)
{
	const char *str = data->data->str + data->token.index;
	char *buf;
	struct error *err;

	if (!bergen_strncmp(str, "<<", 2))
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_LSL;
	else if (!bergen_strncmp(str, ">>", 2))
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_LSR;
	else if (!bergen_strncmp(str, "==", 2))
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_EQ;
	else if (!bergen_strncmp(str, "!=", 2))
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_NE;
	else if (!bergen_strncmp(str, "<=", 2))
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_LE;
	else if (!bergen_strncmp(str, ">=", 2))
		data->token.extra.binary_operator_type = EXPR_BINARY_OPERATOR_TYPE_GE;
	else {
		buf = bergen_strndup_null(str, 2);
		err = error_create("Invalid binary operator: \"%s\"", buf);
		bergen_free(buf);
		return err;
	}

	return NULL;
}

static struct error *evaluate_binary_operator(struct tokenize_data *data)
{
	if (data->token.length == 1)
		return evaluate_binary_operator_1(data);
	else
		return evaluate_binary_operator_2(data);
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
	if (data->paren_levels > 0)
		return error_create("Expected %" PRIuPTR " ')'s at end of expression", data->paren_levels);
	return NULL;
}

static struct error *tokenize_state_constant_consume(struct tokenize_data *data, char c)
{
	struct error *err;

	if (is_constant_middle(c)) {
		return NULL;
	} else if (is_constant_suffix(c)) {
		token_calc_length(data, 1);
		if ((err = evaluate_suffix_constant(data)))
			return err;
		token_append(data);

		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	} else {
		token_calc_length(data, 0);
		if ((err = evaluate_suffix_constant(data)))
			return err;
		token_append(data);

		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	}
}

static struct error *tokenize_state_constant_end(struct tokenize_data *data)
{
	struct error *err;

	token_calc_length(data, 0);
	if ((err = evaluate_suffix_constant(data)))
		return err;
	token_append(data);

	data->consumed_char = 0;
	data->state = &TOKENIZE_STATE_EXPR_END;
	return NULL;
}

static struct error *tokenize_state_prefix_constant_consume(struct tokenize_data *data, char c)
{
	struct error *err;

	if (is_constant_middle(c)) {
		return NULL;
	} else {
		token_calc_length(data, 0);
		if ((err = evaluate_prefix_constant(data)))
			return err;
		token_append(data);

		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	}
}

static struct error *tokenize_state_prefix_constant_end(struct tokenize_data *data)
{
	struct error *err;

	token_calc_length(data, 0);
	if ((err = evaluate_prefix_constant(data)))
		return err;
	token_append(data);

	data->consumed_char = 0;
	data->state = &TOKENIZE_STATE_EXPR_END;
	return NULL;
}

static struct error *tokenize_state_char_constant_consume(struct tokenize_data *data, char c)
{
	struct error *err;
	size_t length = data->index - data->token.index;

	if (length == 1) {
		return NULL;
	} else if (c == '\'') {
		token_calc_length(data, 1);
		if ((err = evaluate_char_constant(data)))
			return err;
		token_append(data);

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
	struct error *err;

	if (is_binary_operator_second(c)) {
		token_calc_length(data, 1);
		if ((err = evaluate_binary_operator(data)))
			return err;
		token_append(data);

		data->state = &TOKENIZE_STATE_EXPR_BEGIN;
		return NULL;
	} else {
		token_calc_length(data, 0);
		if ((err = evaluate_binary_operator(data)))
			return err;
		token_append(data);

		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_BEGIN;
		return NULL;
	}
}

static struct error *tokenize_state_binary_operator_end(struct tokenize_data *data)
{
	data->consumed_char = 0;
	data->state = &TOKENIZE_STATE_EXPR_BEGIN;
	return NULL;
}

static struct error *tokenize_state_label_consume(struct tokenize_data *data, char c)
{
	struct error *err;

	if (is_label_middle(c)) {
		return NULL;
	} else {
		token_calc_length(data, 0);
		if ((err = evaluate_label(data)))
			return err;
		token_append(data);

		data->consumed_char = 0;
		data->state = &TOKENIZE_STATE_EXPR_END;
		return NULL;
	}
}

static struct error *tokenize_state_label_end(struct tokenize_data *data)
{
	struct error *err;

	token_calc_length(data, 0);
	if ((err = evaluate_label(data)))
		return err;
	token_append(data);

	data->consumed_char = 0;
	data->state = &TOKENIZE_STATE_EXPR_END;
	return NULL;
}

struct error *expr_tokenize(struct expr_data *data, struct expr_token_list *tokens)
{
	struct error *err;
	struct tokenize_data tdata;

	tdata.data = data;
	tdata.tokens = tokens;

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

	return NULL;
}

static void apply_binary_operator(enum expr_binary_operator_type op_type, expr_value *result, expr_value rvalue)
{
	switch (op_type) {
	case EXPR_BINARY_OPERATOR_TYPE_ASSIGN:
		*result = rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_PLUS:
		*result += rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_MINUS:
		*result -= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_TIMES:
		*result *= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_DIV:
		*result /= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_MODULO:
		*result %= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_LSL:
		*result <<= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_LSR:
		*result >>= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_EQ:
		*result = (*result == rvalue);
		break;

	case EXPR_BINARY_OPERATOR_TYPE_NE:
		*result = (*result != rvalue);
		break;

	case EXPR_BINARY_OPERATOR_TYPE_LT:
		*result = (*result < rvalue);
		break;

	case EXPR_BINARY_OPERATOR_TYPE_GT:
		*result = (*result > rvalue);
		break;

	case EXPR_BINARY_OPERATOR_TYPE_LE:
		*result = (*result <= rvalue);
		break;

	case EXPR_BINARY_OPERATOR_TYPE_GE:
		*result = (*result >= rvalue);
		break;

	case EXPR_BINARY_OPERATOR_TYPE_AND:
		*result &= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_OR:
		*result |= rvalue;
		break;

	case EXPR_BINARY_OPERATOR_TYPE_XOR:
		*result ^= rvalue;
		break;
	}
}

static size_t expr_evaluate_r(struct expr_data *data, struct expr_token_list *tokens, size_t start_index, expr_value *result);

static size_t apply_unary_operator(struct expr_data *data, struct expr_token_list *tokens, size_t start_index, expr_value *result)
{
	size_t index = start_index;
	expr_value value;
	struct expr_token *token1 = &tokens->tokens[index];
	struct expr_token *token2 = &tokens->tokens[index + 1];

	if (token2->type == EXPR_TOKEN_TYPE_CONSTANT) {
		index += 2;
		value = token2->extra.value;
	} else if (token2->type == EXPR_TOKEN_TYPE_LPAREN) {
		index = expr_evaluate_r(data, tokens, index + 1, &value);
	}

	switch (token1->extra.unary_operator_type) {
	case EXPR_UNARY_OPERATOR_TYPE_INVERT:
		*result = ~value;
		break;

	case EXPR_UNARY_OPERATOR_TYPE_NEGATE:
		*result = -value;
		break;
	}

	return index;
}

static size_t expr_evaluate_r(struct expr_data *data, struct expr_token_list *tokens, size_t start_index, expr_value *result)
{
	size_t index = start_index;
	expr_value value;
	struct expr_token *token;
	enum expr_binary_operator_type op_type = EXPR_BINARY_OPERATOR_TYPE_ASSIGN;

	for (;;) {
		/* First step */
		token = &tokens->tokens[index];
		if (token->type == EXPR_TOKEN_TYPE_CONSTANT) {
			index++;
			apply_binary_operator(op_type, result, token->extra.value);
		} else if (token->type == EXPR_TOKEN_TYPE_LPAREN) {
			index = expr_evaluate_r(data, tokens, index + 1, &value);
			apply_binary_operator(op_type, result, value);
		} else if (token->type == EXPR_TOKEN_TYPE_UNARY_OPERATOR) {
			index = apply_unary_operator(data, tokens, index, &value);
			apply_binary_operator(op_type, result, value);
		}

		/* Second step */
		if (index >= tokens->num_tokens)
			return index;
		token = &tokens->tokens[index];
		if (token->type == EXPR_TOKEN_TYPE_BINARY_OPERATOR) {
			index++;
			op_type = token->extra.binary_operator_type;
		} else if (token->type == EXPR_TOKEN_TYPE_RPAREN) {
			return index;
		}
	}

	return 0;
}

struct error *expr_evaluate(struct expr_data *data, expr_value *result)
{
	struct error *err;
	struct expr_token_list tokens;

	expr_token_list_init(&tokens);
	if ((err = expr_tokenize(data, &tokens))) {
		expr_token_list_destroy(&tokens);
		return err;
	}

	/* Expression is guaranteed to be valid, now evaluate it */
	expr_evaluate_r(data, &tokens, 0, result);

	expr_token_list_destroy(&tokens);
	return NULL;
}
