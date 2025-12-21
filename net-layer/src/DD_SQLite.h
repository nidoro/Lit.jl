#ifndef DD_SQLITE_H
#define DD_SQLITE_H

#include <time.h>
#include <stdio.h>
#include <sqlite3.h>
#include "DD_Array.h"

#define SQ_KILO_BYTES(x) (x*1024LL)
#define SQ_LOCALTIME "datetime('now', 'localtime')"
#define SQ_UTC0 "datetime('now')"

//---------------------------------------
// ASSERT INTERNAL IMPLEMENTATION
//---------------------------------------
#define SQ_Assert(expression) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d)\n", __FILE__, __LINE__);\
        int* ptr = 0;\
        *ptr = 0;\
    }
    
#define SQ_Assert1(expression, msg) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d): ", __FILE__, __LINE__);\
        fprintf(stderr, msg);\
        fprintf(stderr, "\n");\
        int* ptr = 0;\
        *ptr = 0;\
    }
    
#define SQ_Assert2(expression, format, ...) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d): ", __FILE__, __LINE__);\
        fprintf(stderr, format, __VA_ARGS__);\
        fprintf(stderr, "\n");\
        int* ptr = 0;\
        *ptr = 0;\
    }

//--------------------------------------
// ANSI COLORS INTERNAL IMPLEMENTATION
//--------------------------------------
#ifdef SQ_ANSI_COLORS_DISABLE
#define SQ_ANSIColorReset         ""
#define SQ_ANSIColorCodeRed       ""
#define SQ_ANSIColorCodeGreen     ""
#define SQ_ANSIColorCodeYellow    ""
#define SQ_ANSIColorCodeBlue      ""
#define SQ_ANSIColorCodeMagenta   ""
#define SQ_ANSIColorCodeCyan      ""
#else

#define SQ_ANSIColorReset         "\x1b[0m"
#define SQ_ANSIColorCodeRed       "\x1b[31m"
#define SQ_ANSIColorCodeGreen     "\x1b[32m"
#define SQ_ANSIColorCodeYellow    "\x1b[33m"
#define SQ_ANSIColorCodeBlue      "\x1b[34m"
#define SQ_ANSIColorCodeMagenta   "\x1b[35m"
#define SQ_ANSIColorCodeCyan      "\x1b[36m"
#endif

#define SQ_ANSIColorRed(text)     SQ_ANSIColorCodeRed     text SQ_ANSIColorReset
#define SQ_ANSIColorGreen(text)   SQ_ANSIColorCodeGreen   text SQ_ANSIColorReset
#define SQ_ANSIColorYellow(text)  SQ_ANSIColorCodeYellow  text SQ_ANSIColorReset
#define SQ_ANSIColorBlue(text)    SQ_ANSIColorCodeBlue    text SQ_ANSIColorReset
#define SQ_ANSIColorMagenta(text) SQ_ANSIColorCodeMagenta text SQ_ANSIColorReset
#define SQ_ANSIColorCyan(text)    SQ_ANSIColorCodeCyan    text SQ_ANSIColorReset

//--------------
// SQLITE UTILS
//--------------
enum SQ_TableCellType {
    SQ_TableCellType_Text,
    SQ_TableCellType_Integer,
    SQ_TableCellType_RealFloat,
    SQ_TableCellType_Blob,
    SQ_TableCellType_Null
};

struct SQ_TableCell {
    SQ_TableCellType type;
    int size;
    union {
        char* string;
        char* data;
        int integer;
        double realFloat;
    };
    long long int integer64;
};

struct SQ_ReadDest {
    const char* column;
    void* dest;
};

void SQ_FreeTable(SQ_TableCell** table) {
    for (int i = 0; i < arrcount(table); ++i) {
        for (int j = 0; j < arrcount(table[i]); ++j) {
            if (table[i][j].type == SQ_TableCellType_Text) {
                arrfree(table[i][j].string);
            } else if (table[i][j].type == SQ_TableCellType_Blob) {
                arrfree(table[i][j].data);
            }
        }
        arrfree(table[i]);
    }
    arrfree(table);
}

