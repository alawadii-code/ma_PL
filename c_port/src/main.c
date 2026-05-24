#include "mapl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc > 1) {
        const char* filename = argv[1];
        FILE* f = fopen(filename, "r");
        if (!f) {
            fprintf(stderr, "Error reading file: %s\n", filename);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* content = malloc(fsize + 1);
        if (!content) { fclose(f); return 1; }
        fread(content, 1, fsize, f);
        content[fsize] = '\0';
        fclose(f);

        RunResult result = mapl_run(filename, content);
        free(content);
        if (result.has_error) {
            char* err_str = error_as_string(&result.error);
            printf("%s", err_str);
            free(err_str);
            error_free(&result.error);
            return 1;
        }
        if (result.value) {
            char* s = value_to_string(result.value);
            printf("%s\n", s);
            free(s);
        }
        return 0;
    }

    /* REPL */
    printf("  ▄████████  ▄█          ▄███████▄    ▄█    █▄       ▄████████ ▀█████████▄     ▄████████     ███      ▄███████▄\n");
    printf("  ███    ███ ███         ███    ███   ███    ███     ███    ███   ███    ███   ███    ███ ▀█████████▄ ██▀     ▄██\n");
    printf("  ███    ███ ███         ███    ███   ███    ███     ███    ███   ███    ███   ███    █▀     ▀███▀▀██       ▄███▀\n");
    printf("  ███    ███ ███         ███    ███  ▄███▄▄▄▄███▄▄   ███    ███  ▄███▄▄▄██▀   ▄███▄▄▄         ███   ▀  ▀█▀▄███▀▄▄\n");
    printf("▀███████████ ███       ▀█████████▀  ▀▀███▀▀▀▀███▀  ▀███████████ ▀▀███▀▀▀██▄  ▀▀███▀▀▀         ███       ▄███▀   ▀\n");
    printf("  ███    ███ ███         ███          ███    ███     ███    ███   ███    ██▄   ███    █▄      ███     ▄███▀\n");
    printf("  ███    ███ ███▌    ▄   ███          ███    ███     ███    ███   ███    ███   ███    ███     ███     ███▄     ▄█\n");
    printf("  ███    █▀  █████▄▄██  ▄████▀        ███    █▀      ███    █▀  ▄█████████▀    ██████████    ▄████▀    ▀████████▀\n");
    printf("maPL v1.0 - Interactive Shell (C Port)\n");
    printf("Type 'exit()' to quit\n");

    char line[8192];
    while (1) {
        printf("maPL > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        if (strcmp(line, "exit()") == 0) break;
        if (strlen(line) == 0) continue;

        RunResult result = mapl_run("<stdin>", line);
        if (result.has_error) {
            char* err_str = error_as_string(&result.error);
            printf("%s", err_str);
            free(err_str);
            error_free(&result.error);
        } else if (result.value) {
            Value* v = result.value;
            if (v->type == VAL_LIST && v->data.list.count == 1) {
                char* s = value_to_string(v->data.list.items[0]);
                printf("%s\n", s);
                free(s);
            } else {
                char* s = value_to_string(v);
                printf("%s\n", s);
                free(s);
            }
        }
    }
    return 0;
}
