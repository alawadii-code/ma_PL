#include "mapl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* ══════════════════════════════════════════════
   Token
   ══════════════════════════════════════════════ */

const char* TOKEN_NAMES[] = {
    "INT","FLOAT","STRING","IDENTIFIER","KEYWORD",
    "PLUS","MINUS","MUL","DIV","POW",
    "EQ","LPAREN","RPAREN","LSQUARE","RSQUARE",
    "EE","NE","LT","GT","LTE","GTE",
    "COMMA","ARROW","NEWLINE","EOF","NONE"
};

static const char* const KEYWORDS[] = {
    "VAR","AND","OR","NOT","IF","ELIF","ELSE",
    "FOR","TO","STEP","WHILE","FUN","THEN","END",
    "RETURN","CONTINUE","BREAK", NULL
};

int is_keyword(const char* s) {
    for (int i = 0; KEYWORDS[i]; i++)
        if (strcmp(KEYWORDS[i], s) == 0) return 1;
    return 0;
}

Position position_create(int idx, int ln, int col, const char* fn, const char* ftxt) {
    Position p;
    p.idx = idx; p.ln = ln; p.col = col;
    p.fn = fn ? strdup(fn) : NULL;
    p.ftxt = ftxt ? strdup(ftxt) : NULL;
    return p;
}

void position_advance(Position* pos, char c) {
    pos->idx++;
    pos->col++;
    if (c == '\n') { pos->ln++; pos->col = 0; }
}

Position position_copy(const Position* pos) {
    return position_create(pos->idx, pos->ln, pos->col, pos->fn, pos->ftxt);
}

void position_free(Position* p) {
    free(p->fn); p->fn = NULL;
    free(p->ftxt); p->ftxt = NULL;
}

Token token_create(TokenType type, const char* value, Position* pos_start, Position* pos_end) {
    Token t;
    t.type = type;
    t.value = value ? strdup(value) : NULL;
    if (pos_start) {
        t.pos_start = position_copy(pos_start);
        t.pos_end = position_copy(pos_start);
        position_advance(&t.pos_end, 0);
    } else {
        t.pos_start = position_create(0,0,0,NULL,NULL);
        t.pos_end = position_create(0,0,0,NULL,NULL);
    }
    if (pos_end) { position_free(&t.pos_end); t.pos_end = position_copy(pos_end); }
    return t;
}

Token token_create_simple(TokenType type, const char* value) {
    Position p = position_create(0,0,0,NULL,NULL);
    Token t = token_create(type, value, &p, &p);
    position_free(&p);
    return t;
}

int token_matches(const Token* t, TokenType type, const char* value) {
    if (t->type != type) return 0;
    if (value && t->value) return strcmp(t->value, value) == 0;
    return value == NULL && t->value == NULL;
}

void token_free(Token* t) {
    free(t->value);
    position_free(&t->pos_start);
    position_free(&t->pos_end);
}

/* ══════════════════════════════════════════════
   StringWithArrows
   ══════════════════════════════════════════════ */

char* string_with_arrows(const char* text, Position pos_start, Position pos_end) {
    if (!text || text[0] == '\0') return strdup("");
    int text_len = (int)strlen(text);
    int idx_start = pos_start.idx;
    while (idx_start > 0 && text[idx_start] != '\n') idx_start--;
    if (idx_start > 0) idx_start++; /* skip the \n */
    int idx_end = pos_end.idx;
    while (text[idx_end] != '\0' && text[idx_end] != '\n') idx_end++;

    int line_count = pos_end.ln - pos_start.ln + 1;
    /* allocate buffer generously */
    int buf_size = (idx_end - idx_start + line_count * 80 + 100);
    char* result = calloc(buf_size, 1);
    int pos = 0;

    int cur_start = idx_start;
    int cur_end;
    for (int i = 0; i < line_count; i++) {
        cur_end = (int)(strchr(text + cur_start, '\n') ? strchr(text + cur_start, '\n') - text : text_len);
        int line_len = cur_end - cur_start;
        strncat(result + pos, text + cur_start, line_len);
        pos += line_len;
        result[pos++] = '\n';

        int col_start = (i == 0) ? pos_start.col : 0;
        int col_end = (i == line_count - 1) ? pos_end.col : line_len;
        for (int j = 0; j < col_start; j++) result[pos++] = ' ';
        for (int j = col_start; j < col_end; j++) result[pos++] = '^';
        result[pos] = '\0';
        cur_start = cur_end + 1;
    }
    return result;
}

