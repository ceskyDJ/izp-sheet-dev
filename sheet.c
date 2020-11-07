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
#include <stdlib.h>

/**
 * @def MAX_ROW_SIZE Maximum size of one row (in bytes)
 */
#define MAX_ROW_SIZE 10 * 1024
/**
 * @def MAX_CELL_SIZE Maximum size of table cell (in bytes)
 */
#define MAX_CELL_SIZE 100
/**
 * @def DEFAULT_DELIMITER Default delimiter for case user didn't set different
 */
#define DEFAULT_DELIMITER ""
/**
 * @def ANY_NUMBER Numeric representation of '-' state (any row/column number)
 */
#define ANY_NUMBER 0
/**
 * @def INVALID_NUMBER Invalid number of row/column provided in input arguments
 */
#define INVALID_NUMBER -1

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
 * @field data Array of arguments
 * @field size Size of the arguments array
 * @field skipped Number of skipped arguments (already used)
 */
typedef struct inputArguments {
    char **data;
    int size;
    int skipped;
} InputArguments;

void writeProcessedRow(const Row *row);
void writeErrorMessage(const char *message);
ErrorInfo processRow(Row *row, const char **delimiters, const InputArguments *args);
char unifyDelimiters(Row *row, const char **delimiters);
bool isDelimiter(char c, const char **delimiters);
bool checkCellsSizes(const Row *row, char delimiter);
int countColumns(Row *row, char delimiter);
int convertToRowColumnNumber(char *value);

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
        if (strcmp(args.data[args.skipped], "-d") == 0) {
            delimiters = &args.data[args.skipped + 1];
            args.skipped += 2;
        }
    }

    /* ROW PARSING */
    Row row = {.number = 0};
    while (fgets(row.data, MAX_ROW_SIZE, stdin) != NULL) {
        row.size = strlen(row.data);
        row.number++;

        ErrorInfo err;
        if ((err = processRow(&row, (const char **) delimiters, &args)).error) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        writeProcessedRow(&row);
    }

    return EXIT_SUCCESS;
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
ErrorInfo processRow(Row *row, const char **delimiters, const InputArguments *args) {
    ErrorInfo errorInfo = {false};

    // Check max row size
    if (row->data[MAX_ROW_SIZE - 1] != '\0' && row->data[MAX_ROW_SIZE - 1] != '\n') {
        errorInfo.error = true;
        errorInfo.message = "Byla prekrocena maximalni velikost radku.";

        return errorInfo;
    }

    // Delimiters processing
    char delimiter = unifyDelimiters(row, delimiters);
    /*int numberOfColumns =*/ countColumns(row, delimiter);

    // Check cell size
    if (checkCellsSizes(row, delimiter) == false) {
        errorInfo.error = true;
        errorInfo.message = "Byla prekrocena maximalni velikost bunky.";

        return errorInfo;
    }

    // Apply table editing functions
    int newArgsSize = args->size - args->skipped;
    for (int i = args->skipped; i < newArgsSize; i++) {
        // TODO: apply functions from argument on the row
    }

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
 * Checks cells' sizes
 * @param row Row to check cells in
 * @param delimiter Cell delimiter
 * @return Does the row contain only valid-sized cells?
 */
bool checkCellsSizes(const Row *row, char delimiter) {
    int size = 0;
    for (int i = 0; i < row->size; i++) {
        if (row->data[i] != delimiter) {
            size++;
        } else {
            size = 0;
        }

        if (size > MAX_CELL_SIZE) {
            return false;
        }
    }

    return true;
}

/**
 * Counts number of columns in provided row
 * @param row Row
 * @param delimiter Column delimiter
 * @return Number of columns in the row
 */
int countColumns(Row *row, char delimiter) {
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

/**
 * Converts string to row/column number
 * @param value String with expected row/column number
 * @return Row/column number or ANY_NUMBER for '-' or INVALID_NUMBER for invalid value
 */
int convertToRowColumnNumber(char *value) {
    // Special state - can be any row/column number
    if (strcmp(value, "-") == 0) {
        return ANY_NUMBER;
    }

    int result;
    if ((result = strtol(value, NULL, 10)) != 0) {
        return result;
    } else {
        return INVALID_NUMBER;
    }
}