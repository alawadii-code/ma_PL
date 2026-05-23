package mapl;

public class RTResult {
    public Value value;
    public Error error;
    public Value funcReturnValue;
    public boolean loopShouldContinue;
    public boolean loopShouldBreak;

    public RTResult() {
        reset();
    }

    public void reset() {
        value = null;
        error = null;
        funcReturnValue = null;
        loopShouldContinue = false;
        loopShouldBreak = false;
    }

    public Value register(RTResult res) {
        error = res.error;
        funcReturnValue = res.funcReturnValue;
        loopShouldContinue = res.loopShouldContinue;
        loopShouldBreak = res.loopShouldBreak;
        return res.value;
    }

    public RTResult success(Value value) {
        reset();
        this.value = value;
        return this;
    }

    public RTResult successReturn(Value value) {
        reset();
        funcReturnValue = value;
        return this;
    }

    public RTResult successContinue() {
        reset();
        loopShouldContinue = true;
        return this;
    }

    public RTResult successBreak() {
        reset();
        loopShouldBreak = true;
        return this;
    }

    public RTResult failure(Error error) {
        reset();
        this.error = error;
        return this;
    }

    public boolean shouldReturn() {
        return error != null || funcReturnValue != null || loopShouldContinue || loopShouldBreak;
    }
}