/* ══════════════════════════════════════════════
   Error
   ══════════════════════════════════════════════ */

Error error_none(void) {
    Error e = { 0 };
    e.type = ERR_INVALID_SYNTAX; /* unused when has_error = 0 */
    return e;
}

int error_has(const Error* e) { return e->error_name != NULL; }

static Error error_make(ErrorType type, Position pos_start, Position pos_end,
                        const char* name, const char* details, Context* ctx) {
    Error e;
    e.type = type;
    e.pos_start = position_copy(&pos_start);
    e.pos_end = position_copy(&pos_end);
    e.error_name = name ? strdup(name) : NULL;
    e.details = details ? strdup(details) : NULL;
    e.context = ctx;
    return e;
}

Error error_create(ErrorType type, Position pos_start, Position pos_end,
                   const char* error_name, const char* details) {
    return error_make(type, pos_start, pos_end, error_name, details, NULL);
}

Error error_illegal_char(Position ps, Position pe, const char* d) {
    return error_make(ERR_ILLEGAL_CHAR, ps, pe, "Illegal Character", d, NULL);
}
Error error_expected_char(Position ps, Position pe, const char* d) {
    return error_make(ERR_EXPECTED_CHAR, ps, pe, "Expected Character", d, NULL);
}
Error error_invalid_syntax(Position ps, Position pe, const char* d) {
    return error_make(ERR_INVALID_SYNTAX, ps, pe, "Invalid Syntax", d, NULL);
}
Error error_runtime(Position ps, Position pe, const char* d, Context* ctx) {
    return error_make(ERR_RUNTIME, ps, pe, "Runtime Error", d, ctx);
}

char* error_as_string(const Error* err) {
    if (!err || !err->error_name) return strdup("");
    if (err->type == ERR_RUNTIME) {
        /* generate traceback */
        char trace[4096] = {0};
        int pos = 0;
        pos += sprintf(trace + pos, "Traceback (most recent call last):\n");
        Position p = err->pos_start;
        Context* ctx = err->context;
        while (ctx) {
            char line[256];
            sprintf(line, "  File %s, line %d, in %s\n",
                    p.fn ? p.fn : "<unknown>", p.ln + 1, ctx->display_name);
            int len = (int)strlen(line);
            if (pos + len >= 4096) break;
            memmove(trace + len, trace, pos);
            memcpy(trace, line, len);
            pos += len;
            p = ctx->parent_entry_pos;
            ctx = ctx->parent;
        }
        char* arrows = string_with_arrows(err->pos_start.ftxt ? err->pos_start.ftxt : "",
                                          err->pos_start, err->pos_end);
        char* result = malloc(strlen(trace) + strlen(err->error_name) + strlen(err->details) + strlen(arrows) + 20);
        sprintf(result, "%s%s: %s\n\n%s", trace, err->error_name, err->details, arrows);
        free(arrows);
        return result;
    } else {
        char* arrows = string_with_arrows(err->pos_start.ftxt ? err->pos_start.ftxt : "",
                                          err->pos_start, err->pos_end);
        char* result = malloc(strlen(err->error_name) + strlen(err->details) + strlen(arrows) + 50);
        sprintf(result, "%s: %s\nFile %s, line %d\n\n%s",
                err->error_name, err->details,
                err->pos_start.fn ? err->pos_start.fn : "<unknown>",
                err->pos_start.ln + 1, arrows);
        free(arrows);
        return result;
    }
}

