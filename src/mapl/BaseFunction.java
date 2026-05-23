package mapl;

import java.util.List;
import java.util.ArrayList;
import java.util.Scanner;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public abstract class BaseFunction extends Value {
    public String name;

    public BaseFunction(String name) {
        this.name = (name != null) ? name : "<anonymous>";
    }

    public Context generateNewContext() {
        Context newContext = new Context(name, context, posStart);
        newContext.symbolTable = new SymbolTable(context.symbolTable);
        return newContext;
    }

    public RTResult checkArgs(List<String> argNames, List<Value> args) {
        RTResult res = new RTResult();
        if (args.size() > argNames.size()) {
            return res.failure(new Error.RTError(posStart, posEnd,
                args.size() - argNames.size() + " too many args passed into " + this, context));
        }
        if (args.size() < argNames.size()) {
            return res.failure(new Error.RTError(posStart, posEnd,
                argNames.size() - args.size() + " too few args passed into " + this, context));
        }
        return res.success(Value.Number.null_);
    }

    public void populateArgs(List<String> argNames, List<Value> args, Context execCtx) {
        for (int i = 0; i < args.size(); i++) {
            String argName = argNames.get(i);
            Value argValue = args.get(i);
            argValue.setContext(execCtx);
            execCtx.symbolTable.set(argName, argValue);
        }
    }

    public RTResult checkAndPopulateArgs(List<String> argNames, List<Value> args, Context execCtx) {
        RTResult res = new RTResult();
        res.register(checkArgs(argNames, args));
        if (res.shouldReturn()) return res;
        populateArgs(argNames, args, execCtx);
        return res.success(Value.Number.null_);
    }

    public static class Function extends BaseFunction {
        public Node bodyNode;
        public List<String> argNames;
        public boolean shouldAutoReturn;

        public Function(String name, Node bodyNode, List<String> argNames, boolean shouldAutoReturn) {
            super(name);
            this.bodyNode = bodyNode;
            this.argNames = argNames;
            this.shouldAutoReturn = shouldAutoReturn;
        }

        @Override
        public RTResult execute(List<Value> args) {
            RTResult res = new RTResult();
            Interpreter interpreter = new Interpreter();
            Context execCtx = generateNewContext();

            res.register(checkAndPopulateArgs(argNames, args, execCtx));
            if (res.shouldReturn()) return res;

            Value value = res.register(interpreter.visit(bodyNode, execCtx));
            if (res.shouldReturn() && res.funcReturnValue == null) return res;

            Value retValue = (shouldAutoReturn ? value : null);
            if (retValue == null) retValue = res.funcReturnValue;
            if (retValue == null) retValue = Value.Number.null_;
            return res.success(retValue);
        }

        @Override
        public Value copy() {
            Function copy = new Function(name, bodyNode, argNames, shouldAutoReturn);
            copy.setContext(context);
            copy.setPos(posStart, posEnd);
            return copy;
        }

        @Override
        public String toString() { return "<function " + name + ">"; }
    }

    public static class BuiltInFunction extends BaseFunction {
        private static Scanner scanner = new Scanner(System.in);

        public BuiltInFunction(String name) {
            super(name);
        }

        @Override
        public RTResult execute(List<Value> args) {
            RTResult res = new RTResult();
            Context execCtx = generateNewContext();

            switch (name) {
                case "print": {
                    res.register(checkArgs(java.util.Arrays.asList("value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("value"), args, execCtx);
                    System.out.println(execCtx.symbolTable.get("value"));
                    return res.success(Value.Number.null_);
                }
                case "print_ret": {
                    res.register(checkArgs(java.util.Arrays.asList("value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("value"), args, execCtx);
                    return res.success(new Value.StringValue(execCtx.symbolTable.get("value").toString()));
                }
                case "input": {
                    res.register(checkArgs(new ArrayList<>(), args));
                    if (res.shouldReturn()) return res;
                    String text = scanner.nextLine();
                    return res.success(new Value.StringValue(text));
                }
                case "input_int": {
                    res.register(checkArgs(new ArrayList<>(), args));
                    if (res.shouldReturn()) return res;
                    while (true) {
                        String text = scanner.nextLine();
                        try {
                            int number = Integer.parseInt(text);
                            return res.success(new Value.Number(number));
                        } catch (NumberFormatException e) {
                            System.out.println("'" + text + "' must be an integer. Try again!");
                        }
                    }
                }
                case "clear": {
                    res.register(checkArgs(new ArrayList<>(), args));
                    if (res.shouldReturn()) return res;
                    try {
                        if (System.getProperty("os.name").contains("Windows")) {
                            new ProcessBuilder("cmd", "/c", "cls").inheritIO().start().waitFor();
                        } else {
                            Runtime.getRuntime().exec("clear");
                        }
                    } catch (Exception e) { }
                    return res.success(Value.Number.null_);
                }
                case "is_number": {
                    res.register(checkArgs(java.util.Arrays.asList("value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("value"), args, execCtx);
                    boolean isNum = execCtx.symbolTable.get("value") instanceof Value.Number;
                    return res.success(isNum ? Value.Number.true_ : Value.Number.false_);
                }
                case "is_string": {
                    res.register(checkArgs(java.util.Arrays.asList("value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("value"), args, execCtx);
                    boolean isStr = execCtx.symbolTable.get("value") instanceof Value.StringValue;
                    return res.success(isStr ? Value.Number.true_ : Value.Number.false_);
                }
                case "is_list": {
                    res.register(checkArgs(java.util.Arrays.asList("value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("value"), args, execCtx);
                    boolean isList = execCtx.symbolTable.get("value") instanceof Value.ListValue;
                    return res.success(isList ? Value.Number.true_ : Value.Number.false_);
                }
                case "is_function": {
                    res.register(checkArgs(java.util.Arrays.asList("value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("value"), args, execCtx);
                    boolean isFunc = execCtx.symbolTable.get("value") instanceof BaseFunction;
                    return res.success(isFunc ? Value.Number.true_ : Value.Number.false_);
                }
                case "append": {
                    res.register(checkArgs(java.util.Arrays.asList("list", "value"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("list", "value"), args, execCtx);
                    Value listV = execCtx.symbolTable.get("list");
                    if (!(listV instanceof Value.ListValue)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "First argument must be list", execCtx));
                    }
                    ((Value.ListValue) listV).elements.add(execCtx.symbolTable.get("value"));
                    return res.success(Value.Number.null_);
                }
                case "pop": {
                    res.register(checkArgs(java.util.Arrays.asList("list", "index"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("list", "index"), args, execCtx);
                    Value listV = execCtx.symbolTable.get("list");
                    if (!(listV instanceof Value.ListValue)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "First argument must be list", execCtx));
                    }
                    Value indexV = execCtx.symbolTable.get("index");
                    if (!(indexV instanceof Value.Number)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "Second argument must be number", execCtx));
                    }
                    int idx = (int) ((Value.Number) indexV).value;
                    try {
                        Value element = ((Value.ListValue) listV).elements.remove(idx);
                        return res.success(element);
                    } catch (IndexOutOfBoundsException e) {
                        return res.failure(new Error.RTError(posStart, posEnd,
                            "Element at this index could not be removed from list because index is out of bounds", execCtx));
                    }
                }
                case "extend": {
                    res.register(checkArgs(java.util.Arrays.asList("listA", "listB"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("listA", "listB"), args, execCtx);
                    Value listA = execCtx.symbolTable.get("listA");
                    if (!(listA instanceof Value.ListValue)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "First argument must be list", execCtx));
                    }
                    Value listB = execCtx.symbolTable.get("listB");
                    if (!(listB instanceof Value.ListValue)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "Second argument must be list", execCtx));
                    }
                    ((Value.ListValue) listA).elements.addAll(((Value.ListValue) listB).elements);
                    return res.success(Value.Number.null_);
                }
                case "len": {
                    res.register(checkArgs(java.util.Arrays.asList("list"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("list"), args, execCtx);
                    Value listV = execCtx.symbolTable.get("list");
                    if (!(listV instanceof Value.ListValue)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "Argument must be list", execCtx));
                    }
                    return res.success(new Value.Number(((Value.ListValue) listV).elements.size()));
                }
                case "run": {
                    res.register(checkArgs(java.util.Arrays.asList("fn"), args));
                    if (res.shouldReturn()) return res;
                    populateArgs(java.util.Arrays.asList("fn"), args, execCtx);
                    Value fnV = execCtx.symbolTable.get("fn");
                    if (!(fnV instanceof Value.StringValue)) {
                        return res.failure(new Error.RTError(posStart, posEnd, "Argument must be string", execCtx));
                    }
                    String fn = ((Value.StringValue) fnV).value;
                    try (BufferedReader br = new BufferedReader(new FileReader(fn))) {
                        StringBuilder sb = new StringBuilder();
                        String line;
                        while ((line = br.readLine()) != null) {
                            sb.append(line).append("\n");
                        }
                        Error error = maPL.run(fn, sb.toString()).error;
                        if (error != null) {
                            return res.failure(new Error.RTError(posStart, posEnd,
                                "Failed to finish executing script \"" + fn + "\"\n" + error.asString(), execCtx));
                        }
                    } catch (IOException e) {
                        return res.failure(new Error.RTError(posStart, posEnd,
                            "Failed to load script \"" + fn + "\"\n" + e.getMessage(), execCtx));
                    }
                    return res.success(Value.Number.null_);
                }
            }
            return res.failure(new Error.RTError(posStart, posEnd, "Unknown built-in function: " + name, context));
        }

        @Override
        public Value copy() {
            BuiltInFunction copy = new BuiltInFunction(name);
            copy.setContext(context);
            copy.setPos(posStart, posEnd);
            return copy;
        }

        @Override
        public String toString() { return "<built-in function " + name + ">"; }
    }
}
