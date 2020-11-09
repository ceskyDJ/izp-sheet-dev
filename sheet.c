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
#define DEFAULT_DELIMITER " "
/**
 * @def LAST_ROW_NUMBER Replacement for last row number
 */
#define LAST_ROW_NUMBER 0
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

// Input/Output functions
bool loadRow(Row *row);
void writeProcessedRow(const Row *row);
void writeNewRow(char delimiter, int numberOfColumns);
void writeErrorMessage(const char *message);
// Main control and processing
char unifyRowDelimiters(Row *row, const char **delimiters);
ErrorInfo verifyRow(const Row *row, char delimiter);
ErrorInfo applyTableEditingFunctions(Row *row, const InputArguments *args, char delimiter, int *numberOfColumns);
void applyAppendRowFunctions(InputArguments *args, char delimiter, int numberOfColumns);
// Help functions
bool isDelimiter(char c, const char **delimiters);
bool checkCellsSize(const Row *row, char delimiter);
int countColumns(Row *row, char delimiter);
int toRowColNum(char *value, bool specialAllowed);
ErrorInfo getColumnValue(char *value, Row *row, int columnNumber, char delimiter);
ErrorInfo setColumnValue(char *value, Row *row, int columnNumber, char delimiter);
ErrorInfo getColumnOffset(int *offset, Row *row, int columnNumber, char delimiter);

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
    Row row = {.size = 0, .number = 0};
    char delimiter;
    int numberOfColumns;
    while (loadRow(&row) == true) {
        // Delimiter processing
        delimiter = unifyRowDelimiters(&row, (const char **) delimiters);

        // Validation
        if ((err = verifyRow(&row, delimiter)).error == true) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        // Data processing
        if(row.number == 1) {
            numberOfColumns = countColumns(&row, delimiter);
        }

        if ((err = applyTableEditingFunctions(&row, &args, delimiter, &numberOfColumns)).error == true) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        if (row.deleted == false) {
            writeProcessedRow(&row);
        }
    }

    // New content
    applyAppendRowFunctions(&args, delimiter, numberOfColumns);

    return EXIT_SUCCESS;
}

/**
 * Loads a new row from standard input
 * @param row Pointer to Row; it's required to set number and size fields
 * @return Was it successful? If false, no other input is available.
 */
bool loadRow(Row *row) {
    // Delete old data; Size of old row instance is used => first row won't be iterated
    for (int i = 0; i < row->size; i++) {
        row->data[i] = '\0';
    }

    // Try to load new data for the new row; if unsuccessful return false
    if (fgets(row->data, MAX_ROW_SIZE, stdin) == NULL) {
        return false;
    }

    // Update structure with new data
    row->size = (int)strlen(row->data);
    row->number++;
    row->deleted = false;

    return true;
}

/**
 * Writes already processed row to standard output
 * @param row Processed row
 */
void writeProcessedRow(const Row *row) {
    printf("%s", row->data);
}

/**
 * Writes new row to standard output
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns of the new row
 */