void error_free(Error* err) {
    if (!err) return;
    free(err->error_name); err->error_name = NULL;
    free(err->details); err->details = NULL;
    position_free(&err->pos_start);
    position_free(&err->pos_end);
}

/* ══════════════════════════════════════════════
   Node helpers
   ══════════════════════════════════════════════ */

void node_list_init(NodeList* list) { list->items = NULL; list->count = 0; list->capacity = 0; }
void node_list_add(NodeList* list, Node* item) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 8;
        list->items = realloc(list->items, list->capacity * sizeof(Node*));
    }
    list->items[list->count++] = item;
}
void node_list_free(NodeList* list) {
    for (int i = 0; i < list->count; i++) node_free(list->items[i]);
    free(list->items);
    list->items = NULL; list->count = 0; list->capacity = 0;
}

void token_list_init(TokenList* list) { list->items = NULL; list->count = 0; list->capacity = 0; }
void token_list_add(TokenList* list, Token item) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 8;
        list->items = realloc(list->items, list->capacity * sizeof(Token));
    }
    list->items[list->count++] = item;
}
void token_list_clear(TokenList* list) {
    for (int i = 0; i < list->count; i++) token_free(&list->items[i]);
    list->count = 0;
}
void token_list_free(TokenList* list) {
    token_list_clear(list);
    free(list->items);
    list->items = NULL; list->capacity = 0;
}

/* ══════════════════════════════════════════════
   Node creation / free
   ══════════════════════════════════════════════ */

static Node* node_alloc(NodeType type) {
    Node* n = calloc(1, sizeof(Node));
    n->type = type;
    return n;
}

Node* node_create_number(Token tok) {
    Node* n = node_alloc(NODE_NUMBER);
    n->data.tok = tok;
    n->pos_start = position_copy(&tok.pos_start);
    n->pos_end = position_copy(&tok.pos_end);
    return n;
}

Node* node_create_string(Token tok) {
    Node* n = node_alloc(NODE_STRING);
    n->data.tok = tok;
    n->pos_start = position_copy(&tok.pos_start);
    n->pos_end = position_copy(&tok.pos_end);
    return n;
}

Node* node_create_list(NodeList* elements, Position pos_start, Position pos_end) {
    Node* n = node_alloc(NODE_LIST);
    n->data.list = *elements;
    n->pos_start = position_copy(&pos_start);
    n->pos_end = position_copy(&pos_end);
    return n;
}

Node* node_create_var_access(Token tok) {
    Node* n = node_alloc(NODE_VAR_ACCESS);
    n->data.tok = tok;
    n->pos_start = position_copy(&tok.pos_start);
    n->pos_end = position_copy(&tok.pos_end);
    return n;
}

Node* node_create_var_assign(Token var_name, Node* value) {
    Node* n = node_alloc(NODE_VAR_ASSIGN);
    n->data.var_assign.var_name = var_name;
    n->data.var_assign.value = value;
    n->pos_start = position_copy(&var_name.pos_start);
    n->pos_end = position_copy(&value->pos_end);
    return n;
}

Node* node_create_binop(Node* left, Token op, Node* right) {
    Node* n = node_alloc(NODE_BINOP);
    n->data.binop.left = left;
    n->data.binop.op = op;
    n->data.binop.right = right;
    n->pos_start = position_copy(&left->pos_start);
    n->pos_end = position_copy(&right->pos_end);
    return n;
}

Node* node_create_unary_op(Token op, Node* operand) {
    Node* n = node_alloc(NODE_UNARY_OP);
    n->data.unary.op = op;
    n->data.unary.operand = operand;
    n->pos_start = position_copy(&op.pos_start);
    n->pos_end = position_copy(&operand->pos_end);
    return n;
}

