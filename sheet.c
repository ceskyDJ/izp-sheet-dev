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
 * Default delimiter for case user didn't set different
 */
#define DEFAULT_DELIMITER ""

/**
 * @typedef Individual row for processing
 */
typedef struct {
    char data[MAX_ROW_SIZE];
    int size;
} Row;
/**
 * @typedef Error information tells how some action ended
 * @error: Did it end with error? (=> if true, something bad happened; otherwise the operation was successful)
 * @message: Description message in case of error = true
 */
typedef struct {
    bool error;
    char *message;
} ErrorInfo;
/**
 * @typedef Input program arguments from argv
 * @args: Array of arguments
 * @size: Size of the arguments array
 * @skipped: Number of skipped arguments (already used)
 */
typedef struct {
    char **args;
    int size;
    int skipped;
} InputArguments;

void writeProcessedRow(const Row *row);
void writeErrorMessage(const char *message);
ErrorInfo processRow(Row *row, const char **delimiters, int *numberOfColumns);
char unifyDelimiters(Row *row, const char **delimiters);
bool isDelimiter(char c, const char **delimiters);
int countColumns(const Row *row, char delimiter);

/**
 * Main function
 * @param argc Number of input arguments
 * @param argv Input arguments
 * @return Error code (0 => OK, 1 => error)
 */
int main(int argc, char **argv) {
    /* ARGUMENTS PARSING */
    // The first argument is skipped (program path)
    InputArguments args = {argv, argc, 1};

    // Delimiters
    char **delimiters = (char **) &(DEFAULT_DELIMITER);
    if (argc >= 3) {
        if (strcmp(args.args[args.skipped], "-d") == 0) {
            delimiters = &args.args[args.skipped + 1];
            args.skipped += 2;
        }
    }

    /* ROW PARSING */
    Row row;
    int lastNumberOfColumns = -1;

    while (fgets(row.data, MAX_ROW_SIZE, stdin) != NULL) {
        row.size = strlen(row.data);

        int numberOfColumns;
        processRow(&row, (const char **) delimiters, &numberOfColumns);

        if (lastNumberOfColumns == -1) {
            lastNumberOfColumns = numberOfColumns;
        }

        if (numberOfColumns != lastNumberOfColumns) {
            writeErrorMessage("Tabulka nema stejny pocet sloupcu ve vsech radcich.");
            return 1;
        }

        writeProcessedRow(&row);
    }

    return 0;
}

/**
 * Writes already processed row to standard output
 * @param row Processed row
 */
void writeProcessedRow(const Row *row) {
    for (int i = 0; i < row->size; i++) {
        printf("%c", row->data[i]);
    }
}

/**
 * Writes error message to standard error output
 * @param message Error message
 */
void writeErrorMessage(const char *message) {
    fprintf(stderr, "sheet: %s", message);
}

/**
 * Processes provided row by other parameters
 * @param row Input (raw) row
 * @param delimiters Used delimiters
 * @return Error information
 */
ErrorInfo processRow(Row *row, const char **delimiters, int *numberOfColumns) {
    ErrorInfo errorInfo = {false};

    // Delimiters processing
    char delimiter = unifyDelimiters(row, delimiters);
    *numberOfColumns = countColumns(row, delimiter);

    return errorInfo;
}

/**
 * Unifies delimiters in provided row - all will be replaced with the first one
 * @param row Edited row
 * @param delimiters Used delimiters
 * @return Result delimiter
 */
char unifyDelimiters(Row *row, const char **delimiters) {
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
bool isDelimiter(char c, const char **delimiters) {
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

/**
 * Counts number of columns in provided row
 * @param row Row
 * @param delimiter Column delimiter
 * @return Number of columns in the row
 */
int countColumns(const Row *row, char delimiter) {
    // Empty row has always 0 columns
    if (row->size == 0) {
        return 0;
    }

    // Non-empty row has 1 column minimally
    int counter = 1;

    // Every next delimiter =>  next column
    for (int i = 0; i < row->size; i++) {
        if (row->data[i] == delimiter) {
            counter++;
        }
    }

    return counter;
}