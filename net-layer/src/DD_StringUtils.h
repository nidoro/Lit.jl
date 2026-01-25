
#ifndef DD_STRING_UTILS_H
#define DD_STRING_UTILS_H

#include "stdio.h"
#include "DD_Array.h"
#include <ctype.h>

#define SU_Min(a, b) ((a) < (b)) ? (a) : (b)
#define SU_Max(a, b) ((a) > (b)) ? (a) : (b)

struct SU_Replacement {
    const char* replaced;
    const char* replacement;
    int replacedSize;
    int replacementSize;
};

int SU_Concatenate(char* buffer, char** strings, int stringCount, const char* separator) {
    int k = 0;
    int separatorLength = strlen(separator);
    int i = 0;
    for (i = 0; i < stringCount-1; ++i) {
        int length = strlen(strings[i]);
        for (int c = 0; c < length; ++c) {
            buffer[k++] = strings[i][c];
        }
        memcpy(&buffer[k], separator, separatorLength);
        k += separatorLength;
    }

    int length = strlen(strings[i]);
    for (int c = 0; c < length; ++c) {
        buffer[k++] = strings[i][c];
    }
    buffer[k] = 0;
    return k;
}

int SU_Concatenate(char* buffer, char** strings, const char* separator) {
    return SU_Concatenate(buffer, strings, arrcount(strings), separator);
}

bool SU_Contains(char* array, int arraySize, char value) {
    for (int i = 0; i < arraySize; ++i) {
        if (array[i] == value)
            return true;
    }
    return false;
}

bool SU_Contains(char** array, int arraySize, char* string) {
    for (int i = 0; i < arraySize; ++i) {
        if (strcmp(array[i], string) == 0) {
            return true;
        }
    }
    return false;
}

bool SU_Contains(char** array, char* string) {
    return SU_Contains(array, arrcount(array), string);
}

#define SU_UPPERCASE_CHAR_SET "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define SU_LOWERCASE_CHAR_SET "abcdefghijklmnopqrstuvwxyz"
#define SU_NUMBER_CHAR_SET "0123456789"
#define SU_ALPHA_NUM_CHAR_SET SU_UPPERCASE_CHAR_SET SU_LOWERCASE_CHAR_SET SU_NUMBER_CHAR_SET

bool SU_ContainsInvalid(char* string, const char* validSet) {
    int validSetSize = strlen(validSet);
    
    int i = 0;
    while (string[i]) {
        if (!SU_Contains((char*) validSet, validSetSize, string[i])) {
            return true;
        }
        
        ++i;
    }
    return false;
}

int SU_Replace(char* outBuffer, char* inBuffer, int inLength, SU_Replacement* reps) {
    int r = 0;
    while (reps[r].replaced) {
        reps[r].replacedSize = strlen(reps[r].replaced);
        reps[r].replacementSize = strlen(reps[r].replacement);
        ++r;
    }
            
    int i = 0;
    int j = 0;
    while (i < inLength) {
        r = 0;
        int replaced = false;
        while (reps[r].replaced) {
            if (i <= inLength - reps[r].replacedSize && memcmp(inBuffer+i, reps[r].replaced, reps[r].replacedSize) == 0) {
                memcpy(outBuffer+j, reps[r].replacement, reps[r].replacementSize);
                i += reps[r].replacedSize;
                j += reps[r].replacementSize;
                replaced = true;
                break;
            }
            ++r;
        }
        
        if (!replaced) {
            outBuffer[j] = inBuffer[i];
            ++j;
            ++i;
        }
    }
    
    outBuffer[j] = 0;
    return j;
}

int SU_Replace(char* outBuffer, char* inBuffer, int inLength, const char* searchedString, int searchedLength, const char* replacementString, int replacementLength) {
    int i = 0;
    int j = 0;
    while (i < inLength) {
        if (i <= inLength - searchedLength && memcmp(&inBuffer[i], searchedString, searchedLength) == 0) {
            memcpy(&outBuffer[j], replacementString, replacementLength);
            i += searchedLength;
            j += replacementLength;
        } else {
            outBuffer[j] = inBuffer[i];
            ++j;
            ++i;
        }
    }
    outBuffer[j] = 0;
    return j;
}

int SU_Replace(char* outBuffer, char* inBuffer, int inLength, const char* searchedString, const char* replacementString) {
    int searchedLength = strlen(searchedString);
    int replacementLength = strlen(replacementString);
    return SU_Replace(outBuffer, inBuffer, inLength, searchedString, searchedLength, replacementString, replacementLength);
}

bool SU_IsWhiteSpace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

char** SU_Split(const char* string, int len, char separator, int minLineCap, int allocatorIndex) {
    int estimateLineCount = (len / (minLineCap+1)) + 1;
    char* currentString = arrallocate(char, minLineCap, allocatorIndex);
    char terminator = 0;
    char** strings = arrallocate(char*, estimateLineCount, allocatorIndex);
    char* at = (char*) string;
    for (int i = 0; i < len; ++i) {
        if (*at == separator) {
            arradd(currentString, terminator);
            --arrcount(currentString);
            arradd(strings, currentString);
            currentString = arrallocate(char, minLineCap, allocatorIndex);
        } else {
            arradd(currentString, *at);
        }
        ++at;
    }
    
    arradd(currentString, terminator);
    --arrcount(currentString);
    arradd(strings, currentString);

    return strings;
}

