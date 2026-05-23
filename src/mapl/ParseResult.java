package mapl;

public class ParseResult {
    public Error error;
    public Node node;
    public int lastRegisteredAdvanceCount = 0;
    public int advanceCount = 0;
    public int toReverseCount = 0;

    public void registerAdvancement() {
        lastRegisteredAdvanceCount = 1;
        advanceCount++;
    }

    public Node register(ParseResult res) {
        lastRegisteredAdvanceCount = res.advanceCount;
        advanceCount += res.advanceCount;
        if (res.error != null) error = res.error;
        return res.node;
    }

    public Node tryRegister(ParseResult res) {
        if (res.error != null) {
            toReverseCount = res.advanceCount;
            return null;
        }
        return register(res);
    }

    public ParseResult success(Node node) {
        this.node = node;
        return this;
    }

    public ParseResult failure(Error error) {
        if (this.error == null || lastRegisteredAdvanceCount == 0) {
            this.error = error;
        }
        return this;
    }
}
