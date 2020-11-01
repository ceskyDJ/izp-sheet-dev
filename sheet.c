#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ROW_SIZE 10 * 1024

void processRow(char *row, int rowSize, const char *delimiters);
bool isDelimiter(char c, const char *delimiters);

int main(int argc, char **argv) {
    /* ARGUMENTS PARSING */
    // Actual position in input arguments array - from 1 (program path is skipped)
    int argPos = 1;

    // Delimiters
    const char *DEFAULT_DELIMITER = " ";
    char **delimiters = (char **) &DEFAULT_DELIMITER;

    // For ex.: sheet.c -d :
    if (argc >= 3) {
        if (strcmp(argv[argPos], "-d") == 0) {
            delimiters = &argv[argPos + 1];
            argPos += 2;
        }
    }

    /* ROW PARSING */
    char actualRow[MAX_ROW_SIZE];
    int j = 0;
    int c;

    while ((c = getchar()) != EOF) {
        if (c != '\n') {
            actualRow[j] = (unsigned char) c;

            j++;
        } else {
            processRow(actualRow, j, *delimiters);

            printf("\n");

            // Start at the beginning of the array, old values will be replaced
            j = 0;
        }
    }

    return 0;
}

void processRow(char *row, int rowSize, const char *delimiters) {
    for (int i = 0; i < rowSize; i++) {
        // Replace all delimiters with the default one
        if (isDelimiter(row[i], delimiters) && row[i] != delimiters[0]) {
            row[i] = delimiters[0];
        }

        printf("%c", row[i]);
    }
}

bool isDelimiter(char c, const char *delimiters) {
    char delimiter;
    int i = 0;
    while ((delimiter = (delimiters)[i]) != '\0') {
        if (c == delimiter) {
            return true;
        }

        i++;
    }

    return false;
}