sqlite3_stmt* SQ_PrepareStatementVA(sqlite3* dbHandle, char* cmdBuffer, const char* formatString, va_list argList) {
    sqlite3_stmt* result = 0;
    vsprintf(cmdBuffer, formatString, argList);
    
    int errorCode;
    do {
        errorCode = sqlite3_prepare_v2(dbHandle, cmdBuffer, -1, &result, 0);
        
        if (errorCode != SQLITE_OK && errorCode != SQLITE_BUSY)
            SQ_Assert2(errorCode == SQLITE_OK, "sqlite3 command fail (error %d): %s\n", errorCode, cmdBuffer);
    } while (errorCode != SQLITE_OK);
    
    
    return result;
}

sqlite3_stmt* SQ_PrepareStatementVA(sqlite3* dbHandle, const char* formatString, va_list argList) {
    char cmdBuffer[SQ_KILO_BYTES(64)];
    return SQ_PrepareStatementVA(dbHandle, cmdBuffer, formatString, argList);
}

sqlite3_stmt* SQ_PrepareStatement(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    sqlite3_stmt* result = SQ_PrepareStatementVA(dbHandle, formatString, argList);
    va_end(argList);
    return result;
}

sqlite3_stmt* SQ_PrepareStatement2(sqlite3* dbHandle, char* cmdBuffer, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    sqlite3_stmt* result = SQ_PrepareStatementVA(dbHandle, cmdBuffer, formatString, argList);
    va_end(argList);
    return result;
}

SQ_TableCell** SQ_GetQueryResult(sqlite3* dbHandle, sqlite3_stmt* statement, bool keepHeaders, int allocatorIndex) {
    int columnCount = sqlite3_column_count(statement);
    SQ_TableCell** table = 0;
    
    if (keepHeaders && columnCount) {
        table = arrallocate(SQ_TableCell*, 1, allocatorIndex);
        arrcount(table) = arrcap(table);
        table[0] = arrallocate(SQ_TableCell, columnCount, allocatorIndex);
        for (int i = 0; i < columnCount; ++i) {
            const char* columnName = sqlite3_column_origin_name(statement, i);
            SQ_TableCell cell = {};
            if (columnName) {
                cell.type = SQ_TableCellType_Text;
                cell.string = arrstringalloc(columnName, allocatorIndex);
                cell.size = arrcount(cell.string);
            } else {
                cell.type = SQ_TableCellType_Null;
                cell.integer = 0;
            }
            arradd(table[0], cell);
        }
    } else {
        table = arrallocate(SQ_TableCell*, 0, allocatorIndex);
    }
    
    int stepResult = sqlite3_step(statement);
    
    while (stepResult != SQLITE_DONE) {
        switch (stepResult) {
          case SQLITE_ROW: {
            SQ_TableCell* row = arrallocate(SQ_TableCell, columnCount, allocatorIndex);

            for (int i = 0; i < columnCount; ++i) {
                SQ_TableCell cell = {};
                int sqlType = sqlite3_column_type(statement, i);
                switch(sqlType) {
                  case SQLITE_TEXT: {
                    cell.type = SQ_TableCellType_Text;
                    cell.size = sqlite3_column_bytes(statement, i);
                    cell.string = arrstringalloc((char*) sqlite3_column_text(statement, i), cell.size, allocatorIndex);
                    cell.data = cell.string;
                  } break;
                  case SQLITE_INTEGER: {
                    cell.type = SQ_TableCellType_Integer;
                    cell.integer = sqlite3_column_int(statement, i);
                    cell.integer64 = sqlite3_column_int64(statement, i);
                  } break;
                  case SQLITE_FLOAT: {
                    cell.type = SQ_TableCellType_RealFloat;
                    cell.realFloat = sqlite3_column_double(statement, i);
                  } break;
                  case SQLITE_BLOB: {
                    char* data = (char*) sqlite3_column_blob(statement, i);
                    cell.type = SQ_TableCellType_Blob;
                    cell.size = sqlite3_column_bytes(statement, i);
                    cell.data = arrstringalloc(data, cell.size, allocatorIndex);
                  } break;
                  case SQLITE_NULL: {
                    cell.type = SQ_TableCellType_Null;
                    cell.integer = 0;
                  } break;
                  default: break;
                }
                arradd(row, cell);
            }
            arradd(table, row);
          } break;
          
          case SQLITE_BUSY: {
            // ATTENTION: not tested.
            timespec tspec;
            tspec.tv_sec = 0;
            tspec.tv_nsec = 3*1000000;
            nanosleep(&tspec, 0);
          } break;
          
          default: {
            SQ_Assert2(0, "%s\n%s\n", sqlite3_errstr(stepResult), sqlite3_errmsg(dbHandle));
          } break;
        }
        stepResult = sqlite3_step(statement);
    }
    
    return table;
}