Node* node_create_if(IfData* data) {
    Node* n = node_alloc(NODE_IF);
    n->data.if_data = *data;
    if (data->count > 0) {
        n->pos_start = position_copy(&data->cases[0].condition->pos_start);
    } else {
        n->pos_start = position_create(0,0,0,NULL,NULL);
    }
    Node* last = data->has_else ? data->else_case.expr : data->cases[data->count-1].expr;
    n->pos_end = position_copy(&last->pos_end);
    return n;
}

Node* node_create_for(ForData* data) {
    Node* n = node_alloc(NODE_FOR);
    n->data.for_data = *data;
    n->pos_start = position_copy(&data->var_name_tok.pos_start);
    n->pos_end = position_copy(&data->body->pos_end);
    return n;
}

Node* node_create_while(WhileData* data) {
    Node* n = node_alloc(NODE_WHILE);
    n->data.while_data = *data;
    n->pos_start = position_copy(&data->condition->pos_start);
    n->pos_end = position_copy(&data->body->pos_end);
    return n;
}

Node* node_create_func_def(FuncDefData* data) {
    Node* n = node_alloc(NODE_FUNC_DEF);
    n->data.func_def = *data;
    if (data->has_name) n->pos_start = position_copy(&data->var_name_tok.pos_start);
    else if (data->arg_names.count > 0) n->pos_start = position_copy(&data->arg_names.items[0].pos_start);
    else n->pos_start = position_copy(&data->body->pos_start);
    n->pos_end = position_copy(&data->body->pos_end);
    return n;
}

Node* node_create_call(Node* to_call, NodeList* args) {
    Node* n = node_alloc(NODE_CALL);
    n->data.call.to_call = to_call;
    n->data.call.args = *args;
    n->pos_start = position_copy(&to_call->pos_start);
    if (args->count > 0) n->pos_end = position_copy(&args->items[args->count-1]->pos_end);
    else n->pos_end = position_copy(&to_call->pos_end);
    return n;
}

Node* node_create_return(Node* value, Position pos_start, Position pos_end) {
    Node* n = node_alloc(NODE_RETURN);
    n->data.ret.node_to_return = value;
    n->pos_start = position_copy(&pos_start);
    n->pos_end = position_copy(&pos_end);
    return n;
}

Node* node_create_continue(Position pos_start, Position pos_end) {
    Node* n = node_alloc(NODE_CONTINUE);
    n->pos_start = position_copy(&pos_start);
    n->pos_end = position_copy(&pos_end);
    return n;
}

Node* node_create_break(Position pos_start, Position pos_end) {
    Node* n = node_alloc(NODE_BREAK);
    n->pos_start = position_copy(&pos_start);
    n->pos_end = position_copy(&pos_end);
    return n;
}

void node_free(Node* node) {
    if (!node) return;
    switch (node->type) {
        case NODE_NUMBER: case NODE_STRING: case NODE_VAR_ACCESS:
            token_free(&node->data.tok); break;
        case NODE_LIST: node_list_free(&node->data.list); break;
        case NODE_VAR_ASSIGN:
            token_free(&node->data.var_assign.var_name);
            node_free(node->data.var_assign.value); break;
        case NODE_BINOP:
            node_free(node->data.binop.left);
            token_free(&node->data.binop.op);
            node_free(node->data.binop.right); break;
        case NODE_UNARY_OP:
            token_free(&node->data.unary.op);
            node_free(node->data.unary.operand); break;
        case NODE_IF:
            for (int i = 0; i < node->data.if_data.count; i++) {
                node_free(node->data.if_data.cases[i].condition);
                node_free(node->data.if_data.cases[i].expr);
            }
            free(node->data.if_data.cases);
            if (node->data.if_data.has_else) {
                node_free(node->data.if_data.else_case.expr);
            }
            break;
        case NODE_FOR:
            token_free(&node->data.for_data.var_name_tok);
            node_free(node->data.for_data.start_value);
            node_free(node->data.for_data.end_value);
            node_free(node->data.for_data.step_value);
            node_free(node->data.for_data.body); break;
        case NODE_WHILE:
            node_free(node->data.while_data.condition);
            node_free(node->data.while_data.body); break;
        case NODE_FUNC_DEF:
            if (node->data.func_def.has_name) token_free(&node->data.func_def.var_name_tok);
            token_list_free(&node->data.func_def.arg_names);
            node_free(node->data.func_def.body); break;
        case NODE_CALL:
            node_free(node->data.call.to_call);
            node_list_free(&node->data.call.args); break;
        case NODE_RETURN:
            node_free(node->data.ret.node_to_return); break;
        default: break;
    }
    position_free(&node->pos_start);
    position_free(&node->pos_end);
    free(node);
}

