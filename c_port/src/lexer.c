#include "mapl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char DIGITS[] = "0123456789";
static const char LETTERS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char LETTERS_DIGITS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

Lexer* lexer_create(const char* fn, const char* text) {
    Lexer* l = calloc(1, sizeof(Lexer));
    l->fn = strdup(fn);
    l->text = strdup(text);
    l->text_len = (int)strlen(text);
    l->pos = position_create(-1, 0, -1, l->fn, l->text);
    l->has_char = 0;
    lexer_advance(l);
    return l;
}

void lexer_advance(Lexer* l) {
    char c = l->has_char ? l->current_char : '\0';
    position_advance(&l->pos, c);
    if (l->pos.idx < l->text_len) {
        l->current_char = l->text[l->pos.idx];
        l->has_char = 1;
    } else {
        l->has_char = 0;
    }
}

static Token make_number(Lexer* l) {
    char buf[256];
    int pos = 0;
    int dot_count = 0;
    Position ps = position_copy(&l->pos);

    while (l->has_char && strchr("0123456789.", l->current_char)) {
        if (l->current_char == '.') {
            if (dot_count == 1) break;
            dot_count++;
        }
        buf[pos++] = l->current_char;
        if (pos >= 255) break;
        lexer_advance(l);
    }
    buf[pos] = '\0';

    if (dot_count == 0) {
        return token_create(TT_INT, buf, &ps, &l->pos);
    } else {
        return token_create(TT_FLOAT, buf, &ps, &l->pos);
    }
}

static Token make_string(Lexer* l) {
    char buf[4096];
    int pos = 0;
    Position ps = position_copy(&l->pos);
    int esc = 0;
    lexer_advance(l); /* skip opening quote */

    while (l->has_char && (l->current_char != '"' || esc)) {
        if (esc) {
            if (l->current_char == 'n') buf[pos++] = '\n';
            else if (l->current_char == 't') buf[pos++] = '\t';
            else buf[pos++] = l->current_char;
            esc = 0;
        } else {
            if (l->current_char == '\\') esc = 1;
            else buf[pos++] = l->current_char;
        }
        if (pos >= 4095) break;
        lexer_advance(l);
    }
    buf[pos] = '\0';
    lexer_advance(l); /* skip closing quote */
    return token_create(TT_STRING, buf, &ps, &l->pos);
}

static Token make_identifier(Lexer* l) {
    char buf[256];
    Position ps = position_copy(&l->pos);
    int pos = 0;
    while (l->has_char && strchr(LETTERS_DIGITS, l->current_char)) {
        buf[pos++] = l->current_char;
        if (pos >= 255) break;
        lexer_advance(l);
    }
    buf[pos] = '\0';
    TokenType tt = is_keyword(buf) ? TT_KEYWORD : TT_IDENTIFIER;
    return token_create(tt, buf, &ps, &l->pos);
}

static Token make_minus_or_arrow(Lexer* l) {
    Position ps = position_copy(&l->pos);
    lexer_advance(l);
    if (l->has_char && l->current_char == '>') {
        lexer_advance(l);
        return token_create(TT_ARROW, NULL, &ps, &l->pos);
    }
    return token_create(TT_MINUS, NULL, &ps, &l->pos);
}

static void skip_comment(Lexer* l) {
    lexer_advance(l);
    while (l->has_char && l->current_char != '\n') lexer_advance(l);
    if (l->has_char) lexer_advance(l);
}

Token* lexer_make_tokens(Lexer* l, int* out_count, Error* out_error) {
    *out_count = 0;
    int cap = 64;
    Token* tokens = malloc(cap * sizeof(Token));

    while (l->has_char) {
        if (l->current_char == ' ' || l->current_char == '\t') {
            lexer_advance(l);
        } else if (l->current_char == '#') {
            skip_comment(l);
        } else if (l->current_char == ';' || l->current_char == '\n') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_NEWLINE, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (strchr(DIGITS, l->current_char)) {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = make_number(l);
        } else if (strchr(LETTERS, l->current_char)) {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = make_identifier(l);
        } else if (l->current_char == '"') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = make_string(l);
        } else if (l->current_char == '+') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_PLUS, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == '-') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = make_minus_or_arrow(l);
        } else if (l->current_char == '*') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_MUL, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == '/') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_DIV, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == '^') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_POW, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == '(') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_LPAREN, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == ')') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_RPAREN, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == '[') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_LSQUARE, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == ']') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_RSQUARE, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else if (l->current_char == '!') {
            Position ps = position_copy(&l->pos);
            lexer_advance(l);
            if (l->has_char && l->current_char == '=') {
                lexer_advance(l);
                if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
                tokens[(*out_count)++] = token_create(TT_NE, NULL, &ps, &l->pos);
            } else {
                *out_error = error_expected_char(ps, l->pos, "'=' (after '!')");
                for (int i = 0; i < *out_count; i++) token_free(&tokens[i]);
                free(tokens);
                *out_count = 0;
                return NULL;
            }
        } else if (l->current_char == '=') {
            Position ps = position_copy(&l->pos);
            lexer_advance(l);
            TokenType tt = TT_EQ;
            if (l->has_char && l->current_char == '=') {
                lexer_advance(l);
                tt = TT_EE;
            }
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(tt, NULL, &ps, &l->pos);
        } else if (l->current_char == '<') {
            Position ps = position_copy(&l->pos);
            lexer_advance(l);
            TokenType tt = TT_LT;
            if (l->has_char && l->current_char == '=') {
                lexer_advance(l);
                tt = TT_LTE;
            }
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(tt, NULL, &ps, &l->pos);
        } else if (l->current_char == '>') {
            Position ps = position_copy(&l->pos);
            lexer_advance(l);
            TokenType tt = TT_GT;
            if (l->has_char && l->current_char == '=') {
                lexer_advance(l);
                tt = TT_GTE;
            }
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(tt, NULL, &ps, &l->pos);
        } else if (l->current_char == ',') {
            if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
            tokens[(*out_count)++] = token_create(TT_COMMA, NULL, &l->pos, NULL);
            lexer_advance(l);
        } else {
            Position ps = position_copy(&l->pos);
            char c = l->current_char;
            lexer_advance(l);
            char detail[8]; detail[0] = '\''; detail[1] = c; detail[2] = '\''; detail[3] = '\0';
            *out_error = error_illegal_char(ps, l->pos, detail);
            for (int i = 0; i < *out_count; i++) token_free(&tokens[i]);
            free(tokens);
            *out_count = 0;
            return NULL;
        }
    }
    if (*out_count >= cap) { cap *= 2; tokens = realloc(tokens, cap * sizeof(Token)); }
    tokens[(*out_count)++] = token_create(TT_EOF, NULL, &l->pos, NULL);
    return tokens;
}

void lexer_free(Lexer* l) {
    if (!l) return;
    free(l->fn);
    free(l->text);
    position_free(&l->pos);
    free(l);
}