SQ_TableCell** SQ_GetQueryResult(sqlite3* dbHandle, sqlite3_stmt* statement, bool keepHeaders) {
    return SQ_GetQueryResult(dbHandle, statement, keepHeaders, 0);
}

SQ_TableCell** SQ_GetQueryResultVA(sqlite3* dbHandle, char* cmdBuffer, bool keepHeaders, int allocatorIndex, const char* formatString, va_list argList) {
    sqlite3_stmt* statement = SQ_PrepareStatementVA(dbHandle, cmdBuffer, formatString, argList);

    if (!statement) {
        return 0;
    }

    SQ_TableCell** table = SQ_GetQueryResult(dbHandle, statement, keepHeaders, allocatorIndex);

    sqlite3_finalize(statement);

    return table;
}

SQ_TableCell** SQ_GetQueryResultVA(sqlite3* dbHandle, bool keepHeaders, int allocatorIndex, const char* formatString, va_list argList) {
    char queryBuffer[SQ_KILO_BYTES(64)];
    return SQ_GetQueryResultVA(dbHandle, queryBuffer, keepHeaders, allocatorIndex, formatString, argList);
}

SQ_TableCell** SQ_GetQueryResultVA(sqlite3* dbHandle, bool keepHeaders, const char* formatString, va_list argList) {
    return SQ_GetQueryResultVA(dbHandle, keepHeaders, 0, formatString, argList);
}

SQ_TableCell** SQ_GetQueryResult(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, false, formatString, argList);
    va_end(argList);
    return result;
}

SQ_TableCell** SQ_GetQueryResult(sqlite3* dbHandle, int allocatorIndex, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, false, allocatorIndex, formatString, argList);
    va_end(argList);
    return result;
}

SQ_TableCell** SQ_GetQueryResult2(sqlite3* dbHandle, char* cmdBuffer, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, cmdBuffer, false, 0, formatString, argList);
    va_end(argList);
    return result;
}

SQ_TableCell** SQ_GetQueryResultWithHeaders(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, true, formatString, argList);
    va_end(argList);
    return result;
}

SQ_TableCell** SQ_GetQueryResultWithHeaders2(sqlite3* dbHandle, char* cmdBuffer, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, cmdBuffer, true, 0, formatString, argList);
    va_end(argList);
    return result;
}

int SQ_GetQueryResultSizeVA(sqlite3* dbHandle, char* cmdBuffer, bool keepHeaders, const char* formatString, va_list argList) {
    SQ_TableCell** table = SQ_GetQueryResultVA(dbHandle, cmdBuffer, keepHeaders, 0, formatString, argList);
    int size = arrcount(table);
    SQ_FreeTable(table);
    return size;
}

int SQ_GetQueryResultSizeVA(sqlite3* dbHandle, bool keepHeaders, const char* formatString, va_list argList) {
    SQ_TableCell** table = SQ_GetQueryResultVA(dbHandle, keepHeaders, formatString, argList);
    int size = arrcount(table);
    SQ_FreeTable(table);
    return size;
}

int SQ_GetQueryResultSize(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    int result = SQ_GetQueryResultSizeVA(dbHandle, false, formatString, argList);
    va_end(argList);
    return result;
}