/* ══════════════════════════════════════════════
   ParseResult
   ══════════════════════════════════════════════ */

void parse_result_init(ParseResult* res) {
    memset(res, 0, sizeof(ParseResult));
}

Node* parse_result_register(ParseResult* res, ParseResult* other) {
    res->last_registered_advance_count = other->advance_count;
    res->advance_count += other->advance_count;
    if (other->has_error && !res->has_error) {
        res->has_error = 1;
        res->error = other->error;
        /* copy ownership: prevent double free */
        /* we take ownership, so null out the source's fields */
        memset(&other->error, 0, sizeof(Error));
        other->has_error = 0;
    }
    return other->node;
}

Node* parse_result_try_register(ParseResult* res, ParseResult* other) {
    if (other->has_error) {
        res->to_reverse_count = other->advance_count;
        return NULL;
    }
    return parse_result_register(res, other);
}

void parse_result_register_advancement(ParseResult* res) {
    res->last_registered_advance_count = 1;
    res->advance_count++;
}

ParseResult* parse_result_success(ParseResult* res, Node* node) {
    res->node = node;
    return res;
}

ParseResult* parse_result_failure(ParseResult* res, Error err) {
    if (!res->has_error || res->last_registered_advance_count == 0) {
        if (res->has_error) error_free(&res->error);
        res->error = err;
        res->has_error = 1;
    }
    return res;
}

/* ══════════════════════════════════════════════
   SymbolTable
   ══════════════════════════════════════════════ */

SymbolTable* symbol_table_create(SymbolTable* parent) {
    SymbolTable* st = calloc(1, sizeof(SymbolTable));
    st->parent = parent;
    return st;
}

void symbol_table_free(SymbolTable* st) {
    if (!st) return;
    for (int i = 0; i < st->count; i++) {
        free(st->entries[i].name);
        value_free(st->entries[i].value);
    }
    free(st->entries);
    free(st);
}

Value* symbol_table_get(SymbolTable* st, const char* name) {
    for (int i = 0; i < st->count; i++)
        if (strcmp(st->entries[i].name, name) == 0) return st->entries[i].value;
    if (st->parent) return symbol_table_get(st->parent, name);
    return NULL;
}

void symbol_table_set(SymbolTable* st, const char* name, Value* value) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            value_free(st->entries[i].value);
            st->entries[i].value = value;
            return;
        }
    }
    if (st->count >= st->capacity) {
        st->capacity = st->capacity ? st->capacity * 2 : 16;
        st->entries = realloc(st->entries, st->capacity * sizeof(SymbolEntry));
    }
    st->entries[st->count].name = strdup(name);
    st->entries[st->count].value = value;
    st->count++;
}

void symbol_table_remove(SymbolTable* st, const char* name) {
    for (int i = 0; i < st->count; i++) {
        if (strcmp(st->entries[i].name, name) == 0) {
            free(st->entries[i].name);
            value_free(st->entries[i].value);
            st->entries[i] = st->entries[--st->count];
            return;
        }
    }
}

/* ══════════════════════════════════════════════
   Context
   ══════════════════════════════════════════════ */

