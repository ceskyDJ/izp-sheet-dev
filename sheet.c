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
 * @def LOWER_CASE Flag for lower case style
 */
#define LOWER_CASE true
/**
 * @def UPPER_CASE Flag for upper case style
 */
#define UPPER_CASE false

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
/**
 * @typedef Program defined function
 * @field name Name of the function
 * @field params Parameters' values required by function
 * @field strParams String parameters' values required by function
 */
typedef struct function {
    char name[8];
    int params[4];
    char strParams[4][MAX_CELL_SIZE];
} Function;

// Input/Output functions
bool loadRow(Row *row);
void writeProcessedRow(const Row *row);
void writeNewRow(char delimiter, int numberOfColumns);
void writeErrorMessage(const char *message);
// Main control and processing
char unifyRowDelimiters(Row *row, const char **delimiters);
ErrorInfo verifyRow(const Row *row, char delimiter);
ErrorInfo applyTableEditingFunctions(Row *row, const InputArguments *args, char delimiter, int *numberOfColumns);
ErrorInfo applyDataProcessingFunctions(Row *row, const InputArguments *args, char delimiter, int numberOfColumns);
void applyAppendRowFunctions(InputArguments *args, char delimiter, int numberOfColumns);
// Table editing functions
ErrorInfo drows(int from, int to, Row *row);
ErrorInfo icol(int column, Row *row, char delimiter, int *numberOfColumns);
ErrorInfo acol(Row *row, char delimiter, int *numberOfColumns);
ErrorInfo dcols(int from, int to, Row *row, char delimiter);
// Data processing functions
ErrorInfo changeColumnCase(bool newCase, int column, Row *row, char delimiter, int numberOfColumns);
// Help functions
bool isDelimiter(char c, const char **delimiters);
bool checkCellsSize(const Row *row, char delimiter);
int countColumns(Row *row, char delimiter);
ErrorInfo getFunctionFromArgs(Function *function, const InputArguments *args, int *position);
int toRowColNum(char *value, bool specialAllowed);
ErrorInfo getColumnValue(char *value, const Row *row, int columnNumber, char delimiter, int numberOfColumns);
ErrorInfo setColumnValue(const char *value, Row *row, int columnNumber, char delimiter, int numberOfColumns);

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

        if ((err = applyDataProcessingFunctions(&row, &args, delimiter, numberOfColumns)).error == true) {
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
    // Try to load new data for the new row; if unsuccessful return false
    memset(row->data, '\0', row->size);
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

    Function function;
    for (int i = args->skipped; i < args->size; i++) {
        if ((errorInfo = getFunctionFromArgs(&function, args, &i)).error == true) {
            return errorInfo;
        }

        // Column-operated functions are skipped if the row is set as deleted - it doesn't make sense to apply them
        if (streq(function.name, "irow")) {
            if (row->number == function.params[0]) {
                writeNewRow(delimiter, *numberOfColumns);
            }
        } else if (streq(function.name, "drow") || streq(function.name, "drows")) {
            if (streq(function.name, "drow")) {
                function.params[1] = function.params[0];
            }

            if ((errorInfo = drows(function.params[0], function.params[1], row)).error == true) {
                return errorInfo;
            }
        } else if (streq(function.name, "icol")) {
            if ((errorInfo = icol(function.params[0], row, delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
        } else if (row->deleted == false && streq(function.name, "acol")) {
            if ((errorInfo = acol(row, delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
        } else if (row->deleted == false && (streq(function.name, "dcol") || streq(function.name, "dcols"))) {
            if (streq(function.name, "dcol")) {
                function.params[1] = function.params[0];
            }

            if ((errorInfo = dcols(function.params[0], function.params[1], row, delimiter)).error == true) {
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
 * Applies data processing functions on provided row
 * @param row Input row
 * @param args Input program arguments
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 * @return Error information
 */
ErrorInfo applyDataProcessingFunctions(Row *row, const InputArguments *args, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    Function function;
    for (int i = args->skipped; i < args->size; i++) {
        if ((errorInfo = getFunctionFromArgs(&function, args, &i)).error == true) {
            return errorInfo;
        }

        if (streq(function.name, "cset")) {
            if ((errorInfo = setColumnValue(function.strParams[1], row, function.params[0], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
        } else if (streq(function.name, "tolower")) {
            if ((errorInfo = changeColumnCase(LOWER_CASE, function.params[0], row, delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
        } else if (streq(function.name, "toupper")) {
            if ((errorInfo = changeColumnCase(UPPER_CASE, function.params[0], row, delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
        } else if(streq(function.name, "round")) {
            char value[MAX_CELL_SIZE];
            if ((errorInfo = getColumnValue(value, row, function.params[0], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }

            double number = strtod(value, NULL);
            memset(value, '\0', strlen(value));
            sprintf(value, "%.f", number);

            // Should be OK (this column has already been used)
            setColumnValue(value, row, function.params[0], delimiter, numberOfColumns);
        } else if (streq(function.name, "int")) {
            char value[MAX_CELL_SIZE];
            if ((errorInfo = getColumnValue(value, row, function.params[0], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }

            bool decimal = true; // Is decimal part still iterating?
            for (int j = 0; j < (int) strlen(value); j++) {
                if (value[j] == '.') {
                    decimal = false;
                }

                if (decimal == false) {
                    value[j] = '\0';
                }
            }
            // Should be OK (this column has already been used)
            setColumnValue(value, row, function.params[0], delimiter, numberOfColumns);
        } else if (streq(function.name, "copy")) {
            char value[MAX_CELL_SIZE];
            if ((errorInfo = getColumnValue(value, row, function.params[0], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }

            if ((errorInfo = setColumnValue(value, row, function.params[1], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
        } else if (streq(function.name, "swap")) {
            char firstValue[MAX_CELL_SIZE];
            char secondValue[MAX_CELL_SIZE];
            if ((errorInfo = getColumnValue(firstValue, row, function.params[0], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
            if ((errorInfo = getColumnValue(secondValue, row, function.params[1], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }

            // Column numbers should be OK, so errors aren't expected
            setColumnValue(firstValue, row, function.params[1], delimiter, numberOfColumns);
            setColumnValue(secondValue, row, function.params[0], delimiter, numberOfColumns);
        } else if (streq(function.name, "move")) {
            if (function.params[1] > function.params[0]) {
                errorInfo.error = true;
                errorInfo.message = "Funkce move vyzaduje prvni cislo v parametech vetsi nez druhe.";

                return errorInfo;
            }

            char moving[2 * MAX_CELL_SIZE];
            char second[MAX_CELL_SIZE];
            if ((errorInfo = getColumnValue(moving, row, function.params[0], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }
            if ((errorInfo = getColumnValue(second, row, function.params[1], delimiter, numberOfColumns)).error == true) {
                return errorInfo;
            }

            // Delete column for move
            // Should be OK -> from this column has been successfully extracted before
            dcols(function.params[0], function.params[0], row, delimiter);

            // Add the moving column before second selected column
            moving[strlen(moving)] = delimiter;
            strcat(moving, second);
            setColumnValue(moving, row, function.params[1], delimiter, numberOfColumns);
        }
    }

    return errorInfo;
}

/**
 * Marks rows from selected interval as deleted
 * @param from First selected row
 * @param to Last selected row
 * @param row Actual row
 * @return Error information
 */
ErrorInfo drows(int from, int to, Row *row) {
    ErrorInfo errorInfo = {false};

    if (from > to) {
        errorInfo.error = true;
        errorInfo.message = "Byl zadan chybny interval - prvni cislo musi byt mensi nez druhe.";

        return errorInfo;
    }

    if (row->number >= from && row->number <= to) {
        row->deleted = true;
    }

    return errorInfo;
}

/**
 * Adds column before selected column
 * @param column Selected column
 * @param row Row to change
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 * @return Error information
 */
ErrorInfo icol(int column, Row *row, char delimiter, int *numberOfColumns) {
    ErrorInfo errorInfo;

    char columnValue[MAX_CELL_SIZE];
    if ((errorInfo = getColumnValue(columnValue, row, column, delimiter, *numberOfColumns)).error == true) {
        return errorInfo;
    }

    char newColumnValue[MAX_CELL_SIZE];
    memset(newColumnValue, '\0', sizeof(newColumnValue));
    newColumnValue[0] = delimiter;
    strcat(newColumnValue, columnValue);

    // Row should exists - last operation on it ended with success
    setColumnValue(newColumnValue, row, column, delimiter, *numberOfColumns);

    // Do it only once
    if (row->number == 1) {
        (*numberOfColumns)++;
    }

    return errorInfo;
}

/**
 * Append column to the end of the row
 * @param row Row to change
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 * @return Error info
 */
ErrorInfo acol(Row *row, char delimiter, int *numberOfColumns) {
    ErrorInfo errorInfo = {false};

    if ((row->size + 1) > MAX_ROW_SIZE) {
        errorInfo.error = true;
        errorInfo.message = "Provedenim prikazu acol byla prekrocena maximalni velikost radku.";

        return errorInfo;
    }

    row->data[row->size - 1] = delimiter;
    row->data[row->size] = '\n';
    row->size++;

    // Do it only once
    if (row->number == 1) {
        (*numberOfColumns)++;
    }

    return errorInfo;
}

/**
 * Deletes row's columns from selected range
 * @param from First selected column number
 * @param to Last selected column number
 * @param row Operated row
 * @param delimiter Column delimiter
 * @return Error information
 */
ErrorInfo dcols(int from, int to, Row *row, char delimiter) {
    ErrorInfo errorInfo = {false};

    if (from > to) {
        errorInfo.error = true;
        errorInfo.message = "Byl zadan chybny interval - prvni cislo musi byt mensi nez druhe.";

        return errorInfo;
    }

    // Backup for future recovery + clean row
    char rowBackup[MAX_ROW_SIZE];
    memmove(rowBackup, row->data, MAX_ROW_SIZE);
    memset(row->data, '\0', MAX_ROW_SIZE);

    // Recovery only data of non-deleted columns
    int counter = 1; // Actual number of column, column numbering starts from 1
    int dataIndex = 0;
    for (int j = 0; j < row->size; j++) {
        if (!(counter >= from && counter <= to)) {
            row->data[dataIndex] = rowBackup[j];
            dataIndex++;
        } else if (j == (row->size - 1)) {
            // The last column is being removed, so end delimiter must be deleted
            dataIndex--;
            row->data[dataIndex] = '\0';
        }

        if(rowBackup[j] == delimiter) {
            counter++;
        }
    }

    // Recount row's size and ensure \n at the end of the row
    row->size = (int)strlen(row->data);
    if (row->data[row->size - 1] != '\n') {
        row->data[row->size] = '\n';
        row->size++;
    }

    return errorInfo;
}

/**
 * Changes column's case of selected column
 * @param newCase New case (LOWER_CASE or UPPER_CASE)
 * @param column Selected column
 * @param row Row contains the column
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 * @return Error information
 */
ErrorInfo changeColumnCase(bool newCase, int column, Row *row, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo;
    char value[MAX_CELL_SIZE];

    if ((errorInfo = getColumnValue(value, row, column, delimiter, numberOfColumns)).error == true) {
        return errorInfo;
    }

    int move;
    char start;
    if (newCase == LOWER_CASE) {
        start = 'A';
        move = ('a' - 'A');
    } else {
        start = 'a';
        move = -('a' - 'A');
    }

    for (int j = 0; j < (int)strlen(value); j++) {
        if (value[j] >= start && value[j] <= start + ('z' - 'a')) {
            value[j] = (char)(value[j] + move);
        }
    }
    // It should be OK (it has been read from this column yet)
    setColumnValue(value, row, column, delimiter, numberOfColumns);

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
 * Extract function from program input arguments
 * @param function Pointer for save found function
 * @param args Input program arguments
 * @param position Actual position in input program arguments
 * @return Error information
 */
ErrorInfo getFunctionFromArgs(Function *function, const InputArguments *args, int *position) {
    ErrorInfo errorInfo = {false};
    char *functions[16] = {
            "arow", "irow", "drow", "drows", "icol", "acol", "dcol", "dcols", "cset", "tolower", "toupper", "round",
            "int", "copy", "swap", "move"
    };
    int funcArgs[16] = {0, 1, 1, 2, 1, 0, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2};

    // Prepare arguments for functions
    for (int j = 0; j < (int)(sizeof(functions) / sizeof(char**)); j++) {
        if (streq(args->data[*position], functions[j])) {
            strcpy(function->name, functions[j]);

            for (int i = 0; i < funcArgs[j]; i++) {
                int index = *position + i + 1; // Index of argument in InputArguments
                if (index >= args->size || (function->params[i] = toRowColNum(args->data[index], false)) == INVALID_NUMBER) {
                    // There is an exception... (function that accepts string value as one of its params)
                    if (!(streq(function->name, "cset") && i == 1)) {
                        errorInfo.error = true;
                        errorInfo.message = "Chybne cislo radku/sloupce, povolena jsou cela cisla od 1.";

                        return errorInfo;
                    } else {
                        memset(function->strParams[i], '\0', sizeof(function->strParams[i]));
                        memmove(function->strParams[i], args->data[index], strlen(args->data[index]));
                    }
                }
            }

            // Move iterator of arguments array by function arguments
            *position += funcArgs[j];
            // Function was found, doesn't make sense to continue searching
            return errorInfo;
        } else {
            memset(function->name, '\0', sizeof(function->name));
        }
    }

    // Function not found --> function name must be bad
    errorInfo.error = true;
    errorInfo.message = "Neplatny nazev funkce.";

    return errorInfo;
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
 * @param value Pointer for return value (value is without '\n')
 * @param row Row contains the column
 * @param columnNumber Number of selected column
 * @param delimiter Column delimiter
 * @return Error information
 */
ErrorInfo getColumnValue(char *value, const Row *row, int columnNumber, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    if (columnNumber > numberOfColumns) {
        errorInfo.error = true;
        errorInfo.message = "Sloupec s pozadovanym cislem neexistuje.";
    }

    int counter = 1;
    int j = 0;
    memset(value, '\0', MAX_CELL_SIZE);
    for (int i = 0; i < row->size; i++) {
        // \n is "delimiter" for the last column
        if (row->data[i] == delimiter || row->data[i] == '\n') {
            counter++;
        } else if (counter == columnNumber) {
            value[j] = row->data[i];
            j++;
        }
    }

    return errorInfo;
}

/**
 * Sets value of selected column
 * @param value New column's value
 * @param row Row contains the column
 * @param columnNumber Column's number
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 * @return Error information
 */
ErrorInfo setColumnValue(const char *value, Row *row, int columnNumber, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    if (columnNumber > numberOfColumns) {
        errorInfo.error = true;
        errorInfo.message = "Sloupec s pozadovanym cislem neexistuje.";
    }

    // Backup row data
    char rowBackup[MAX_ROW_SIZE];
    for (int i = 0; i < MAX_ROW_SIZE; i++) {
        rowBackup[i] = row->data[i];
    }

    int counter = 1;
    int backupIndex, dataIndex;
    for (dataIndex = backupIndex = 0; dataIndex < MAX_ROW_SIZE;) {
        if (counter == columnNumber) {
            // Replace row data with new value's content
            int i = 0;
            while (value[i] != '\0') {
                row->data[i + dataIndex] = value[i];
                i++;
            }
            // Move index of row data to a new position (new value can has diff length)
            dataIndex = backupIndex + i;

            // Delimiter is after the end of normal column, \n is at the end of the last column
            while (rowBackup[backupIndex] != delimiter && rowBackup[backupIndex] != '\n') {
                backupIndex++;
            }
        }

        // Loading unchanged data from backup
        row->data[dataIndex] = rowBackup[backupIndex];

        // Mark next column
        if (rowBackup[backupIndex] == delimiter || rowBackup[backupIndex] == '\n') {
            counter++;
        }

        dataIndex++;
        backupIndex++;
    }

    // Function removes \n in the last column, so put it back
    if (columnNumber == numberOfColumns) {
        row->data[dataIndex] = '\n';
    }

    // Count new size after changes
    row->size = (int)strlen(row->data);

    return errorInfo;
}