void writeNewRow(char delimiter, int numberOfColumns) {
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
 * Applies table editing functions on provided row
 * @param row Input (raw) row
 * @param args Program's input arguments
 * @param delimiters Column delimiter
 * @param numberOfColumns Number of column in each row
 * @return Error information
 */
ErrorInfo applyTableEditingFunctions(Row *row, const InputArguments *args, char delimiter, int *numberOfColumns) {
    ErrorInfo errorInfo = {false};
    char *functions[4] = {"irow", "drow", "drows", "acol"};
    int funcArgs[4] = {1, 1, 2, 0};

    // Apply table editing functions
    int numbers[2];
    for (int i = args->skipped; i < args->size; i++) {
        // Prepare arguments for functions
        for (int j = 0; j < (int)(sizeof(functions) / sizeof(char**)); j++) {
            if (streq(args->data[i], functions[j])) {
                for (int k = 0; k < funcArgs[j]; k++) {
                    int index = i + k + 1; // Index of argument in InputArguments
                    if (index >= args->size || (numbers[k] = toRowColNum(args->data[index], false)) == INVALID_NUMBER) {
                        errorInfo.error = true;
                        errorInfo.message = "Chybne cislo radku/sloupce, povolena jsou cela cisla od 1.";
                    }
                }
            }
        }

        // Column-operated functions are skipped if the row is set as deleted - it doesn't make sense to apply them
        if (streq(args->data[i], "irow")) {
            if (row->number == numbers[0]) {
                writeNewRow(delimiter, *numberOfColumns);
            }
        } else if (streq(args->data[i], "drow")) {
            if (row->number == numbers[0]) {
                row->deleted = true;
            }
        } else if (streq(args->data[i], "drows")) {
            if (row->number >= numbers[0] && row->number <= numbers[1]) {
                row->deleted = true;
            }
        } else if (row->deleted == false && streq(args->data[i], "acol")) {
            if ((row->size + 1) < MAX_ROW_SIZE) {
                row->data[row->size - 1] = delimiter;
                row->data[row->size] = '\n';
                row->size++;
            } else {
                errorInfo.error = true;
                errorInfo.message = "Provedenim prikazu acol byla prekrocena maximalni velikost radku.";

                return errorInfo;
            }
        }
    }

    return errorInfo;
}

/**
 * Applies append row functions to output
 * @param args Program input arguments
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns
 */
void applyAppendRowFunctions(InputArguments *args, char delimiter, int numberOfColumns) {
    for (int i = args->skipped; i < args->size; i++) {
        if (streq(args->data[i], "arow")) {
            writeNewRow(delimiter, numberOfColumns);
        }
    }
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
 * @param specialAllowed Is special state allowed? ('-' for last row)
 * @return Row/column number or LAST_ROW_NUMBER for '-' or INVALID_NUMBER for invalid value
 */
int toRowColNum(char *value, bool specialAllowed) {
    // Special state - the last row number
    if (streq(value, "-") && specialAllowed == true) {
        return LAST_ROW_NUMBER;
    }

    int result;
    if ((result = (int)strtol(value, NULL, 10)) >= 1) {
        return result;
    } else {
        return INVALID_NUMBER;
    }
}

/**
 * Returns value of the selected column
 * @param value Pointer for return value
 * @param row Row contains the column
 * @param columnNumber Number of selected column
 * @param delimiter Column delimiter
 * @return Error information
 */
ErrorInfo getColumnValue(char *value, Row *row, int columnNumber, char delimiter) {
    // Clean old value
    for (int i = 0; i < (int)sizeof(value); i++) {
        value[i] = '\0';
    }

    // Selected column's offset in row (start of the column)
    ErrorInfo errorInfo;
    int offset;
    if ((errorInfo = getColumnOffset(&offset, row, columnNumber, delimiter)).error == true) {
        return errorInfo;
    }

    char c;
    int i = 0;
    while ((c = row->data[offset + i]) != delimiter) {
        value[i] = c;

        i++;
    }

    return errorInfo;
}

/**
 * Returns selected column's offset in his row
 * @param offset Pointer to save offset from the row beginning
 * @param row Row contains the column
 * @param columnNumber Selected column's number
 * @param delimiter Column delimiter
 * @return Error information
 */
ErrorInfo getColumnOffset(int *offset, Row *row, int columnNumber, char delimiter) {
    ErrorInfo errorInfo = {false};

    int counter = 0;
    for (int i = 0; i < row->size; i++) {
        if (row->data[i] == delimiter || i == 0) {
            counter++;

            if (counter == columnNumber) {
                // First column doesn't have delimiter before (but other have --> + 1 to skip it)
                *offset = (i != 0 ? i + 1 : 0);

                return errorInfo;
            }
        }
    }

    errorInfo.error = true;
    errorInfo.message = "Sloupec s pozadovanym cislem nebyl nalezen.";

    return errorInfo;
}

/**
 * Sets value of selected column
 * @param value New value
 * @param row Row contains the column
 * @param columnNumber Selected row's number
 * @param delimiter Column delimiter
 * @return Error information
 */
ErrorInfo setColumnValue(char *value, Row *row, int columnNumber, char delimiter) {
    // End offset; function returns index of start of the column, here end of the column is needed
    ErrorInfo errorInfo;
    int endOffset;
    if ((errorInfo = getColumnOffset(&endOffset, row, columnNumber + 1, delimiter)).error == true) {
        return errorInfo;
    }
    endOffset--;

    // Start offset; shouldn't cause error because column with bigger value was found before
    int startOffset;
    getColumnOffset(&startOffset, row, columnNumber, delimiter);

    // Create backup of string that will be moved due to change
    char rowBackup[MAX_ROW_SIZE];
    for (int i = 0; i < MAX_ROW_SIZE; i++) {
        rowBackup[i] = row->data[i];
    }

    // Replace data in row with new column value
    int endOfEdit = startOffset + (int)strlen(value);
    for (int i = 0; i < endOfEdit; i++) {
        row->data[startOffset + i] = value[i];
    }

    // Revert other data
    int i = 0;
    while (rowBackup[endOffset + i] != '\0') {
        row->data[endOfEdit + i] = rowBackup[endOffset + i];
        i++;
    }

    // Update row size
    if (endOfEdit + i <= MAX_ROW_SIZE) {
        row->size = endOfEdit + i;

        return errorInfo;
    } else {
        errorInfo.error = true;
        errorInfo.message = "Pri uprave sloupce doslo k prekroceni maximalni velikosti radku.";

        return errorInfo;
    }
}