Context* context_create(const char* display_name, Context* parent, Position parent_entry_pos) {
    Context* ctx = calloc(1, sizeof(Context));
    ctx->display_name = strdup(display_name);
    ctx->parent = parent;
    if (parent_entry_pos.fn) ctx->parent_entry_pos = position_copy(&parent_entry_pos);
    ctx->symbol_table = NULL;
    return ctx;
}

void context_free(Context* ctx) {
    if (!ctx) return;
    free(ctx->display_name);
    if (!ctx->parent) symbol_table_free(ctx->symbol_table);
    free(ctx);
}

/* ══════════════════════════════════════════════
   Value
   ══════════════════════════════════════════════ */

Value* value_number(double n) {
    Value* v = calloc(1, sizeof(Value));
    v->type = VAL_NUMBER;
    v->data.number = n;
    return v;
}

Value* value_string(const char* s) {
    Value* v = calloc(1, sizeof(Value));
    v->type = VAL_STRING;
    v->data.string = s ? strdup(s) : NULL;
    return v;
}

Value* value_list(void) {
    Value* v = calloc(1, sizeof(Value));
    v->type = VAL_LIST;
    return v;
}

void value_list_append(Value* list, Value* item) {
    if (list->type != VAL_LIST) return;
    if (list->data.list.count >= list->data.list.capacity) {
        list->data.list.capacity = list->data.list.capacity ? list->data.list.capacity * 2 : 8;
        list->data.list.items = realloc(list->data.list.items,
                                         list->data.list.capacity * sizeof(Value*));
    }
    list->data.list.items[list->data.list.count++] = item;
}

int value_list_len(const Value* v) {
    return (v->type == VAL_LIST) ? v->data.list.count : 0;
}

Value* value_list_get(const Value* list, int index) {
    if (list->type != VAL_LIST || index < 0 || index >= list->data.list.count) return NULL;
    return list->data.list.items[index];
}

void value_list_remove(Value* list, int index) {
    if (list->type != VAL_LIST || index < 0 || index >= list->data.list.count) return;
    for (int i = index; i < list->data.list.count - 1; i++)
        list->data.list.items[i] = list->data.list.items[i+1];
    list->data.list.count--;
}

Value* value_copy(const Value* v) {
    if (!v) return NULL;
    Value* c = calloc(1, sizeof(Value));
    c->type = v->type;
    c->pos_start = position_copy(&v->pos_start);
    c->pos_end = position_copy(&v->pos_end);
    c->context = v->context;
    switch (v->type) {
        case VAL_NUMBER: c->data.number = v->data.number; break;
        case VAL_STRING: c->data.string = v->data.string ? strdup(v->data.string) : NULL; break;
        case VAL_LIST:
            c->data.list.count = v->data.list.count;
            c->data.list.capacity = v->data.list.capacity;
            if (c->data.list.capacity > 0) {
                c->data.list.items = calloc(c->data.list.capacity, sizeof(Value*));
                for (int i = 0; i < c->data.list.count; i++)
                    c->data.list.items[i] = value_copy(v->data.list.items[i]);
            }
            break;
    }
    return c;
}

void value_free(Value* v) {
    if (!v) return;
    position_free(&v->pos_start);
    position_free(&v->pos_end);
    if (v->type == VAL_STRING) free(v->data.string);
    else if (v->type == VAL_LIST) {
        for (int i = 0; i < v->data.list.count; i++) value_free(v->data.list.items[i]);
        free(v->data.list.items);
    }
    free(v);
}

int value_is_true(const Value* v) {
    switch (v->type) {
        case VAL_NUMBER: return v->data.number != 0;
        case VAL_STRING: return v->data.string && v->data.string[0] != '\0';
        case VAL_LIST: return v->data.list.count > 0;
    }
    return 0;
}

