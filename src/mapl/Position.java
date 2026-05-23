package mapl;

public class Position {
    public int idx, ln, col;
    public String fn, ftxt;

    public Position(int idx, int ln, int col, String fn, String ftxt) {
        this.idx = idx;
        this.ln = ln;
        this.col = col;
        this.fn = fn;
        this.ftxt = ftxt;
    }

    public Position advance(char currentChar) {
        idx++;
        col++;
        if (currentChar == '\n') {
            ln++;
            col = 0;
        }
        return this;
    }

    public Position copy() {
        return new Position(idx, ln, col, fn, ftxt);
    }
}
