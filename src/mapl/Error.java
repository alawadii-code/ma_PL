package mapl;

public class Error {
    public Position posStart, posEnd;
    public String errorName, details;

    public Error(Position posStart, Position posEnd, String errorName, String details) {
        this.posStart = posStart;
        this.posEnd = posEnd;
        this.errorName = errorName;
        this.details = details;
    }

    public String asString() {
        StringBuilder result = new StringBuilder();
        result.append(errorName).append(": ").append(details).append("\n");
        result.append("File ").append(posStart.fn).append(", line ").append(posStart.ln + 1);
        result.append("\n\n").append(StringWithArrows.stringWithArrows(posStart.ftxt, posStart, posEnd));
        return result.toString();
    }

    public static class IllegalCharError extends Error {
        public IllegalCharError(Position posStart, Position posEnd, String details) {
            super(posStart, posEnd, "Illegal Character", details);
        }
    }

    public static class ExpectedCharError extends Error {
        public ExpectedCharError(Position posStart, Position posEnd, String details) {
            super(posStart, posEnd, "Expected Character", details);
        }
    }

    public static class InvalidSyntaxError extends Error {
        public InvalidSyntaxError(Position posStart, Position posEnd, String details) {
            super(posStart, posEnd, "Invalid Syntax", details);
        }
    }

    public static class RTError extends Error {
        public Context context;

        public RTError(Position posStart, Position posEnd, String details, Context context) {
            super(posStart, posEnd, "Runtime Error", details);
            this.context = context;
        }

        @Override
        public String asString() {
            StringBuilder result = new StringBuilder();
            result.append(generateTraceback());
            result.append(errorName).append(": ").append(details);
            result.append("\n\n").append(StringWithArrows.stringWithArrows(posStart.ftxt, posStart, posEnd));
            return result.toString();
        }

        private String generateTraceback() {
            StringBuilder result = new StringBuilder();
            Position pos = posStart;
            Context ctx = context;
            while (ctx != null) {
                result.insert(0, "  File " + pos.fn + ", line " + (pos.ln + 1) + ", in " + ctx.displayName + "\n");
                pos = ctx.parentEntryPos;
                ctx = ctx.parent;
            }
            return "Traceback (most recent call last):\n" + result.toString();
        }
    }
}
