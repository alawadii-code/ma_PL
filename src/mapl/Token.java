package mapl;

public class Token {
    public static final String
        TT_INT = "INT",
        TT_FLOAT = "FLOAT",
        TT_STRING = "STRING",
        TT_IDENTIFIER = "IDENTIFIER",
        TT_KEYWORD = "KEYWORD",
        TT_PLUS = "PLUS",
        TT_MINUS = "MINUS",
        TT_MUL = "MUL",
        TT_DIV = "DIV",
        TT_POW = "POW",
        TT_EQ = "EQ",
        TT_LPAREN = "LPAREN",
        TT_RPAREN = "RPAREN",
        TT_LSQUARE = "LSQUARE",
        TT_RSQUARE = "RSQUARE",
        TT_EE = "EE",
        TT_NE = "NE",
        TT_LT = "LT",
        TT_GT = "GT",
        TT_LTE = "LTE",
        TT_GTE = "GTE",
        TT_COMMA = "COMMA",
        TT_ARROW = "ARROW",
        TT_NEWLINE = "NEWLINE",
        TT_EOF = "EOF";

    public static final String[] KEYWORDS = {
        "VAR", "AND", "OR", "NOT", "IF", "ELIF", "ELSE",
        "FOR", "TO", "STEP", "WHILE", "FUN", "THEN", "END",
        "RETURN", "CONTINUE", "BREAK"
    };

    public String type;
    public Object value;
    public Position posStart, posEnd;

    public Token(String type, Object value, Position posStart, Position posEnd) {
        this.type = type;
        this.value = value;
        if (posStart != null) {
            this.posStart = posStart.copy();
            this.posEnd = posStart.copy();
            this.posEnd.advance((char) 0);
        }
        if (posEnd != null) {
            this.posEnd = posEnd.copy();
        }
    }

    public boolean matches(String type, String value) {
        return this.type.equals(type) && this.value != null && this.value.equals(value);
    }

    @Override
    public String toString() {
        if (value != null) return type + ":" + value;
        return type;
    }
}
