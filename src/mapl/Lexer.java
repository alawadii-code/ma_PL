package mapl;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.HashSet;
import java.util.Arrays;
import java.util.Map;
import java.util.HashMap;

public class Lexer {
    private static final String DIGITS = "0123456789";
    private static final String LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    private static final String LETTERS_DIGITS = LETTERS + DIGITS;
    private static final Set<String> KEYWORD_SET = new HashSet<>(Arrays.asList(Token.KEYWORDS));

    public String fn, text;
    public Position pos;
    public Character currentChar;

    public Lexer(String fn, String text) {
        this.fn = fn;
        this.text = text;
        pos = new Position(-1, 0, -1, fn, text);
        currentChar = null;
        advance();
    }

    public void advance() {
        pos.advance(currentChar != null ? currentChar : '\0');
        currentChar = pos.idx < text.length() ? text.charAt(pos.idx) : null;
    }

    public Result makeTokens() {
        List<Token> tokens = new ArrayList<>();
        while (currentChar != null) {
            if (currentChar == ' ' || currentChar == '\t') {
                advance();
            } else if (currentChar == '#') {
                skipComment();
            } else if (currentChar == ';' || currentChar == '\n') {
                tokens.add(new Token(Token.TT_NEWLINE, (Object) null, pos.copy(), null));
                advance();
            } else if (DIGITS.indexOf(currentChar) != -1) {
                tokens.add(makeNumber());
            } else if (LETTERS.indexOf(currentChar) != -1) {
                tokens.add(makeIdentifier());
            } else if (currentChar == '"') {
                tokens.add(makeString());
            } else if (currentChar == '+') {
                tokens.add(new Token(Token.TT_PLUS, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == '-') {
                tokens.add(makeMinusOrArrow());
            } else if (currentChar == '*') {
                tokens.add(new Token(Token.TT_MUL, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == '/') {
                tokens.add(new Token(Token.TT_DIV, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == '^') {
                tokens.add(new Token(Token.TT_POW, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == '(') {
                tokens.add(new Token(Token.TT_LPAREN, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == ')') {
                tokens.add(new Token(Token.TT_RPAREN, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == '[') {
                tokens.add(new Token(Token.TT_LSQUARE, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == ']') {
                tokens.add(new Token(Token.TT_RSQUARE, (Object) null, pos.copy(), null));
                advance();
            } else if (currentChar == '!') {
                Result r = makeNotEquals();
                if (r.error != null) return Result.fromTokens(null, r.error);
                tokens.add(r.token);
            } else if (currentChar == '=') {
                tokens.add(makeEquals());
            } else if (currentChar == '<') {
                tokens.add(makeLessThan());
            } else if (currentChar == '>') {
                tokens.add(makeGreaterThan());
            } else if (currentChar == ',') {
                tokens.add(new Token(Token.TT_COMMA, (Object) null, pos.copy(), null));
                advance();
            } else {
                Position posStart = pos.copy();
                char c = currentChar;
                advance();
                return Result.fromTokens(null, new Error.IllegalCharError(posStart, pos, "'" + c + "'"));
            }
        }
        tokens.add(new Token(Token.TT_EOF, (Object) null, pos.copy(), null));
        return Result.fromTokens(tokens, null);
    }

    private Token makeNumber() {
        StringBuilder numStr = new StringBuilder();
        int dotCount = 0;
        Position posStart = pos.copy();
        while (currentChar != null && (DIGITS + ".").indexOf(currentChar) != -1) {
            if (currentChar == '.') {
                if (dotCount == 1) break;
                dotCount++;
            }
            numStr.append(currentChar);
            advance();
        }
        if (dotCount == 0) {
            return new Token(Token.TT_INT, Integer.parseInt(numStr.toString()), posStart, pos);
        } else {
            return new Token(Token.TT_FLOAT, Double.parseDouble(numStr.toString()), posStart, pos);
        }
    }

    private Token makeString() {
        StringBuilder str = new StringBuilder();
        Position posStart = pos.copy();
        boolean escapeCharacter = false;
        advance();

        Map<Character, Character> escapeChars = new HashMap<>();
        escapeChars.put('n', '\n');
        escapeChars.put('t', '\t');

        while (currentChar != null && (currentChar != '"' || escapeCharacter)) {
            if (escapeCharacter) {
                str.append(escapeChars.getOrDefault(currentChar, currentChar));
                escapeCharacter = false;
            } else {
                if (currentChar == '\\') {
                    escapeCharacter = true;
                } else {
                    str.append(currentChar);
                }
            }
            advance();
        }
        advance();
        return new Token(Token.TT_STRING, str.toString(), posStart, pos);
    }

    private Token makeIdentifier() {
        StringBuilder idStr = new StringBuilder();
        Position posStart = pos.copy();
        while (currentChar != null && (LETTERS_DIGITS + "_").indexOf(currentChar) != -1) {
            idStr.append(currentChar);
            advance();
        }
        String value = idStr.toString();
        String tokType = KEYWORD_SET.contains(value) ? Token.TT_KEYWORD : Token.TT_IDENTIFIER;
        return new Token(tokType, value, posStart, pos);
    }

    private Token makeMinusOrArrow() {
        Position posStart = pos.copy();
        advance();
        if (currentChar != null && currentChar == '>') {
            advance();
            return new Token(Token.TT_ARROW, (Object) null, posStart, pos);
        }
        return new Token(Token.TT_MINUS, (Object) null, posStart, pos);
    }

    private Result makeNotEquals() {
        Position posStart = pos.copy();
        advance();
        if (currentChar != null && currentChar == '=') {
            advance();
            return Result.fromToken(new Token(Token.TT_NE, (Object) null, posStart, pos), null);
        }
        advance();
        return Result.fromTokens(null, new Error.ExpectedCharError(posStart, pos, "'=' (after '!')"));
    }

    private Token makeEquals() {
        String tokType = Token.TT_EQ;
        Position posStart = pos.copy();
        advance();
        if (currentChar != null && currentChar == '=') {
            advance();
            tokType = Token.TT_EE;
        }
        return new Token(tokType, (Object) null, posStart, pos);
    }

    private Token makeLessThan() {
        String tokType = Token.TT_LT;
        Position posStart = pos.copy();
        advance();
        if (currentChar != null && currentChar == '=') {
            advance();
            tokType = Token.TT_LTE;
        }
        return new Token(tokType, (Object) null, posStart, pos);
    }

    private Token makeGreaterThan() {
        String tokType = Token.TT_GT;
        Position posStart = pos.copy();
        advance();
        if (currentChar != null && currentChar == '=') {
            advance();
            tokType = Token.TT_GTE;
        }
        return new Token(tokType, (Object) null, posStart, pos);
    }

    private void skipComment() {
        advance();
        while (currentChar != null && currentChar != '\n') {
            advance();
        }
        advance();
    }

    public static class Result {
        public List<Token> tokens;
        public Error error;
        public Token token;
        private Result(List<Token> tokens, Token token, Error error) {
            this.tokens = tokens;
            this.token = token;
            this.error = error;
        }
        public static Result fromTokens(List<Token> tokens, Error error) {
            return new Result(tokens, null, error);
        }
        public static Result fromToken(Token token, Error error) {
            return new Result(null, token, error);
        }
    }
}
