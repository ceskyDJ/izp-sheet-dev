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
 * @def MAX_ROW_SIZE Maximum size of one row (in bytes)
 */
#define MAX_ROW_SIZE 10 * 1024
/**
 * @def DEFAULT_DELIMITER Default delimiter for case user didn't set different
 */
#define DEFAULT_DELIMITER ""

/**
 * @typedef Row Individual row for processing
 * @field data Row content
 * @field size Row size (number of contained chars)
 * @field number Row number (from 1)
 */
typedef struct row {
    char data[MAX_ROW_SIZE];
    int size;
    int number;
} Row;
/**
 * @typedef Error information tells how some action ended
 * @field error Did it end with error? (=> if true, something bad happened; otherwise the operation was successful)
 * @field message Description message in case of error = true
 */
typedef struct errorInfo {
    bool error;
    char *message;
} ErrorInfo;
/**
 * @typedef Input program arguments from argv
 * @field args Array of arguments
 * @field size Size of the arguments array
 * @field skipped Number of skipped arguments (already used)
 */
typedef struct inputArguments {
    char **args;
    int size;
    int skipped;
} InputArguments;

void writeProcessedRow(const Row *row);
void writeErrorMessage(const char *message);
ErrorInfo processRow(Row *row, const char **delimiters);
char unifyDelimiters(Row *row, const char **delimiters);
bool isDelimiter(char c, const char **delimiters);

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
    Row row = {.number = 0};
    while (fgets(row.data, MAX_ROW_SIZE, stdin) != NULL) {
        row.size = strlen(row.data);
        row.number++;

        ErrorInfo err;
        if ((err = processRow(&row, (const char **) delimiters)).error) {
            writeErrorMessage(err.message);

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
ErrorInfo processRow(Row *row, const char **delimiters) {
    ErrorInfo errorInfo = {false};

    // Check max row size
    if (row->data[MAX_ROW_SIZE - 1] != '\0' && row->data[MAX_ROW_SIZE - 1] != '\n') {
        errorInfo.error = true;
        errorInfo.message = "Byla prekrocena maximalni velikost radku.";

        return errorInfo;
    }

    // Delimiters processing
    /*char delimiter =*/ unifyDelimiters(row, delimiters);

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