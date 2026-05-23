package mapl;

public abstract class Node {
    public Position posStart, posEnd;

    public static class NumberNode extends Node {
        public Token tok;
        public NumberNode(Token tok) {
            this.tok = tok;
            posStart = tok.posStart;
            posEnd = tok.posEnd;
        }
        @Override public String toString() { return tok.toString(); }
    }

    public static class StringNode extends Node {
        public Token tok;
        public StringNode(Token tok) {
            this.tok = tok;
            posStart = tok.posStart;
            posEnd = tok.posEnd;
        }
        @Override public String toString() { return tok.toString(); }
    }

    public static class ListNode extends Node {
        public java.util.List<Node> elementNodes;
        public ListNode(java.util.List<Node> elementNodes, Position posStart, Position posEnd) {
            this.elementNodes = elementNodes;
            this.posStart = posStart;
            this.posEnd = posEnd;
        }
    }

    public static class VarAccessNode extends Node {
        public Token varNameTok;
        public VarAccessNode(Token varNameTok) {
            this.varNameTok = varNameTok;
            posStart = varNameTok.posStart;
            posEnd = varNameTok.posEnd;
        }
    }

    public static class VarAssignNode extends Node {
        public Token varNameTok;
        public Node valueNode;
        public VarAssignNode(Token varNameTok, Node valueNode) {
            this.varNameTok = varNameTok;
            this.valueNode = valueNode;
            posStart = varNameTok.posStart;
            posEnd = valueNode.posEnd;
        }
    }

    public static class BinOpNode extends Node {
        public Node leftNode, rightNode;
        public Token opTok;
        public BinOpNode(Node leftNode, Token opTok, Node rightNode) {
            this.leftNode = leftNode;
            this.opTok = opTok;
            this.rightNode = rightNode;
            posStart = leftNode.posStart;
            posEnd = rightNode.posEnd;
        }
        @Override public String toString() { return "(" + leftNode + ", " + opTok + ", " + rightNode + ")"; }
    }

    public static class UnaryOpNode extends Node {
        public Token opTok;
        public Node node;
        public UnaryOpNode(Token opTok, Node node) {
            this.opTok = opTok;
            this.node = node;
            posStart = opTok.posStart;
            posEnd = node.posEnd;
        }
        @Override public String toString() { return "(" + opTok + ", " + node + ")"; }
    }

    public static class IfNode extends Node {
        public java.util.List<Case> cases;
        public Case elseCase;
        public IfNode(java.util.List<Case> cases, Case elseCase) {
            this.cases = cases;
            this.elseCase = elseCase;
            posStart = cases.get(0).condition != null ? cases.get(0).condition.posStart : cases.get(0).expr.posStart;
            Node lastNode = elseCase != null ? elseCase.expr : cases.get(cases.size() - 1).expr;
            posEnd = lastNode.posEnd;
        }
        public static class Case {
            public Node condition, expr;
            public boolean shouldReturnNull;
            public Case(Node condition, Node expr, boolean shouldReturnNull) {
                this.condition = condition;
                this.expr = expr;
                this.shouldReturnNull = shouldReturnNull;
            }
        }
    }

    public static class ForNode extends Node {
        public Token varNameTok;
        public Node startValueNode, endValueNode, stepValueNode, bodyNode;
        public boolean shouldReturnNull;
        public ForNode(Token varNameTok, Node startValueNode, Node endValueNode,
                       Node stepValueNode, Node bodyNode, boolean shouldReturnNull) {
            this.varNameTok = varNameTok;
            this.startValueNode = startValueNode;
            this.endValueNode = endValueNode;
            this.stepValueNode = stepValueNode;
            this.bodyNode = bodyNode;
            this.shouldReturnNull = shouldReturnNull;
            posStart = varNameTok.posStart;
            posEnd = bodyNode.posEnd;
        }
    }

    public static class WhileNode extends Node {
        public Node conditionNode, bodyNode;
        public boolean shouldReturnNull;
        public WhileNode(Node conditionNode, Node bodyNode, boolean shouldReturnNull) {
            this.conditionNode = conditionNode;
            this.bodyNode = bodyNode;
            this.shouldReturnNull = shouldReturnNull;
            posStart = conditionNode.posStart;
            posEnd = bodyNode.posEnd;
        }
    }

    public static class FuncDefNode extends Node {
        public Token varNameTok;
        public java.util.List<Token> argNameToks;
        public Node bodyNode;
        public boolean shouldAutoReturn;
        public FuncDefNode(Token varNameTok, java.util.List<Token> argNameToks,
                           Node bodyNode, boolean shouldAutoReturn) {
            this.varNameTok = varNameTok;
            this.argNameToks = argNameToks;
            this.bodyNode = bodyNode;
            this.shouldAutoReturn = shouldAutoReturn;
            if (varNameTok != null) posStart = varNameTok.posStart;
            else if (!argNameToks.isEmpty()) posStart = argNameToks.get(0).posStart;
            else posStart = bodyNode.posStart;
            posEnd = bodyNode.posEnd;
        }
    }

    public static class CallNode extends Node {
        public Node nodeToCall;
        public java.util.List<Node> argNodes;
        public CallNode(Node nodeToCall, java.util.List<Node> argNodes) {
            this.nodeToCall = nodeToCall;
            this.argNodes = argNodes;
            posStart = nodeToCall.posStart;
            if (!argNodes.isEmpty()) posEnd = argNodes.get(argNodes.size() - 1).posEnd;
            else posEnd = nodeToCall.posEnd;
        }
    }

    public static class ReturnNode extends Node {
        public Node nodeToReturn;
        public ReturnNode(Node nodeToReturn, Position posStart, Position posEnd) {
            this.nodeToReturn = nodeToReturn;
            this.posStart = posStart;
            this.posEnd = posEnd;
        }
    }

    public static class ContinueNode extends Node {
        public ContinueNode(Position posStart, Position posEnd) {
            this.posStart = posStart;
            this.posEnd = posEnd;
        }
    }

    public static class BreakNode extends Node {
        public BreakNode(Position posStart, Position posEnd) {
            this.posStart = posStart;
            this.posEnd = posEnd;
        }
    }
}