int SQ_GetQueryResultSize2(sqlite3* dbHandle, char* cmdBuffer, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    int result = SQ_GetQueryResultSizeVA(dbHandle, cmdBuffer, false, formatString, argList);
    va_end(argList);
    return result;
}

SQ_TableCell SQ_GetAggregateFunctionResultVA(sqlite3* dbHandle, const char* formatString, va_list argList) {
    SQ_TableCell** table = SQ_GetQueryResultVA(dbHandle, false, formatString, argList);
    SQ_TableCell result = {};
    if (arrcount(table)) {
        result = table[0][0];
    }
    SQ_FreeTable(table);
    return result;
}

int SQ_GetAggregateFunctionResultInt(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    int result = SQ_GetAggregateFunctionResultVA(dbHandle, formatString, argList).integer;
    va_end(argList);
    return result;
}

long long int SQ_GetAggregateFunctionResultInt64(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    long long int result = SQ_GetAggregateFunctionResultVA(dbHandle, formatString, argList).integer64;
    va_end(argList);
    return result;
}

double SQ_GetAggregateFunctionResultDouble(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    double result = SQ_GetAggregateFunctionResultVA(dbHandle, formatString, argList).realFloat;
    va_end(argList);
    return result;
}

void SQ_PrintTable(SQ_TableCell** table) {
    for (int i = 0; i < arrcount(table); ++i) {
        for (int j = 0; j < arrcount(table[i]); ++j) {
            if (table[i][j].type == SQ_TableCellType_Text) {
                printf("%s ", table[i][j].string);
            } else if (table[i][j].type == SQ_TableCellType_Integer) {
                printf("%d ", table[i][j].integer);
            } else if (table[i][j].type == SQ_TableCellType_RealFloat) {
                printf("%f ", table[i][j].realFloat);
            } else if (table[i][j].type == SQ_TableCellType_Blob) {
                printf("Blob(%d) ", table[i][j].size);
            }
        }
        printf("\n");
    }
}

bool SQ_ExecuteCommand(sqlite3* dbHandle, sqlite3_stmt* statement) {
    SQ_TableCell** result = SQ_GetQueryResult(dbHandle, statement, false);
    
    if (result) {
        SQ_FreeTable(result);
        return true;
    }
    return false;
}

bool SQ_ExecuteCommandVA(sqlite3* dbHandle, int allocatorIndex, const char* formatString, va_list argList) {
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, false, allocatorIndex, formatString, argList);
    if (result) {
        SQ_FreeTable(result);
        return true;
    }
    return false;
}

bool SQ_ExecuteCommand(sqlite3* dbHandle, int allocatorIndex, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    bool result = SQ_ExecuteCommandVA(dbHandle, allocatorIndex, formatString, argList);
    va_end(argList);
    return result;
}

bool SQ_ExecuteCommand(sqlite3* dbHandle, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    bool result = SQ_ExecuteCommandVA(dbHandle, 0, formatString, argList);
    va_end(argList);
    return result;
}

bool SQ_ExecuteCommand2(sqlite3* dbHandle, char* cmdBuffer, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    SQ_TableCell** result = SQ_GetQueryResultVA(dbHandle, cmdBuffer, false, 0, formatString, argList);
    va_end(argList);
    if (result) {
        SQ_FreeTable(result);
        return true;
    }
    return false;
}

bool SQ_ExecuteUpdateTextCommand(sqlite3* dbHandle, const char* text, long long int textSize, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    sqlite3_stmt* statement = SQ_PrepareStatementVA(dbHandle, formatString, argList);
    va_end(argList);
    sqlite3_bind_text(statement, 1, text, textSize, 0);
    bool result = SQ_ExecuteCommand(dbHandle, statement);
    sqlite3_finalize(statement);
    return result;
}

