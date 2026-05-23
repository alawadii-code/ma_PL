package mapl;

import java.util.List;

public class maPL {
    public static SymbolTable globalSymbolTable;

    static {
        globalSymbolTable = new SymbolTable();
        globalSymbolTable.set("NULL", Value.Number.null_);
        globalSymbolTable.set("FALSE", Value.Number.false_);
        globalSymbolTable.set("TRUE", Value.Number.true_);
        globalSymbolTable.set("MATH_PI", Value.Number.mathPI);
        globalSymbolTable.set("PRINT", new BaseFunction.BuiltInFunction("print"));
        globalSymbolTable.set("PRINT_RET", new BaseFunction.BuiltInFunction("print_ret"));
        globalSymbolTable.set("INPUT", new BaseFunction.BuiltInFunction("input"));
        globalSymbolTable.set("INPUT_INT", new BaseFunction.BuiltInFunction("input_int"));
        globalSymbolTable.set("CLEAR", new BaseFunction.BuiltInFunction("clear"));
        globalSymbolTable.set("CLS", new BaseFunction.BuiltInFunction("clear"));
        globalSymbolTable.set("IS_NUM", new BaseFunction.BuiltInFunction("is_number"));
        globalSymbolTable.set("IS_STR", new BaseFunction.BuiltInFunction("is_string"));
        globalSymbolTable.set("IS_LIST", new BaseFunction.BuiltInFunction("is_list"));
        globalSymbolTable.set("IS_FUN", new BaseFunction.BuiltInFunction("is_function"));
        globalSymbolTable.set("APPEND", new BaseFunction.BuiltInFunction("append"));
        globalSymbolTable.set("POP", new BaseFunction.BuiltInFunction("pop"));
        globalSymbolTable.set("EXTEND", new BaseFunction.BuiltInFunction("extend"));
        globalSymbolTable.set("LEN", new BaseFunction.BuiltInFunction("len"));
        globalSymbolTable.set("RUN", new BaseFunction.BuiltInFunction("run"));
    }

    public static class RunResult {
        public Value value;
        public Error error;
        public RunResult(Value value, Error error) {
            this.value = value;
            this.error = error;
        }
    }

    public static RunResult run(String fn, String text) {
        Lexer lexer = new Lexer(fn, text);
        Lexer.Result lexResult = lexer.makeTokens();
        if (lexResult.error != null) return new RunResult(null, lexResult.error);

        Parser parser = new Parser(lexResult.tokens);
        ParseResult ast = parser.parse();
        if (ast.error != null) return new RunResult(null, ast.error);

        Interpreter interpreter = new Interpreter();
        Context context = new Context("<program>");
        context.symbolTable = globalSymbolTable;

        RTResult result = interpreter.visit(ast.node, context);
        return new RunResult(result.value, result.error);
    }
}
