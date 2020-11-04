/**
 * Sheet
 *
 * Simple spreadsheet editor with pipeline processing
 *
 * @author Michal Šmahel <xsmahe01@stud.fit.vutbr.cz>
 * @date October-November 2020
 * @version 1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/**
 * Maximum size of one row (in bytes)
 */
#define MAX_ROW_SIZE 10 * 1024

/**
 * @typedef Individual row for processing
 */
typedef struct row {
    char data[MAX_ROW_SIZE];
    int size;
} Row;
/**
 * @typedef Error information tells how some action ended
 * @error: Did it end with error? (=> if true, something bad happened; otherwise the operation was successful)
 * @message: Description message in case of error = true
 */
typedef struct errorInfo {
    bool error;
    char *message;
} ErrorInfo;

void writeProcessedRow(Row *row);
ErrorInfo processRow(Row *row, char **delimiters);
char unifyDelimiters(Row *row, char **delimiters);
bool isDelimiter(char c, char **delimiters);

/**
 * Main function
 * @param argc Number of input arguments
 * @param argv Input arguments
 * @return Error code (0 => OK, 1 => error)
 */
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
    Row row;
    int j = 0;
    int c;

    // Do-while for cases \n is missing at the end of file (for ex. TXT files created in Jetbrains' IDEs)
    // The last char hasn't be \n
    do {
        c = getchar();

        if (c != '\n' && c != EOF) {
            row.data[j] = (unsigned char) c;

            j++;
        } else {
            row.size = j;
            processRow(&row, delimiters);

            writeProcessedRow(&row);

            if (c != EOF) {
                printf("\n");

                // Start at the beginning of the array, old values will be replaced
                j = 0;
            }
        }
    } while (c != EOF);

    return 0;
}

/**
 * Writes already processed row to standard output
 * @param row Processed row
 */
void writeProcessedRow(Row *row) {
    for (int i = 0; i < row->size; i++) {
        printf("%c", row->data[i]);
    }
}

/**
 * Processes provided row by other parameters
 * @param row Input (raw) row
 * @param delimiters Used delimiters
 * @return Error information
 */
ErrorInfo processRow(Row *row, char **delimiters) {
    ErrorInfo errorInfo = {false};

    // Delimiters processing
    char delimiter = unifyDelimiters(row, delimiters);

    return errorInfo;
}

/**
 * Unifies delimiters in provided row - all will be replaced with the first one
 * @param row Edited row
 * @param delimiters Used delimiters
 * @return Result delimiter
 */
char unifyDelimiters(Row *row, char **delimiters) {
    char mainDelimiter = (*delimiters)[0];

    for (int i = 0; i < row->size; i++) {
        if (isDelimiter(row->data[i], delimiters) && row->data[i] != mainDelimiter) {
            row->data[i] = mainDelimiter;
        }
    }

    return mainDelimiter;
}

/**
 * Checks if the provided char is a delimiter
 * @param c Char for checking
 * @param delimiters Used delimiters
 * @return Is the char a delimiter?
 */
bool isDelimiter(char c, char **delimiters) {
    char delimiter;
    int i = 0;
    while ((delimiter = (*delimiters)[i]) != '\0') {
        if (c == delimiter) {
            return true;
        }

        i++;
    }

    return false;
}