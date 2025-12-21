// ATTENTION:
// This library parses "pseudo-json" files:
// The input is expected to have a fixed width encoding where each byte is a character.
// This means that if the input is UTF-8 encoded and some of the characters require more
// than one byte to be encoded, we treat each byte as being one character. This may be a 
// problem or not, it depends on the usage, really. If the ecosystem where it is being
// used never use 'weird' characters (a.k.a. characters that need more than one byte to
// be encoded), then everything should work fine.

#ifndef DD_JSON_H
#define DD_JSON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Internal dd_array API
// ----------------------

#define DDJSON_arrheader(arr)                  (*(DDJSON_arrheader*) (((char*)(arr)) - sizeof(DDJSON_arrheader)))
#define DDJSON_arrcount(arr)                   DDJSON_arrheader(arr).count
#define DDJSON_arrcap(arr)                     DDJSON_arrheader(arr).capacity
#define DDJSON_arrtypesize(arr)                DDJSON_arrheader(arr).typesize
#define DDJSON_arrlast(arr)                    arr[DDJSON_arrcount(arr)-1]
#define DDJSON_arradd(arr, instance)           DDJSON_arrAdd((void*&)arr, (void*)&instance)
#define DDJSON_arralloc(type, capacity)        (type*) DDJSON_arrMalloc(sizeof(type), capacity)
#define DDJSON_arrfree(arr)                    DDJSON_arrFree((void*&)arr)
#define DDJSON_arrclear(arr)                   DDJSON_arrClear((void*&)arr)
#define DDJSON_arrremove(arr, index)           DDJSON_arrRemove((void*&)arr, index)
#define DDJSON_arrremovematch(arr, instance)   DDJSON_arrRemove((void*&)arr, DDJSON_arrFind((void*&)arr, (void*)&instance))
#define DDJSON_arrfind(arr, instance)          DDJSON_arrFind((void*&)arr, (void*)&instance)

struct DDJSON_arrheader {
    int typesize;
    int capacity;
    int count;
};

void* DDJSON_arrMalloc(int typesize, int capacity) {
    DDJSON_arrheader* header = (DDJSON_arrheader*) malloc(sizeof(DDJSON_arrheader) + typesize*capacity);
    header->typesize = typesize;
    header->capacity = capacity;
    header->count = 0;
    return (void*) (header+1);
}

void DDJSON_arrFree(void*& arr) {
    free(((char*)(arr)) - sizeof(DDJSON_arrheader));
    arr = 0;
}

void DDJSON_arrAdd(void*& array, void* instance) {
    if (DDJSON_arrcount(array) + 1 > DDJSON_arrcap(array)) {
        void* newarray = DDJSON_arrMalloc(DDJSON_arrtypesize(array), DDJSON_arrcount(array) + 1);
        memcpy(newarray, array, DDJSON_arrcount(array) * DDJSON_arrtypesize(array));
        DDJSON_arrcount(newarray) = DDJSON_arrcount(array);
        DDJSON_arrFree(array);
        array = newarray;
    }
    char* dest = ((char*)(array)) + DDJSON_arrcount(array)*DDJSON_arrtypesize(array);
    memcpy(dest, instance, DDJSON_arrtypesize(array));
    ++DDJSON_arrcount(array);
}

void DDJSON_arrRemove(void*& array, int index) {
    for (int i = index+1; i < DDJSON_arrcount(array); ++i) {
        char* dest   = ((char*)array) + (i-1)*DDJSON_arrtypesize(array);
        char* source = ((char*)array) + i*DDJSON_arrtypesize(array);
        memcpy(dest, source, DDJSON_arrtypesize(array));
    }
    --DDJSON_arrcount(array);
}

int DDJSON_arrFind(void*& array, void* instance) {
    for (int i = 0; i < DDJSON_arrcount(array); ++i) {
        char* testItem = ((char*)array) + i*DDJSON_arrtypesize(array);
        if (memcmp(testItem, instance, DDJSON_arrtypesize(array)) == 0) {
            return i;
        }
    }
    return -1;
}

void DDJSON_arrClear(void*& array) {
    void* newArray = DDJSON_arrMalloc(DDJSON_arrtypesize(array), 0);
    DDJSON_arrFree(array);
    array = newArray;
}

// Internal dd_string_iterator API
// --------------------------------

struct DDJSON_StringIterator {
    char* begin;
    int size;
    int at;
    int line;
    int column;
};

typedef bool (*DDJSON_StringIteratorAdvanceConditionCheck)(DDJSON_StringIterator iter);

char& DDJSON_at(DDJSON_StringIterator* iter) {
    return iter->begin[iter->at];
}

#define DDJSON_min(a, b) (a) < (b) ? (a) : (b)
#define DDJSON_max(a, b) (a) > (b) ? (a) : (b)

char& DDJSON_peek(DDJSON_StringIterator* iter, int count=1) {
    int i = iter->at+count;
    i = DDJSON_max(i, 0);
    i = DDJSON_min(i, iter->size-1);
    return iter->begin[i];
}

bool DDJSON_isWhiteSpace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool DDJSON_isNumber(char c) {
    return c >= '0' && c <= '9';
}

bool DDJSON_isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool DDJSON_isAlphaNum(char c) {
    return DDJSON_isAlpha(c) || DDJSON_isNumber(c);
}

bool DDJSON_isWhiteSpace(DDJSON_StringIterator iter) {
    return DDJSON_isWhiteSpace(DDJSON_at(&iter));
}

bool DDJSON_isNumber(DDJSON_StringIterator iter) {
    return DDJSON_isNumber(DDJSON_at(&iter));
}

bool DDJSON_isAlpha(DDJSON_StringIterator iter) {
    return DDJSON_isAlpha(DDJSON_at(&iter));
}

bool DDJSON_isAlphaNum(DDJSON_StringIterator iter) {
    return DDJSON_isAlphaNum(DDJSON_at(&iter));
}

bool DDJSON_isAtTheEnd(DDJSON_StringIterator iter) {
    return iter.at >= iter.size;
}

bool DDJSON_startsWith(DDJSON_StringIterator* iter, const char* string, int stringSize) {
    return memcmp(&DDJSON_at(iter), string, stringSize) == 0;
}

bool DDJSON_startsWith(DDJSON_StringIterator* iter, const char* string) {
    return DDJSON_startsWith(iter, string, strlen(string));
}

bool DDJSON_endsWith(DDJSON_StringIterator* iter, const char* string) {
    int stringSize = strlen(string);
    if (iter->size >= stringSize) {
        return memcmp(iter->begin + iter->size - stringSize, string, stringSize) == 0;
    } else {
        return false;
    }
}

DDJSON_StringIterator DDJSON_createIterator(const char* string) {
    DDJSON_StringIterator result = {};
    result.begin = (char*)string;
    result.size = strlen(string);
    result.line = 1;
    result.column = 1;
    return result;
}

DDJSON_StringIterator DDJSON_createIterator(const char* string, int size) {
    DDJSON_StringIterator result = {};
    result.begin = (char*)string;
    result.size = size;
    result.line = 1;
    result.column = 1;
    return result;
}

int DDJSON_advance(DDJSON_StringIterator* iter) {
    if (DDJSON_at(iter) == '\n') {
        ++iter->line;
        iter->column = 1;
    }
    
    int result = 1;
    int target = iter->at + 1;
    if (target >= iter->size) {
        target = iter->size;
        result = target - iter->at;
    }
    
    iter->at += result;
    iter->column += result;
    return result;
}

int DDJSON_advance(DDJSON_StringIterator* iter, int count) {
    int result = 0;
    for (int i = 0; i < count; ++i) {
        if (!DDJSON_advance(iter))
            return result;
        ++result;
    }
    return result;
}

int DDJSON_goback(DDJSON_StringIterator* iter, int count=1) {
    for (int i = 0; i < count; ++i) {
        if (DDJSON_at(iter) == '\n') {
            --iter->line;
            DDJSON_StringIterator it = *iter;
            while (it.at > 0 && DDJSON_peek(&it, -1) != '\n') {
                DDJSON_goback(&it);
            }
            iter->column = iter->at - it.at;
        }
    }
    
    int result = count;
    int target = iter->at + count;
    if (target < 0) {
        target = 0;
        result = target - iter->at;
    }
    
    iter->at += result;
    iter->column += result;
    return result;
}

DDJSON_StringIterator DDJSON_advanceWhileCondition(DDJSON_StringIterator* iter, DDJSON_StringIteratorAdvanceConditionCheck advanceCondition, bool include=false) {
    DDJSON_StringIterator result = {};
    result.begin = &DDJSON_at(iter);
    while (advanceCondition(*iter)) {
        ++result.size;
        DDJSON_advance(iter);
        if (DDJSON_isAtTheEnd(*iter))
            return result;
    }
    if (include && result.size) {
        ++result.size;
        DDJSON_advance(iter);
    }
    return result;
}

