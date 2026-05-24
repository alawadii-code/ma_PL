#include "mapl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IfData* parse_if_cases(Parser* p, ParseResult* res, const char* case_keyword, int* has_err);

Parser* parser_create(Token* tokens, int count) {
    Parser* p = calloc(1, sizeof(Parser));
    p->tokens = tokens;
    p->count = count;
    p->tok_idx = -1;
    parser_advance(p);
    return p;
}

void parser_advance(Parser* p) {
    p->tok_idx++;
    if (p->tok_idx >= 0 && p->tok_idx < p->count)
        p->current_tok = p->tokens[p->tok_idx];
}

void parser_reverse(Parser* p, int amount) {
    p->tok_idx -= amount;
    if (p->tok_idx >= 0 && p->tok_idx < p->count)
        p->current_tok = p->tokens[p->tok_idx];
}

void parser_free(Parser* p) {
    free(p);
}

/* forward declarations */
static ParseResult* statements(Parser* p);
static ParseResult* statement(Parser* p);
static ParseResult* expr(Parser* p);
static ParseResult* comp_expr(Parser* p);
static ParseResult* arith_expr(Parser* p);
static ParseResult* term(Parser* p);
static ParseResult* factor(Parser* p);
static ParseResult* power(Parser* p);
static ParseResult* call(Parser* p);
static ParseResult* atom(Parser* p);
static ParseResult* list_expr(Parser* p);
static ParseResult* if_expr(Parser* p);
static ParseResult* for_expr(Parser* p);
static ParseResult* while_expr(Parser* p);
static ParseResult* func_def(Parser* p);

/* binary operator helper */
typedef ParseResult* (*ParseFn)(Parser*);

typedef struct { TokenType type; const char* keyword; } OpEntry;

static int op_in_list(Parser* p, OpEntry* ops, int n) {
    for (int i = 0; i < n; i++) {
        if (ops[i].keyword) {
            if (token_matches(&p->current_tok, ops[i].type, ops[i].keyword)) return 1;
        } else {
            if (p->current_tok.type == ops[i].type) return 1;
        }
    }
    return 0;
}

static ParseResult* bin_op(Parser* p, ParseFn func_a, OpEntry* ops, int n, ParseFn func_b) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    Node* left = parse_result_register(res, func_a(p));
    if (res->has_error) return res;

    while (op_in_list(p, ops, n)) {
        Token op_tok = p->current_tok;
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* right = parse_result_register(res, func_b(p));
        if (res->has_error) {
            if (!res->has_error) res->error = error_none();
            return res;
        }
        left = node_create_binop(left, op_tok, right);
    }
    parse_result_success(res, left);
    return res;
}

/* ── parse ── */
ParseResult* parser_parse(Parser* p) {
    ParseResult* res = statements(p);
    if (!res->has_error && p->current_tok.type != TT_EOF) {
        parse_result_failure(res, error_invalid_syntax(
            p->current_tok.pos_start, p->current_tok.pos_end,
            "Token cannot appear after previous tokens"));
    }
    return res;
}

/* ── statements ── */
static ParseResult* statements(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    NodeList list;
    node_list_init(&list);
    Position pos_start = position_copy(&p->current_tok.pos_start);

    while (p->current_tok.type == TT_NEWLINE) {
        parse_result_register_advancement(res);
        parser_advance(p);
    }

    Node* stmt = parse_result_register(res, statement(p));
    if (res->has_error) { node_list_free(&list); return res; }
    node_list_add(&list, stmt);

    int more = 1;
    while (more) {
        int nl_count = 0;
        while (p->current_tok.type == TT_NEWLINE) {
            parse_result_register_advancement(res);
            parser_advance(p);
            nl_count++;
        }
        if (nl_count == 0) more = 0;
        if (!more) break;

        ParseResult* r2 = calloc(1, sizeof(ParseResult));
        parse_result_init(r2);
        Node* s = parse_result_try_register(res, statement(p));
        if (!s) {
            parser_reverse(p, res->to_reverse_count);
            more = 0;
            free(r2);
            continue;
        }
        node_list_add(&list, s);
        free(r2);
    }

    Node* list_node = node_create_list(&list, pos_start, p->current_tok.pos_end);
    parse_result_success(res, list_node);
    return res;
}