bool SQ_ExecuteUpdateBlobCommand(sqlite3* dbHandle, char* blob, long long int blobSize, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    sqlite3_stmt* statement = SQ_PrepareStatementVA(dbHandle, formatString, argList);
    va_end(argList);
    sqlite3_bind_blob(statement, 1, blob, blobSize, 0);
    bool result = SQ_ExecuteCommand(dbHandle, statement);
    sqlite3_finalize(statement);
    return result;
}

sqlite3* SQ_OpenDB(const char* path) {
    sqlite3* dbHandle = 0;
    sqlite3_open(path, &dbHandle);
    return dbHandle;
}

void SQ_CloseDB(sqlite3* dbHandle) {
    sqlite3_close(dbHandle);
}

SQ_TableCell SQ_GetCell(SQ_TableCell** table, int row, const char* columnName) {
    int column = 0;
    for (int j = 0; j < arrcount(table[0]); ++j) {
        if (table[0][j].type == SQ_TableCellType_Text && strcmp(table[0][j].string, columnName)==0) {
            column = j;
            break;
        }
    }
    return table[row][column];
}

int SQ_GetCellInt(SQ_TableCell** table, int row, const char* columnName) {
    SQ_TableCell cell = SQ_GetCell(table, row, columnName);
    return cell.integer;
}

long long int SQ_GetCellInt64(SQ_TableCell** table, int row, const char* columnName) {
    SQ_TableCell cell = SQ_GetCell(table, row, columnName);
    return cell.integer64;    
}

const char* SQ_GetCellString(SQ_TableCell** table, int row, const char* columnName) {
    SQ_TableCell cell = SQ_GetCell(table, row, columnName);
    if (cell.type == SQ_TableCellType_Text) {
        return cell.string;
    } else {
        return "";
    }
}

char* SQ_GetCellBlob(SQ_TableCell** table, int row, const char* columnName) {
    SQ_TableCell cell = SQ_GetCell(table, row, columnName);
    return cell.data;
}

bool SQ_SelectBlobVA(sqlite3* dbHandle, int allocatorIndex, char* dest, const char* column, const char* from, const char* whereFormatString, va_list argList) {
    char queryBuffer[SQ_KILO_BYTES(64)];
    int at = 0;
    at += sprintf(queryBuffer+at, "SELECT %s FROM %s", column, from);
    
    char buf[1024] = {};
    vsprintf(buf, whereFormatString, argList);
    at += sprintf(queryBuffer+at, " WHERE %s", buf);
    
    SQ_TableCell** queryResult = SQ_GetQueryResult(dbHandle, allocatorIndex, queryBuffer);
    bool result = arrcount(queryResult);
    
    if (result && (queryResult[0][0].type == SQ_TableCellType_Blob || queryResult[0][0].type == SQ_TableCellType_Text)) {
        memcpy(dest, queryResult[0][0].data, queryResult[0][0].size);
    }
    
    SQ_FreeTable(queryResult);
    
    return result;
}

bool SQ_SelectStringVA(sqlite3* dbHandle, int allocatorIndex, char* dest, const char* column, const char* from, const char* whereFormatString, va_list argList) {
    char queryBuffer[SQ_KILO_BYTES(64)];
    int at = 0;
    at += sprintf(queryBuffer+at, "SELECT %s FROM %s", column, from);
    
    char buf[1024] = {};
    vsprintf(buf, whereFormatString, argList);
    at += sprintf(queryBuffer+at, " WHERE %s", buf);
    
    SQ_TableCell** queryResult = SQ_GetQueryResult(dbHandle, allocatorIndex, queryBuffer);
    bool result = arrcount(queryResult);
    
    if (result && queryResult[0][0].type == SQ_TableCellType_Text) {
        strncpy(dest, queryResult[0][0].string, arrcount(queryResult[0][0].string));
        dest[arrcount(queryResult[0][0].string)] = 0;
    }
    
    SQ_FreeTable(queryResult);
    
    return result;
}

bool SQ_SelectString(sqlite3* dbHandle, int allocatorIndex, char* dest, const char* column, const char* from, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    bool result = SQ_SelectStringVA(dbHandle, allocatorIndex, dest, column, from, whereFormatString, argList);
    va_end(argList);
    return result;
}