DDJSON_StringIterator DDJSON_advanceWhileNotCondition(DDJSON_StringIterator* iter, DDJSON_StringIteratorAdvanceConditionCheck advanceCondition, bool include=false) {
    DDJSON_StringIterator result = {};
    result.begin = &DDJSON_at(iter);
    while (!advanceCondition(*iter)) {
        ++result.size;
        DDJSON_advance(iter);
        if (DDJSON_isAtTheEnd(*iter))
            return result;
    }
    if (include && result.size) {
        ++result.size;
        DDJSON_advance(iter);
    }
    return result;
}

DDJSON_StringIterator DDJSON_advanceWhileChar(DDJSON_StringIterator* iter, char stopChar, bool include=false) {
    DDJSON_StringIterator result = {};
    result.begin = &DDJSON_at(iter);
    while (DDJSON_at(iter) == stopChar) {
        ++result.size;
        DDJSON_advance(iter);
        if (DDJSON_isAtTheEnd(*iter))
            return result;
    }
    if (include && result.size) {
        ++result.size;
        DDJSON_advance(iter);
    }
    return result;
}

DDJSON_StringIterator DDJSON_advanceWhileNotChar(DDJSON_StringIterator* iter, char stopChar, bool include=false) {
    DDJSON_StringIterator result = {};
    result.begin = &DDJSON_at(iter);
    while (DDJSON_at(iter) != stopChar) {
        DDJSON_advance(iter);
        ++result.size;
        if (DDJSON_isAtTheEnd(*iter))
            return result;
    }
    if (include && result.size) {
        ++result.size;
        DDJSON_advance(iter);
    }
    return result;
}

DDJSON_StringIterator DDJSON_advanceWhileNotString(DDJSON_StringIterator* iter, const char* delim, bool include=false) {
    DDJSON_StringIterator result = {};
    result.begin = &DDJSON_at(iter);
    int delimSize = strlen(delim);
    while (DDJSON_startsWith(iter, delim, delimSize)) {
        DDJSON_advance(iter);
        ++result.size;
        if (DDJSON_isAtTheEnd(*iter))
            return result;
    }
    if (include && result.size) {
        result.size += delimSize;
        DDJSON_advance(iter, delimSize);
    }
    return result;
}

DDJSON_StringIterator DDJSON_advanceWhileAlpha(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileCondition(iter, DDJSON_isAlpha, include);
}

DDJSON_StringIterator DDJSON_advanceWhileAlphaNum(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileCondition(iter, DDJSON_isAlphaNum, include);
}

DDJSON_StringIterator DDJSON_advanceWhileNumber(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileCondition(iter, DDJSON_isNumber, include);
}

DDJSON_StringIterator DDJSON_advanceWhileNotNumber(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileNotCondition(iter, DDJSON_isNumber, include);
}

DDJSON_StringIterator DDJSON_advanceWhileWhiteSpace(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileCondition(iter, DDJSON_isWhiteSpace, include);
}

DDJSON_StringIterator DDJSON_advanceWhileNotWhiteSpace(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileNotCondition(iter, DDJSON_isWhiteSpace, include);
}

DDJSON_StringIterator DDJSON_advanceWhileNotNewLine(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileNotChar(iter, '\n', include);
}

DDJSON_StringIterator DDJSON_advanceWhileNotNull(DDJSON_StringIterator* iter, bool include=false) {
    return DDJSON_advanceWhileNotChar(iter, '\0', include);
}

bool DDJSON_equals(DDJSON_StringIterator iter, const char* cstr) {
    return (iter.size - iter.at == (int)strlen(cstr))
        && (memcmp(iter.begin + iter.at, cstr, iter.size - iter.at) == 0);
}

double DDJSON_toDouble(DDJSON_StringIterator iter) {
    char buf[128] = {};
    memcpy(buf, iter.begin, iter.size);
    buf[iter.size] = 0;
    return atof(buf);
}

float DDJSON_toFloat(DDJSON_StringIterator iter) {
    char buf[128] = {};
    memcpy(buf, iter.begin, iter.size);
    buf[iter.size] = 0;
    return atof(buf);
}

int DDJSON_toInt(DDJSON_StringIterator iter) {
    char buf[32];
    memcpy(buf, iter.begin, iter.size);
    buf[iter.size] = 0;
    return strtol(buf, 0, 10);
}

char* DDJSON_toString(DDJSON_StringIterator iter) {
    char* string = (char*) malloc(iter.size+1);
    memcpy(string, iter.begin, iter.size);
    string[iter.size] = 0;
    return string;
}

bool DDJSON_consume(DDJSON_StringIterator* strit, char c) {
    if (DDJSON_at(strit) == c) {
        DDJSON_advance(strit);
        return true;
    }
    return false;
}

bool DDJSON_require(DDJSON_StringIterator* strit, char c) {
    if (DDJSON_at(strit) != c) {
        return false;
    }
    return true;
}

bool DDJSON_requireAndConsume(DDJSON_StringIterator* strit, char c) {
    if (!DDJSON_consume(strit, c)) {
        return false;
    }
    return true;
}

struct DDJSON_Error {
    // TODO: implement error codes
    int code;
    bool outputToStdout;
    int colored;
    char* buffer;
    int bufferSize;
    int line;
    int column;
    int charIndex;
};

void DDJSON_error(DDJSON_StringIterator* strit, bool colored, const char* errorString) {
    const char* ANSIColorReset = "";
    const char* ANSIColorRed   = "";
    const char* ANSIColorBlue  = "";
    
#ifndef DDJSON_STRING_ITERATOR_DISABLE_COLORED_OUTPUT
    if (colored) {
        ANSIColorReset = "\x1b[0m";
        ANSIColorRed   = "\x1b[31m";
        ANSIColorBlue  = "\x1b[34m";
    }
#endif
    
    printf("%s", ANSIColorRed);
    printf("Error at line %d: ", strit->line);
    printf("%s", errorString);
    printf("%s\n", ANSIColorReset);
    
    DDJSON_StringIterator iter = *strit;
    
    int contextLines = 1;
    
    int linesBack = 0;
    while (linesBack <= contextLines) {
        if (iter.at > 0) {
            if (DDJSON_peek(&iter, -1) == '\n') {
                ++linesBack;
            }
            DDJSON_goback(&iter, -1);
        } else {
            break;
        }
    }
    
    if (DDJSON_at(&iter) == '\n')
        DDJSON_advance(&iter);
    
    DDJSON_StringIterator iter2 = *strit;
    iter2 = *strit;
    DDJSON_advanceWhileNotNewLine(&iter2);
    
    int lineNumber = DDJSON_max(1, strit->line - contextLines);
    while (lineNumber < strit->line) {
        DDJSON_StringIterator line = DDJSON_advanceWhileNotNewLine(&iter);
        printf("%s%3d | %s%.*s\n", ANSIColorBlue, lineNumber, ANSIColorReset, line.size, line.begin);
        DDJSON_advance(&iter);
        ++lineNumber;
    }
    
    printf("%s%3d | %s", ANSIColorBlue, lineNumber, ANSIColorReset);
    printf("%.*s", strit->at - iter.at, &DDJSON_at(&iter));
    DDJSON_advance(&iter, strit->at - iter.at);
    
    if (DDJSON_at(&iter)) {
        printf("%s%c%s", ANSIColorRed, DDJSON_at(&iter), ANSIColorReset);
    }
    DDJSON_advance(&iter);
    
    DDJSON_StringIterator line = DDJSON_advanceWhileNotNewLine(&iter);
    printf("%.*s\n", line.size, line.begin);
}

DDJSON_Error JS_CreateError(char* buffer, int bufferSize) {
    DDJSON_Error result = {};
    result.buffer = buffer;
    result.bufferSize = bufferSize;
    return result;
}

