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
 * @def streq(first, second) Check if first equals second
 */
#define streq(first, second) strcmp(first, second) == 0

/**
 * @typedef Row Individual row for processing
 * @field data Row content
 * @field size Row size (number of contained chars)
 * @field number Row number (from 1)
 * @field deleted Is the row mark as deleted?
 */
typedef struct row {
    char data[MAX_ROW_SIZE];
    int size;
    int number;
    bool deleted;
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

// Output functions
void writeProcessedRow(const Row *row);
void writeNewRow(int numberOfColumns, char delimiter);
void writeErrorMessage(const char *message);
// Main control and processing
char unifyRowDelimiters(Row *row, const char **delimiters);
ErrorInfo verifyRow(const Row *row, char delimiter);
ErrorInfo applyTableEditingFunctions(Row *row, const InputArguments *args/*, char delimiter, int numberOfColumns*/);
// Help functions
bool isDelimiter(char c, const char **delimiters);
bool checkCellsSize(const Row *row, char delimiter);
int countColumns(Row *row, char delimiter);
int toRowColNum(char *value);

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
    char **delimiters;
    if (argc >= 3) {
        if (streq(args.data[args.skipped], "-d")) {
            delimiters = &args.data[args.skipped + 1];
            args.skipped += 2;
        }
    } else {
        const char *DELIMITER = DEFAULT_DELIMITER;
        delimiters = (char **) &DELIMITER;
    }

    /* ROW PARSING */
    ErrorInfo err;
    Row row = {.number = 0};
    /*int numberOfColumns;*/
    while (fgets(row.data, MAX_ROW_SIZE, stdin) != NULL) {
        row.size = strlen(row.data);
        row.number++;
        row.deleted = false;

        // Delimiter processing
        char delimiter = unifyRowDelimiters(&row, (const char **) delimiters);

        // Validation
        if ((err = verifyRow(&row, delimiter)).error == true) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        // Data processing
        /*if(row.number == 1) {
            numberOfColumns = countColumns(&row, delimiter);
        }*/

        if ((err = applyTableEditingFunctions(&row, &args/*, delimiter, numberOfColumns*/)).error == true) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        if (row.deleted == false) {
            writeProcessedRow(&row);
        }
    }

    return EXIT_SUCCESS;
}

/**
 * Writes already processed row to standard output
 * @param row Processed row
 */
void writeProcessedRow(const Row *row) {
    for (int i = 0; i < row->size; i++) {
        putchar(row->data[i]);
    }
}

/**
 * Writes new row to standard output
 * @param numberOfColumns Number of columns of the new row
 * @param delimiter Column delimiter
 */
void writeNewRow(int numberOfColumns, char delimiter) {
    for (int i = 0; i < numberOfColumns - 1; i++) {
        putchar(delimiter);
    }
    putchar('\n');
}

/**
 * Writes error message to standard error output
 * @param message Error message
 */
void writeErrorMessage(const char *message) {
    fprintf(stderr, "sheet: %s", message);
}

/**
 * Unifies delimiters in provided row - all will be replaced with the first one
 * @param row Edited row
 * @param delimiters Used delimiters
 * @return Result delimiter
 */
char unifyRowDelimiters(Row *row, const char **delimiters) {
    char mainDelimiter = (*delimiters)[0];

    for (int i = 0; i < row->size; i++) {
        if (isDelimiter(row->data[i], delimiters) && row->data[i] != mainDelimiter) {
            row->data[i] = mainDelimiter;
        }
    }

    return mainDelimiter;
}

/**
 * Verifies input conditions for row
 * @param row Row to be checked
 * @param delimiter Column delimiter
 * @return Is the row valid?
 */
ErrorInfo verifyRow(const Row *row, char delimiter) {
    ErrorInfo errorInfo = {false};

    // Check max row size
    if (row->data[MAX_ROW_SIZE - 1] != '\0' && row->data[MAX_ROW_SIZE - 1] != '\n') {
        errorInfo.error = true;
        errorInfo.message = "Byla prekrocena maximalni velikost radku.";

        return errorInfo;
    }

    // Check cell size
    if (checkCellsSize(row, delimiter) == false) {
        errorInfo.error = true;
        errorInfo.message = "Byla prekrocena maximalni velikost bunky.";

        return errorInfo;
    }

    return errorInfo;
}

/**
 * Processes provided row by other parameters
 * @param row Input (raw) row
 * @param args Program's input arguments
 * @param delimiters Column delimiter
 * @param numberOfColumns Number of column in each row
 * @return Error information
 */
ErrorInfo applyTableEditingFunctions(Row *row, const InputArguments *args/*, char delimiter, int numberOfColumns*/) {
    ErrorInfo errorInfo = {false};
    char *functions[2] = {"drow", "drows"};
    int funcArgs[2] = {1, 2};

    // Apply table editing functions
    int numbers[2];
    for (int i = args->skipped; i < (args->size - 1); i++) {
        // Prepare arguments for functions
        for (int j = 0; j < (int)(sizeof(functions) / sizeof(char**)); j++) {
            if (streq(args->data[i], functions[j])) {
                for (int k = 0; k < funcArgs[j]; k++) {
                    int index = i + k + 1; // Index of argument in InputArguments
                    if (index > (args->size - 1) || (numbers[k] = toRowColNum(args->data[index])) == INVALID_NUMBER) {
                        errorInfo.error = true;
                        errorInfo.message = "Chybné číslo řádku/sloupce, povolena jsou celá čísla od 1.";
                    }
                }
            }
        }

        if (streq(args->data[i], "drow")) {
            if (row->number == numbers[0]) {
                row->deleted = true;

                return errorInfo; // Doesn't make sense to continue, when the row would be deleted
            }
        } else if (streq(args->data[i], "drows")) {
            if (row->number >= numbers[0] && row->number <= numbers[1]) {
                row->deleted = true;

                return errorInfo; // Doesn't make sense to continue, when the row would be deleted
            }
        }
    }

    return errorInfo;
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
 * Checks cells' size
 * @param row Row to check cells in
 * @param delimiter Cell delimiter
 * @return Does the row contain only valid-sized cells?
 */
bool checkCellsSize(const Row *row, char delimiter) {
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
int toRowColNum(char *value) {
    // Special state - can be any row/column number
    if (streq(value, "-")) {
        return ANY_NUMBER;
    }

    int result;
    if ((result = strtol(value, NULL, 10)) != 0) {
        return result;
    } else {
        return INVALID_NUMBER;
    }
}