# maPL

<pre>
▄████████  ▄█          ▄███████▄    ▄█    █▄       ▄████████ ▀█████████▄     ▄████████     ███      ▄███████▄
  ███    ███ ███         ███    ███   ███    ███     ███    ███   ███    ███   ███    ███ ▀█████████▄ ██▀     ▄██
  ███    ███ ███         ███    ███   ███    ███     ███    ███   ███    ███   ███    █▀     ▀███▀▀██       ▄███▀
  ███    ███ ███         ███    ███  ▄███▄▄▄▄███▄▄   ███    ███  ▄███▄▄▄██▀   ▄███▄▄▄         ███   ▀  ▀█▀▄███▀▄▄
▀███████████ ███       ▀█████████▀  ▀▀███▀▀▀▀███▀  ▀███████████ ▀▀███▀▀▀██▄  ▀▀███▀▀▀         ███       ▄███▀   ▀
  ███    ███ ███         ███          ███    ███     ███    ███   ███    ██▄   ███    █▄      ███     ▄███▀
  ███    ███ ███▌    ▄   ███          ███    ███     ███    ███   ███    ███   ███    ███     ███     ███▄     ▄█
  ███    █▀  █████▄▄██  ▄████▀        ███    █▀      ███    █▀  ▄█████████▀    ██████████    ▄████▀    ▀████████▀
</pre>

