#include <stdio.h>

#define MAX_ROW_SIZE 10 * 1024

void someWorkWithRow(char row[], int rowSize);

int main(int argc, char **argv) {
    char actualRow[MAX_ROW_SIZE];
    int i = 0;
    int c;

    while ((c = getchar()) != EOF) {
        if (c != '\n') {
            actualRow[i] = (unsigned char) c;

            i++;
        } else {
            // TODO: Process the row...
            someWorkWithRow(actualRow, i);

            printf("\n");

            // Start at the beginning of the array, old values will be replaced
            i = 0;
        }
    }

    return 0;
}

void someWorkWithRow(char row[], int rowSize) {
    for (int i = 0; i < rowSize; i++) {
        printf("%c", row[i]);
    }
}