bool SQ_SelectString(sqlite3* dbHandle, char* dest, const char* column, const char* from, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    bool result = SQ_SelectStringVA(dbHandle, 0, dest, column, from, whereFormatString, argList);
    va_end(argList);
    return result;
}

bool SQ_SelectBlob(sqlite3* dbHandle, char* dest, const char* column, const char* from, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    bool result = SQ_SelectBlobVA(dbHandle, 0, dest, column, from, whereFormatString, argList);
    va_end(argList);
    return result;
}

bool SQ_SelectInteger(sqlite3* dbHandle, int* dest, const char* column, const char* from, const char* whereFormatString, ...) {
    char queryBuffer[SQ_KILO_BYTES(64)];
    int at = 0;
    at += sprintf(queryBuffer+at, "SELECT %s FROM %s", column, from);
    
    va_list argList;
    va_start(argList, whereFormatString);
    char buf[1024];
    vsprintf(buf, whereFormatString, argList);
    va_end(argList);
    at += sprintf(queryBuffer+at, " WHERE %s", buf);
    
    SQ_TableCell** queryResult = SQ_GetQueryResult(dbHandle, queryBuffer);
    bool result = arrcount(queryResult);
    
    if (result) {
        *dest = queryResult[0][0].integer;
    }
    
    SQ_FreeTable(queryResult);
    
    return result;
}

bool SQ_SelectFloat(sqlite3* dbHandle, float* dest, const char* column, const char* from, const char* whereFormatString, ...) {
    char queryBuffer[SQ_KILO_BYTES(64)];
    int at = 0;
    at += sprintf(queryBuffer+at, "SELECT %s FROM %s", column, from);
    
    va_list argList;
    va_start(argList, whereFormatString);
    char buf[1024];
    vsprintf(buf, whereFormatString, argList);
    va_end(argList);
    at += sprintf(queryBuffer+at, " WHERE %s", buf);
    
    SQ_TableCell** queryResult = SQ_GetQueryResult(dbHandle, queryBuffer);
    bool result = arrcount(queryResult);
    
    if (result) {
        *dest = queryResult[0][0].realFloat;
    }
    
    SQ_FreeTable(queryResult);
    
    return result;
}

bool SQ_Select1VA(sqlite3* dbHandle, int allocatorIndex, SQ_ReadDest* reader, const char* from, const char* whereFormatString, va_list argList) {
    char queryBuffer[SQ_KILO_BYTES(64)];
    int at = 0;
    at += sprintf(queryBuffer+at, "SELECT ");
    
    int i = 0;
    at += sprintf(queryBuffer+at, "%s", (char*) reader[i].column);
    
    ++i;
    while (reader[i].column) {
        at += sprintf(queryBuffer+at, ", %s", reader[i].column);
        ++i;
    }
    
    at += sprintf(queryBuffer+at, " FROM %s", from);
    
    if (whereFormatString) {
        char buf[1024] = {};
        vsprintf(buf, whereFormatString, argList);
        at += sprintf(queryBuffer+at, " WHERE %s", buf);
    }
    
    SQ_TableCell** queryResult = SQ_GetQueryResult(dbHandle, allocatorIndex, queryBuffer);
    bool result = arrcount(queryResult);
    
    if (arrcount(queryResult)) {
        i = 0;
        while (reader[i].column) {
            switch (queryResult[0][i].type) {
              case SQ_TableCellType_Text: {
                strncpy((char*) reader[i].dest, queryResult[0][i].string, arrcount(queryResult[0][i].string));
                ((char*) reader[i].dest)[arrcount(queryResult[0][i].string)] = 0;
              } break;
              case SQ_TableCellType_Integer: {
                *(int*) reader[i].dest = queryResult[0][i].integer;
              } break;
              case SQ_TableCellType_RealFloat: {
                *(double*) reader[i].dest = queryResult[0][i].realFloat;
              } break;
              case SQ_TableCellType_Blob: {
                strncpy((char*) reader[i].dest, (char*) queryResult[0][i].data, arrcount(queryResult[0][i].data));
                ((char*) reader[i].dest)[arrcount(queryResult[0][i].data)] = 0;
              } break;
              case SQ_TableCellType_Null: break;
              default: break;
            }
            ++i;
        }
    }
    
    SQ_FreeTable(queryResult);
    return result;
}

