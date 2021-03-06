/*
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
 * @def MAX_FUNCTIONS Maximum functions in program input arguments
 */
#define MAX_FUNCTIONS 100
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
 * @def NO_SELECTION No selection function set
 */
#define NO_SELECTION -1
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
 * @field last Is this row the last?
 */
typedef struct row {
    char data[MAX_ROW_SIZE];
    int size;
    int number;
    bool deleted;
    bool last;
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
 * @typedef Program defined function for row selection
 * @field name Name of the function
 * @field params Parameters' values required by function
 * @field strParams String parameters' values required by function
 */
typedef struct selectFunction {
    char name[11];
    int params[2];
    char strParams[2][MAX_CELL_SIZE];
} SelectFunction;
/**
 * @typedef Program defined function
 * @field name Name of the function
 * @field params Parameters' values required by function
 * @field strParams String parameters' values required by function
 * @field from The first selected row (NO_SELECTION if selection not active)
 * @field to The last selected row (LAST_ROW_NUMBER if the end of file)
 */
typedef struct function {
    char name[8];
    int params[4];
    char strParams[4][MAX_CELL_SIZE];
    SelectFunction selectFunction;
} Function;

// Input/Output functions
bool loadRow(Row *row, char *preloadedData);
void writeProcessedRow(const Row *row);
void writeNewRow(char delimiter, int numberOfColumns);
void writeErrorMessage(const char *message);
// Main control and processing
char unifyRowDelimiters(Row *row, const char **delimiters);
ErrorInfo verifyRow(const Row *row, char delimiter);
ErrorInfo parseInputArguments(Function *functions, const InputArguments *args);
ErrorInfo applyTableEditingFunction(Row *row, Function function, char delimiter, int *numberOfColumns);
ErrorInfo applyDataProcessingFunction(Row *row, Function function, char delimiter, int numberOfColumns);
void applyAppendRowFunctions(InputArguments *args, char delimiter, int numberOfColumns);
ErrorInfo acceptsSelection(bool *result, Row *row, SelectFunction *selection, char delimiter, int numberOfColumns);
// Table editing functions
ErrorInfo drows(int from, int to, Row *row);
ErrorInfo icol(int column, Row *row, char delimiter, int *numberOfColumns);
ErrorInfo acol(Row *row, char delimiter, int *numberOfColumns);
ErrorInfo dcols(int from, int to, Row *row, char delimiter);
// Data processing functions
ErrorInfo cset(int column, char *value, Row *row, char delimiter, int numberOfColumns);
void changeColumnCase(bool newCase, int column, Row *row, char delimiter, int numberOfColumns);
ErrorInfo roundColumnValue(int column, Row *row, char delimiter, int numberOfColumns);
ErrorInfo removeColumnDecimalPart(int column, Row *row, char delimiter, int numberOfColumns);
void copy(int from, int to, Row *row, char delimiter, int numberOfColumns);
void swap(int first, int second, Row *row, char delimiter, int numberOfColumns);
void move(int column, int beforeColumn, Row *row, char delimiter, int numberOfColumns);
// Help functions
bool isDelimiter(char c, const char **delimiters);
bool checkCellsSize(const Row *row, char delimiter);
int countColumns(Row *row, char delimiter);
ErrorInfo getFunctionFromArgs(Function *function, const InputArguments *args, int *position);
int toRowColNum(char *value, bool specialAllowed);
void getColumnValue(char *value, const Row *row, int columnNumber, char delimiter, int numberOfColumns);
void setColumnValue(const char *value, Row *row, int columnNumber, char delimiter, int numberOfColumns);
bool isValidNumber(char *number);

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
    // EDIT: Forgotten check of -d argument existence
    if (argc >= 3 && streq(args.data[args.skipped], "-d")) {
        delimiters = &args.data[args.skipped + 1];
        args.skipped += 2;
    } else {
        const char *DELIMITER = DEFAULT_DELIMITER;
        delimiters = (char **) &DELIMITER;
    }

    /* ROW PARSING */
    ErrorInfo err;
    Row row = {.size = 0, .number = 0, .last = false};
    char delimiter;
    int numberOfColumns;
    int inputNumOfCols; // Number of columns in input table
    char preloadedData[MAX_ROW_SIZE];
    while (loadRow(&row, preloadedData) == true) {
        // Delimiter processing
        delimiter = unifyRowDelimiters(&row, (const char **) delimiters);

        // Validation
        if ((err = verifyRow(&row, delimiter)).error == true) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        // Arguments processing
        Function functions[MAX_FUNCTIONS];
        if ((err = parseInputArguments(functions, &args)).error == true) {
            writeErrorMessage(err.message);

            return EXIT_FAILURE;
        }

        // Data processing
        if(row.number == 1) {
            inputNumOfCols = numberOfColumns = countColumns(&row, delimiter);
        } else if (countColumns(&row, delimiter) != inputNumOfCols) {
            writeErrorMessage("Kazdy radek musi mit stejny pocet sloupcu.");

            return EXIT_FAILURE;
        }

        int i = 0;
        bool tableChanged = false;
        bool dataChanged = false;
        while (functions[i].name[0] != '\0') {
            SelectFunction selection = functions[i].selectFunction;

            // Table editing functions
            if ((err = applyTableEditingFunction(&row, functions[i], delimiter, &numberOfColumns)).error == true) {
                writeErrorMessage(err.message);

                return EXIT_FAILURE;
            } else if (err.message == NULL) {
                // Selections on table editing functions are forbidden
                if (selection.name[0] != '\0') {
                    writeErrorMessage("Funkce pro vyber radku neni mozne pouzit na funkce menici tabulku.");

                    return EXIT_FAILURE;
                }

                // Does not make sense to continue with processing if the row was marked as deleted
                if (row.deleted == true) {
                    break;
                }

                tableChanged = true;
                i++;
                continue;
            }

            if (dataChanged == true) {
                writeErrorMessage("Je mozne pouzit pouze jednu funkci pro zpracovani dat.");

                return EXIT_FAILURE;
            }

            // Row selection (don't modify some rows with actual function)
            bool accepts;
            if ((err = acceptsSelection(&accepts, &row, &selection, delimiter, numberOfColumns)).error == true) {
                writeErrorMessage(err.message);

                return EXIT_FAILURE;
            }

            // Skip applying the function on this row
            if (accepts == false) {
                i++;
                continue;
            }

            // Data processing functions
            if ((err = applyDataProcessingFunction(&row, functions[i], delimiter, numberOfColumns)).error == true) {
                writeErrorMessage(err.message);

                return EXIT_FAILURE;
            } else if (err.message == NULL) {
                dataChanged = true;
            }

            i++;
        }

        if (tableChanged && dataChanged) {
            writeErrorMessage("Je mozne pouzit pouze funkce pro zmenu tabulky nebo pouze pro zpracovani dat.");

            return EXIT_FAILURE;
        }

        // Write output
        if (row.deleted == false) {
            writeProcessedRow(&row);
        }
    }

    // Empty input
    if (row.number == 0) {
        writeErrorMessage("Prazdny vstup neni povolen.");

        return EXIT_FAILURE;
    }

    // New content
    applyAppendRowFunctions(&args, delimiter, numberOfColumns);

    return EXIT_SUCCESS;
}