char* value_to_string(const Value* v) {
    char buf[256];
    switch (v->type) {
        case VAL_NUMBER: {
            double d = v->data.number;
            if (d == floor(d) && !isinf(d)) sprintf(buf, "%lld", (long long)d);
            else sprintf(buf, "%g", d);
            return strdup(buf);
        }
        case VAL_STRING: return strdup(v->data.string ? v->data.string : "");
        case VAL_LIST: {
            char* result = strdup("[");
            for (int i = 0; i < v->data.list.count; i++) {
                if (i > 0) {
                    char* old = result;
                    result = malloc(strlen(result) + 3);
                    sprintf(result, "%s, ", old);
                    free(old);
                }
                char* elem_str = value_to_string(v->data.list.items[i]);
                char* old = result;
                result = malloc(strlen(result) + strlen(elem_str) + 1);
                sprintf(result, "%s%s", old, elem_str);
                free(old);
                free(elem_str);
            }
            char* old = result;
            result = malloc(strlen(result) + 2);
            sprintf(result, "%s]", old);
            free(old);
            return result;
        }
    }
    return strdup("");
}

/* ══════════════════════════════════════════════
   Value operations
   ══════════════════════════════════════════════ */

static RTResult illegal_op(Value* a, Value* b) {
    RTResult res; rt_result_init(&res);
    if (!b) b = a;
    rt_result_failure(&res, error_runtime(a->pos_start, b->pos_end, "Illegal operation", a->context));
    return res;
}

RTResult value_added_to(const Value* a, const Value* b) {
    RTResult res; rt_result_init(&res);
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
        Value* r = value_number(a->data.number + b->data.number);
        r->context = a->context;
        rt_result_success(&res, r);
    } else if (a->type == VAL_STRING && b->type == VAL_STRING) {
        char* concat = malloc(strlen(a->data.string) + strlen(b->data.string) + 1);
        sprintf(concat, "%s%s", a->data.string, b->data.string);
        Value* r = value_string(concat);
        free(concat);
        r->context = a->context;
        rt_result_success(&res, r);
    } else if (a->type == VAL_LIST) {
        Value* new_list = value_copy(a);
        value_list_append(new_list, value_copy(b));
        rt_result_success(&res, new_list);
    } else {
        return illegal_op((Value*)a, (Value*)b);
    }
    return res;
}

RTResult value_subbed_by(const Value* a, const Value* b) {
    RTResult res; rt_result_init(&res);
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
        Value* r = value_number(a->data.number - b->data.number);
        r->context = a->context;
        rt_result_success(&res, r);
    } else if (a->type == VAL_LIST && b->type == VAL_NUMBER) {
        int idx = (int)b->data.number;
        if (idx >= 0 && idx < a->data.list.count) {
            Value* new_list = value_copy(a);
            value_list_remove(new_list, idx);
            rt_result_success(&res, new_list);
        } else {
            rt_result_failure(&res, error_runtime(b->pos_start, b->pos_end,
                "Element at this index could not be removed from list because index is out of bounds",
                a->context));
        }
    } else {
        return illegal_op((Value*)a, (Value*)b);
    }
    return res;
}

RTResult value_multed_by(const Value* a, const Value* b) {
    RTResult res; rt_result_init(&res);
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
        Value* r = value_number(a->data.number * b->data.number);
        r->context = a->context;
        rt_result_success(&res, r);
    } else if (a->type == VAL_STRING && b->type == VAL_NUMBER) {
        int n = (int)b->data.number;
        char* result = malloc(strlen(a->data.string) * n + 1);
        result[0] = '\0';
        for (int i = 0; i < n; i++) strcat(result, a->data.string);
        Value* r = value_string(result);
        free(result);
        r->context = a->context;
        rt_result_success(&res, r);
    } else if (a->type == VAL_LIST && b->type == VAL_LIST) {
        Value* new_list = value_copy(a);
        for (int i = 0; i < b->data.list.count; i++)
            value_list_append(new_list, value_copy(b->data.list.items[i]));
        rt_result_success(&res, new_list);
    } else {
        return illegal_op((Value*)a, (Value*)b);
    }
    return res;
}