char** SU_Split(const char* string, int len, char separator) {
    return SU_Split(string, len, separator, 0, 0);
}

char** SU_SplitWithEstimate(const char* string, int len, char separator, int minLineCap, int stringCountEstimate) {
    char* currentString = arralloc(char, minLineCap);
    char terminator = 0;
    char** strings = arralloc(char*, stringCountEstimate);
    char* at = (char*) string;
    for (int i = 0; i < len; ++i) {
        if (*at == separator) {
            arradd(currentString, terminator);
            --arrcount(currentString);
            arradd(strings, currentString);
            currentString = arralloc(char, minLineCap);
        } else {
            arradd(currentString, *at);
        }
        ++at;
    }
    
    arradd(currentString, terminator);
    --arrcount(currentString);
    arradd(strings, currentString);

    return strings;
}

char** SU_SplitWithEstimate(const char* string, int len, char separator, int stringCountEstimate) {
    return SU_SplitWithEstimate(string, len, separator, 0, stringCountEstimate);
}
    
void SU_FreeArrayOfStrings(char** strings) {
    for (int i = 0; i < arrcount(strings); ++i) {
        arrfree(strings[i]);
    }
    arrfree(strings);
}

void SU_PrintArrayOfStrings(char** strings) {
    for (int i = 0; i < arrcount(strings); ++i) {
        printf("%s\n", strings[i]);
    }
}

bool SU_StartsWith(char* testString, const char* prefix) {
    if (strlen(testString) < strlen(prefix)) {
        return false;
    }
    
    int i = 0;
    while (testString[i] && prefix[i]) {
        if (testString[i] != prefix[i])
            return false;
        ++i;
    }

    return true;
}

bool SU_EndsWith(char* testString, const char* suffix) {
    int i = strlen(testString) - strlen(suffix);
    if (i >= 0) {
        return SU_StartsWith(testString+i, suffix);
    }
    return false;
}

int SU_GetCharOccurrencesCount(char* string, char c) {
    int count = 0;
    char* s = string;
    while (*s) {
        if (*s == c) {
            ++count;
        }
        ++s;
    }
    return count;
}

int SU_GetCharOccurrencesCount(char* haystack, const char* needle) {
    int result = 0;
    char* s = haystack;
    int needleSize = strlen(needle);
    
    while (*s) {
        if (memcmp(s, needle, needleSize)==0) {
            ++result;
            s += needleSize;
        } else {
            ++s;
        }
    }
    return result;
}

int SU_Find(char* string, const char* searchedString) {
    int stringSize = strlen(string);
    int searchedStringSize = strlen(searchedString);
    for (int i = 0; i <= stringSize - searchedStringSize; ++i) {
        if (SU_StartsWith(string, searchedString)) {
            return i;
        }
        ++string;
    }
    return -1;
}

int SU_Find(char** array, int arraySize, const char* searchedString) {
    for (int i = 0; i < arraySize; ++i) {
        if (strcmp(array[i], searchedString) == 0) {
            return i;
        }
    }
    return -1;
}

void SU_CopyUntil(char* dest, char* source, const char* delim, bool include=false) {
    int k = 0;
    while (source[k]) {
        if (SU_StartsWith(source+k, delim)) {
            break;
        }
        ++k;
    }
    
    memcpy(dest, source, k);
    
    if (include) {
        strcpy(dest+k, delim);
    } else {
        dest[k] = 0;
    }
}

bool SU_AreStringsEqual(const char* a, const char* b) {
    int aSize = strlen(a);
    int bSize = strlen(b);
    if (aSize == bSize) {
        return (memcmp(a, b, aSize) == 0);
    } else {
        return false;
    }
}

bool SU_IsStringGreater(const char* a, const char* b) {
    int aSize = strlen(a);
    int bSize = strlen(b);
    int compareCount = SU_Min(aSize, bSize);
    for (int i = 0; i < compareCount; ++i) {
        if (a[i] > b[i]) {
            return true;
        } else if (b[i] > a[i]) {
            return false;
        }
    }
    return false;
}

void SU_Capitalize(char* string) {
    char* c = string;
    while (*c) {
        *c = toupper(*c);
        ++c;
    }
}

bool SU_IsEmpty(char* string) {
    return string[0] == 0;
}

const char* SU_GetEmptyIfNull(const char* string) {
    return string ? string : "";
}

void SU_ToLower(char* output, const char* string) {
    int i;
    for(i = 0; string[i]; i++){
        output[i] = tolower(string[i]);
    }
    output[i] = 0;
}

void SU_ToLower(char* string) {
    SU_ToLower(string, (const char*) string);
}

#endif