/***********************************************************************************************Input/Output functions*/
/**
 * Loads a new row from standard input
 * @param row Pointer to Row; it's required to set number and size fields
 * @param preloadedData Data preloaded in previous loadRow() call
 * @return Was it successful? If false, no other input is available.
 */
bool loadRow(Row *row, char *preloadedData) {
    // Previous row was the last one
    if (row->last) {
        return false;
    }

    // First loading
    if (row->number == 0) {
        memset(preloadedData, '\0', row->size);

        // Try to load data for the first row; if unsuccessful return false
        if (fgets(preloadedData, MAX_ROW_SIZE, stdin) == NULL) {
            return false;
        }
    }

    // Load data of the current row
    memset(row->data, '\0', row->size);
    memmove(row->data, preloadedData, strlen(preloadedData));

    // Update structure with new data
    row->size = (int)strlen(row->data);
    row->number++;
    row->deleted = false;

    // Try to preload new data for next row; if unsuccessful set row as the last
    if (fgets(preloadedData, MAX_ROW_SIZE, stdin) == NULL) {
        row->last = true;
    } else {
        row->last = false;
    }

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

/******************************************************************************************Main control and processing*/
/**
 * Unifies delimiters in provided row - all will be replaced with the first one
 * @param row Edited row
 * @param delimiters Used delimiters
 * @return Result delimiter
 */
char unifyRowDelimiters(Row *row, const char **delimiters) {
    char mainDelimiter = (*delimiters)[0];

    for (int i = 0; i < row->size; i++) {
        if (isDelimiter(row->data[i], delimiters) && (row->data[i] != mainDelimiter)) {
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
    if (row->size > MAX_ROW_SIZE) {
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
 * Parses arguments into functions (instances of Function data type)
 * @param functions Pointer to array fro saving parsed output
 * @param args Program input arguments
 * @return Error information
 */
ErrorInfo parseInputArguments(Function *functions, const InputArguments *args) {
    ErrorInfo errorInfo = {false};

    int funcIndex = 0;
    for (int i = args->skipped; i < args->size; i++) {
        Function function;
        if ((errorInfo = getFunctionFromArgs(&function, args, &i)).error == true) {
            return errorInfo;
        }

        functions[funcIndex] = function;
        funcIndex++;
    }

    return errorInfo;
}

/**
 * Applies table editing function on provided row
 * @param row Input (raw) row
 * @param function Function to use
 * @param delimiters Column delimiter
 * @param numberOfColumns Number of column in each row
 * @return Error information
 */
ErrorInfo applyTableEditingFunction(Row *row, Function function, char delimiter, int *numberOfColumns) {
    ErrorInfo errorInfo = {false};

    // Column-operated functions are skipped if the row is set as deleted - it doesn't make sense to apply them
    if (streq(function.name, "irow")) {
        if (row->number == function.params[0]) {
            writeNewRow(delimiter, *numberOfColumns);
        }

        return errorInfo;
    } else if (streq(function.name, "drow") || streq(function.name, "drows")) {
        if (streq(function.name, "drow")) {
            function.params[1] = function.params[0];
        }

        return drows(function.params[0], function.params[1], row);
    } else if (streq(function.name, "icol")) {
        return icol(function.params[0], row, delimiter, numberOfColumns);
    } else if (streq(function.name, "acol")) {
        return acol(row, delimiter, numberOfColumns);
    } else if (streq(function.name, "dcol") || streq(function.name, "dcols")) {
        if (streq(function.name, "dcol")) {
            function.params[1] = function.params[0];
        }

        return dcols(function.params[0], function.params[1], row, delimiter);
    }

    errorInfo.message = "NO_FUNCTION_USED";
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
 * Applies data processing function on provided row
 * @param row Input row
 * @param function Function to use
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 * @return Error information
 */
ErrorInfo applyDataProcessingFunction(Row *row, Function function, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    if (streq(function.name, "cset")) {
        return cset(function.params[0], function.strParams[1], row, delimiter, numberOfColumns);
    } else if (streq(function.name, "tolower")) {
        changeColumnCase(LOWER_CASE, function.params[0], row, delimiter, numberOfColumns);

        return errorInfo;
    } else if (streq(function.name, "toupper")) {
        changeColumnCase(UPPER_CASE, function.params[0], row, delimiter, numberOfColumns);

        return errorInfo;
    } else if(streq(function.name, "round")) {
        return roundColumnValue(function.params[0], row, delimiter, numberOfColumns);
    } else if (streq(function.name, "int")) {
        return removeColumnDecimalPart(function.params[0], row, delimiter, numberOfColumns);
    } else if (streq(function.name, "copy")) {
        copy(function.params[0], function.params[1], row, delimiter, numberOfColumns);

        return errorInfo;
    } else if (streq(function.name, "swap")) {
        swap(function.params[0], function.params[1], row, delimiter, numberOfColumns);

        return errorInfo;
    } else if (streq(function.name, "move")) {
        move(function.params[0], function.params[1], row, delimiter, numberOfColumns);

        return errorInfo;
    }

    errorInfo.message = "NO_FUNCTION_USED";
    return errorInfo;
}

/**
 * Checks if the row accepts the selection
 * @param result Accepts this row provided selection?
 * @param row Row for check
 * @param selection Operated selection
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 * @return Error information
 */
ErrorInfo acceptsSelection(bool *result, Row *row, SelectFunction *selection, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    if (streq(selection->name, "rows")) {
        if ((selection->params[0] != NO_SELECTION) && (selection->params[1] != LAST_ROW_NUMBER)) {
            // Normal selection
            if ((row->number >= selection->params[0]) && (row->number <= selection->params[1])) {
                *result = true;
                return errorInfo;
            }
        } else if ((selection->params[0] == LAST_ROW_NUMBER) && (selection->params[1] == LAST_ROW_NUMBER)) {
            // Selection for the last file only
            if (row->last == true) {
                *result = true;
                return errorInfo;
            }
        } else if ((selection->params[0] != NO_SELECTION) && (selection->params[1] == LAST_ROW_NUMBER)) {
            // Selection from N to end of file
            if (row->number >= selection->params[0]) {
                *result = true;
                return errorInfo;
            }
        }

        *result = false;
        return errorInfo;
    } else if (streq(selection->name, "beginswith")) {
        char value[MAX_CELL_SIZE];
        getColumnValue(value, row, selection->params[0], delimiter, numberOfColumns);

        char *found = strstr(value, selection->strParams[1]);
        if (found != NULL && streq(found, value)) {
            *result = true;
            return errorInfo;
        }

        *result = false;
        return errorInfo;
    } else if (streq(selection->name, "contains")) {
        char value[MAX_CELL_SIZE];
        getColumnValue(value, row, selection->params[0], delimiter, numberOfColumns);

        if (strstr(value, selection->strParams[1]) != NULL) {
            *result = true;
            return errorInfo;
        }

        *result = false;
        return errorInfo;
    }

    // No selection used --> row can be changed
    *result = true;
    return errorInfo;
}

/**********************************************************************************************Table editing functions*/
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
    ErrorInfo errorInfo = {false};

    if ((row->size + 1) > MAX_ROW_SIZE) {
        errorInfo.error = true;
        errorInfo.message = "Provedenim prikazu icol byla prekrocena maximalni velikost radku.";

        return errorInfo;
    }

    char columnValue[MAX_CELL_SIZE];
    getColumnValue(columnValue, row, column, delimiter, *numberOfColumns);

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
        if (!((counter >= from) && (counter <= to))) {
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

/********************************************************************************************Data processing functions*/
/**
 * Sets selected column's value
 * @param column Selected column's number
 * @param value Value to set to the column
 * @param row Row contains the column
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 * @return Error information
 */
ErrorInfo cset(int column, char *value, Row *row, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    if ((int)strlen(value) > MAX_CELL_SIZE) {
        errorInfo.error = true;
        errorInfo.message = "Hodnota predana funkci cset prekracuje maximalni velikost bunky.";

        return errorInfo;
    }

    setColumnValue(value, row, column, delimiter, numberOfColumns);

    return errorInfo;
}

/**
 * Changes column's case of selected column
 * @param newCase New case (LOWER_CASE or UPPER_CASE)
 * @param column Selected column
 * @param row Row contains the column
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 */
void changeColumnCase(bool newCase, int column, Row *row, char delimiter, int numberOfColumns) {
    char value[MAX_CELL_SIZE];
    getColumnValue(value, row, column, delimiter, numberOfColumns);

    int shift;
    char start;
    if (newCase == LOWER_CASE) {
        start = 'A';
        shift = ('a' - 'A');
    } else {
        start = 'a';
        shift = -('a' - 'A');
    }

    for (int j = 0; j < (int)strlen(value); j++) {
        if ((value[j] >= start) && (value[j] <= start + ('z' - 'a'))) {
            value[j] = (char)(value[j] + shift);
        }
    }
    // It should be OK (it has been read from this column yet)
    setColumnValue(value, row, column, delimiter, numberOfColumns);
}

/**
 * Rounds value in selected column
 * @param column Selected column
 * @param row Row contains the column
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 * @return Error information
 */
ErrorInfo roundColumnValue(int column, Row *row, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    char value[MAX_CELL_SIZE];
    getColumnValue(value, row, column, delimiter, numberOfColumns);

    // The cells must contains valid number
    if (isValidNumber(value) == false) {
        errorInfo.error = true;
        errorInfo.message = "Funkci round nelze provest na bunce, ktera neobsahuje validni cislo.";

        return errorInfo;
    }

    double number = strtod(value, NULL);
    memset(value, '\0', strlen(value));
    sprintf(value, "%.f", number);

    // Should be OK (this column has already been used)
    setColumnValue(value, row, column, delimiter, numberOfColumns);

    return errorInfo;
}

/**
 * Removes decimal part from selected column's value (only removes, without rounding)
 * @param column Selected column
 * @param row Row contains the column
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 * @return Error information
 */
ErrorInfo removeColumnDecimalPart(int column, Row *row, char delimiter, int numberOfColumns) {
    ErrorInfo errorInfo = {false};

    char value[MAX_CELL_SIZE];
    getColumnValue(value, row, column, delimiter, numberOfColumns);

    // The cells must contains valid number
    if (isValidNumber(value) == false) {
        errorInfo.error = true;
        errorInfo.message = "Funkci round nelze provest na bunce, ktera neobsahuje validni cislo.";

        return errorInfo;
    }

    double number = strtod(value, NULL);
    memset(value, '\0', strlen(value));
    sprintf(value, "%d", (int)number);
    // Should be OK (this column has already been used)
    setColumnValue(value, row, column, delimiter, numberOfColumns);

    return errorInfo;
}

/**
 * Copies column's value to another column
 * @param from Source column
 * @param to Target column
 * @param row Row contains columns
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 */
void copy(int from, int to, Row *row, char delimiter, int numberOfColumns) {
    // One of the selected columns doesn't exists, so this function can't change anything
    if ((from > numberOfColumns) || (to > numberOfColumns)) {
        return;
    }

    char value[MAX_CELL_SIZE];
    getColumnValue(value, row, from, delimiter, numberOfColumns);

    setColumnValue(value, row, to, delimiter, numberOfColumns);
}

/**
 * Swaps values of selected columns
 * @param first First selected column
 * @param second Second selected column
 * @param row Row contains columns
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of columns in the row
 */
void swap(int first, int second, Row *row, char delimiter, int numberOfColumns) {
    // One of the selected columns doesn't exists, so this function can't change anything
    if ((first > numberOfColumns) || (second > numberOfColumns)) {
        return;
    }

    char firstValue[MAX_CELL_SIZE];
    char secondValue[MAX_CELL_SIZE];
    getColumnValue(firstValue, row, first, delimiter, numberOfColumns);
    getColumnValue(secondValue, row, second, delimiter, numberOfColumns);

    // Column numbers should be OK, so errors aren't expected
    setColumnValue(firstValue, row, second, delimiter, numberOfColumns);
    setColumnValue(secondValue, row, first, delimiter, numberOfColumns);
}

/**
 * Moves selected column before another selected column
 * @param column Selected column to move
 * @param beforeColumn Selected column to move the first before
 * @param row Row contains the column
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 */
void move(int column, int beforeColumn, Row *row, char delimiter, int numberOfColumns) {
    // One of the selected columns doesn't exists, so this function can't change anything
    if ((column > numberOfColumns) || (beforeColumn > numberOfColumns)) {
        return;
    }

    // Operation on one a the same column --> it doesn't make sense to do anything
    if (column == beforeColumn) {
        return;
    }

    char moving[2 * MAX_CELL_SIZE];
    char second[MAX_CELL_SIZE];
    getColumnValue(moving, row, column, delimiter, numberOfColumns);
    getColumnValue(second, row, beforeColumn, delimiter, numberOfColumns);

    // Delete column for move
    // Should be OK -> from this column has been successfully extracted before
    dcols(column, column, row, delimiter);

    // In this case column numbers will be moved by 1 to left and it's important to count with it
    if (beforeColumn > column) {
        beforeColumn--;
    }

    // Add the moving column before second selected column
    moving[strlen(moving)] = delimiter;
    strcat(moving, second);
    setColumnValue(moving, row, beforeColumn, delimiter, numberOfColumns);
}

/*******************************************************************************************************Help functions*/
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

    // Reset function's parameters to defaults
    memset(function->selectFunction.name, '\0', sizeof(function->selectFunction.name));

    // Find function
    for (int j = 0; j < (int)(sizeof(functions) / sizeof(char**)); j++) {
        if (streq(args->data[*position], functions[j])) {
            strcpy(function->name, functions[j]);

            // Prepare arguments for the function
            for (int i = 0; i < funcArgs[j]; i++) {
                int index = *position + i + 1; // Index of argument in InputArguments
                function->params[i] = toRowColNum(args->data[index], false);
                if ((index >= args->size) || function->params[i] == INVALID_NUMBER) {
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
        } else if (streq(args->data[*position], "rows")) {
            // Select interval of rows
            SelectFunction selection = {.name = "rows"};
            if ((selection.params[0] = toRowColNum(args->data[++(*position)], true)) == INVALID_NUMBER) {
                errorInfo.error = true;
                errorInfo.message = "Chybne cislo ve vyberu pocatecniho radku, povolena jsou cela cisla od 1.";

                return errorInfo;
            }

            if ((selection.params[1] = toRowColNum(args->data[++(*position)], true)) == INVALID_NUMBER) {
                errorInfo.error = true;
                errorInfo.message = "Chybne cislo ve vyberu koncoveho radku, povolena jsou cela cisla od 1 a '-'.";

                return errorInfo;
            }

            if (selection.params[1] != LAST_ROW_NUMBER && selection.params[1] < selection.params[0]) {
                errorInfo.error = true;
                errorInfo.message = "Chybne poradi argumentu funkce rows, prvni cislo musi byt mensi nebo rovno.";

                return errorInfo;
            }

            // Save selection a move position to next function
            function->selectFunction = selection;
            (*position)++;
        } else if (streq(args->data[*position], "beginswith")) {
            // Select rows begin with something
            SelectFunction selection = {.name = "beginswith"};

            if ((selection.params[0] = toRowColNum(args->data[++(*position)], false)) == INVALID_NUMBER) {
                errorInfo.error = true;
                errorInfo.message = "Chybne cislo ve vyberu sloupce, povolena jsou cela cisla od 1.";

                return errorInfo;
            }

            (*position)++;
            memmove(selection.strParams[1], args->data[*position], strlen(args->data[*position]));

            // Save selection a move position to next function
            function->selectFunction = selection;
            (*position)++;
        } else if (streq(args->data[*position], "contains")) {
            // Select rows contain something
            SelectFunction selection = {.name = "contains"};

            if ((selection.params[0] = toRowColNum(args->data[++(*position)], false)) == INVALID_NUMBER) {
                errorInfo.error = true;
                errorInfo.message = "Chybne cislo ve vyberu sloupce, povolena jsou cela cisla od 1.";

                return errorInfo;
            }

            (*position)++;
            memmove(selection.strParams[1], args->data[*position], strlen(args->data[*position]));

            // Save selection a move position to next function
            function->selectFunction = selection;
            (*position)++;
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
    if (streq(value, "-") && (specialAllowed == true)) {
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
 * @param Pointer to save value of the selected column (without '\n')
 * @param row Row contains the column
 * @param columnNumber Number of selected column
 * @param delimiter Column delimiter
 */
void getColumnValue(char *value, const Row *row, int columnNumber, char delimiter, int numberOfColumns) {
    memset(value, '\0', MAX_CELL_SIZE);

    // Column that doesn't exists, so it doesn't have any value
    if (columnNumber > numberOfColumns) {
        return;
    }

    int counter = 1;
    int j = 0;
    for (int i = 0; i < row->size; i++) {
        // \n is "delimiter" for the last column
        if ((row->data[i] == delimiter) || (row->data[i] == '\n')) {
            counter++;
        } else if (counter == columnNumber) {
            value[j] = row->data[i];
            j++;
        }
    }
}

/**
 * Sets value of selected column
 * @param value New column's value
 * @param row Row contains the column
 * @param columnNumber Column's number
 * @param delimiter Column delimiter
 * @param numberOfColumns Number of column in the row
 */
void setColumnValue(const char *value, Row *row, int columnNumber, char delimiter, int numberOfColumns) {
    // Can't be applied because the column doesn't exists
    if (columnNumber > numberOfColumns) {
        return;
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
            while ((rowBackup[backupIndex] != delimiter) && (rowBackup[backupIndex] != '\n')) {
                backupIndex++;
            }
        }

        // Loading unchanged data from backup
        row->data[dataIndex] = rowBackup[backupIndex];

        // Mark next column
        if ((rowBackup[backupIndex] == delimiter) || (rowBackup[backupIndex] == '\n')) {
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
}

/**
 * Checks if the string contains valid number
 * @param number String for testing
 * @return Is valid number in the string?
 */
bool isValidNumber(char *number) {
    bool decimalPoint = false; // Was decimal point found?
    for (int i = 0; i < (int) strlen(number); i++) {
        if (((number[i] < '0') || (number[i] > '9')) && (i == 0 && (number[i] != '-'))) {
            if (number[i] == '.' && decimalPoint == false) {
                decimalPoint = true;
            } else {
                return false;
            }
        }
    }

    return true;
}