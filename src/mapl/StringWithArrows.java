package mapl;

public class StringWithArrows {
    public static String stringWithArrows(String text, Position posStart, Position posEnd) {
        StringBuilder result = new StringBuilder();
        int idxStart = Math.max(text.lastIndexOf('\n', posStart.idx), 0);
        int idxEnd = text.indexOf('\n', idxStart + 1);
        if (idxEnd < 0) idxEnd = text.length();

        int lineCount = posEnd.ln - posStart.ln + 1;
        for (int i = 0; i < lineCount; i++) {
            String line = text.substring(idxStart, idxEnd);
            int colStart = (i == 0) ? posStart.col : 0;
            int colEnd = (i == lineCount - 1) ? posEnd.col : line.length() - 1;

            result.append(line).append("\n");
            for (int j = 0; j < colStart; j++) result.append(' ');
            for (int j = colStart; j < colEnd; j++) result.append('^');

            idxStart = idxEnd;
            idxEnd = text.indexOf('\n', idxStart + 1);
            if (idxEnd < 0) idxEnd = text.length();
        }
        return result.toString().replace("\t", "");
    }
}
