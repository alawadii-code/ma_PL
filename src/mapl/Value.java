package mapl;

import java.util.ArrayList;
import java.util.List;

public abstract class Value {
    public Position posStart, posEnd;
    public Context context;

    public Value setPos(Position posStart, Position posEnd) {
        this.posStart = posStart;
        this.posEnd = posEnd;
        return this;
    }

    public Value setContext(Context context) {
        this.context = context;
        return this;
    }

    public RTResult addedTo(Value other) { return illegalOperation(other); }
    public RTResult subbedBy(Value other) { return illegalOperation(other); }
    public RTResult multedBy(Value other) { return illegalOperation(other); }
    public RTResult divedBy(Value other) { return illegalOperation(other); }
    public RTResult powedBy(Value other) { return illegalOperation(other); }
    public RTResult getComparisonEq(Value other) { return illegalOperation(other); }
    public RTResult getComparisonNe(Value other) { return illegalOperation(other); }
    public RTResult getComparisonLt(Value other) { return illegalOperation(other); }
    public RTResult getComparisonGt(Value other) { return illegalOperation(other); }
    public RTResult getComparisonLte(Value other) { return illegalOperation(other); }
    public RTResult getComparisonGte(Value other) { return illegalOperation(other); }
    public RTResult andedBy(Value other) { return illegalOperation(other); }
    public RTResult oredBy(Value other) { return illegalOperation(other); }
    public RTResult notted() { return illegalOperation(null); }
    public RTResult execute(List<Value> args) {
        return new RTResult().failure(new Error.RTError(posStart, posEnd, "Illegal operation", context));
    }
    public abstract Value copy();
    public boolean isTrue() { return false; }

    public RTResult illegalOperation(Value other) {
        if (other == null) other = this;
        return new RTResult().failure(new Error.RTError(posStart, other.posEnd, "Illegal operation", context));
    }

    // Number
    public static class Number extends Value {
        public double value;
        public Number(double value) { this.value = value; }

        @Override public RTResult addedTo(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value + n.value).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult subbedBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value - n.value).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult multedBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value * n.value).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult divedBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                if (n.value == 0) {
                    return new RTResult().failure(new Error.RTError(other.posStart, other.posEnd, "Division by zero", context));
                }
                return new RTResult().success(new Number(this.value / n.value).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult powedBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(Math.pow(this.value, n.value)).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult getComparisonEq(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value == n.value ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult getComparisonNe(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value != n.value ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult getComparisonLt(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value < n.value ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult getComparisonGt(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value > n.value ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult getComparisonLte(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value <= n.value ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult getComparisonGte(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number(this.value >= n.value ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult andedBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number((isTrue() && n.isTrue()) ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult oredBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                return new RTResult().success(new Number((isTrue() || n.isTrue()) ? 1 : 0).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult notted() {
            return new RTResult().success(new Number(isTrue() ? 0 : 1).setContext(context));
        }
        @Override public Value copy() {
            Number copy = new Number(value);
            copy.setPos(posStart, posEnd);
            copy.setContext(context);
            return copy;
        }
        @Override public boolean isTrue() { return value != 0; }
        @Override public String toString() {
            if (value == Math.floor(value) && !Double.isInfinite(value)) {
                return String.valueOf((long) value);
            }
            return String.valueOf(value);
        }

        public static Number null_ = new Number(0);
        public static Number false_ = new Number(0);
        public static Number true_ = new Number(1);
        public static Number mathPI = new Number(Math.PI);
    }

    // String
    public static class StringValue extends Value {
        public String value;
        public StringValue(String value) { this.value = value; }

        @Override public RTResult addedTo(Value other) {
            if (other instanceof StringValue) {
                StringValue s = (StringValue) other;
                return new RTResult().success(new StringValue(this.value + s.value).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public RTResult multedBy(Value other) {
            if (other instanceof Number) {
                Number n = (Number) other;
                StringBuilder sb = new StringBuilder();
                for (int i = 0; i < (int) n.value; i++) sb.append(this.value);
                return new RTResult().success(new StringValue(sb.toString()).setContext(context));
            }
            return illegalOperation(other);
        }
        @Override public boolean isTrue() { return value.length() > 0; }
        @Override public Value copy() {
            StringValue copy = new StringValue(value);
            copy.setPos(posStart, posEnd);
            copy.setContext(context);
            return copy;
        }
        @Override public String toString() { return value; }
    }

    // List
    public static class ListValue extends Value {
        public List<Value> elements;
        public ListValue(List<Value> elements) { this.elements = new ArrayList<>(elements); }

        @Override public RTResult addedTo(Value other) {
            ListValue newList = (ListValue) this.copy();
            newList.elements.add(other);
            return new RTResult().success(newList);
        }
        @Override public RTResult subbedBy(Value other) {
            if (other instanceof Number) {
                ListValue newList = (ListValue) this.copy();
                int idx = (int) ((Number) other).value;
                if (idx >= 0 && idx < newList.elements.size()) {
                    newList.elements.remove(idx);
                    return new RTResult().success(newList);
                }
                return new RTResult().failure(new Error.RTError(
                    other.posStart, other.posEnd,
                    "Element at this index could not be removed from list because index is out of bounds",
                    context
                ));
            }
            return illegalOperation(other);
        }
        @Override public RTResult multedBy(Value other) {
            if (other instanceof ListValue) {
                ListValue newList = (ListValue) this.copy();
                newList.elements.addAll(((ListValue) other).elements);
                return new RTResult().success(newList);
            }
            return illegalOperation(other);
        }
        @Override public RTResult divedBy(Value other) {
            if (other instanceof Number) {
                int idx = (int) ((Number) other).value;
                if (idx >= 0 && idx < elements.size()) {
                    return new RTResult().success(elements.get(idx));
                }
                return new RTResult().failure(new Error.RTError(
                    other.posStart, other.posEnd,
                    "Element at this index could not be retrieved from list because index is out of bounds",
                    context
                ));
            }
            return illegalOperation(other);
        }
        @Override public Value copy() {
            ListValue copy = new ListValue(elements);
            copy.setPos(posStart, posEnd);
            copy.setContext(context);
            return copy;
        }
        @Override public String toString() {
            StringBuilder sb = new StringBuilder("[");
            for (int i = 0; i < elements.size(); i++) {
                if (i > 0) sb.append(", ");
                sb.append(elements.get(i));
            }
            sb.append("]");
            return sb.toString();
        }
    }
}
