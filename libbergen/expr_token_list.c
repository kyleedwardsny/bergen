/*
 * libbergen/expr_token_list.c
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