An interpreter for a BASIC-like programming language, written in Java 17. Originally ported from [py-myopl-code](https://github.com/JohnBishopUK/py-myopl-code) (a Python tutorial series by John Bishop).

## 🎮 How to Use (Non-Technical)

### Requirements
- Java 17 or higher installed on your computer

### Running
You can run maPL in two ways:

**Interactive Shell:**
```bash
java -cp out Main
```
Then type code directly. Example session:
```
maPL > PRINT("Hello, World!")
maPL > VAR x = 42
maPL > PRINT(x * 2)
maPL > exit()
```

**Run a file:**
Create a `.mapl` file and run it:
```bash
java -cp out Main my_program.mapl
```

### Code Examples

```basic
# Hello World
PRINT("Hello, World!")

# Variables
VAR name = "Alice"
VAR age = 25
PRINT(name)
PRINT(age)

# Math
PRINT(1 + 2 * 3)
PRINT(2 ^ 10)
PRINT(10 / 3)

# If statements
VAR x = 10
IF x > 5 THEN
    PRINT("big")
ELSE
    PRINT("small")
END

# One-line if
IF x > 5 THEN PRINT("big")

# Loops
FOR i = 0 TO 5 THEN PRINT(i)

VAR j = 0
WHILE j < 3 THEN
    PRINT(j)
    VAR j = j + 1
END

# Lists
VAR list = [10, 20, 30]
PRINT(list)
PRINT(list / 1)    # Indexing (returns 20)

# Functions
FUN sq(x) -> x * x
PRINT(sq(5))

FUN fact(n)
    IF n <= 1 THEN
        RETURN 1
    ELSE
        RETURN n * fact(n - 1)
    END
END
PRINT(fact(5))

# Built-in functions
PRINT(IS_NUM(42))       # prints 1 (true)
PRINT(IS_STR("hi"))     # prints 1 (true)
PRINT(IS_LIST([1,2]))   # prints 1 (true)
```

---

## 🏗 Architecture (Technical)

maPL is a tree-walking interpreter with four stages:

### 1. Lexer (`Lexer.java`)
Tokenizes source text into a list of tokens. Token types include:
- `INT`, `FLOAT` — numeric literals
- `STRING` — string literals (supports `\n` and `\t` escapes)
- `IDENTIFIER` — variable/function names
- `KEYWORD` — `VAR`, `IF`, `THEN`, `ELSE`, `ELIF`, `END`, `FOR`, `TO`, `STEP`, `WHILE`, `FUN`, `RETURN`, `CONTINUE`, `BREAK`, `AND`, `OR`, `NOT`
- Operators: `PLUS`, `MINUS`, `MUL`, `DIV`, `POW`, `EQ`, `EE`, `NE`, `LT`, `GT`, `LTE`, `GTE`, `ARROW`, `LPAREN`, `RPAREN`, `LSQUARE`, `RSQUARE`, `COMMA`
- `NEWLINE` — line/statement separator
- `EOF` — end of file
- Comments start with `#` and extend to end of line

### 2. Parser (`Parser.java`)
Recursive descent parser that builds an AST (Abstract Syntax Tree). Grammar:

```
program     → statement*
statement   → RETURN expr | CONTINUE | BREAK | expr
expr        → VAR IDENTIFIER = expr | comp_expr ((AND|OR) comp_expr)*
comp_expr   → NOT comp_expr | arith_expr ((==|!=|<|>|<=|>=) arith_expr)*
arith_expr  → term ((+|-) term)*
term        → factor ((*|/) factor)*
factor      → (+|-) factor | power
power       → call (^ factor)*
call        → atom (LPAREN (expr (COMMA expr)*)? RPAREN)?
atom        → INT | FLOAT | STRING | IDENTIFIER
            | LPAREN expr RPAREN
            | LSQUARE list_expr RSQUARE
            | IF expr THEN (stmt | NEWLINE stmts END)
            | FOR IDENTIFIER = expr TO expr (STEP expr)? THEN (stmt | NEWLINE stmts END)
            | WHILE expr THEN (stmt | NEWLINE stmts END)
            | FUN (IDENTIFIER)? LPAREN (IDENTIFIER (COMMA IDENTIFIER)*)? RPAREN (ARROW expr | NEWLINE stmts END)
```

### 3. Interpreter (`Interpreter.java`)
Visits each AST node and evaluates it:

| Node | Evaluation |
|------|-----------|
| `NumberNode` | Returns `Number` value |
| `StringNode` | Returns `String` value |
| `ListNode` | Evaluates elements, returns `List` |
| `VarAccessNode` | Looks up variable in `SymbolTable` |
| `VarAssignNode` | Sets variable in `SymbolTable` |
| `BinOpNode` | Applies arithmetic/comparison/logical operators |
| `UnaryOpNode` | Unary minus or `NOT` |
| `IfNode` | Evaluates conditions, executes matching branch |
| `ForNode` | Iterates with optional step, auto-scopes loop var |
| `WhileNode` | Loops while condition is true |
| `FuncDefNode` | Creates `Function` object, optionally stores in scope |
| `CallNode` | Evaluates arguments, calls function |
| `ReturnNode` | Captures return value, unwinds stack |
| `ContinueNode` | Signals loop to continue |
| `BreakNode` | Signals loop to break |

### 4. Runtime (`Value.java`, `BaseFunction.java`)

**Value types:**
- `Value.Number` — wraps `double`, supports arithmetic, comparisons, logical ops
- `Value.StringValue` — wraps `String`, supports concatenation (`+`) and repetition (`*`)
- `Value.ListValue` — wraps `List<Value>`, supports append (`+`), remove (`-`), concat (`*`), index (`/`)

**Functions:**
- `BaseFunction.Function` — user-defined functions with lexical scoping
- `BaseFunction.BuiltInFunction` — native Java functions exposed to the language

**Scoping:**
- `Context` — execution context with display name, parent chain, and symbol table
- `SymbolTable` — nested scope with parent chain for lexical lookup

### Built-in Functions

| Name | Args | Description |
|------|------|-------------|
| `PRINT` | value | Prints value to stdout |
| `PRINT_RET` | value | Returns value as string |
| `INPUT` | — | Reads a line from stdin |
| `INPUT_INT` | — | Reads an integer from stdin |
| `CLEAR` / `CLS` | — | Clears the console |
| `IS_NUM` | value | Returns 1 if value is a number |
| `IS_STR` | value | Returns 1 if value is a string |
| `IS_LIST` | value | Returns 1 if value is a list |
| `IS_FUN` | value | Returns 1 if value is a function |
| `APPEND` | list, value | Appends value to list |
| `POP` | list, index | Removes and returns element at index |
| `EXTEND` | listA, listB | Extends listA with listB's elements |
| `LEN` | list | Returns list length |
| `RUN` | filename | Executes a maPL script file |

### Constants

| Name | Value |
|------|-------|
| `NULL` | 0 |
| `FALSE` | 0 |
| `TRUE` | 1 |
| `MATH_PI` | 3.141592653589793 |

---

## 📁 Project Structure

```
ma_PL/
├── src/
│   ├── Main.java              # Entry point (REPL + file runner)
│   └── mapl/
│       ├── Token.java         # Token types and Token class
│       ├── Position.java      # Source position tracking
│       ├── Error.java         # Error types (lexer, parser, runtime)
│       ├── StringWithArrows.java  # Error message formatting
│       ├── Lexer.java         # Tokenizer
│       ├── Node.java          # AST node types
│       ├── ParseResult.java   # Parser result wrapper
│       ├── Parser.java        # Recursive descent parser
│       ├── Value.java         # Runtime values (Number, String, List)
│       ├── RTResult.java      # Runtime result wrapper
│       ├── Context.java       # Execution context
│       ├── SymbolTable.java   # Variable scope table
│       ├── Interpreter.java   # AST evaluator
│       ├── BaseFunction.java  # User & built-in functions
│       └── maPL.java          # Run function + global symbol table
├── out/                       # Compiled .class files
├── .gitignore
├── ma_PL.iml                  # IntelliJ IDEA module
└── README.md
```

---

## 🔧 Building

```bash
# Compile
javac -d out src/mapl/*.java src/Main.java

# Run interactive shell
java -cp out Main

# Run a file
java -cp out Main program.mapl
```

The project uses **JDK 17** and has no external dependencies. It can be opened directly in IntelliJ IDEA.
