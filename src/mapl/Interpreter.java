package mapl;

import java.util.ArrayList;
import java.util.List;
import java.util.function.BiFunction;

public class Interpreter {
    public RTResult visit(Node node, Context context) {
        if (node instanceof Node.NumberNode) return visitNumberNode((Node.NumberNode) node, context);
        if (node instanceof Node.StringNode) return visitStringNode((Node.StringNode) node, context);
        if (node instanceof Node.ListNode) return visitListNode((Node.ListNode) node, context);
        if (node instanceof Node.VarAccessNode) return visitVarAccessNode((Node.VarAccessNode) node, context);
        if (node instanceof Node.VarAssignNode) return visitVarAssignNode((Node.VarAssignNode) node, context);
        if (node instanceof Node.BinOpNode) return visitBinOpNode((Node.BinOpNode) node, context);
        if (node instanceof Node.UnaryOpNode) return visitUnaryOpNode((Node.UnaryOpNode) node, context);
        if (node instanceof Node.IfNode) return visitIfNode((Node.IfNode) node, context);
        if (node instanceof Node.ForNode) return visitForNode((Node.ForNode) node, context);
        if (node instanceof Node.WhileNode) return visitWhileNode((Node.WhileNode) node, context);
        if (node instanceof Node.FuncDefNode) return visitFuncDefNode((Node.FuncDefNode) node, context);
        if (node instanceof Node.CallNode) return visitCallNode((Node.CallNode) node, context);
        if (node instanceof Node.ReturnNode) return visitReturnNode((Node.ReturnNode) node, context);
        if (node instanceof Node.ContinueNode) return visitContinueNode((Node.ContinueNode) node, context);
        if (node instanceof Node.BreakNode) return visitBreakNode((Node.BreakNode) node, context);
        return new RTResult().failure(new Error.RTError(node.posStart, node.posEnd,
            "No visit method defined for " + node.getClass().getSimpleName(), context));
    }

    private RTResult visitNumberNode(Node.NumberNode node, Context context) {
        return new RTResult().success(
            new Value.Number(((Number) node.tok.value).doubleValue()).setContext(context).setPos(node.posStart, node.posEnd)
        );
    }

    private RTResult visitStringNode(Node.StringNode node, Context context) {
        return new RTResult().success(
            new Value.StringValue((String) node.tok.value).setContext(context).setPos(node.posStart, node.posEnd)
        );
    }

    private RTResult visitListNode(Node.ListNode node, Context context) {
        RTResult res = new RTResult();
        List<Value> elements = new ArrayList<>();
        for (Node elementNode : node.elementNodes) {
            elements.add(res.register(visit(elementNode, context)));
            if (res.shouldReturn()) return res;
        }
        return res.success(new Value.ListValue(elements).setContext(context).setPos(node.posStart, node.posEnd));
    }

    private RTResult visitVarAccessNode(Node.VarAccessNode node, Context context) {
        RTResult res = new RTResult();
        String varName = (String) node.varNameTok.value;
        Value value = context.symbolTable.get(varName);
        if (value == null) {
            return res.failure(new Error.RTError(node.posStart, node.posEnd,
                "'" + varName + "' is not defined", context));
        }
        value = value.copy().setPos(node.posStart, node.posEnd).setContext(context);
        return res.success(value);
    }

    private RTResult visitVarAssignNode(Node.VarAssignNode node, Context context) {
        RTResult res = new RTResult();
        String varName = (String) node.varNameTok.value;
        Value value = res.register(visit(node.valueNode, context));
        if (res.shouldReturn()) return res;
        context.symbolTable.set(varName, value);
        return res.success(value);
    }

    private RTResult visitBinOpNode(Node.BinOpNode node, Context context) {
        RTResult res = new RTResult();
        Value left = res.register(visit(node.leftNode, context));
        if (res.shouldReturn()) return res;
        Value right = res.register(visit(node.rightNode, context));
        if (res.shouldReturn()) return res;

        RTResult result = null;
        switch (node.opTok.type) {
            case Token.TT_PLUS: result = left.addedTo(right); break;
            case Token.TT_MINUS: result = left.subbedBy(right); break;
            case Token.TT_MUL: result = left.multedBy(right); break;
            case Token.TT_DIV: result = left.divedBy(right); break;
            case Token.TT_POW: result = left.powedBy(right); break;
            case Token.TT_EE: result = left.getComparisonEq(right); break;
            case Token.TT_NE: result = left.getComparisonNe(right); break;
            case Token.TT_LT: result = left.getComparisonLt(right); break;
            case Token.TT_GT: result = left.getComparisonGt(right); break;
            case Token.TT_LTE: result = left.getComparisonLte(right); break;
            case Token.TT_GTE: result = left.getComparisonGte(right); break;
        }
        if (node.opTok.matches(Token.TT_KEYWORD, "AND")) result = left.andedBy(right);
        if (node.opTok.matches(Token.TT_KEYWORD, "OR")) result = left.oredBy(right);

        if (result == null) {
            return res.failure(new Error.RTError(node.posStart, node.posEnd, "Invalid operator", context));
        }
        if (result.error != null) return res.failure(result.error);
        return res.success(result.value.setPos(node.posStart, node.posEnd));
    }

    private RTResult visitUnaryOpNode(Node.UnaryOpNode node, Context context) {
        RTResult res = new RTResult();
        Value number = res.register(visit(node.node, context));
        if (res.shouldReturn()) return res;

        RTResult result = null;
        if (node.opTok.type.equals(Token.TT_MINUS)) {
            result = number.multedBy(new Value.Number(-1));
        } else if (node.opTok.matches(Token.TT_KEYWORD, "NOT")) {
            result = number.notted();
        }
        if (result == null || result.error != null) {
            if (result != null) return res.failure(result.error);
            return res.failure(new Error.RTError(node.posStart, node.posEnd, "Invalid unary operation", context));
        }
        return res.success(result.value.setPos(node.posStart, node.posEnd));
    }