void DDJSON_error(DDJSON_StringIterator* strit, DDJSON_Error* error, const char* errorString) {
    error->line = strit->line;
    error->column = strit->column;
    error->charIndex = strit->at;
    
    if (error->outputToStdout) {
        DDJSON_error(strit, error->colored, errorString);
    }
    
    if (error->buffer == 0 || error->bufferSize == 0)
        return;
    
    const char* ANSIColorReset = "";
    const char* ANSIColorRed   = "";
    const char* ANSIColorBlue  = "";
    
#ifndef DDJSON_STRING_ITERATOR_DISABLE_COLORED_OUTPUT
    if (error->colored) {
        ANSIColorReset = "\x1b[0m";
        ANSIColorRed   = "\x1b[31m";
        ANSIColorBlue  = "\x1b[34m";
    }
#endif
    
    char* out = error->buffer;
    int outSize = error->bufferSize;
    int o = 0;
    
    o += snprintf(out+o, outSize-o, "%s", ANSIColorRed);
    o += snprintf(out+o, outSize-o, "Error at line %d: ", strit->line);
    o += snprintf(out+o, outSize-o, "%s", errorString);
    o += snprintf(out+o, outSize-o, "%s\n", ANSIColorReset);
    
    DDJSON_StringIterator iter = *strit;
    
    int contextLines = 1;
    
    int linesBack = 0;
    while (linesBack <= contextLines) {
        if (iter.at > 0) {
            if (DDJSON_peek(&iter, -1) == '\n') {
                ++linesBack;
            }
            DDJSON_goback(&iter, -1);
        } else {
            break;
        }
    }
    
    if (DDJSON_at(&iter) == '\n')
        DDJSON_advance(&iter);
    
    DDJSON_StringIterator iter2 = *strit;
    iter2 = *strit;
    DDJSON_advanceWhileNotNewLine(&iter2);
    
    int lineNumber = DDJSON_max(1, strit->line - contextLines);
    while (lineNumber < strit->line) {
        DDJSON_StringIterator line = DDJSON_advanceWhileNotNewLine(&iter);
        o += snprintf(out+o, outSize-o, "%s%3d | %s%.*s\n", ANSIColorBlue, lineNumber, ANSIColorReset, line.size, line.begin);
        DDJSON_advance(&iter);
        ++lineNumber;
    }
    
    o += snprintf(out+o, outSize-o, "%s%3d | %s", ANSIColorBlue, lineNumber, ANSIColorReset);
    o += snprintf(out+o, outSize-o, "%.*s", strit->at - iter.at, &DDJSON_at(&iter));
    DDJSON_advance(&iter, strit->at - iter.at);
    
    if (DDJSON_at(&iter)) {
        o += snprintf(out+o, outSize-o, "%s%c%s", ANSIColorRed, DDJSON_at(&iter), ANSIColorReset);
    }
    DDJSON_advance(&iter);
    
    DDJSON_StringIterator line = DDJSON_advanceWhileNotNewLine(&iter);
    o += snprintf(out+o, outSize-o, "%.*s\n", line.size, line.begin);
}

// JSON API
// ---------

#ifndef JS__MAX_DEPTH
#define JS__MAX_DEPTH 256
#endif

typedef int JS_Flags;
JS_Flags JS_Type_None         = 0;
JS_Flags JS_Type_Dict         = 0x1;
JS_Flags JS_Type_Array        = 0x2;
JS_Flags JS_Type_String       = 0x4;
JS_Flags JS_Type_Boolean      = 0x8;
JS_Flags JS_Type_Number       = 0x10;
JS_Flags JS_Type_Integer      = 0x20;
JS_Flags JS_Type_Float        = 0x40;
JS_Flags JS_Type_Null         = 0x80;
JS_Flags JS_Option_Required   = 0x100;

struct JS_JSON;

struct JS_DictPair {
    char* key;
    JS_JSON* value;
};

struct JS_JSON {
    JS_Flags type;
    int size;
    union {
        JS_DictPair* pairs;
        JS_JSON** array;
        char* string;
        bool boolean;
        float number;
    };
    double number64;
    
    // internal
    DDJSON_StringIterator iterator;
};

void JS_Free(JS_JSON* j) {
    if (j->type == JS_Type_Dict) {
        for (int i = 0; i < DDJSON_arrcount(j->pairs); ++i) {
            JS_Free(j->pairs[i].value);
            free(j->pairs[i].key);
        }
        
        DDJSON_arrfree(j->pairs);
    } else if (j->type == JS_Type_Array) {
        for (int i = 0; i < DDJSON_arrcount(j->array); ++i) {
            JS_Free(j->array[i]);
        }
        DDJSON_arrfree(j->array);
    } else if (j->type == JS_Type_String) {
        free(j->string);
    }
    free(j);
}

void JS_Free(JS_DictPair pair) {
    free(pair.key);
    JS_Free(pair.value);
}

bool JS_IsDict(JS_JSON* j) {return j->type == JS_Type_Dict;};
bool JS_IsArray(JS_JSON* j) {return j->type == JS_Type_Array;};
bool JS_IsString(JS_JSON* j) {return j->type == JS_Type_String;};
bool JS_IsBoolean(JS_JSON* j) {return j->type == JS_Type_Boolean;};
bool JS_IsNumber(JS_JSON* j) {return j->type == JS_Type_Number;};
bool JS_IsNull(JS_JSON* j) {return j->type == JS_Type_Null;};

char* DDJSON_parseDictKey(DDJSON_StringIterator* strit, DDJSON_Error* error) {
    int startLine = strit->line;
    DDJSON_advance(strit); // skip '"'
    DDJSON_StringIterator stringContent = DDJSON_advanceWhileNotChar(strit, '"');
    if (strit->line != startLine) {
        DDJSON_error(strit, error, "a string must start and end in the same line");
        return 0;
    } else if (!DDJSON_consume(strit, '"')) {
        DDJSON_error(strit, error, "missing '\"' at the end of the string");
        return 0;
    }
    
    return DDJSON_toString(stringContent);
}

bool DDJSON_isHexDigit(char c) {
    return (c >= '0' && c <= '9')
        || (c >= 'A' && c <= 'F')
        || (c >= 'a' && c <= 'f');
}

JS_JSON* DDJSON_parseString(DDJSON_StringIterator* strit, DDJSON_Error* error) {
    DDJSON_StringIterator resultIter = *strit;
    DDJSON_advance(strit); // skip '"'
    
    DDJSON_StringIterator rawString = {};
    rawString.begin = &DDJSON_at(strit);
    rawString.at = 0;
    rawString.size = 0;
    
    int stringSize = 0;
    
    while (DDJSON_at(strit) != '"') {
        if (DDJSON_consume(strit, '\\')) {
            if (DDJSON_consume(strit, '"')
             || DDJSON_consume(strit, '\\')
             || DDJSON_consume(strit, '/')
             || DDJSON_consume(strit, 'b')
             || DDJSON_consume(strit, 'f')
             || DDJSON_consume(strit, 'n')
             || DDJSON_consume(strit, 'r')
             || DDJSON_consume(strit, 't')) {
                
            } else if (DDJSON_consume(strit, 'u')) {
                if (strit->size - strit->at >= 4) {
                    if (DDJSON_isHexDigit(DDJSON_peek(strit, 0))
                     && DDJSON_isHexDigit(DDJSON_peek(strit, 1))
                     && DDJSON_isHexDigit(DDJSON_peek(strit, 2))
                     && DDJSON_isHexDigit(DDJSON_peek(strit, 3))) {
                        char hexString[] = {'0', 'x', DDJSON_peek(strit, 0), DDJSON_peek(strit, 1), DDJSON_peek(strit, 2), DDJSON_peek(strit, 3), 0};
                        long hexValue = strtol(hexString, 0, 16);
                        if (hexValue <= 255) {
                            DDJSON_advance(strit, 4);
                        } else {
                            DDJSON_error(strit, error, "char code is greater than 255 (\\u00FF), which is not supported");
                            return 0;
                        }
                    } else {
                        DDJSON_error(strit, error, "expected 4 hexadecimal digits after \\u");
                        return 0;
                    }
                } else {
                    DDJSON_error(strit, error, "expected 4 hexadecimal digits after \\u");
                    return 0;
                }
            } else {
                DDJSON_error(strit, error, "unknown espace sequence");
                return 0;
            }
        } else if ((unsigned char) DDJSON_at(strit) <= (unsigned char) 31) {
            DDJSON_error(strit, error, "char codes below 32 (\\u0000 to \\u001F) must be escaped");
            return 0;
        } else {
            DDJSON_advance(strit);
        }
        ++stringSize;
    }
    
    rawString.size = (&DDJSON_at(strit)) - (&DDJSON_at(&rawString));
    char* string = (char*) malloc(stringSize+1);
    string[stringSize] = 0;
    char* at = string;
    
    while (!DDJSON_isAtTheEnd(rawString)) {
        if (DDJSON_consume(&rawString, '\\')) {
            if (DDJSON_consume(&rawString, '"')) {
                *at = '"';
            } else if (DDJSON_consume(&rawString, '\\')) {
                *at = '\\';
            } else if (DDJSON_consume(&rawString, '/')) {
                *at = '/';
            } else if (DDJSON_consume(&rawString, 'b')) {
                *at = '\b';
            } else if (DDJSON_consume(&rawString, 'f')) {
                *at = '\f';
            } else if (DDJSON_consume(&rawString, 'n')) {
                *at = '\n';
            } else if (DDJSON_consume(&rawString, 'r')) {
                *at = '\r';
            } else if (DDJSON_consume(&rawString, 't')) {
                *at = '\t';
            } else if (DDJSON_consume(&rawString, 'u')) {
                char hexString[] = {'0', 'x', DDJSON_peek(&rawString, 0), DDJSON_peek(&rawString, 1), DDJSON_peek(&rawString, 2), DDJSON_peek(&rawString, 3), 0};
                *at = (char) strtol(hexString, 0, 16);
                DDJSON_advance(&rawString, 4);
            }
        } else {
            *at = DDJSON_at(&rawString);
            DDJSON_advance(&rawString);
        }
        ++at;
    }
    
    DDJSON_advance(strit); // skip '"'
    
    JS_JSON* result = (JS_JSON*) malloc(sizeof(JS_JSON));
    result->type = JS_Type_String;
    result->string = string;
    result->size = strlen(string);
    result->iterator = resultIter;
    return result;
}