bool SQ_Select1(sqlite3* dbHandle, int allocatorIndex, SQ_ReadDest* reader, const char* from, const char* whereFormatString, ...) {
    va_list argList;
    bool result = false;
    if (whereFormatString) {
        va_start(argList, whereFormatString);
        result = SQ_Select1VA(dbHandle, allocatorIndex, reader, from, whereFormatString, argList);
        va_end(argList);
    } else {
        result = SQ_Select1VA(dbHandle, allocatorIndex, reader, from, whereFormatString, argList);
    }
    return result;
}

bool SQ_Select1(sqlite3* dbHandle, SQ_ReadDest* reader, const char* from, const char* whereFormatString, ...) {
    va_list argList;
    bool result = false;
    if (whereFormatString) {
        va_start(argList, whereFormatString);
        result = SQ_Select1VA(dbHandle, 0, reader, from, whereFormatString, argList);
        va_end(argList);
    } else {
        result = SQ_Select1VA(dbHandle, 0, reader, from, whereFormatString, argList);
    }
    return result;
}

void SQ_UpdateStringVA(sqlite3* dbHandle, char* newString, int newStringSize, const char* table, const char* column, const char* whereFormatString, va_list argList) {
    char queryBuffer[SQ_KILO_BYTES(64)] = {};
    
    char buf[1024] = {};
    vsprintf(buf, whereFormatString, argList);
    
    sprintf(queryBuffer, "UPDATE `%s` SET `%s`=? WHERE %s", table, column, buf);
    sqlite3_stmt* statement = SQ_PrepareStatement(dbHandle, queryBuffer);
    sqlite3_bind_text(statement, 1, newString, newStringSize, 0);
    SQ_ExecuteCommand(dbHandle, statement);
    sqlite3_finalize(statement);
}

void SQ_UpdateString(sqlite3* dbHandle, char* newString, int newStringSize, const char* table, const char* column, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    SQ_UpdateStringVA(dbHandle, newString, newStringSize, table, column, whereFormatString, argList);
    va_end(argList);
}

void SQ_UpdateString(sqlite3* dbHandle, char* newString, const char* table, const char* column, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    SQ_UpdateStringVA(dbHandle, newString, strlen(newString), table, column, whereFormatString, argList);
    va_end(argList);
}

int SQ_GetLastRowId(sqlite3* dbHandle, const char* tableName) {
    return SQ_GetAggregateFunctionResultInt(dbHandle, "SELECT rowid FROM `%s` ORDER BY ROWID DESC LIMIT 1", tableName);
}

int SQ_GetFirstRowId(sqlite3* dbHandle, const char* tableName) {
    return SQ_GetAggregateFunctionResultInt(dbHandle, "SELECT rowid FROM `%s` ORDER BY ROWID ASC LIMIT 1", tableName);
}

void SQ_DeleteRows(sqlite3* dbHandle, const char* table, const char* whereFormatString, ...) {
    char queryBuffer[SQ_KILO_BYTES(64)] = {};
    
    va_list argList;
    va_start(argList, whereFormatString);
    char buf[1024];
    vsprintf(buf, whereFormatString, argList);
    va_end(argList);
    
    sprintf(queryBuffer, "DELETE FROM `%s` WHERE %s", table, buf);
    SQ_ExecuteCommand(dbHandle, queryBuffer);
}

void SQ_ClearTable(sqlite3* dbHandle, const char* table) {
    SQ_ExecuteCommand(dbHandle, "DELETE FROM `%s`", table);
}

void SQ_DeleteFirstRow(sqlite3* dbHandle, const char* table) {
    int rowId = SQ_GetFirstRowId(dbHandle, table);
    SQ_DeleteRows(dbHandle, table, "ROWID=%d", rowId);
}