/* ── statement ── */
static ParseResult* statement(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    Position pos_start = position_copy(&p->current_tok.pos_start);

    if (token_matches(&p->current_tok, TT_KEYWORD, "RETURN")) {
        parse_result_register_advancement(res);
        parser_advance(p);
        ParseResult* r2 = calloc(1, sizeof(ParseResult));
        parse_result_init(r2);
        Node* e = parse_result_try_register(res, expr(p));
        free(r2);
        if (!e) parser_reverse(p, res->to_reverse_count);
        parse_result_success(res, node_create_return(e, pos_start, p->current_tok.pos_start));
        return res;
    }

    if (token_matches(&p->current_tok, TT_KEYWORD, "CONTINUE")) {
        parse_result_register_advancement(res);
        parser_advance(p);
        parse_result_success(res, node_create_continue(pos_start, p->current_tok.pos_start));
        return res;
    }

    if (token_matches(&p->current_tok, TT_KEYWORD, "BREAK")) {
        parse_result_register_advancement(res);
        parser_advance(p);
        parse_result_success(res, node_create_break(pos_start, p->current_tok.pos_start));
        return res;
    }

    Node* e = parse_result_register(res, expr(p));
    if (res->has_error) {
        parse_result_failure(res, error_invalid_syntax(
            p->current_tok.pos_start, p->current_tok.pos_end,
            "Expected 'RETURN', 'CONTINUE', 'BREAK', 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"));
    }
    parse_result_success(res, e);
    return res;
}

/* ── expr ── */
static ParseResult* expr(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);

    if (token_matches(&p->current_tok, TT_KEYWORD, "VAR")) {
        parse_result_register_advancement(res);
        parser_advance(p);

        if (p->current_tok.type != TT_IDENTIFIER) {
            parse_result_failure(res, error_invalid_syntax(
                p->current_tok.pos_start, p->current_tok.pos_end, "Expected identifier"));
            return res;
        }
        Token var_name = p->current_tok;
        parse_result_register_advancement(res);
        parser_advance(p);

        if (p->current_tok.type != TT_EQ) {
            parse_result_failure(res, error_invalid_syntax(
                p->current_tok.pos_start, p->current_tok.pos_end, "Expected '='"));
            return res;
        }
        parse_result_register_advancement(res);
        parser_advance(p);

        Node* expr_node = parse_result_register(res, expr(p));
        if (res->has_error) return res;
        /* need to copy the token since we'll keep it in the node */
        Token vc = var_name;
        var_name.value = strdup(var_name.value); /* owned copy */
        vc.pos_start = position_copy(&var_name.pos_start);
        vc.pos_end = position_copy(&var_name.pos_end);
        Token final_name = token_create(TT_IDENTIFIER, var_name.value, &var_name.pos_start, &var_name.pos_end);
        parse_result_success(res, node_create_var_assign(final_name, expr_node));
        return res;
    }

    OpEntry ops[] = {{TT_KEYWORD, "AND"}, {TT_KEYWORD, "OR"}};
    ParseResult* r = bin_op(p, comp_expr, ops, 2, comp_expr);
    if (r->has_error) {
        parse_result_failure(r, error_invalid_syntax(
            p->current_tok.pos_start, p->current_tok.pos_end,
            "Expected 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"));
    }
    return r;
}

/* ── comp_expr ── */
static ParseResult* comp_expr(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);

    if (token_matches(&p->current_tok, TT_KEYWORD, "NOT")) {
        Token op_tok = p->current_tok;
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* node = parse_result_register(res, comp_expr(p));
        if (res->has_error) return res;
        parse_result_success(res, node_create_unary_op(op_tok, node));
        return res;
    }

    OpEntry ops[] = {
        {TT_EE, NULL}, {TT_NE, NULL}, {TT_LT, NULL},
        {TT_GT, NULL}, {TT_LTE, NULL}, {TT_GTE, NULL}
    };
    return bin_op(p, arith_expr, ops, 6, arith_expr);
}

/* ── arith_expr ── */
static ParseResult* arith_expr(Parser* p) {
    OpEntry ops[] = {{TT_PLUS, NULL}, {TT_MINUS, NULL}};
    return bin_op(p, term, ops, 2, term);
}

/* ── term ── */
static ParseResult* term(Parser* p) {
    OpEntry ops[] = {{TT_MUL, NULL}, {TT_DIV, NULL}};
    return bin_op(p, factor, ops, 2, factor);
}