    private RTResult visitIfNode(Node.IfNode node, Context context) {
        RTResult res = new RTResult();
        for (Node.IfNode.Case case_ : node.cases) {
            Value conditionValue = res.register(visit(case_.condition, context));
            if (res.shouldReturn()) return res;
            if (conditionValue.isTrue()) {
                Value exprValue = res.register(visit(case_.expr, context));
                if (res.shouldReturn()) return res;
                return res.success(case_.shouldReturnNull ? Value.Number.null_ : exprValue);
            }
        }
        if (node.elseCase != null) {
            Value exprValue = res.register(visit(node.elseCase.expr, context));
            if (res.shouldReturn()) return res;
            return res.success(node.elseCase.shouldReturnNull ? Value.Number.null_ : exprValue);
        }
        return res.success(Value.Number.null_);
    }

    private RTResult visitForNode(Node.ForNode node, Context context) {
        RTResult res = new RTResult();
        List<Value> elements = new ArrayList<>();

        Value startValue = res.register(visit(node.startValueNode, context));
        if (res.shouldReturn()) return res;
        Value endValue = res.register(visit(node.endValueNode, context));
        if (res.shouldReturn()) return res;

        double stepVal;
        if (node.stepValueNode != null) {
            Value stepValue = res.register(visit(node.stepValueNode, context));
            if (res.shouldReturn()) return res;
            stepVal = ((Value.Number) stepValue).value;
        } else {
            stepVal = 1;
        }

        double i = ((Value.Number) startValue).value;
        double end = ((Value.Number) endValue).value;

        boolean condition;
        if (stepVal >= 0) {
            while (i < end) {
                context.symbolTable.set((String) node.varNameTok.value, new Value.Number(i));
                i += stepVal;

                Value value = res.register(visit(node.bodyNode, context));
                if (res.shouldReturn() && !res.loopShouldContinue && !res.loopShouldBreak) return res;
                if (res.loopShouldContinue) { res.loopShouldContinue = false; continue; }
                if (res.loopShouldBreak) break;
                elements.add(value);
            }
        } else {
            while (i > end) {
                context.symbolTable.set((String) node.varNameTok.value, new Value.Number(i));
                i += stepVal;

                Value value = res.register(visit(node.bodyNode, context));
                if (res.shouldReturn() && !res.loopShouldContinue && !res.loopShouldBreak) return res;
                if (res.loopShouldContinue) { res.loopShouldContinue = false; continue; }
                if (res.loopShouldBreak) break;
                elements.add(value);
            }
        }

        return res.success(
            node.shouldReturnNull ? Value.Number.null_
                : new Value.ListValue(elements).setContext(context).setPos(node.posStart, node.posEnd)
        );
    }

    private RTResult visitWhileNode(Node.WhileNode node, Context context) {
        RTResult res = new RTResult();
        List<Value> elements = new ArrayList<>();

        while (true) {
            Value condition = res.register(visit(node.conditionNode, context));
            if (res.shouldReturn()) return res;
            if (!condition.isTrue()) break;

            Value value = res.register(visit(node.bodyNode, context));
            if (res.shouldReturn() && !res.loopShouldContinue && !res.loopShouldBreak) return res;
            if (res.loopShouldContinue) { res.loopShouldContinue = false; continue; }
            if (res.loopShouldBreak) break;
            elements.add(value);
        }

        return res.success(
            node.shouldReturnNull ? Value.Number.null_
                : new Value.ListValue(elements).setContext(context).setPos(node.posStart, node.posEnd)
        );
    }

    private RTResult visitFuncDefNode(Node.FuncDefNode node, Context context) {
        RTResult res = new RTResult();

        String funcName = node.varNameTok != null ? (String) node.varNameTok.value : null;
        List<String> argNames = new ArrayList<>();
        for (Token tok : node.argNameToks) argNames.add((String) tok.value);

        BaseFunction.Function funcValue = new BaseFunction.Function(funcName, node.bodyNode, argNames, node.shouldAutoReturn);
        funcValue.setContext(context).setPos(node.posStart, node.posEnd);

        if (node.varNameTok != null) {
            context.symbolTable.set(funcName, funcValue);
        }
        return res.success(funcValue);
    }

    private RTResult visitCallNode(Node.CallNode node, Context context) {
        RTResult res = new RTResult();
        List<Value> args = new ArrayList<>();

        Value valueToCall = res.register(visit(node.nodeToCall, context));
        if (res.shouldReturn()) return res;
        valueToCall = valueToCall.copy().setPos(node.posStart, node.posEnd);

        for (Node argNode : node.argNodes) {
            args.add(res.register(visit(argNode, context)));
            if (res.shouldReturn()) return res;
        }

        RTResult retRes = valueToCall.execute(args);
        Value returnVal = res.register(retRes);
        if (res.shouldReturn()) return res;
        returnVal = returnVal.copy().setPos(node.posStart, node.posEnd).setContext(context);
        return res.success(returnVal);
    }

    private RTResult visitReturnNode(Node.ReturnNode node, Context context) {
        RTResult res = new RTResult();
        Value value;
        if (node.nodeToReturn != null) {
            value = res.register(visit(node.nodeToReturn, context));
            if (res.shouldReturn()) return res;
        } else {
            value = Value.Number.null_;
        }
        return res.successReturn(value);
    }

    private RTResult visitContinueNode(Node.ContinueNode node, Context context) {
        return new RTResult().successContinue();
    }

    private RTResult visitBreakNode(Node.BreakNode node, Context context) {
        return new RTResult().successBreak();
    }
}
