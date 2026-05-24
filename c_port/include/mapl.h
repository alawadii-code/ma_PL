#ifndef MAPL_H
#define MAPL_H

#include <stddef.h>

/* ── Token ── */
typedef enum {
    TT_INT, TT_FLOAT, TT_STRING, TT_IDENTIFIER, TT_KEYWORD,
    TT_PLUS, TT_MINUS, TT_MUL, TT_DIV, TT_POW,
    TT_EQ, TT_LPAREN, TT_RPAREN, TT_LSQUARE, TT_RSQUARE,
    TT_EE, TT_NE, TT_LT, TT_GT, TT_LTE, TT_GTE,
    TT_COMMA, TT_ARROW, TT_NEWLINE, TT_EOF, TT_NONE
} TokenType;

extern const char* TOKEN_NAMES[];

typedef struct {
    int idx, ln, col;
    char* fn;
    char* ftxt;
} Position;

Position position_create(int idx, int ln, int col, const char* fn, const char* ftxt);
void position_advance(Position* pos, char c);
Position position_copy(const Position* pos);

typedef struct Token {
    TokenType type;
    char* value;          /* owned string or NULL */
    Position pos_start;
    Position pos_end;
} Token;

Token token_create(TokenType type, const char* value, Position* pos_start, Position* pos_end);
Token token_create_simple(TokenType type, const char* value);
int token_matches(const Token* t, TokenType type, const char* value);
void token_free(Token* t);

/* ── Error ── */
typedef enum { ERR_ILLEGAL_CHAR, ERR_EXPECTED_CHAR, ERR_INVALID_SYNTAX, ERR_RUNTIME } ErrorType;

struct Context;

typedef struct {
    ErrorType type;
    Position pos_start;
    Position pos_end;
    char* error_name;
    char* details;
    struct Context* context; /* only for runtime errors */
} Error;

Error error_create(ErrorType type, Position pos_start, Position pos_end,
                   const char* error_name, const char* details);
Error error_illegal_char(Position pos_start, Position pos_end, const char* details);
Error error_expected_char(Position pos_start, Position pos_end, const char* details);
Error error_invalid_syntax(Position pos_start, Position pos_end, const char* details);
struct RTResult; /* forward */
Error error_runtime(Position pos_start, Position pos_end, const char* details, struct Context* context);
char* error_as_string(const Error* err);
void error_free(Error* err);

/* ── AST Nodes ── */
typedef enum {
    NODE_NUMBER, NODE_STRING, NODE_LIST, NODE_VAR_ACCESS,
    NODE_VAR_ASSIGN, NODE_BINOP, NODE_UNARY_OP, NODE_IF,
    NODE_FOR, NODE_WHILE, NODE_FUNC_DEF, NODE_CALL,
    NODE_RETURN, NODE_CONTINUE, NODE_BREAK
} NodeType;

typedef struct Node Node;
typedef struct NodeList { Node** items; int count; int capacity; } NodeList;
typedef struct TokenList { Token* items; int count; int capacity; } TokenList;

typedef struct {
    Node* condition;
    Node* expr;
    int should_return_null;
} IfCase;

typedef struct {
    IfCase* cases;
    int count;
    int capacity;
    IfCase else_case;
    int has_else;
} IfData;

typedef struct {
    Token var_name_tok;
    Node* start_value;
    Node* end_value;
    Node* step_value;
    Node* body;
    int should_return_null;
} ForData;

typedef struct {
    Node* condition;
    Node* body;
    int should_return_null;
} WhileData;

typedef struct {
    Token var_name_tok; /* valid if has_name != 0 */
    int has_name;
    TokenList arg_names;
    Node* body;
    int should_auto_return;
} FuncDefData;

typedef struct {
    Node* to_call;
    NodeList args;
} CallData;

typedef struct {
    Node* node_to_return;
} ReturnData;

struct Node {
    NodeType type;
    Position pos_start;
    Position pos_end;
    union {
        Token tok;                       /* NODE_NUMBER, NODE_STRING, NODE_VAR_ACCESS */
        NodeList list;                   /* NODE_LIST */
        struct { Token var_name; Node* value; } var_assign; /* NODE_VAR_ASSIGN */
        struct { Node *left, *right; Token op; } binop;     /* NODE_BINOP */
        struct { Token op; Node* operand; } unary;           /* NODE_UNARY_OP */
        IfData if_data;                  /* NODE_IF */
        ForData for_data;                /* NODE_FOR */
        WhileData while_data;            /* NODE_WHILE */
        FuncDefData func_def;            /* NODE_FUNC_DEF */
        CallData call;                   /* NODE_CALL */
        ReturnData ret;                  /* NODE_RETURN */
    } data;
};

Node* node_create_number(Token tok);
Node* node_create_string(Token tok);
Node* node_create_list(NodeList* elements, Position pos_start, Position pos_end);
Node* node_create_var_access(Token tok);
Node* node_create_var_assign(Token var_name, Node* value);
Node* node_create_binop(Node* left, Token op, Node* right);
Node* node_create_unary_op(Token op, Node* operand);
Node* node_create_if(IfData* data);
Node* node_create_for(ForData* data);
Node* node_create_while(WhileData* data);
Node* node_create_func_def(FuncDefData* data);
Node* node_create_call(Node* to_call, NodeList* args);
Node* node_create_return(Node* value, Position pos_start, Position pos_end);
Node* node_create_continue(Position pos_start, Position pos_end);
Node* node_create_break(Position pos_start, Position pos_end);
void node_free(Node* node);
int is_keyword(const char* s);
void position_free(Position* p);

/* ── ParseResult ── */
typedef struct {
    Error error;
    int has_error;
    Node* node;
    int last_registered_advance_count;
    int advance_count;
    int to_reverse_count;
} ParseResult;