/* ── factor ── */
static ParseResult* factor(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    Token tok = p->current_tok;

    if (tok.type == TT_PLUS || tok.type == TT_MINUS) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* factor_node = parse_result_register(res, factor(p));
        if (res->has_error) return res;
        parse_result_success(res, node_create_unary_op(tok, factor_node));
        return res;
    }
    return power(p);
}

/* ── power ── */
static ParseResult* power(Parser* p) {
    OpEntry ops[] = {{TT_POW, NULL}};
    return bin_op(p, call, ops, 1, factor);
}

/* ── call ── */
static ParseResult* call(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    Node* atom_node = parse_result_register(res, atom(p));
    if (res->has_error) return res;

    if (p->current_tok.type == TT_LPAREN) {
        parse_result_register_advancement(res);
        parser_advance(p);
        NodeList args;
        node_list_init(&args);

        if (p->current_tok.type == TT_RPAREN) {
            parse_result_register_advancement(res);
            parser_advance(p);
        } else {
            Node* arg = parse_result_register(res, expr(p));
            if (res->has_error) {
                node_list_free(&args);
                parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end,
                    "Expected ')', 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"));
                return res;
            }
            node_list_add(&args, arg);
            while (p->current_tok.type == TT_COMMA) {
                parse_result_register_advancement(res);
                parser_advance(p);
                arg = parse_result_register(res, expr(p));
                if (res->has_error) { node_list_free(&args); return res; }
                node_list_add(&args, arg);
            }
            if (p->current_tok.type != TT_RPAREN) {
                node_list_free(&args);
                parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected ',' or ')'"));
                return res;
            }
            parse_result_register_advancement(res);
            parser_advance(p);
        }
        parse_result_success(res, node_create_call(atom_node, &args));
        return res;
    }
    parse_result_success(res, atom_node);
    return res;
}

/* ── atom ── */
static ParseResult* atom(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    Token tok = p->current_tok;

    if (tok.type == TT_INT || tok.type == TT_FLOAT) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Token copy_tok = token_create(tok.type, tok.value, &tok.pos_start, &tok.pos_end);
        parse_result_success(res, node_create_number(copy_tok));
    } else if (tok.type == TT_STRING) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Token copy_tok = token_create(tok.type, tok.value, &tok.pos_start, &tok.pos_end);
        parse_result_success(res, node_create_string(copy_tok));
    } else if (tok.type == TT_IDENTIFIER) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Token copy_tok = token_create(tok.type, tok.value, &tok.pos_start, &tok.pos_end);
        parse_result_success(res, node_create_var_access(copy_tok));
    } else if (tok.type == TT_LPAREN) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* expr_node = parse_result_register(res, expr(p));
        if (res->has_error) return res;
        if (p->current_tok.type == TT_RPAREN) {
            parse_result_register_advancement(res);
            parser_advance(p);
            parse_result_success(res, expr_node);
        } else {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected ')'"));
        }
    } else if (tok.type == TT_LSQUARE) {
        ParseResult* r2 = list_expr(p);
        if (r2->has_error) { res->has_error = r2->has_error; res->error = r2->error; free(r2); return res; }
        parse_result_success(res, r2->node);
        r2->node = NULL; free(r2);
    } else if (token_matches(&tok, TT_KEYWORD, "IF")) {
        ParseResult* r2 = if_expr(p);
        if (r2->has_error) { res->has_error = r2->has_error; res->error = r2->error; free(r2); return res; }
        parse_result_success(res, r2->node);
        r2->node = NULL; free(r2);
    } else if (token_matches(&tok, TT_KEYWORD, "FOR")) {
        ParseResult* r2 = for_expr(p);
        if (r2->has_error) { res->has_error = r2->has_error; res->error = r2->error; free(r2); return res; }
        parse_result_success(res, r2->node);
        r2->node = NULL; free(r2);
    } else if (token_matches(&tok, TT_KEYWORD, "WHILE")) {
        ParseResult* r2 = while_expr(p);
        if (r2->has_error) { res->has_error = r2->has_error; res->error = r2->error; free(r2); return res; }
        parse_result_success(res, r2->node);
        r2->node = NULL; free(r2);
    } else if (token_matches(&tok, TT_KEYWORD, "FUN")) {
        ParseResult* r2 = func_def(p);
        if (r2->has_error) { res->has_error = r2->has_error; res->error = r2->error; free(r2); return res; }
        parse_result_success(res, r2->node);
        r2->node = NULL; free(r2);
    } else {
        parse_result_failure(res, error_invalid_syntax(
            tok.pos_start, tok.pos_end,
            "Expected int, float, identifier, '+', '-', '(', '[', 'IF', 'FOR', 'WHILE', 'FUN'"));
    }
    return res;
}