bool SQ_RowExistsVA(sqlite3* dbHandle, int allocatorIndex, const char* table, const char* whereFormatString, va_list argList) {
    char queryBuffer[SQ_KILO_BYTES(64)] = {};
    char buf[1024] = {};
    vsprintf(buf, whereFormatString, argList);
    
    sprintf(queryBuffer, "SELECT 0 FROM `%s` WHERE %s", table, buf);
    SQ_TableCell** queryResult = SQ_GetQueryResult(dbHandle, allocatorIndex, queryBuffer);
    bool result = arrcount(queryResult);
    SQ_FreeTable(queryResult);
    
    return result;
}

bool SQ_RowExists(sqlite3* dbHandle, int allocatorIndex, const char* table, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    bool result = SQ_RowExistsVA(dbHandle, allocatorIndex, table, whereFormatString, argList);
    va_end(argList);
    return result;
}

bool SQ_RowExists(sqlite3* dbHandle, const char* table, const char* whereFormatString, ...) {
    va_list argList;
    va_start(argList, whereFormatString);
    bool result = SQ_RowExistsVA(dbHandle, 0, table, whereFormatString, argList);
    va_end(argList);
    return result;
}

bool SQ_TableExists(sqlite3* dbHandle, const char* tableName) {
    return SQ_RowExists(dbHandle, "sqlite_master", "type='table' AND name='%s'", tableName);
}

int SQ_GetTableSize(sqlite3* dbHandle, const char* tableName) {
    return SQ_GetAggregateFunctionResultInt(dbHandle, "SELECT COUNT(*) FROM `%s`", tableName);
}

/*
** Perform an online backup of database pDb to the database file named
** by zFilename. This function copies 5 database pages from pDb to
** zFilename, then unlocks pDb and sleeps for 250 ms, then repeats the
** process until the entire database is backed up.
** 
** The third argument passed to this function must be a pointer to a progress
** function. After each set of 5 pages is backed up, the progress function
** is invoked with two integer parameters: the number of pages left to
** copy, and the total number of pages in the source file. This information
** may be used, for example, to update a GUI progress bar.
**
** While this function is running, another thread may use the database pDb, or
** another process may access the underlying database file via a separate 
** connection.
**
** If the backup process is successfully completed, SQLITE_OK is returned.
** Otherwise, if an error occurs, an SQLite error code is returned.
*/
int SQ_CreateBackup(
  sqlite3 *pDb,               /* Database to back up */
  const char *zFilename,      /* Name of file to back up to */
  void(*xProgress)(int, int)  /* Progress function to invoke */     
){
  int rc;                     /* Function return code */
  sqlite3 *pFile;             /* Database connection opened on zFilename */
  sqlite3_backup *pBackup;    /* Backup handle used to copy data */

  /* Open the database file identified by zFilename. */
  rc = sqlite3_open(zFilename, &pFile);
  if( rc==SQLITE_OK ){

    /* Open the sqlite3_backup object used to accomplish the transfer */
    pBackup = sqlite3_backup_init(pFile, "main", pDb, "main");
    if( pBackup ){

      /* Each iteration of this loop copies 5 database pages from database
      ** pDb to the backup database. If the return value of backup_step()
      ** indicates that there are still further pages to copy, sleep for
      ** 250 ms before repeating. */
      do {
        rc = sqlite3_backup_step(pBackup, 500);
        if (xProgress) {
            xProgress(
                sqlite3_backup_remaining(pBackup),
                sqlite3_backup_pagecount(pBackup)
            );
        }
        if( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED ) {
          sqlite3_sleep(16);
        }
      } while( rc==SQLITE_OK || rc==SQLITE_BUSY || rc==SQLITE_LOCKED );

      /* Release resources allocated by backup_init(). */
      (void)sqlite3_backup_finish(pBackup);
    }
    rc = sqlite3_errcode(pFile);
  }
  
  /* Close the database connection opened on database file zFilename
  ** and return the result of this function. */
  (void)sqlite3_close(pFile);
  return rc;
}


#endif