RTResult value_dived_by(const Value* a, const Value* b) {
    RTResult res; rt_result_init(&res);
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
        if (b->data.number == 0) {
            rt_result_failure(&res, error_runtime(b->pos_start, b->pos_end, "Division by zero", a->context));
            return res;
        }
        Value* r = value_number(a->data.number / b->data.number);
        r->context = a->context;
        rt_result_success(&res, r);
    } else if (a->type == VAL_LIST && b->type == VAL_NUMBER) {
        int idx = (int)b->data.number;
        if (idx >= 0 && idx < a->data.list.count) {
            rt_result_success(&res, value_copy(a->data.list.items[idx]));
        } else {
            rt_result_failure(&res, error_runtime(b->pos_start, b->pos_end,
                "Element at this index could not be retrieved from list because index is out of bounds",
                a->context));
        }
    } else {
        return illegal_op((Value*)a, (Value*)b);
    }
    return res;
}

RTResult value_powed_by(const Value* a, const Value* b) {
    RTResult res; rt_result_init(&res);
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER) {
        Value* r = value_number(pow(a->data.number, b->data.number));
        r->context = a->context;
        rt_result_success(&res, r);
    } else {
        return illegal_op((Value*)a, (Value*)b);
    }
    return res;
}

static RTResult cmp_result(int cond, Context* ctx) {
    RTResult res; rt_result_init(&res);
    Value* r = value_number(cond ? 1 : 0);
    r->context = ctx;
    rt_result_success(&res, r);
    return res;
}

RTResult value_eq(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(a->data.number == b->data.number, a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_ne(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(a->data.number != b->data.number, a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_lt(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(a->data.number < b->data.number, a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_gt(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(a->data.number > b->data.number, a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_lte(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(a->data.number <= b->data.number, a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_gte(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(a->data.number >= b->data.number, a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_anded(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(value_is_true(a) && value_is_true(b), a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_ored(const Value* a, const Value* b) {
    if (a->type == VAL_NUMBER && b->type == VAL_NUMBER)
        return cmp_result(value_is_true(a) || value_is_true(b), a->context);
    return illegal_op((Value*)a, (Value*)b);
}

RTResult value_notted(const Value* a) {
    return cmp_result(!value_is_true(a), a->context);
}

/* ══════════════════════════════════════════════
   RTResult
   ══════════════════════════════════════════════ */

void rt_result_init(RTResult* res) { memset(res, 0, sizeof(RTResult)); }

Value* rt_result_register(RTResult* res, RTResult* other) {
    if (other->has_error) {
        res->error = other->error;
        res->has_error = 1;
        memset(&other->error, 0, sizeof(Error));
        other->has_error = 0;
    }
    res->func_return_value = other->func_return_value;
    res->loop_should_continue = other->loop_should_continue;
    res->loop_should_break = other->loop_should_break;
    return other->value;
}

RTResult* rt_result_success(RTResult* res, Value* value) {
    memset(res, 0, sizeof(RTResult));
    res->value = value;
    return res;
}

RTResult* rt_result_success_return(RTResult* res, Value* value) {
    memset(res, 0, sizeof(RTResult));
    res->func_return_value = value;
    return res;
}

RTResult* rt_result_success_continue(RTResult* res) {
    memset(res, 0, sizeof(RTResult));
    res->loop_should_continue = 1;
    return res;
}

RTResult* rt_result_success_break(RTResult* res) {
    memset(res, 0, sizeof(RTResult));
    res->loop_should_break = 1;
    return res;
}

RTResult* rt_result_failure(RTResult* res, Error err) {
    memset(res, 0, sizeof(RTResult));
    res->error = err;
    res->has_error = 1;
    return res;
}

int rt_result_should_return(const RTResult* res) {
    return res->has_error || res->func_return_value || res->loop_should_continue || res->loop_should_break;
}