void parse_result_init(ParseResult* res);
Node* parse_result_register(ParseResult* res, ParseResult* other);
Node* parse_result_try_register(ParseResult* res, ParseResult* other);
void parse_result_register_advancement(ParseResult* res);
ParseResult* parse_result_success(ParseResult* res, Node* node);
ParseResult* parse_result_failure(ParseResult* res, Error err);

/* ── Value ── */
typedef enum { VAL_NUMBER, VAL_STRING, VAL_LIST } ValueType;
struct Value;

typedef struct Value {
    ValueType type;
    Position pos_start;
    Position pos_end;
    struct Context* context;
    union {
        double number;
        char* string;
        struct { struct Value** items; int count; int capacity; } list;
    } data;
} Value;

typedef struct ValueList { Value** items; int count; int capacity; } ValueList;

struct RTResult {
    Value* value;
    Error error;
    int has_error;
    Value* func_return_value;
    int loop_should_continue;
    int loop_should_break;
};
typedef struct RTResult RTResult;

void rt_result_init(RTResult* res);
Value* rt_result_register(RTResult* res, RTResult other);
RTResult* rt_result_success(RTResult* res, Value* value);
RTResult* rt_result_success_return(RTResult* res, Value* value);
RTResult* rt_result_success_continue(RTResult* res);
RTResult* rt_result_success_break(RTResult* res);
RTResult* rt_result_failure(RTResult* res, Error err);
int rt_result_should_return(const RTResult* res);

Value* value_copy(const Value* v);
void value_free(Value* v);
Value* value_number(double n);
Value* value_string(const char* s);
Value* value_list(void);
void value_list_append(Value* list, Value* item);
Value* value_list_get(const Value* list, int index);
void value_list_remove(Value* list, int index);
int value_is_true(const Value* v);
char* value_to_string(const Value* v);

RTResult value_added_to(const Value* a, const Value* b);
RTResult value_subbed_by(const Value* a, const Value* b);
RTResult value_multed_by(const Value* a, const Value* b);
RTResult value_dived_by(const Value* a, const Value* b);
RTResult value_powed_by(const Value* a, const Value* b);
RTResult value_eq(const Value* a, const Value* b);
RTResult value_ne(const Value* a, const Value* b);
RTResult value_lt(const Value* a, const Value* b);
RTResult value_gt(const Value* a, const Value* b);
RTResult value_lte(const Value* a, const Value* b);
RTResult value_gte(const Value* a, const Value* b);
RTResult value_anded(const Value* a, const Value* b);
RTResult value_ored(const Value* a, const Value* b);
RTResult value_notted(const Value* a);
RTResult value_execute(Value* func, ValueList* args);

/* ── SymbolTable ── */
typedef struct SymbolEntry { char* name; Value* value; } SymbolEntry;
typedef struct SymbolTable SymbolTable;
struct SymbolTable {
    SymbolEntry* entries;
    int count;
    int capacity;
    SymbolTable* parent;
};

SymbolTable* symbol_table_create(SymbolTable* parent);
void symbol_table_free(SymbolTable* st);
Value* symbol_table_get(SymbolTable* st, const char* name);
void symbol_table_set(SymbolTable* st, const char* name, Value* value);
void symbol_table_remove(SymbolTable* st, const char* name);

/* ── Context ── */
typedef struct Context {
    char* display_name;
    struct Context* parent;
    Position parent_entry_pos;
    SymbolTable* symbol_table;
} Context;

Context* context_create(const char* display_name, Context* parent, Position parent_entry_pos);
void context_free(Context* ctx);

/* ── Parser ── */
typedef struct {
    Token* tokens;
    int count;
    int tok_idx;
    Token current_tok;
} Parser;

Parser* parser_create(Token* tokens, int count);
void parser_advance(Parser* p);
void parser_reverse(Parser* p, int amount);
ParseResult* parser_parse(Parser* p);
void parser_free(Parser* p);

/* ── Lexer ── */
typedef struct {
    char* fn;
    char* text;
    int text_len;
    Position pos;
    char current_char;
    int has_char;
} Lexer;

Lexer* lexer_create(const char* fn, const char* text);
void lexer_advance(Lexer* l);
Token* lexer_make_tokens(Lexer* l, int* out_count, Error* out_error);
void lexer_free(Lexer* l);

/* ── Interpreter ── */
RTResult interpreter_visit(Node* node, Context* ctx);

/* ── Function (prototypes needed) ── */
RTResult function_execute_user(Node* body, char** arg_names, int arg_count,
                               int should_auto_return, Value** args, int arg_count_call,
                               Context* closure_ctx, Position pos_start, Position pos_end,
                               const char* name);
RTResult function_execute_builtin(const char* name, Value** args, int arg_count,
                                  Position pos_start, Position pos_end, Context* ctx);

/* ── maPL ── */
typedef struct { Value* value; Error error; int has_error; } RunResult;
RunResult mapl_run(const char* fn, const char* text);
SymbolTable* mapl_global_symbol_table(void);

/* ── StringWithArrows ── */
char* string_with_arrows(const char* text, Position pos_start, Position pos_end);

/* ── NodeList / TokenList helpers ── */
void node_list_init(NodeList* list);
void node_list_add(NodeList* list, Node* item);
void node_list_free(NodeList* list);
void token_list_init(TokenList* list);
void token_list_add(TokenList* list, Token item);
void token_list_clear(TokenList* list);
void token_list_free(TokenList* list);

/* ── ValueList helpers ── */
void value_list_init(ValueList* list);
void value_list_add(ValueList* list, Value* item);
void value_list_free(ValueList* list);

/* error convenience */
int error_has(const Error* e);
Error error_none(void);

#endif /* MAPL_H */