bool DDJSON_isInteger(double value) {
    return value - ((int) value) == 0.f;
}

bool DDJSON_isInteger(float value) {
    return value - ((int) value) == 0.f;
}

char* DDJSON_mallocAndCopyString(const char* copied) {
    int len = strlen(copied);
    char* result = (char*) malloc(len+1);
    memcpy(result, copied, len+1);
    return result;
}

JS_JSON* DDJSON_parseNumber(DDJSON_StringIterator* strit, DDJSON_Error* error) {
    JS_JSON* result = (JS_JSON*) malloc(sizeof(JS_JSON));
    result->type = JS_Type_Number;
    result->iterator = *strit;
    
    float signal = 1;
    
    if (DDJSON_consume(strit, '-')) {
        signal = -1;
    }
    
    DDJSON_StringIterator numberIt = DDJSON_advanceWhileNumber(strit);
    
    if (numberIt.size == 0) {
        DDJSON_error(strit, error, "expected a number");
        free(result);
        return 0;
    }
    
    if (numberIt.size > 1 && numberIt.begin[0] == '0') {
        strit->at -= numberIt.size;
        DDJSON_error(strit, error, "unnexpected leading zero");
        free(result);
        return 0;
    }
    
    if (DDJSON_consume(strit, '.')) {
        ++numberIt.size;
        if (DDJSON_isNumber(*strit)) {
            numberIt.size += DDJSON_advanceWhileNumber(strit).size;
        } else {
            DDJSON_error(strit, error, "expected a number");
            free(result);
            return 0;
        }
    }
    
    if (DDJSON_consume(strit, 'e') || DDJSON_consume(strit, 'E')) {
        ++numberIt.size;
        
        if (DDJSON_consume(strit, '+') || DDJSON_consume(strit, '-')) {
            ++numberIt.size;
        }
        
        if (DDJSON_isNumber(*strit)) {
            numberIt.size += DDJSON_advanceWhileNumber(strit).size;
        } else {
            DDJSON_error(strit, error, "expected a number");
            free(result);
            return 0;
        }
    }
    
    result->number = signal * DDJSON_toFloat(numberIt);
    result->number64 = signal * DDJSON_toDouble(numberIt);
    
    return result;
}

JS_JSON* DDJSON_parseDict(DDJSON_StringIterator* strit, int depth, DDJSON_Error* error);
JS_JSON* DDJSON_parseArray(DDJSON_StringIterator* strit, int depth, DDJSON_Error* error);

JS_JSON* DDJSON_parseValue(DDJSON_StringIterator* strit, int depth, DDJSON_Error* error) {
    JS_JSON* result = 0;
    if (DDJSON_at(strit) == '"') {
        result = DDJSON_parseString(strit, error);
    } else if (DDJSON_at(strit) == '{') {
        result = DDJSON_parseDict(strit, depth+1, error);
    } else if (DDJSON_at(strit) == '[') {
        result = DDJSON_parseArray(strit, depth+1, error);
    } else if (DDJSON_isNumber(*strit) || DDJSON_at(strit) == '-') {
        result = DDJSON_parseNumber(strit, error);
    } else if (DDJSON_startsWith(strit, "true") && !DDJSON_isAlpha(DDJSON_peek(strit, 4))) {
        result = (JS_JSON*) malloc(sizeof(JS_JSON));
        result->type = JS_Type_Boolean;
        result->boolean = true;
        result->iterator = *strit;
        DDJSON_advance(strit, 4);
    } else if (DDJSON_startsWith(strit, "false") && !DDJSON_isAlpha(DDJSON_peek(strit, 5))) {
        result = (JS_JSON*) malloc(sizeof(JS_JSON));
        result->type = JS_Type_Boolean;
        result->boolean = false;
        result->iterator = *strit;
        DDJSON_advance(strit, 5);
    } else if (DDJSON_startsWith(strit, "null") && !DDJSON_isAlpha(DDJSON_peek(strit, 4))) {
        result = (JS_JSON*) malloc(sizeof(JS_JSON));
        *result = {};
        result->type = JS_Type_Null;
        result->iterator = *strit;
        DDJSON_advance(strit, 4);
    } else {
        DDJSON_error(strit, error, "value is none of these: object, array, string, number, boolean or null");
    }
    return result;
}

JS_JSON* DDJSON_parseArray(DDJSON_StringIterator* strit, int depth, DDJSON_Error* error) {
    if (depth > JS__MAX_DEPTH) {
        char errorString[32];
        sprintf(errorString, "depth limit reached: %d", depth);
        DDJSON_error(strit, error, errorString);
        return 0;
    }
    
    JS_JSON* result = (JS_JSON*) malloc(sizeof(JS_JSON));
    result->type = JS_Type_Array;
    result->iterator = *strit;
    result->array = DDJSON_arralloc(JS_JSON*, 0);
    
    DDJSON_advance(strit); // skip '['
    DDJSON_advanceWhileWhiteSpace(strit);
    
    if (!DDJSON_consume(strit, ']')) {
        JS_JSON* item = DDJSON_parseValue(strit, depth, error);
        if (!item) {
            JS_Free(result);
            return 0;
        }
        DDJSON_arradd(result->array, item);
        DDJSON_advanceWhileWhiteSpace(strit);
        
        while (DDJSON_consume(strit, ',')) {
            DDJSON_advanceWhileWhiteSpace(strit);
            item = DDJSON_parseValue(strit, depth, error);
            if (!item) {
                JS_Free(result);
                return 0;
            }
            DDJSON_arradd(result->array, item);
            DDJSON_advanceWhileWhiteSpace(strit);
        }
        
        if (!DDJSON_requireAndConsume(strit, ']')) {
            JS_Free(result);
            DDJSON_error(strit, error, "expected ']' or ',' after value");
            return 0;
        }
    }
    
    result->size = DDJSON_arrcount(result->array);
    
    return result;
}

JS_DictPair DDJSON_parseDictPair(DDJSON_StringIterator* strit, int depth, DDJSON_Error* error) {
    JS_DictPair result = {};
    
    result.key = DDJSON_parseDictKey(strit, error);
    if (!result.key)
        return {};
    
    DDJSON_advanceWhileWhiteSpace(strit);
    
    if (DDJSON_requireAndConsume(strit, ':')) {
        DDJSON_advanceWhileWhiteSpace(strit);
        result.value = DDJSON_parseValue(strit, depth, error);
        
        if (!result.value) {
            free(result.key);
            result = {};
        }
    } else {
        DDJSON_error(strit, error, "expected ':' separating key and value");
        free(result.key);
        result = {};
    }
    
    return result;
}

JS_JSON* DDJSON_parseDict(DDJSON_StringIterator* strit, int depth, DDJSON_Error* error) {
    if (depth > JS__MAX_DEPTH) {
        char errorString[32];
        sprintf(errorString, "depth limit reached: %d", depth);
        DDJSON_error(strit, error, errorString);
        return 0;
    }
    
    JS_JSON* result = (JS_JSON*) malloc(sizeof(JS_JSON));
    result->type = JS_Type_Dict;
    result->iterator = *strit;
    result->pairs = DDJSON_arralloc(JS_DictPair, 0);
    
    DDJSON_advance(strit); // skip '{'
    DDJSON_advanceWhileWhiteSpace(strit);
    
    if (!DDJSON_consume(strit, '}')) {
        if (DDJSON_require(strit, '"')) {
            JS_DictPair pair = DDJSON_parseDictPair(strit, depth+1, error);
            if (!pair.key) {
                JS_Free(result);
                return 0;
            }
            DDJSON_arradd(result->pairs, pair);
            DDJSON_advanceWhileWhiteSpace(strit);
            
            while (DDJSON_consume(strit, ',')) {
                DDJSON_advanceWhileWhiteSpace(strit);
                pair = {};
                if (DDJSON_require(strit, '"')) {
                    pair = DDJSON_parseDictPair(strit, depth+1, error);
                    if (!pair.key) {
                        JS_Free(result);
                        return 0;
                    }
                    DDJSON_arradd(result->pairs, pair);
                } else {
                    JS_Free(result);
                    DDJSON_error(strit, error, "expected another pair for the dictionary");
                    return 0;
                }
                DDJSON_advanceWhileWhiteSpace(strit);
            }
            
            if (!DDJSON_requireAndConsume(strit, '}')) {
                JS_Free(result);
                DDJSON_error(strit, error, "expected '}' or ',' after value");
                return 0;
            }
        } else {
            JS_Free(result);
            DDJSON_error(strit, error, "expected double-quoted string as key");
            return 0;
        }
    }
    
    result->size = DDJSON_arrcount(result->pairs);
    
    return result;
}

