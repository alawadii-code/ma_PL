package mapl;

import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import java.util.AbstractMap.SimpleEntry;

public class Parser {
    public List<Token> tokens;
    public int tokIdx;
    public Token currentTok;

    public Parser(List<Token> tokens) {
        this.tokens = tokens;
        tokIdx = -1;
        advance();
    }

    public void advance() {
        tokIdx++;
        updateCurrentTok();
    }

    public void reverse(int amount) {
        tokIdx -= amount;
        updateCurrentTok();
    }

    private void updateCurrentTok() {
        if (tokIdx >= 0 && tokIdx < tokens.size()) {
            currentTok = tokens.get(tokIdx);
        }
    }

    public ParseResult parse() {
        ParseResult res = statements();
        if (res.error == null && !currentTok.type.equals(Token.TT_EOF)) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd,
                "Token cannot appear after previous tokens"
            ));
        }
        return res;
    }

    private ParseResult statements() {
        ParseResult res = new ParseResult();
        List<Node> statements = new ArrayList<>();
        Position posStart = currentTok.posStart.copy();

        while (currentTok.type.equals(Token.TT_NEWLINE)) {
            res.registerAdvancement();
            advance();
        }

        Node statement = res.register(statement());
        if (res.error != null) return res;
        statements.add(statement);

        boolean moreStatements = true;
        while (true) {
            int newlineCount = 0;
            while (currentTok.type.equals(Token.TT_NEWLINE)) {
                res.registerAdvancement();
                advance();
                newlineCount++;
            }
            if (newlineCount == 0) moreStatements = false;
            if (!moreStatements) break;

            Node stmt = res.tryRegister(statement());
            if (stmt == null) {
                reverse(res.toReverseCount);
                moreStatements = false;
                continue;
            }
            statements.add(stmt);
        }

        return res.success(new Node.ListNode(statements, posStart, currentTok.posEnd.copy()));
    }

    private ParseResult statement() {
        ParseResult res = new ParseResult();
        Position posStart = currentTok.posStart.copy();

        if (currentTok.matches(Token.TT_KEYWORD, "RETURN")) {
            res.registerAdvancement();
            advance();
            Node expr = res.tryRegister(expr());
            if (expr == null) reverse(res.toReverseCount);
            return res.success(new Node.ReturnNode(expr, posStart, currentTok.posStart.copy()));
        }

        if (currentTok.matches(Token.TT_KEYWORD, "CONTINUE")) {
            res.registerAdvancement();
            advance();
            return res.success(new Node.ContinueNode(posStart, currentTok.posStart.copy()));
        }

        if (currentTok.matches(Token.TT_KEYWORD, "BREAK")) {
            res.registerAdvancement();
            advance();
            return res.success(new Node.BreakNode(posStart, currentTok.posStart.copy()));
        }

        Node expr = res.register(expr());
        if (res.error != null) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd,
                "Expected 'RETURN', 'CONTINUE', 'BREAK', 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"
            ));
        }
        return res.success(expr);
    }

    private ParseResult expr() {
        ParseResult res = new ParseResult();

        if (currentTok.matches(Token.TT_KEYWORD, "VAR")) {
            res.registerAdvancement();
            advance();

            if (!currentTok.type.equals(Token.TT_IDENTIFIER)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected identifier"
                ));
            }
            Token varName = currentTok;
            res.registerAdvancement();
            advance();

            if (!currentTok.type.equals(Token.TT_EQ)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected '='"
                ));
            }
            res.registerAdvancement();
            advance();

            Node exprNode = res.register(expr());
            if (res.error != null) return res;
            return res.success(new Node.VarAssignNode(varName, exprNode));
        }

        Node node = res.register(binOp(this::compExpr,
            Arrays.asList(
                new SimpleEntry<>(Token.TT_KEYWORD, "AND"),
                new SimpleEntry<>(Token.TT_KEYWORD, "OR")
            )));
        if (res.error != null) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd,
                "Expected 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"
            ));
        }
        return res.success(node);
    }

    private ParseResult compExpr() {
        ParseResult res = new ParseResult();

        if (currentTok.matches(Token.TT_KEYWORD, "NOT")) {
            Token opTok = currentTok;
            res.registerAdvancement();
            advance();
            Node node = res.register(compExpr());
            if (res.error != null) return res;
            return res.success(new Node.UnaryOpNode(opTok, node));
        }

        Node node = res.register(binOp(this::arithExpr,
            Arrays.asList(
                new SimpleEntry<>(Token.TT_EE, null),
                new SimpleEntry<>(Token.TT_NE, null),
                new SimpleEntry<>(Token.TT_LT, null),
                new SimpleEntry<>(Token.TT_GT, null),
                new SimpleEntry<>(Token.TT_LTE, null),
                new SimpleEntry<>(Token.TT_GTE, null)
            )));
        if (res.error != null) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd,
                "Expected int, float, identifier, '+', '-', '(', '[', 'IF', 'FOR', 'WHILE', 'FUN' or 'NOT'"
            ));
        }
        return res.success(node);
    }

    private ParseResult arithExpr() {
        return binOp(this::term, Arrays.asList(
            new SimpleEntry<>(Token.TT_PLUS, null),
            new SimpleEntry<>(Token.TT_MINUS, null)
        ));
    }

    private ParseResult term() {
        return binOp(this::factor, Arrays.asList(
            new SimpleEntry<>(Token.TT_MUL, null),
            new SimpleEntry<>(Token.TT_DIV, null)
        ));
    }

    private ParseResult factor() {
        ParseResult res = new ParseResult();
        Token tok = currentTok;

        if (tok.type.equals(Token.TT_PLUS) || tok.type.equals(Token.TT_MINUS)) {
            res.registerAdvancement();
            advance();
            Node factorNode = res.register(factor());
            if (res.error != null) return res;
            return res.success(new Node.UnaryOpNode(tok, factorNode));
        }
        return power();
    }

    private ParseResult power() {
        return binOp(this::call, Arrays.asList(
            new SimpleEntry<>(Token.TT_POW, null)
        ), this::factor);
    }

    private ParseResult call() {
        ParseResult res = new ParseResult();
        Node atom = res.register(atom());
        if (res.error != null) return res;

        if (currentTok.type.equals(Token.TT_LPAREN)) {
            res.registerAdvancement();
            advance();
            List<Node> argNodes = new ArrayList<>();

            if (currentTok.type.equals(Token.TT_RPAREN)) {
                res.registerAdvancement();
                advance();
            } else {
                argNodes.add(res.register(expr()));
                if (res.error != null) {
                    return res.failure(new Error.InvalidSyntaxError(
                        currentTok.posStart, currentTok.posEnd,
                        "Expected ')', 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"
                    ));
                }
                while (currentTok.type.equals(Token.TT_COMMA)) {
                    res.registerAdvancement();
                    advance();
                    argNodes.add(res.register(expr()));
                    if (res.error != null) return res;
                }
                if (!currentTok.type.equals(Token.TT_RPAREN)) {
                    return res.failure(new Error.InvalidSyntaxError(
                        currentTok.posStart, currentTok.posEnd, "Expected ',' or ')'"
                    ));
                }
                res.registerAdvancement();
                advance();
            }
            return res.success(new Node.CallNode(atom, argNodes));
        }
        return res.success(atom);
    }

    private ParseResult atom() {
        ParseResult res = new ParseResult();
        Token tok = currentTok;

        if (tok.type.equals(Token.TT_INT) || tok.type.equals(Token.TT_FLOAT)) {
            res.registerAdvancement();
            advance();
            return res.success(new Node.NumberNode(tok));
        } else if (tok.type.equals(Token.TT_STRING)) {
            res.registerAdvancement();
            advance();
            return res.success(new Node.StringNode(tok));
        } else if (tok.type.equals(Token.TT_IDENTIFIER)) {
            res.registerAdvancement();
            advance();
            return res.success(new Node.VarAccessNode(tok));
        } else if (tok.type.equals(Token.TT_LPAREN)) {
            res.registerAdvancement();
            advance();
            Node exprNode = res.register(expr());
            if (res.error != null) return res;
            if (currentTok.type.equals(Token.TT_RPAREN)) {
                res.registerAdvancement();
                advance();
                return res.success(exprNode);
            } else {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected ')'"
                ));
            }
        } else if (tok.type.equals(Token.TT_LSQUARE)) {
            Node listExpr = res.register(listExpr());
            if (res.error != null) return res;
            return res.success(listExpr);
        } else if (tok.matches(Token.TT_KEYWORD, "IF")) {
            Node ifExpr = res.register(ifExpr());
            if (res.error != null) return res;
            return res.success(ifExpr);
        } else if (tok.matches(Token.TT_KEYWORD, "FOR")) {
            Node forExpr = res.register(forExpr());
            if (res.error != null) return res;
            return res.success(forExpr);
        } else if (tok.matches(Token.TT_KEYWORD, "WHILE")) {
            Node whileExpr = res.register(whileExpr());
            if (res.error != null) return res;
            return res.success(whileExpr);
        } else if (tok.matches(Token.TT_KEYWORD, "FUN")) {
            Node funcDef = res.register(funcDef());
            if (res.error != null) return res;
            return res.success(funcDef);
        }

        return res.failure(new Error.InvalidSyntaxError(
            tok.posStart, tok.posEnd,
            "Expected int, float, identifier, '+', '-', '(', '[', 'IF', 'FOR', 'WHILE', 'FUN'"
        ));
    }

    private ParseResult listExpr() {
        ParseResult res = new ParseResult();
        List<Node> elementNodes = new ArrayList<>();
        Position posStart = currentTok.posStart.copy();

        if (!currentTok.type.equals(Token.TT_LSQUARE)) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected '['"
            ));
        }
        res.registerAdvancement();
        advance();

        if (currentTok.type.equals(Token.TT_RSQUARE)) {
            res.registerAdvancement();
            advance();
        } else {
            elementNodes.add(res.register(expr()));
            if (res.error != null) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd,
                    "Expected ']', 'VAR', 'IF', 'FOR', 'WHILE', 'FUN', int, float, identifier, '+', '-', '(', '[' or 'NOT'"
                ));
            }
            while (currentTok.type.equals(Token.TT_COMMA)) {
                res.registerAdvancement();
                advance();
                elementNodes.add(res.register(expr()));
                if (res.error != null) return res;
            }
            if (!currentTok.type.equals(Token.TT_RSQUARE)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected ',' or ']'"
                ));
            }
            res.registerAdvancement();
            advance();
        }
        return res.success(new Node.ListNode(elementNodes, posStart, currentTok.posEnd.copy()));
    }

    private static class IfCaseResult {
        List<Node.IfNode.Case> cases;
        Node.IfNode.Case elseCase;
        Error error;
        IfCaseResult(List<Node.IfNode.Case> cases, Node.IfNode.Case elseCase) {
            this.cases = cases;
            this.elseCase = elseCase;
        }
        IfCaseResult(Error error) { this.error = error; }
    }

    private ParseResult ifExpr() {
        List<Node.IfNode.Case> cases = new ArrayList<>();
        IfCaseResult icr = parseIfCases("IF");
        if (icr.error != null) return new ParseResult().failure(icr.error);
        return new ParseResult().success(new Node.IfNode(icr.cases, icr.elseCase));
    }

    private IfCaseResult parseIfCases(String caseKeyword) {
        List<Node.IfNode.Case> cases = new ArrayList<>();
        Node.IfNode.Case elseCase = null;

        // parse IF/ELIF
        ParseResult res = new ParseResult();
        if (!currentTok.matches(Token.TT_KEYWORD, caseKeyword)) {
            return new IfCaseResult(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected '" + caseKeyword + "'"
            ));
        }
        res.registerAdvancement();
        advance();

        Node condition;
        {
            ParseResult r2 = expr();
            if (r2.error != null) return new IfCaseResult(r2.error);
            condition = r2.node;
        }

        if (!currentTok.matches(Token.TT_KEYWORD, "THEN")) {
            return new IfCaseResult(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'THEN'"
            ));
        }
        advance();

        if (currentTok.type.equals(Token.TT_NEWLINE)) {
            advance();
            ParseResult r2 = statements();
            if (r2.error != null) return new IfCaseResult(r2.error);
            cases.add(new Node.IfNode.Case(condition, r2.node, true));

            if (currentTok.matches(Token.TT_KEYWORD, "END")) {
                advance();
            } else {
                IfCaseResult next = parseElifOrElse();
                if (next.error != null) return next;
                cases.addAll(next.cases);
                elseCase = next.elseCase;
            }
        } else {
            ParseResult r2 = statement();
            if (r2.error != null) return new IfCaseResult(r2.error);
            cases.add(new Node.IfNode.Case(condition, r2.node, false));

            IfCaseResult next = parseElifOrElse();
            if (next.error != null) return next;
            cases.addAll(next.cases);
            elseCase = next.elseCase;
        }
        return new IfCaseResult(cases, elseCase);
    }

    private IfCaseResult parseElifOrElse() {
        if (currentTok.matches(Token.TT_KEYWORD, "ELIF")) {
            return parseIfCases("ELIF");
        }
        // ELSE
        Node.IfNode.Case elseCase = null;
        if (currentTok.matches(Token.TT_KEYWORD, "ELSE")) {
            advance();

            if (currentTok.type.equals(Token.TT_NEWLINE)) {
                advance();
                ParseResult r2 = statements();
                if (r2.error != null) return new IfCaseResult(r2.error);
                elseCase = new Node.IfNode.Case(null, r2.node, true);

                if (currentTok.matches(Token.TT_KEYWORD, "END")) {
                    advance();
                } else {
                    return new IfCaseResult(new Error.InvalidSyntaxError(
                        currentTok.posStart, currentTok.posEnd, "Expected 'END'"
                    ));
                }
            } else {
                ParseResult r2 = statement();
                if (r2.error != null) return new IfCaseResult(r2.error);
                elseCase = new Node.IfNode.Case(null, r2.node, false);
            }
        }
        return new IfCaseResult(new ArrayList<>(), elseCase);
    }

    private ParseResult forExpr() {
        ParseResult res = new ParseResult();

        if (!currentTok.matches(Token.TT_KEYWORD, "FOR")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'FOR'"
            ));
        }
        res.registerAdvancement();
        advance();

        if (!currentTok.type.equals(Token.TT_IDENTIFIER)) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected identifier"
            ));
        }
        Token varName = currentTok;
        res.registerAdvancement();
        advance();

        if (!currentTok.type.equals(Token.TT_EQ)) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected '='"
            ));
        }
        res.registerAdvancement();
        advance();

        Node startValue = res.register(expr());
        if (res.error != null) return res;

        if (!currentTok.matches(Token.TT_KEYWORD, "TO")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'TO'"
            ));
        }
        res.registerAdvancement();
        advance();

        Node endValue = res.register(expr());
        if (res.error != null) return res;

        Node stepValue = null;
        if (currentTok.matches(Token.TT_KEYWORD, "STEP")) {
            res.registerAdvancement();
            advance();
            stepValue = res.register(expr());
            if (res.error != null) return res;
        }

        if (!currentTok.matches(Token.TT_KEYWORD, "THEN")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'THEN'"
            ));
        }
        res.registerAdvancement();
        advance();

        if (currentTok.type.equals(Token.TT_NEWLINE)) {
            res.registerAdvancement();
            advance();
            Node body = res.register(statements());
            if (res.error != null) return res;

            if (!currentTok.matches(Token.TT_KEYWORD, "END")) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected 'END'"
                ));
            }
            res.registerAdvancement();
            advance();
            return res.success(new Node.ForNode(varName, startValue, endValue, stepValue, body, true));
        }

        Node body2 = res.register(statement());
        if (res.error != null) return res;
        return res.success(new Node.ForNode(varName, startValue, endValue, stepValue, body2, false));
    }

    private ParseResult whileExpr() {
        ParseResult res = new ParseResult();

        if (!currentTok.matches(Token.TT_KEYWORD, "WHILE")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'WHILE'"
            ));
        }
        res.registerAdvancement();
        advance();

        Node condition = res.register(expr());
        if (res.error != null) return res;

        if (!currentTok.matches(Token.TT_KEYWORD, "THEN")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'THEN'"
            ));
        }
        res.registerAdvancement();
        advance();

        if (currentTok.type.equals(Token.TT_NEWLINE)) {
            res.registerAdvancement();
            advance();
            Node body = res.register(statements());
            if (res.error != null) return res;

            if (!currentTok.matches(Token.TT_KEYWORD, "END")) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected 'END'"
                ));
            }
            res.registerAdvancement();
            advance();
            return res.success(new Node.WhileNode(condition, body, true));
        }

        Node body2 = res.register(statement());
        if (res.error != null) return res;
        return res.success(new Node.WhileNode(condition, body2, false));
    }

    private ParseResult funcDef() {
        ParseResult res = new ParseResult();

        if (!currentTok.matches(Token.TT_KEYWORD, "FUN")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'FUN'"
            ));
        }
        res.registerAdvancement();
        advance();

        Token varNameTok = null;
        if (currentTok.type.equals(Token.TT_IDENTIFIER)) {
            varNameTok = currentTok;
            res.registerAdvancement();
            advance();
            if (!currentTok.type.equals(Token.TT_LPAREN)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected '('"
                ));
            }
        } else {
            if (!currentTok.type.equals(Token.TT_LPAREN)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected identifier or '('"
                ));
            }
        }

        res.registerAdvancement();
        advance();
        List<Token> argNameToks = new ArrayList<>();

        if (currentTok.type.equals(Token.TT_IDENTIFIER)) {
            argNameToks.add(currentTok);
            res.registerAdvancement();
            advance();
            while (currentTok.type.equals(Token.TT_COMMA)) {
                res.registerAdvancement();
                advance();
                if (!currentTok.type.equals(Token.TT_IDENTIFIER)) {
                    return res.failure(new Error.InvalidSyntaxError(
                        currentTok.posStart, currentTok.posEnd, "Expected identifier"
                    ));
                }
                argNameToks.add(currentTok);
                res.registerAdvancement();
                advance();
            }
            if (!currentTok.type.equals(Token.TT_RPAREN)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected ',' or ')'"
                ));
            }
        } else {
            if (!currentTok.type.equals(Token.TT_RPAREN)) {
                return res.failure(new Error.InvalidSyntaxError(
                    currentTok.posStart, currentTok.posEnd, "Expected identifier or ')'"
                ));
            }
        }

        res.registerAdvancement();
        advance();

        if (currentTok.type.equals(Token.TT_ARROW)) {
            res.registerAdvancement();
            advance();
            Node body = res.register(expr());
            if (res.error != null) return res;
            return res.success(new Node.FuncDefNode(varNameTok, argNameToks, body, true));
        }

        if (!currentTok.type.equals(Token.TT_NEWLINE)) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected '->' or NEWLINE"
            ));
        }
        res.registerAdvancement();
        advance();

        Node body = res.register(statements());
        if (res.error != null) return res;

        if (!currentTok.matches(Token.TT_KEYWORD, "END")) {
            return res.failure(new Error.InvalidSyntaxError(
                currentTok.posStart, currentTok.posEnd, "Expected 'END'"
            ));
        }
        res.registerAdvancement();
        advance();

        return res.success(new Node.FuncDefNode(varNameTok, argNameToks, body, false));
    }

    @FunctionalInterface
    private interface ParseFn { ParseResult parse(); }

    private ParseResult binOp(ParseFn funcA, List<SimpleEntry<String, String>> ops) {
        return binOp(funcA, ops, funcA);
    }

    private ParseResult binOp(ParseFn funcA, List<SimpleEntry<String, String>> ops, ParseFn funcB) {
        ParseResult res = new ParseResult();
        Node left = res.register(funcA.parse());
        if (res.error != null) return res;

        while (opInList(currentTok, ops)) {
            Token opTok = currentTok;
            res.registerAdvancement();
            advance();
            Node right = res.register(funcB.parse());
            if (res.error != null) return res;
            left = new Node.BinOpNode(left, opTok, right);
        }
        return res.success(left);
    }

    private boolean opInList(Token tok, List<SimpleEntry<String, String>> ops) {
        for (SimpleEntry<String, String> op : ops) {
            if (op.getValue() == null) {
                if (tok.type.equals(op.getKey())) return true;
            } else {
                if (tok.matches(op.getKey(), op.getValue())) return true;
            }
        }
        return false;
    }
}
