package mapl;

import java.util.HashMap;
import java.util.Map;

public class SymbolTable {
    public Map<String, Value> symbols;
    public SymbolTable parent;

    public SymbolTable(SymbolTable parent) {
        this.symbols = new HashMap<>();
        this.parent = parent;
    }

    public SymbolTable() {
        this(null);
    }

    public Value get(String name) {
        Value value = symbols.get(name);
        if (value == null && parent != null) {
            return parent.get(name);
        }
        return value;
    }

    public void set(String name, Value value) {
        symbols.put(name, value);
    }

    public void remove(String name) {
        symbols.remove(name);
    }
}
