#include <stdio.h>
#include <string.h>

#define MAX_ROW_SIZE 10 * 1024

void someWorkWithRow(char row[], int rowSize);

int main(int argc, char **argv) {
    /* ARGUMENTS PARSING */
    // Actual position in input arguments array - from 1 (program path is skipped)
    int argPos = 1;

    // Delimiters
    char const *DEFAULT_DELIMITER = " ";
    char **delimiters = (char **) &DEFAULT_DELIMITER;

    // For ex.: sheet.c -d :
    if (argc >= 3) {
        if (strcmp(argv[argPos], "-d") == 0) {
            delimiters = &argv[argPos + 1];
            argPos += 2;
        }
    }

    // TODO: Remove! This is just for testing
    printf("Delimiters: ");
    char delimiter;
    int i = 0;
    while ((delimiter = (*delimiters)[i]) != '\0') {
        char *format;

        if (i != 0) {
            format = ", '%c'";
        } else {
            format = "'%c'";
        }

        printf(format, delimiter);

        i++;
    }
    printf("\n");

    /* ROW PARSING */
    char actualRow[MAX_ROW_SIZE];
    int j = 0;
    int c;

    while ((c = getchar()) != EOF) {
        if (c != '\n') {
            actualRow[j] = (unsigned char) c;

            j++;
        } else {
            // TODO: Process the row...
            someWorkWithRow(actualRow, j);

            printf("\n");

            // Start at the beginning of the array, old values will be replaced
            j = 0;
        }
    }

    return 0;
}

void someWorkWithRow(char row[], int rowSize) {
    for (int i = 0; i < rowSize; i++) {
        printf("%c", row[i]);
    }
}