/* ── list_expr ── */
static ParseResult* list_expr(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    NodeList elements;
    node_list_init(&elements);
    Position pos_start = position_copy(&p->current_tok.pos_start);

    if (p->current_tok.type != TT_LSQUARE) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected '['"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    if (p->current_tok.type == TT_RSQUARE) {
        parse_result_register_advancement(res);
        parser_advance(p);
    } else {
        Node* el = parse_result_register(res, expr(p));
        if (res->has_error) { node_list_free(&elements); return res; }
        node_list_add(&elements, el);
        while (p->current_tok.type == TT_COMMA) {
            parse_result_register_advancement(res);
            parser_advance(p);
            el = parse_result_register(res, expr(p));
            if (res->has_error) { node_list_free(&elements); return res; }
            node_list_add(&elements, el);
        }
        if (p->current_tok.type != TT_RSQUARE) {
            node_list_free(&elements);
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected ',' or ']'"));
            return res;
        }
        parse_result_register_advancement(res);
        parser_advance(p);
    }
    parse_result_success(res, node_create_list(&elements, pos_start, p->current_tok.pos_end));
    return res;
}

/* ── if_expr ── */
static IfData* parse_if_cases(Parser* p, ParseResult* res, const char* case_keyword, int* has_err) {
    IfData* id = calloc(1, sizeof(IfData));
    id->has_else = 0;
    *has_err = 0;

    if (!token_matches(&p->current_tok, TT_KEYWORD, case_keyword)) {
        parse_result_failure(res, error_invalid_syntax(
            p->current_tok.pos_start, p->current_tok.pos_end, "Expected keyword"));
        *has_err = 1; return id;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    ParseResult* r2 = calloc(1, sizeof(ParseResult));
    parse_result_init(r2);
    Node* cond = parse_result_register(res, expr(p));
    free(r2);
    if (res->has_error) { *has_err = 1; return id; }

    if (!token_matches(&p->current_tok, TT_KEYWORD, "THEN")) {
        parse_result_failure(res, error_invalid_syntax(
            p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'THEN'"));
        *has_err = 1; return id;
    }
    parser_advance(p);

    IfCase* c = calloc(1, sizeof(IfCase));
    int should_ret_null = 0;

    if (p->current_tok.type == TT_NEWLINE) {
        parser_advance(p);
        should_ret_null = 1;
        ParseResult* r3 = statements(p);
        if (r3->has_error) { *has_err = 1; free(r3); free(c); return id; }
        c->condition = cond;
        c->expr = r3->node;
        c->should_return_null = 1;
        free(r3);
        id->count = 1; id->cases = c;
        id->capacity = 1;

        if (token_matches(&p->current_tok, TT_KEYWORD, "END")) {
            parser_advance(p);
        } else {
            /* parse ELIF or ELSE */
            if (token_matches(&p->current_tok, TT_KEYWORD, "ELIF")) {
                int err2 = 0;
                IfData* next = parse_if_cases(p, res, "ELIF", &err2);
                if (err2) { *has_err = 1; return id; }
                /* merge */
                int total = 1 + next->count + (next->has_else ? 1 : 0);
                IfCase* merged = calloc(total, sizeof(IfCase));
                merged[0] = *c; free(c);
                for (int i = 0; i < next->count; i++) merged[1 + i] = next->cases[i];
                if (next->has_else) merged[1 + next->count] = next->else_case;
                free(next->cases);
                id->count = 1 + next->count;
                if (next->has_else) { id->has_else = 1; id->else_case = next->else_case; }
                id->cases = merged;
                id->capacity = total;
                free(next);
            } else if (token_matches(&p->current_tok, TT_KEYWORD, "ELSE")) {
                parser_advance(p);
                if (p->current_tok.type == TT_NEWLINE) {
                    parser_advance(p);
                    ParseResult* r4 = statements(p);
                    if (r4->has_error) { *has_err = 1; free(r4); return id; }
                    id->has_else = 1;
                    id->else_case.condition = NULL;
                    id->else_case.expr = r4->node;
                    id->else_case.should_return_null = 1;
                    free(r4);
                    if (token_matches(&p->current_tok, TT_KEYWORD, "END")) parser_advance(p);
                    else { parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'END'")); *has_err = 1; return id; }
                } else {
                    ParseResult* r4 = statement(p);
                    if (r4->has_error) { *has_err = 1; free(r4); return id; }
                    id->has_else = 1;
                    id->else_case.condition = NULL;
                    id->else_case.expr = r4->node;
                    id->else_case.should_return_null = 0;
                    free(r4);
                }
            }
        }
    } else {
        ParseResult* r3 = statement(p);
        if (r3->has_error) { *has_err = 1; free(c); free(r3); return id; }
        c->condition = cond;
        c->expr = r3->node;
        c->should_return_null = 0;
        free(r3);
        id->count = 1; id->cases = c;
        id->capacity = 1;

        if (token_matches(&p->current_tok, TT_KEYWORD, "ELIF")) {
            int err2 = 0;
            IfData* next = parse_if_cases(p, res, "ELIF", &err2);
            if (err2) { *has_err = 1; return id; }
            int total = 1 + next->count + (next->has_else ? 1 : 0);
            IfCase* merged = calloc(total, sizeof(IfCase));
            merged[0] = *c; free(c);
            for (int i = 0; i < next->count; i++) merged[1 + i] = next->cases[i];
            if (next->has_else) merged[1 + next->count] = next->else_case;
            free(next->cases);
            id->count = 1 + next->count;
            if (next->has_else) { id->has_else = 1; id->else_case = next->else_case; }
            id->cases = merged;
            id->capacity = total;
            free(next);
        } else if (token_matches(&p->current_tok, TT_KEYWORD, "ELSE")) {
            parser_advance(p);
            if (p->current_tok.type == TT_NEWLINE) {
                parser_advance(p);
                ParseResult* r4 = statements(p);
                if (r4->has_error) { *has_err = 1; free(r4); return id; }
                id->has_else = 1;
                id->else_case.condition = NULL;
                id->else_case.expr = r4->node;
                id->else_case.should_return_null = 1;
                free(r4);
                if (token_matches(&p->current_tok, TT_KEYWORD, "END")) parser_advance(p);
                else { parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'END'")); *has_err = 1; return id; }
            } else {
                ParseResult* r4 = statement(p);
                if (r4->has_error) { *has_err = 1; free(r4); return id; }
                id->has_else = 1;
                id->else_case.condition = NULL;
                id->else_case.expr = r4->node;
                id->else_case.should_return_null = 0;
                free(r4);
            }
        }
    }
    return id;
}

static ParseResult* if_expr(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);
    int has_err = 0;
    IfData* id = parse_if_cases(p, res, "IF", &has_err);
    if (has_err) { free(id); return res; }
    parse_result_success(res, node_create_if(id));
    free(id);
    return res;
}

/* ── for_expr ── */
static ParseResult* for_expr(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);

    if (!token_matches(&p->current_tok, TT_KEYWORD, "FOR")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'FOR'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    if (p->current_tok.type != TT_IDENTIFIER) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected identifier"));
        return res;
    }
    Token var_name = p->current_tok;
    Token var_name_c = token_create(TT_IDENTIFIER, var_name.value, &var_name.pos_start, &var_name.pos_end);
    parse_result_register_advancement(res);
    parser_advance(p);

    if (p->current_tok.type != TT_EQ) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected '='"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    Node* sv = parse_result_register(res, expr(p));
    if (res->has_error) return res;

    if (!token_matches(&p->current_tok, TT_KEYWORD, "TO")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'TO'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    Node* ev = parse_result_register(res, expr(p));
    if (res->has_error) return res;

    Node* stepv = NULL;
    if (token_matches(&p->current_tok, TT_KEYWORD, "STEP")) {
        parse_result_register_advancement(res);
        parser_advance(p);
        stepv = parse_result_register(res, expr(p));
        if (res->has_error) return res;
    }

    if (!token_matches(&p->current_tok, TT_KEYWORD, "THEN")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'THEN'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    ForData fd;
    fd.var_name_tok = var_name_c;
    fd.start_value = sv;
    fd.end_value = ev;
    fd.step_value = stepv;
    fd.body = NULL;
    fd.should_return_null = 0;

    if (p->current_tok.type == TT_NEWLINE) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* body = parse_result_register(res, statements(p));
        if (res->has_error) return res;
        fd.body = body;
        fd.should_return_null = 1;
        if (!token_matches(&p->current_tok, TT_KEYWORD, "END")) {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'END'"));
            return res;
        }
        parse_result_register_advancement(res);
        parser_advance(p);
    } else {
        Node* body = parse_result_register(res, statement(p));
        if (res->has_error) return res;
        fd.body = body;
    }

    parse_result_success(res, node_create_for(&fd));
    return res;
}

/* ── while_expr ── */
static ParseResult* while_expr(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);

    if (!token_matches(&p->current_tok, TT_KEYWORD, "WHILE")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'WHILE'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    Node* cond = parse_result_register(res, expr(p));
    if (res->has_error) return res;

    if (!token_matches(&p->current_tok, TT_KEYWORD, "THEN")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'THEN'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    WhileData wd;
    wd.condition = cond;
    wd.body = NULL;
    wd.should_return_null = 0;

    if (p->current_tok.type == TT_NEWLINE) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* body = parse_result_register(res, statements(p));
        if (res->has_error) return res;
        wd.body = body;
        wd.should_return_null = 1;
        if (!token_matches(&p->current_tok, TT_KEYWORD, "END")) {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'END'"));
            return res;
        }
        parse_result_register_advancement(res);
        parser_advance(p);
    } else {
        Node* body = parse_result_register(res, statement(p));
        if (res->has_error) return res;
        wd.body = body;
    }

    parse_result_success(res, node_create_while(&wd));
    return res;
}

/* ── func_def ── */
static ParseResult* func_def(Parser* p) {
    ParseResult* res = calloc(1, sizeof(ParseResult));
    parse_result_init(res);

    if (!token_matches(&p->current_tok, TT_KEYWORD, "FUN")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'FUN'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    FuncDefData fd;
    memset(&fd, 0, sizeof(FuncDefData));
    fd.has_name = 0;
    token_list_init(&fd.arg_names);

    if (p->current_tok.type == TT_IDENTIFIER) {
        fd.has_name = 1;
        fd.var_name_tok = token_create(TT_IDENTIFIER, p->current_tok.value,
                                        &p->current_tok.pos_start, &p->current_tok.pos_end);
        parse_result_register_advancement(res);
        parser_advance(p);
        if (p->current_tok.type != TT_LPAREN) {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected '('"));
            return res;
        }
    } else {
        if (p->current_tok.type != TT_LPAREN) {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected identifier or '('"));
            return res;
        }
    }

    parse_result_register_advancement(res);
    parser_advance(p);

    if (p->current_tok.type == TT_IDENTIFIER) {
        token_list_add(&fd.arg_names,
            token_create(TT_IDENTIFIER, p->current_tok.value,
                         &p->current_tok.pos_start, &p->current_tok.pos_end));
        parse_result_register_advancement(res);
        parser_advance(p);
        while (p->current_tok.type == TT_COMMA) {
            parse_result_register_advancement(res);
            parser_advance(p);
            if (p->current_tok.type != TT_IDENTIFIER) {
                parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected identifier"));
                return res;
            }
            token_list_add(&fd.arg_names,
                token_create(TT_IDENTIFIER, p->current_tok.value,
                             &p->current_tok.pos_start, &p->current_tok.pos_end));
            parse_result_register_advancement(res);
            parser_advance(p);
        }
        if (p->current_tok.type != TT_RPAREN) {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected ',' or ')'"));
            return res;
        }
    } else {
        if (p->current_tok.type != TT_RPAREN) {
            parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected identifier or ')'"));
            return res;
        }
    }

    parse_result_register_advancement(res);
    parser_advance(p);

    if (p->current_tok.type == TT_ARROW) {
        parse_result_register_advancement(res);
        parser_advance(p);
        Node* body = parse_result_register(res, expr(p));
        if (res->has_error) return res;
        fd.body = body;
        fd.should_auto_return = 1;
        parse_result_success(res, node_create_func_def(&fd));
        return res;
    }

    if (p->current_tok.type != TT_NEWLINE) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected '->' or NEWLINE"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    Node* body = parse_result_register(res, statements(p));
    if (res->has_error) return res;

    if (!token_matches(&p->current_tok, TT_KEYWORD, "END")) {
        parse_result_failure(res, error_invalid_syntax(p->current_tok.pos_start, p->current_tok.pos_end, "Expected 'END'"));
        return res;
    }
    parse_result_register_advancement(res);
    parser_advance(p);

    fd.body = body;
    fd.should_auto_return = 0;
    parse_result_success(res, node_create_func_def(&fd));
    return res;
}