JS_JSON* JS_Parse(const char* jsonString, DDJSON_Error* error) {
    DDJSON_StringIterator strit = DDJSON_createIterator(jsonString);
    
    JS_JSON* result = 0;
    
    DDJSON_advanceWhileWhiteSpace(&strit);
    if (DDJSON_at(&strit) == '{') {
        result = DDJSON_parseDict(&strit, 1, error);
    } else if (DDJSON_at(&strit) == '[') {
        result = DDJSON_parseArray(&strit, 1, error);
    } else {
        DDJSON_error(&strit, error, "expected object or array");
    }
    
    if (result) {
        DDJSON_advanceWhileWhiteSpace(&strit);
        if (!DDJSON_isAtTheEnd(strit)) {
            DDJSON_error(&strit, error, "expected end of file"); 
            JS_Free(result);
            result = 0;
        }
    }
    
    return result;
}

JS_JSON* JS_Parse(const char* jsonString) {
    DDJSON_Error error = {};
    error.outputToStdout = true;
    error.colored = true;
    
    return JS_Parse(jsonString, &error);
}

JS_JSON* JS_ParseFile(const char* filePath, DDJSON_Error* error, char** fileContent) {
    FILE* file = fopen(filePath, "rb");
    
    if (!file) {
        fprintf(stderr, "[DD_JSON] Error: failed to load file %s\n", filePath);
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* jsonString = (char*) malloc(fileSize+1);
    fread(jsonString, fileSize, 1, file);
    fclose(file);
    
    jsonString[fileSize] = 0;
    
    JS_JSON* result = JS_Parse(jsonString, error);
    
    if (!fileContent) {
        free(jsonString);
    } else {
        *fileContent = jsonString;
    }
    
    return result;
}

JS_JSON* JS_ParseFile(const char* filePath, char** fileContent) {
    DDJSON_Error error = {};
    error.outputToStdout = true;
    error.colored = true;
    
    return JS_ParseFile(filePath, &error, fileContent);
}

JS_JSON* JS_ParseFile(const char* filePath) {
    return JS_ParseFile(filePath, 0);
}

// TODO: I think it would be better if we stored the raw string, as
// well as the unescaped string, which we can do on JS_Parse,
// and then we don't have to call this, but rather just read j->rawString.
char* JS__EscapeSpecialCharacters(JS_JSON* input) {
    char* output = (char*) calloc(1, 2*input->size+1);
    
    int at = 0;
    
    for (int k = 0; k < input->size; ++k) {
        char c = input->string[k];
        
        if (c == '"' || c == '\\' /*|| c == '/'*/) {// We don't HAVE to escape forward slashes
            output[at++] = '\\';
            output[at++] = c;
        } else if (c == '\b') {
            output[at++] = '\\';
            output[at++] = 'b';
        } else if (c == '\f') {
            output[at++] = '\\';
            output[at++] = 'f';
        } else if (c == '\n') {
            output[at++] = '\\';
            output[at++] = 'n';
        } else if (c == '\r') {
            output[at++] = '\\';
            output[at++] = 'r';
        } else if (c == '\t') {
            output[at++] = '\\';
            output[at++] = 't';
        } else {
            output[at++] = c;
            // TODO: handle unicode \u
        }
    }
    
    return output;
}

void DDJSON_printSpace(FILE* file, int space) {
    char fmt[16];
    sprintf(fmt, "%%%ds", space);
    fprintf(file, fmt, "");
}

void DDJSON_recursivePrint(FILE* file, JS_JSON* j, int identation, int currentPadding) {
    if (j->type == JS_Type_Dict) {
        fprintf(file, "{");
        if (DDJSON_arrcount(j->pairs)) {
            fprintf(file, "\n");
            DDJSON_printSpace(file, currentPadding);
            fprintf(file, "\"%s\" : ", j->pairs[0].key);
            DDJSON_recursivePrint(file, j->pairs[0].value, identation, currentPadding + identation);
            for (int i = 1; i < DDJSON_arrcount(j->pairs); ++i) {
                fprintf(file, ",\n");
                DDJSON_printSpace(file, currentPadding);
                fprintf(file, "\"%s\" : ", j->pairs[i].key);
                DDJSON_recursivePrint(file, j->pairs[i].value, identation, currentPadding + identation);
            }
            fprintf(file, "\n");
            currentPadding -= identation;
            DDJSON_printSpace(file, currentPadding);
        }
        fprintf(file, "}");
    } else if (j->type == JS_Type_Array) {
        fprintf(file, "[");
        if (DDJSON_arrcount(j->array)) {
            fprintf(file, "\n");
            DDJSON_printSpace(file, currentPadding);
            DDJSON_recursivePrint(file, j->array[0], identation, currentPadding + identation);
            for (int i = 1; i < DDJSON_arrcount(j->array); ++i) {
                fprintf(file, ",\n");
                DDJSON_printSpace(file, currentPadding);
                DDJSON_recursivePrint(file, j->array[i], identation, currentPadding + identation);
            }
            fprintf(file, "\n");
            currentPadding -= identation;
            DDJSON_printSpace(file, currentPadding);
        }
        fprintf(file, "]");
    } else if (j->type == JS_Type_Number) {
        if (DDJSON_isInteger(j->number64)) {
            fprintf(file, "%.0f", j->number64);
        } else {
            fprintf(file, "%f", j->number64);
        }
    } else if (j->type == JS_Type_String) {
        char* escaped = JS__EscapeSpecialCharacters(j);
        fprintf(file, "\"%s\"", escaped);
        free(escaped);
    } else if (j->type == JS_Type_Boolean) {
        fprintf(file, "%s", j->boolean ? "true" : "false");
    } else if (j->type == JS_Type_Null) {
        fprintf(file, "null");
    }
}

void JS_Print(FILE* file, JS_JSON* j, int identation) {
    DDJSON_recursivePrint(file, j, identation, identation);
    fprintf(file, "\n");
}

void JS_Print(JS_JSON* j, int identation=2) {
    JS_Print(stdout, j, identation);
}

int DDJSON_dumpSpace(char* buffer, int bufferSize, int space) {
    if (bufferSize <= 0) return 0;
    char fmt[16];
    sprintf(fmt, "%%%ds", space);
    int result = snprintf(buffer, bufferSize, fmt, "");
    return result;
}

int DDJSON_recursiveDump(JS_JSON* j, char* buffer, int bufferSize, int identation, int currentPadding) {
    if (bufferSize <= 0) return 0;
    char* at = buffer;
    char* end = buffer+bufferSize;
    if (j->type == JS_Type_Dict) {
        at += snprintf(at, end-at, "{");
        if (DDJSON_arrcount(j->pairs)) {
            at += snprintf(at, end-at, "\n");
            at += DDJSON_dumpSpace(at, end-at, currentPadding);
            at += snprintf(at, end-at, "\"%s\" : ", j->pairs[0].key);
            at += DDJSON_recursiveDump(j->pairs[0].value, at, end-at, identation, currentPadding + identation);
            for (int i = 1; i < DDJSON_arrcount(j->pairs); ++i) {
                at += snprintf(at, end-at, ",\n");
                at += DDJSON_dumpSpace(at, end-at, currentPadding);
                at += snprintf(at, end-at, "\"%s\" : ", j->pairs[i].key);
                at += DDJSON_recursiveDump(j->pairs[i].value, at, end-at, identation, currentPadding + identation);
            }
            at += snprintf(at, end-at, "\n");
            currentPadding -= identation;
            at += DDJSON_dumpSpace(at, end-at, currentPadding);
        }
        at += snprintf(at, end-at, "}");
    } else if (j->type == JS_Type_Array) {
        at += snprintf(at, end-at, "[");
        if (DDJSON_arrcount(j->array)) {
            at += snprintf(at, end-at, "\n");
            at += DDJSON_dumpSpace(at, end-at, currentPadding);
            at += DDJSON_recursiveDump(j->array[0], at, end-at, identation, currentPadding + identation);
            for (int i = 1; i < DDJSON_arrcount(j->array); ++i) {
                at += snprintf(at, end-at, ",\n");
                at += DDJSON_dumpSpace(at, end-at, currentPadding);
                at += DDJSON_recursiveDump(j->array[i], at, end-at, identation, currentPadding + identation);
            }
            at += snprintf(at, end-at, "\n");
            currentPadding -= identation;
            at += DDJSON_dumpSpace(at, end-at, currentPadding);
        }
        at += snprintf(at, end-at, "]");
    } else if (j->type == JS_Type_Number) {
        if (DDJSON_isInteger(j->number)) {
            at += snprintf(at, end-at, "%.0f", j->number);
        } else {
            at += snprintf(at, end-at, "%f", j->number);
        }
    } else if (j->type == JS_Type_String) {
        char* escaped = JS__EscapeSpecialCharacters(j);
        at += snprintf(at, end-at, "\"%s\"", escaped);
        free(escaped);
    } else if (j->type == JS_Type_Boolean) {
        at += snprintf(at, end-at, "%s", j->boolean ? "true" : "false");
    } else if (j->type == JS_Type_Null) {
        at += snprintf(at, end-at, "null");
    }
    return at - buffer;
}

int DDJSON_recursiveDumpCompact(JS_JSON* j, char* buffer, int bufferSize) {
    if (bufferSize <= 0) return 0;
    char* at = buffer;
    char* end = buffer+bufferSize;
    if (j->type == JS_Type_Dict) {
        at += snprintf(at, end-at, "{");
        if (DDJSON_arrcount(j->pairs)) {
            at += snprintf(at, end-at, "\"%s\":", j->pairs[0].key);
            at += DDJSON_recursiveDumpCompact(j->pairs[0].value, at, end-at);
            for (int i = 1; i < DDJSON_arrcount(j->pairs); ++i) {
                at += snprintf(at, end-at, ",");
                at += snprintf(at, end-at, "\"%s\":", j->pairs[i].key);
                at += DDJSON_recursiveDumpCompact(j->pairs[i].value, at, end-at);
            }
        }
        at += snprintf(at, end-at, "}");
    } else if (j->type == JS_Type_Array) {
        at += snprintf(at, end-at, "[");
        if (DDJSON_arrcount(j->array)) {
            at += DDJSON_recursiveDumpCompact(j->array[0], at, end-at);
            for (int i = 1; i < DDJSON_arrcount(j->array); ++i) {
                at += snprintf(at, end-at, ",");
                at += DDJSON_recursiveDumpCompact(j->array[i], at, end-at);
            }
        }
        at += snprintf(at, end-at, "]");
    } else if (j->type == JS_Type_Number) {
        if (DDJSON_isInteger(j->number)) {
            at += snprintf(at, end-at, "%.0f", j->number);
        } else {
            at += snprintf(at, end-at, "%f", j->number);
        }
    } else if (j->type == JS_Type_String) {
        char* escaped = JS__EscapeSpecialCharacters(j);
        at += snprintf(at, end-at, "\"%s\"", escaped);
        free(escaped);
    } else if (j->type == JS_Type_Boolean) {
        at += snprintf(at, end-at, "%s", j->boolean ? "true" : "false");
    } else if (j->type == JS_Type_Null) {
        at += snprintf(at, end-at, "null");
    }
    return at - buffer;
}

int JS_Dump(JS_JSON* j, char* buffer, int bufferSize, int identation=2) {
    char* at = buffer;
    char* end = buffer + bufferSize;
    at += DDJSON_recursiveDump(j, at, bufferSize, identation, identation);
    at += snprintf(at, end-at, "\n");
    return at - buffer;
}

int JS_DumpCompact(JS_JSON* j, char* buffer, int bufferSize) {
    char* at = buffer;
    char* end = buffer + bufferSize;
    at += DDJSON_recursiveDumpCompact(j, at, bufferSize);
    return at - buffer;
}

JS_JSON* JS_Create(JS_Flags type=JS_Type_Dict) {
    JS_JSON* j = (JS_JSON*) malloc(sizeof(JS_JSON));
    *j = {};
    j->type = type;
    if (type == JS_Type_Dict) {
        j->pairs = DDJSON_arralloc(JS_DictPair, 0);
    } else if (type == JS_Type_Array) {
        j->array = DDJSON_arralloc(JS_JSON*, 0);
    }
    return j;
}

int JS_GetPairIndex(JS_JSON* json, const char* key) {
    if (json->type != JS_Type_Dict) {
        printf("dd_json: not a dict");
        return -1;
    }
    
    for (int i = 0; i < DDJSON_arrcount(json->pairs); ++i) {
        if (strcmp(json->pairs[i].key, key) == 0) {
            return i;
        }
    }
    
    return -1;
}

JS_DictPair* JS_GetDictPair(JS_JSON* json, const char* key) {
    int index = JS_GetPairIndex(json, key);
    if (index >= 0) {
        return &json->pairs[index];
    }
    return 0;
}

JS_DictPair* JS_ChangeKey(JS_JSON* json, const char* key, const char* replacement) {
    JS_DictPair* pair = JS_GetDictPair(json, key);
    if (pair) {
        free(pair->key);
        pair->key = DDJSON_mallocAndCopyString(replacement);
    }
    return pair;
}

JS_JSON* JS_Get(JS_JSON* json, int index) {
    if (json->type != JS_Type_Array) {
        printf("dd_json: not an array\n");
        return 0;
    }
    if (index >= DDJSON_arrcount(json->array)) {
        printf("dd_json: index is out of bounds\n");
        return 0;
    }
    return json->array[index];
}

JS_JSON* JS_Get(JS_JSON* json, const char* key) {
    if (json->type != JS_Type_Dict) {
        printf("dd_json: not a dictionary\n");
        return 0;
    }
    
    int i = JS_GetPairIndex(json, key);
    return i >= 0 ? json->pairs[i].value : 0;
}

JS_JSON* JS_GetChildOrItem(JS_JSON* json, const char* keyOrIndex) {
    if (json->type == JS_Type_Array) {
        char* c = (char*) keyOrIndex;
        bool isValidNumber = true;
        while (*c) {
            if (!DDJSON_isNumber(*c)) {
                isValidNumber = false;
                break;
            }
            ++c;
        }
        
        if (isValidNumber) {
            int index = atoi(keyOrIndex);
            return JS_Get(json, index);
        } else {
            return 0;
        }
    } else if (json->type == JS_Type_Dict) {
        return JS_Get(json, keyOrIndex);
    } else {
        fprintf(stderr, "DD_JSON: Not an array or dictionary\n");
        return 0;
    }
}

JS_JSON* JS_GetPath(JS_JSON* json, const char* path) {
    DDJSON_StringIterator iter = DDJSON_createIterator(path);
    
    DDJSON_StringIterator it = DDJSON_advanceWhileNotChar(&iter, '/');
    char* key = DDJSON_toString(it);
    JS_JSON* result = JS_GetChildOrItem(json, key);
    
    if (result) {
        while (!DDJSON_isAtTheEnd(iter)) {
            free(key);
            DDJSON_advance(&iter);
            it = DDJSON_advanceWhileNotChar(&iter, '/');
            key = DDJSON_toString(it);
            result = JS_GetChildOrItem(result, key);
            if (!result) {
                break;
            }
        }
    }
    
    free(key);
    return result;
}

char* JS_GetString(JS_JSON* json, const char* path) {
    JS_JSON* j = JS_GetPath(json, path);
    if (j && j->type == JS_Type_String) {
        return j->string;
    } else {
        return 0;
    }
}

bool JS_GetBoolean(JS_JSON* json, const char* path) {
    JS_JSON* j = JS_GetPath(json, path);
    if (j && j->type == JS_Type_Boolean) {
        return j->boolean;
    } else {
        return false;
    }
}

float JS_GetNumber(JS_JSON* json, const char* path) {
    JS_JSON* j = JS_GetPath(json, path);
    if (j && j->type == JS_Type_Number) {
        return j->number;
    } else {
        return 0;
    }
}

double JS_GetNumber64(JS_JSON* json, const char* path) {
    JS_JSON* j = JS_GetPath(json, path);
    if (j && j->type == JS_Type_Number) {
        return j->number64;
    } else {
        return 0;
    }
}

int JS_GetInt(JS_JSON* json, const char* path) {
    return (int) JS_GetNumber64(json, path);
}

void JS_Dive(JS_JSON** json, int index) {
    if ((*json)->type != JS_Type_Array) {
        printf("dd_json: not an array\n");
    } else {
        *json = JS_Get(*json, index);
    }
}

void JS_Dive(JS_JSON** json, const char* key) {
    if ((*json)->type != JS_Type_Dict) {
        printf("dd_json: not a dictionary\n");
    } else {
        *json = JS_Get(*json, key);
    }
}

void JS_Remove(JS_JSON* json, int index) {
    JS_JSON* j = JS_Get(json, index);
    if (j) {
        JS_Free(j);
        DDJSON_arrremove(json->array, index);
        json->size -= 1;
    }
}

void JS_Remove(JS_JSON* json, const char* key) {
    int i = JS_GetPairIndex(json, key);
    if (i >= 0) {
        JS_Free(json->pairs[i]);
        DDJSON_arrremove(json->pairs, i);
        json->size -= 1;
    }
}

int JS_FindInArray(JS_JSON* json, const char* value) {
    if (json->type == JS_Type_Array) {
        for (int i = 0; i < json->size; ++i) {
            if (json->array[i]->type == JS_Type_String) {
                if (strcmp(json->array[i]->string, value) == 0) {
                    return i;
                }
            }
        }
        return -1;
    } else {
        return -1;
    }
}

int JS_FindInArray(JS_JSON* json, const char* key, bool value) {
    if (json->type == JS_Type_Array) {
        for (int i = 0; i < json->size; ++i) {
            if (json->array[i]->type == JS_Type_Dict) {
                JS_JSON* j = JS_Get(json->array[i], key);
                if (j && j->type == JS_Type_Boolean && j->boolean == value) {
                    return i;
                }
            }
        }
        return -1;
    } else {
        return -1;
    }
}

int JS_FindInArray(JS_JSON* json, const char* path, int value) {
    if (json->type == JS_Type_Array) {
        for (int i = 0; i < json->size; ++i) {
            if (json->array[i]->type == JS_Type_Dict) {
                JS_JSON* j = JS_GetPath(json->array[i], path);
                if (j && j->type == JS_Type_Number && j->number == value) {
                    return i;
                }
            }
        }
        return -1;
    } else {
        return -1;
    }
}

int JS_FindInArray(JS_JSON* json, const char* path, const char* value) {
    if (json->type == JS_Type_Array) {
        for (int i = 0; i < json->size; ++i) {
            if (json->array[i]->type == JS_Type_Dict) {
                JS_JSON* j = JS_GetPath(json->array[i], path);
                if (j && j->type == JS_Type_String && strcmp(j->string, value) == 0) {
                    return i;
                }
            }
        }
        return -1;
    } else {
        return -1;
    }
}

JS_JSON* JS_Set(JS_JSON* json, const char* key, double number) {
    JS_JSON* j = JS_Get(json, key);
    if (!j) {
        JS_DictPair pair = {};
        pair.key = DDJSON_mallocAndCopyString(key);
        pair.value = JS_Create(JS_Type_Number);
        pair.value->number = number;
        DDJSON_arradd(json->pairs, pair);
        ++json->size;
        j = DDJSON_arrlast(json->pairs).value;
    } else {
        j->type = JS_Type_Number;
        j->number = number;
        j->number64 = number;
    }
    return j;
}

JS_JSON* JS_Set(JS_JSON* json, const char* key, int number) {
    return JS_Set(json, key, (float) number);
}

JS_JSON* JS_Set(JS_JSON* json, const char* key, long long int number) {
    return JS_Set(json, key, (double) number);
}

JS_JSON* JS_Set(JS_JSON* json, const char* key, const char* string) {
    JS_JSON* j = JS_Get(json, key);
    if (!j) {
        JS_DictPair pair = {};
        pair.key = DDJSON_mallocAndCopyString(key);
        pair.value = JS_Create(JS_Type_String);
        pair.value->string = DDJSON_mallocAndCopyString(string);
        pair.value->size = strlen(pair.value->string);
        DDJSON_arradd(json->pairs, pair);
        ++json->size;
        j = DDJSON_arrlast(json->pairs).value;
    } else {
        if (j->string)
            free(j->string);
        j->type = JS_Type_String;
        j->string = DDJSON_mallocAndCopyString(string);
        j->size = strlen(j->string);
    }
    return j;
}

JS_JSON* JS_SetBool(JS_JSON* json, const char* key, bool boolean) {
    JS_JSON* j = JS_Get(json, key);
    if (!j) {
        JS_DictPair pair = {};
        pair.key = DDJSON_mallocAndCopyString(key);
        pair.value = JS_Create(JS_Type_Boolean);
        pair.value->boolean = boolean;
        DDJSON_arradd(json->pairs, pair);
        ++json->size;
        j = DDJSON_arrlast(json->pairs).value;
    } else {
        j->type = JS_Type_Boolean;
        j->boolean = boolean;
    }
    return j;
}

JS_JSON* JS_SetEmpty(JS_JSON* json, const char* key, JS_Flags valueType) {
    JS_DictPair* pair = 0;
    int p = JS_GetPairIndex(json, key);
    if (p >= 0) {
        JS_Free(json->pairs[p].value);
        pair = &json->pairs[p];
    } else {
        JS_DictPair newPair = {};
        newPair.key = DDJSON_mallocAndCopyString(key);
        DDJSON_arradd(json->pairs, newPair);
        pair = &DDJSON_arrlast(json->pairs);
        ++json->size;
    }
    
    pair->value = JS_Create(valueType);
    
    return pair->value;
}

JS_JSON* JS_Set(JS_JSON* j1, const char* key, JS_JSON* j2) {
    JS_DictPair* pair = 0;
    int p = JS_GetPairIndex(j1, key);
    if (p >= 0) {
        JS_Free(j1->pairs[p].value);
        pair = &j1->pairs[p];
    } else {
        JS_DictPair newPair = {};
        newPair.key = DDJSON_mallocAndCopyString(key);
        DDJSON_arradd(j1->pairs, newPair);
        pair = &DDJSON_arrlast(j1->pairs);
        ++j1->size;
    }
    
    pair->value = j2;
    
    return pair->value;
}

// Set value of JSON array index
//-------------------------------

JS_JSON* JS_Set(JS_JSON* json, int index, float number) {
    JS_JSON* j = JS_Get(json, index);
    if (j) {
        JS_Free(j);
        j = JS_Create(JS_Type_Number);
        j->number = number;
        json->array[index] = j;
    }
    return j;
}

JS_JSON* JS_Set(JS_JSON* json, int index, int number) {
    return JS_Set(json, index, (float) number);
}

JS_JSON* JS_Set(JS_JSON* json, int index, double number) {
    return JS_Set(json, index, (float) number);
}

JS_JSON* JS_Set(JS_JSON* j1, int index, JS_JSON* j2) {
    JS_JSON* j = JS_Get(j1, index);
    if (j) {
        JS_Free(j);
        j1->array[index] = j2;
    }
    return j2;
}

JS_JSON* JS_Add(JS_JSON* json, float number) {
    if (json->type != JS_Type_Array) {
        printf("dd_json: not an array");
        return 0;
    }
    JS_JSON* j = JS_Create(JS_Type_Number);
    j->number = number;
    DDJSON_arradd(json->array, j);
    ++json->size;
    return DDJSON_arrlast(json->array);
}

JS_JSON* JS_Add(JS_JSON* json, const char* string) {
    if (json->type != JS_Type_Array) {
        printf("dd_json: not an array");
        return 0;
    }
    JS_JSON* j = JS_Create(JS_Type_String);
    j->string = DDJSON_mallocAndCopyString(string);
    DDJSON_arradd(json->array, j);
    ++json->size;
    return DDJSON_arrlast(json->array);
}

JS_JSON* JS_AddBool(JS_JSON* json, bool value) {
    if (json->type != JS_Type_Array) {
        printf("dd_json: not an array");
        return 0;
    }
    JS_JSON* j = JS_Create(JS_Type_Boolean);
    j->boolean = value;
    DDJSON_arradd(json->array, j);
    ++json->size;
    return DDJSON_arrlast(json->array);
}

JS_JSON* JS_Add(JS_JSON* json, JS_Flags valueType) {
    if (json->type != JS_Type_Array) {
        printf("dd_json: not an array");
        return 0;
    }
    JS_JSON* j = JS_Create(valueType);
    DDJSON_arradd(json->array, j);
    ++json->size;
    return j;
}

JS_JSON* JS_Add(JS_JSON* j1, JS_JSON* j2) {
    if (j1->type != JS_Type_Array) {
        printf("dd_json: not an array");
        return 0;
    }
    DDJSON_arradd(j1->array, j2);
    ++j1->size;
    return j2;
}

int JS_Count(JS_JSON* json) {
    if (json->type == JS_Type_Array) {
        return DDJSON_arrcount(json->array);
    } else if (json->type == JS_Type_Dict) {
        return DDJSON_arrcount(json->pairs);
    } else {
        printf("dd_json: json is not an array nor dict");
        return -1;
    }
}

JS_JSON* JS_Copy(JS_JSON* json) {
    JS_JSON* j = (JS_JSON*) malloc(sizeof(JS_JSON));
    if (json->type == JS_Type_Dict) {
        j->type = JS_Type_Dict;
        j->pairs = DDJSON_arralloc(JS_DictPair, DDJSON_arrcap(json->pairs));
        for (int i = 0; i < DDJSON_arrcount(json->pairs); ++i) {
            JS_DictPair pair = {};
            pair.key = DDJSON_mallocAndCopyString(json->pairs[i].key);
            pair.value = JS_Copy(json->pairs[i].value);
            DDJSON_arradd(j->pairs, pair);
        }
    } else if (json->type == JS_Type_Array) {
        j->type = JS_Type_Array;
        j->array = DDJSON_arralloc(JS_JSON*, DDJSON_arrcap(json->array));
        for (int i = 0; i < DDJSON_arrcount(json->array); ++i) {
            JS_JSON* item = JS_Copy(json->array[i]);
            DDJSON_arradd(j->array, item);
        }
    } else if (json->type == JS_Type_Number) {
        j->type = JS_Type_Number;
        j->number = json->number;
        j->number64 = json->number64;
    } else if (json->type == JS_Type_String) {
        j->type = JS_Type_String;
        j->string = DDJSON_mallocAndCopyString(json->string);
        j->size = json->size;
    } else if (json->type == JS_Type_Boolean) {
        j->type = JS_Type_Boolean;
        j->boolean = json->boolean;
    } else if (json->type == JS_Type_Null) {
        j->type = JS_Type_Null;
        j->boolean = json->boolean;
    } else {
        free(j);
        j = 0;
    }
    
    return j;
}

// JS_Reader
//---------------
enum JS_Return {
    JS_Return_FoundValid,
    JS_Return_FoundInvalid,
    JS_Return_NotFound
};

struct JS_Reader {
    const char* path;
    JS_Flags type;
    void* dest;
    
    // internal
    JS_Return r;
    char* erroneousPath;
    JS_JSON* jError;
};

int JS_GetPaths(JS_JSON* jroot, char* path, char* resolvedPath, char**& result) {
    if (path[0] == 0) {
        char* entry = DDJSON_mallocAndCopyString(resolvedPath+1);
        DDJSON_arradd(result, entry);
        return 0;
    }
    
    DDJSON_StringIterator iter = DDJSON_createIterator(path);
    DDJSON_StringIterator itUpToWildCard = DDJSON_advanceWhileNotChar(&iter, '*');
    
    if (itUpToWildCard.size == 0) {
        if (strlen(path) >= 2) {
            path += 2;
        } else {
            path += 1;
        }
                
        if (jroot->type == JS_Type_Array) {
            for (int i = 0; i < jroot->size; ++i) {
                char resolved[256] = {};
                sprintf(resolved, "%s/%d", resolvedPath, i);
                
                JS_GetPaths(jroot->array[i], path, resolved, result);
            }
        } else if (jroot->type == JS_Type_Dict) {
            for (int i = 0; i < jroot->size; ++i) {
                JS_DictPair* pair = &jroot->pairs[i];
                char resolved[256] = {};
                sprintf(resolved, "%s/%s", resolvedPath, pair->key);
                
                JS_GetPaths(pair->value, path, resolved, result);
            }
        } else {
            return 0;
        }
    } else if (itUpToWildCard.size > 1 && (jroot->type == JS_Type_Dict || jroot->type == JS_Type_Array)) {
        char newRootPath[256];
        if (DDJSON_endsWith(&itUpToWildCard, "/")) {
            sprintf(newRootPath, "%.*s", itUpToWildCard.size-1, itUpToWildCard.begin);
        } else {
            sprintf(newRootPath, "%.*s", itUpToWildCard.size, itUpToWildCard.begin);
        }
        
        sprintf(resolvedPath + strlen(resolvedPath), "/%s", newRootPath);
        
        jroot = JS_GetPath(jroot, newRootPath);
        
        if (jroot) {
            path += itUpToWildCard.size;
            
            JS_GetPaths(jroot, path, resolvedPath, result);
        }
    } else {
        // TODO INVALID PATH
    }
    
    return 0;
}

char** JS_GetPaths(JS_JSON* jroot, const char* pathTemplate) {
    char* p = (char*) pathTemplate;
    char resolvedPath[256] = {};
    char** result = DDJSON_arralloc(char*, 0);
    JS_GetPaths(jroot, p, resolvedPath, result);
    return result;
}

bool JS_Parse(JS_Reader* reader, JS_JSON* jroot) {
    bool result = true;
    int k = 0;
    while (reader[k].type) {
        JS_Reader& r = reader[k++];
        
        char** paths = JS_GetPaths(jroot, r.path);
        
        if (DDJSON_arrcount(paths)) {
            for (int i = 0; i < DDJSON_arrcount(paths); ++i) {
                JS_JSON* j = JS_GetPath(jroot, paths[i]);
                
                if (j) {
                    if (r.type & j->type || (j->type == JS_Type_Number && (r.type & JS_Type_Float || r.type & JS_Type_Integer))) {
                        if (r.dest) {
                            if (r.type & JS_Type_String) {
                                strcpy((char*) r.dest, j->string);
                            } else if (r.type & JS_Type_Integer) {
                                *(int*) r.dest = (int) j->number;
                            } else if (r.type & JS_Type_Float) {
                                *(float*) r.dest = (float) j->number;
                            } else if (r.type & JS_Type_Boolean) {
                                *(bool*) r.dest = j->boolean;
                            }
                        }
                        
                        r.r = JS_Return_FoundValid;
                    } else {
                        r.r = JS_Return_FoundInvalid;
                        r.erroneousPath = DDJSON_mallocAndCopyString(paths[i]);
                        r.jError = j;
                        result = false;
                        break;
                    }
                } else {
                    r.r = JS_Return_NotFound;
                    if (r.type & JS_Option_Required) {
                        r.erroneousPath = DDJSON_mallocAndCopyString(paths[i]);
                        r.jError = j;
                        result = false;
                        break;
                    }
                }
            }
        } else {
            r.r = JS_Return_NotFound;
            if (r.type & JS_Option_Required) {
                r.erroneousPath = DDJSON_mallocAndCopyString(r.path);
                r.jError = jroot;
                result = false;
                break;
            }
        }
        
        if (!result) {
            break;
        }
        
        for (int i = 0; i < DDJSON_arrcount(paths); ++i) {
            free(paths[i]);
        }
        DDJSON_arrfree(paths);
    }
    
    return result;
}

bool JS_ParseFile(JS_Reader* reader, const char* filePath) {
    JS_JSON* jroot = JS_ParseFile(filePath);
    bool result = true;
    if (jroot) {
        result = JS_Parse(reader, jroot);
        JS_Free(jroot);
    } else {
        result = false;
    }
    return result;
}

const char* JS_ValueRequiredString(int type) {
    if (type & JS_Type_String) {
        return "STRING";
    } else if (type & JS_Type_Number) {
        return "NUMBER";
    } else if (type & JS_Type_Integer) {
        return "INTEGER";
    } else if (type & JS_Type_Float) {
        return "FLOAT";
    } else if (type & JS_Type_Boolean) {
        return "BOOLEAN";
    } else if (type & JS_Type_Array) {
        return "ARRAY";
    } else if (type & JS_Type_Dict) {
        return "DICTIONARY";
    } else if (type & JS_Type_Null) {
        return "NULL";
    }
    return "";
}

DDJSON_Error JS_GetError(JS_Reader* reader, char* buffer, int bufferSize) {
    DDJSON_Error result = {};
    result.buffer = buffer;
    result.bufferSize = bufferSize;
    
    int k = 0;
    while (reader[k].type) {
        JS_Reader& r = reader[k++];
        
        if (r.r == JS_Return_FoundInvalid) {
            sprintf(buffer, "Expected '%s' to be of type %s, but got %s", r.erroneousPath, JS_ValueRequiredString(r.type), JS_ValueRequiredString(r.jError->type));
            break;
        } else if (r.type & JS_Option_Required && r.r == JS_Return_NotFound) {
            sprintf(buffer, "Required %s field is missing: '%s'", JS_ValueRequiredString(r.type), r.erroneousPath);
            break;
        }
    }
    
    return result;
}

void JS_PrintError(JS_Reader* reader) {
    char buffer[512];
    
    int k = 0;
    while (reader[k].type) {
        JS_Reader& r = reader[k++];
        
        if (r.r == JS_Return_FoundInvalid) {
            sprintf(buffer, "Value has to be of type %s, but it is of type %s", JS_ValueRequiredString(r.type), JS_ValueRequiredString(r.jError->type));
            
            DDJSON_error(&r.jError->iterator, true, buffer);
            break;
        } else if (r.type & JS_Option_Required && r.r == JS_Return_NotFound) {
            fprintf(stderr, "Missing %s at JSON path \"%s\"'\n", JS_ValueRequiredString(r.type), r.erroneousPath);
            break;
        }
    }
}

void JS_Print(JS_Reader* reader) {
    int k = 0;
    while (reader[k].type) {
        JS_Reader& r = reader[k++];
        if (r.r == JS_Return_FoundValid && r.dest) {
            if (r.type & JS_Type_String) {
                printf("%s: %s\n", r.path, (char*) r.dest);
            } else if (r.type & JS_Type_Integer) {
                printf("%s: %d\n", r.path, *(int*) r.dest);
            } else if (r.type & JS_Type_Float) {
                printf("%s: %.2f\n", r.path, *(float*) r.dest);
            } else if (r.type & JS_Type_Boolean) {
                printf("%s: %s\n", r.path, *(bool*) r.dest ? "true" : "false");
            }
        } else if (r.r == JS_Return_FoundInvalid) {
            printf("%s: invalid!\n", r.path);
        }
    }
}

// ITERATOR
// ITERATOR
// ITERATOR

struct JS_Iterator {
    JS_JSON* over;
    int index;
    char* key;
    union {
        JS_JSON* item;
        JS_JSON* value;
    };
};

JS_Iterator JS_ForEach(JS_JSON* j) {
    JS_Iterator iter = {};
    iter.over = j;
    iter.index = -1;
    return iter;
}

bool JS_Next(JS_Iterator* it) {
    if (!it->over) return false;
    
    JS_Flags type = it->over->type;
    if (type != JS_Type_Array && type != JS_Type_Dict) return false;
    
    ++it->index;
    if (it->index < it->over->size) {
        if (type == JS_Type_Array) {
            it->item = it->over->array[it->index];
        } else {
            it->key = it->over->pairs[it->index].key;
            it->value = it->over->pairs[it->index].value;
        }
        return true;
    } else {
        return false;
    }
}

JS_JSON* JS_Unwrap(JS_Iterator it) {
    return it.item;
}


#endif







