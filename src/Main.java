import mapl.*;
import java.util.Scanner;

public class Main {
    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        if (args.length > 0) {
            String filename = args[0];
            try {
                java.nio.file.Path path = java.nio.file.Paths.get(filename);
                String content = new String(java.nio.file.Files.readAllBytes(path));
                maPL.RunResult result = maPL.run(filename, content);
                if (result.error != null) {
                    System.out.println(result.error.asString());
                } else if (result.value != null) {
                    System.out.println(result.value);
                }
            } catch (java.io.IOException e) {
                System.out.println("Error reading file: " + e.getMessage());
            }
            return;
        }

        System.out.println("  ▄████████  ▄█          ▄███████▄    ▄█    █▄       ▄████████ ▀█████████▄     ▄████████     ███      ▄███████▄");
        System.out.println("  ███    ███ ███         ███    ███   ███    ███     ███    ███   ███    ███   ███    ███ ▀█████████▄ ██▀     ▄██");
        System.out.println("  ███    ███ ███         ███    ███   ███    ███     ███    ███   ███    ███   ███    █▀     ▀███▀▀██       ▄███▀");
        System.out.println("  ███    ███ ███         ███    ███  ▄███▄▄▄▄███▄▄   ███    ███  ▄███▄▄▄██▀   ▄███▄▄▄         ███   ▀  ▀█▀▄███▀▄▄");
        System.out.println("▀███████████ ███       ▀█████████▀  ▀▀███▀▀▀▀███▀  ▀███████████ ▀▀███▀▀▀██▄  ▀▀███▀▀▀         ███       ▄███▀   ▀");
        System.out.println("  ███    ███ ███         ███          ███    ███     ███    ███   ███    ██▄   ███    █▄      ███     ▄███▀");
        System.out.println("  ███    ███ ███▌    ▄   ███          ███    ███     ███    ███   ███    ███   ███    ███     ███     ███▄     ▄█");
        System.out.println("  ███    █▀  █████▄▄██  ▄████▀        ███    █▀      ███    █▀  ▄█████████▀    ██████████    ▄████▀    ▀████████▀");
        System.out.println("maPL v1.0 - Interactive Shell");
        System.out.println("Type 'exit()' to quit");

        while (true) {
            System.out.print("maPL > ");
            if (!scanner.hasNextLine()) break;
            String text = scanner.nextLine();
            if (text.trim().isEmpty()) continue;
            if (text.trim().equals("exit()")) break;

            maPL.RunResult result = maPL.run("<stdin>", text);
            if (result.error != null) {
                System.out.println(result.error.asString());
            } else if (result.value != null && result.value instanceof Value.ListValue) {
                Value.ListValue list = (Value.ListValue) result.value;
                if (list.elements.size() == 1) {
                    System.out.println(list.elements.get(0));
                } else {
                    System.out.println(list);
                }
            } else if (result.value != null) {
                System.out.println(result.value);
            }
        }
    }
}
