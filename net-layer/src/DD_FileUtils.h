#ifndef DD_FILE_UTILS_H
#define DD_FILE_UTILS_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

// Assert placeholder if needed
//------------------------------
#ifndef DD_Assert
  #define DD_Assert(...)
  #define DD_Assert1(...)
  #define DD_Assert2(...)
#endif

// PRIMITIVES INTERNAL TYPEDEFS
//------------------------------
#include <stdint.h>

// 8 bits integer types
typedef int8_t  FU_s8;
typedef uint8_t FU_u8;

// 16 bits integer types
typedef int16_t  FU_s16;
typedef uint16_t FU_u16;

// 32 bits integer types
typedef int32_t  FU_s32;
typedef uint32_t FU_u32;

// 64 bits integer types
typedef int64_t  FU_s64;
typedef uint64_t FU_u64;

// Real types
typedef float  FU_f32;
typedef double FU_f64;

#define FU_s32_max 2147483647
#define FU_u32_max 4294967295

#define FU_s64_max 9223372036854775807
#define FU_u64_max 18446744073709551615

//------------
// UTF8 UTILS
//------------
char* FU_GetFirstInvalidUTF8Sequence(const char* str, int len) {
    char* s = (char*) str;
    
    while ('\0' != *s) {
      if (0xf0 == (0xf8 & *s)) {
        // ensure each of the 3 following bytes in this 4-byte
        // utf8 codepoint began with 0b10xxxxxx
        if ((0x80 != (0xc0 & s[1])) || (0x80 != (0xc0 & s[2])) ||
            (0x80 != (0xc0 & s[3]))) {
          return s;
        }
    
        // ensure that our utf8 codepoint ended after 4 bytes
        if (0x80 == (0xc0 & s[4])) {
          return s;
        }
    
        // ensure that the top 5 bits of this 4-byte utf8
        // codepoint were not 0, as then we could have used
        // one of the smaller encodings
        if ((0 == (0x07 & s[0])) && (0 == (0x30 & s[1]))) {
          return s;
        }
    
        // 4-byte utf8 code point (began with 0b11110xxx)
        s += 4;
      } else if (0xe0 == (0xf0 & *s)) {
        // ensure each of the 2 following bytes in this 3-byte
        // utf8 codepoint began with 0b10xxxxxx
        if ((0x80 != (0xc0 & s[1])) || (0x80 != (0xc0 & s[2]))) {
          return s;
        }
    
        // ensure that our utf8 codepoint ended after 3 bytes
        if (0x80 == (0xc0 & s[3])) {
          return s;
        }
    
        // ensure that the top 5 bits of this 3-byte utf8
        // codepoint were not 0, as then we could have used
        // one of the smaller encodings
        if ((0 == (0x0f & s[0])) && (0 == (0x20 & s[1]))) {
          return s;
        }
    
        // 3-byte utf8 code point (began with 0b1110xxxx)
        s += 3;
      } else if (0xc0 == (0xe0 & *s)) {
        // ensure the 1 following byte in this 2-byte
        // utf8 codepoint began with 0b10xxxxxx
        if (0x80 != (0xc0 & s[1])) {
          return s;
        }
    
        // ensure that our utf8 codepoint ended after 2 bytes
        if (0x80 == (0xc0 & s[2])) {
          return s;
        }
    
        // ensure that the top 4 bits of this 2-byte utf8
        // codepoint were not 0, as then we could have used
        // one of the smaller encodings
        if (0 == (0x1e & s[0])) {
          return s;
        }
    
        // 2-byte utf8 code point (began with 0b110xxxxx)
        s += 2;
      } else if (0x00 == (0x80 & *s)) {
        // 1-byte ascii (began with 0b0xxxxxxx)
        s += 1;
      } else {
        // we have an invalid 0b1xxxxxxx utf8 code point entry
        return s;
      }
    }
    
    if (len >= 0) {
        int l = s - str;
        return l == len ? 0 : s;
    } else {
        return 0;
    }
}

char* FU_GetFirstInvalidUTF8Sequence(const char* str) {
    return FU_GetFirstInvalidUTF8Sequence(str, -1);
}

bool FU_IsUTF8Valid(const char* str) {
    return FU_GetFirstInvalidUTF8Sequence(str) == 0;
}

bool FU_IsUTF8Valid(const char* str, int len) {
    return FU_GetFirstInvalidUTF8Sequence(str, len) == 0;
}

//----------------------------
// ENTIRE FILE IMPLEMENTATION
//----------------------------
struct FU_Allocator {
    void* (*malloc)(void* data, unsigned long long amount);
    void (*free)(void* data, void* memory);
    void* data;
};

void* FU__callocCaller(void* data, unsigned long long amount) {
    return calloc(1, amount);
}

void FU__freeCaller(void* data, void* memory) {
    return free(memory);
}

FU_Allocator FU__DefaultAllocator = {
    FU__callocCaller,
    FU__freeCaller
};

struct FU_EntireFile {
    char* content;
    unsigned long long size;
    FU_Allocator allocator;
};

void FU_Free(FU_EntireFile entireFile) {
    entireFile.allocator.free(entireFile.allocator.data, entireFile.content);
    entireFile.content = 0;
    entireFile.size = 0;
}

FILE* FU_OpenFileVA(const char* mode, const char* formatString, va_list argList) {
    char path[2048] = {};
    vsprintf(path, formatString, argList);
    FILE* file = fopen(path, mode);
    return file;
}

FILE* FU_OpenFile(const char* mode, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FILE* f = FU_OpenFileVA(mode, formatString, argList);
    va_end(argList);
    return f;
}

void FU_CloseFile(FILE* file) {
    fclose(file);
}

void FU_GetRandomString(char* result, int size) {
    FILE* devurandom = fopen("/dev/urandom", "rb");
    fread(result, size, 1, devurandom);
    fclose(devurandom);

    const char* charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
    int charSetLen = strlen(charSet);
    
    for (int i = 0; i < size; ++i) {
        int idx = ((unsigned int) result[i]) % charSetLen;
        result[i] = charSet[idx];
    }
    
    result[size] = 0;
}

FU_EntireFile FU_LoadEntireFile(FILE* file, FU_Allocator allocator = FU__DefaultAllocator) {
    FU_EntireFile entireFile = {};
    
    if (fseek(file, 0, SEEK_END) == -1) {
        DD_Assert2(0, "Failed fseek: %s (errno=%d)", strerror(errno), errno);
    }
    
    int fsize = ftell(file);
    
    if (fseek(file, 0, SEEK_SET) == -1) {
        DD_Assert2(0, "Failed fseek: %s (errno=%d)", strerror(errno), errno);
    }
    
    entireFile.content = (char*) allocator.malloc(allocator.data, fsize+1);
    fread(entireFile.content, fsize, 1, file);

    entireFile.content[fsize] = 0;
    entireFile.size = fsize;
    entireFile.allocator = allocator;
    return entireFile;
}

FU_EntireFile FU_LoadEntireFileVA(FU_Allocator allocator, const char* formatString, va_list argList) {
    FILE* f = FU_OpenFileVA("rb", formatString, argList);
    
    if (!f) {
        return {};
    } else {
        FU_EntireFile result = FU_LoadEntireFile(f, allocator);
        fclose(f);
        return result;
    }
}

FU_EntireFile FU_LoadEntireFile(const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FU_EntireFile result = FU_LoadEntireFileVA(FU__DefaultAllocator, formatString, argList);
    va_end(argList);
    return result;
}

FU_EntireFile FU_LoadEntireFile(FU_Allocator allocator, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FU_EntireFile result = FU_LoadEntireFileVA(allocator, formatString, argList);
    va_end(argList);
    return result;
}

FILE* FU_OpenPipeVA(const char* mode, const char* formatString, va_list argList) {
    char path[2048] = {};
    vsprintf(path, formatString, argList);
    FILE* file = popen(path, mode);
    return file;
}

FILE* FU_OpenPipe(const char* mode, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FILE* f = FU_OpenPipeVA(mode, formatString, argList);
    va_end(argList);
    return f;
}

bool FU_GetLastLine(char* buffer, int bufferSize, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FILE* file = FU_OpenFileVA("rb", formatString, argList);
    va_end(argList);
    
    
    if (file) {
        fseek(file, 0, SEEK_END);
        signed long long fileSize = ftell(file);
        
        signed long long at = fileSize - 1;
        
        int charsWritten = 0;
        bool foundLineBreak = false;
        
        char buf[1024] = {};
        
        while (at > 0 && !foundLineBreak) {
            at -= sizeof(buf) - 1;
            int readSize = sizeof(buf) - 1;
            
            if (at < 0) {
                readSize += at;
                at = 0;
            }
            
            fseek(file, at, SEEK_SET);
            fread(buf, readSize, 1, file);
            
            buf[readSize] = 0;
            
            for (int i = readSize-1; i >= 0; --i) {
                if (buf[i] == '\n') {
                    foundLineBreak = true;
                    break;
                } else {
                    buffer[charsWritten] = buf[i];
                    ++charsWritten;
                    
                    if (charsWritten >= bufferSize) {
                        foundLineBreak = true;
                        break;
                    }
                }
            }
        }
        
        for (int i = 0; i < charsWritten / 2; ++i) {
            int j = charsWritten - i - 1;
            char aux = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = aux;
        }
        
        buffer[charsWritten] = 0;
        
        fclose(file);
    } else {
        return false;
    }
    
    return true;
}

// Pipe utils
//-------------
FU_EntireFile FU_LoadEntirePipe(FILE* file, FU_Allocator allocator = FU__DefaultAllocator) {
    FU_EntireFile entireFile = {};
    
    unsigned long long cap = 2048;
    entireFile.allocator = allocator;
    entireFile.content = (char*) allocator.malloc(allocator.data, cap);
    
    char buf[2048] = {};
    while (fgets(buf, sizeof(buf), file)) {
        int readCount = strlen(buf);
        
        if (entireFile.size + readCount >= cap) {
            cap += 2048;
            char* newContent = (char*) allocator.malloc(allocator.data, cap);
            memcpy(newContent, entireFile.content, entireFile.size);
            memcpy(newContent + entireFile.size, buf, readCount);
            unsigned long long newSize = entireFile.size + readCount;
            newContent[newSize] = 0;
            
            FU_Free(entireFile);
            entireFile.content = newContent;
            entireFile.size = newSize;
        } else {
            memcpy(entireFile.content + entireFile.size, buf, readCount);
            unsigned long long newSize = entireFile.size + readCount;
            entireFile.size = newSize;
            entireFile.content[newSize] = 0;
        }
    }
    
    return entireFile;
}

FU_EntireFile FU_LoadEntirePipeVA(FU_Allocator allocator, const char* formatString, va_list argList) {
    FILE* f = FU_OpenPipeVA("r", formatString, argList);
    
    if (!f) {
        return {};
    } else {
        FU_EntireFile result = FU_LoadEntirePipe(f, allocator);
        pclose(f);
        return result;
    }
}

FU_EntireFile FU_LoadEntirePipe(const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FU_EntireFile result = FU_LoadEntirePipeVA(FU__DefaultAllocator, formatString, argList);
    va_end(argList);
    return result;
}

FU_EntireFile FU_LoadEntirePipe(FU_Allocator allocator, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    FU_EntireFile result = FU_LoadEntirePipeVA(allocator, formatString, argList);
    va_end(argList);
    return result;
}

signed long long FU_GetFileSize(const char* formatString, ...) {
    char path[2048];
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    signed long long result = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    fclose(f);
    
    return result;
}

bool FU_Append(const char* content, const char* formatString, ...) {
    char path[2048];
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    
    FILE* file = fopen(path, "a");
    if (!file) return false;
    
    fwrite(content, 1, strlen(content), file);
    
    fclose(file);
    
    return true;
}

bool FU_IsFileReadable(const char* formatString, ...) {
    char path[2048];
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    
    FILE* f = fopen(path, "r");
    bool result = f;
    if (f) fclose(f);
    return result;
}

bool FU_IsDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

bool FU_IsRegularFile(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) return false;
    return S_ISREG(path_stat.st_mode);
}

bool FU_PathExists(const char *path) {
    struct stat path_stat;
    return stat(path, &path_stat) == 0;
}

bool FU_DeleteFile(const char *path) {
    return remove(path) == 0;
}

FILE* FU_OpenTempFile(char* pathReturned) {
    char buf[64] = {};
    char* path = pathReturned;
    if (!path) path = buf;
    
    do {
        char fileName[32] = {};
        FU_GetRandomString(fileName, sizeof(fileName)-1);
        sprintf(path, "/tmp/%s", fileName);
    } while (FU_IsRegularFile(path));
    
    FILE* result = fopen(path, "w+");
    
    return result;
}

FILE* FU_WriteTempFile(char* content, char* pathReturned) {
    FILE* file = FU_OpenTempFile(pathReturned);
    fwrite(content, 1, strlen(content), file);
    return file;
}

bool FU_WriteEntireFile(char* path, char* content, int contentSize) {
    FILE* file = FU_OpenFile("wb", path);
    if (file) {
        fwrite(content, 1, contentSize, file);
        fclose(file);
        return true;
    } else {
        return false;
    }
}

// PATH
//-------
void FU_FixDirectoryPath(char* path) {
    int size = strlen(path);
    while (size > 0 && (path[size-1] == '/' || path[size-1] == ' ')) {
        path[--size] = 0;
    }
}

char* FU_GetBaseNameStart(char* path) {
    int at = strlen(path)-1;
    for ( ; at >= 0; --at) {
        if (path[at] == '/') {
            break;
        }
    }
    return path + at + 1;
}

void FU_GetBaseNameWithoutExtension(char* result, char* path) {
    char* start = FU_GetBaseNameStart(path);
    
    int pathSize = strlen(path);
    int at = pathSize-1;
    for ( ; at >= 0; --at) {
        if (path[at] == '.') {
            break;
        }
    }
    
    strncpy(result, start, strlen(start) - (pathSize - at));
}

void FU_GetDirName(char* path, char* result) {
    char* basenameStart = FU_GetBaseNameStart(path);
    result[0] = 0;
    int resultLen = basenameStart - path;
    if (resultLen > 0) {
        memcpy(result, path, resultLen);
        result[resultLen] = 0;
    }
}

// FILE READER IMPLEMENTATION
// ----------------------------
struct FU_FileReader {
    char* content;
    int size;
    int at;
    bool error;
};

bool FU_IsWhiteSpace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool FU_IsNumber(char c) {
    return c >= '0' && c <= '9';
}

bool FU_IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool FU_IsAlphaNum(char c) {
    return FU_IsAlpha(c) || FU_IsNumber(c);
}

void skipWhiteSpace(FU_FileReader* reader) {
    while (FU_IsWhiteSpace(reader->content[reader->at])) {
        ++reader->at;
        if (reader->at >= reader->size)
            break;
    }
}

void skipEntireLine(FU_FileReader* reader) {
    while (reader->content[reader->at] != '\n') {
        ++reader->at;
        if (reader->at >= reader->size)
            break;
    }
    ++reader->at;
}

void skipUntilCharIsReached(FU_FileReader* reader, char stopChar) {
    while (reader->content[reader->at] != stopChar) {
        ++reader->at;
        if (reader->at >= reader->size)
            break;
    }
    ++reader->at;
}

char* at(FU_FileReader* reader) {
    return reader->content + reader->at;
}

char charAt(FU_FileReader* reader) {
    return reader->content[reader->at];
}

void advance(FU_FileReader* reader, int count = 1) {
    int target = reader->at + count;
    reader->at = target < reader->size ? target : reader->size;
}

bool compare(FU_FileReader reader, const char* cstr) {
    return (reader.size - reader.at == (int)strlen(cstr))
        && (memcmp(reader.content + reader.at, cstr, reader.size - reader.at) == 0);
}

FU_FileReader readUntilCharIsReached(FU_FileReader* reader, char stopChar) {
    FU_FileReader token = {};
    token.content = &reader->content[reader->at];
    token.size = 0;
    while (reader->content[reader->at] != stopChar) {
        ++reader->at;
        ++token.size;
        if (reader->at >= reader->size)
            return token;
    }
    return token;
}

FU_FileReader readRemainder(FU_FileReader* reader) {
    return readUntilCharIsReached(reader, '\0');
}

FU_FileReader readNextInteger(FU_FileReader* reader) {
    FU_FileReader token = {};
    while (!FU_IsNumber(reader->content[reader->at])) {
        ++reader->at;
        if (reader->at >= reader->size) {
            return token;
        }
    }
    
    token.content = &reader->content[reader->at];
    token.size = 1;
    
    reader->at++;
    
    while (FU_IsNumber(reader->content[reader->at])) {
        ++reader->at;
        ++token.size;
        if (reader->at >= reader->size) {
            return token;
        }
    }

    return token;
}

int getNextInteger(FU_FileReader* reader) {
    FU_FileReader token = readNextInteger(reader);
    DD_Assert(token.content);
    return strtol(token.content, 0, 10);
}

int getNextIntegerBinary(FU_FileReader* reader) {
    int integer = *((int*)&reader->content[reader->at]);
    reader->at += sizeof(int);
    return integer;
}

bool FU_IsAtTheEnd(FU_FileReader* reader) {
    return reader->at >= reader->size;
}

bool FU_GetBytes(FU_FileReader* reader, void* dest, FU_s32 byteCount) {
    if (reader->error) return false;
    if (byteCount >= 0) {
        if (reader->at + byteCount <= reader->size) {
            memcpy(dest, reader->content + reader->at, byteCount);
            reader->at += byteCount;
            return true;
        } else {
            reader->error = true;
            return false;
        }
    } else {
        reader->error = true;
        return false;
    }
}

bool FU_GetInt32(FU_FileReader* reader, FU_s32* dest) {
    return FU_GetBytes(reader, (void*) dest, sizeof(FU_s32));
}

void FU_Rewind(FU_FileReader* reader, FU_s32 bytes) {
    reader->at -= bytes;
};

bool FU_GetInt32(FU_FileReader* reader, FU_s32* dest, FU_s32 min, FU_s32 max) {
    if (reader->error) return false;
    if (FU_GetInt32(reader, dest)) {
        if (min <= *dest && *dest <= max) {
            return true;
        } else {
            reader->error = true;
            return false;
        }
    } else {
        reader->error = true;
        return false;
    }
}

bool FU_GetInt32(FU_FileReader* reader, FU_s32* dest, FU_s32 min) {
    return FU_GetInt32(reader, dest, min, FU_s32_max);
}

bool FU_GetUInt32(FU_FileReader* reader, FU_u32* dest) {
    return FU_GetBytes(reader, (void*) dest, sizeof(FU_u32));
}


bool FU_GetUInt32(FU_FileReader* reader, FU_u32* dest, FU_u32 min, FU_u32 max) {
    if (reader->error) return false;
    if (FU_GetUInt32(reader, dest)) {
        if (min <= *dest && *dest <= max) {
            return true;
        } else {
            reader->error = true;
            return false;
        }
    } else {
        reader->error = true;
        return false;
    }
}

bool FU_GetUInt32(FU_FileReader* reader, FU_u32* dest, FU_u32 min) {
    return FU_GetUInt32(reader, dest, min, FU_u32_max);
}

bool FU_GetInt64(FU_FileReader* reader, FU_s64* dest) {
    return FU_GetBytes(reader, (void*) dest, sizeof(FU_s64));
}

bool FU_GetInt64(FU_FileReader* reader, FU_s64* dest, FU_s64 min, FU_s64 max) {
    if (reader->error) return false;
    
    if (FU_GetInt64(reader, dest)) {
        if (min <= *dest && *dest <= max) {
            return true;
        } else {
            reader->error = true;
            return false;
        }
    } else {
        reader->error = true;
        return false;
    }
}

bool FU_GetBool32(FU_FileReader* reader, bool* dest) {
    FU_s32 value = 0;
    if (FU_GetInt32(reader, &value)) {
        *dest = value;
        return true;
    } else {
        reader->error = true;
        return false;
    }
}

bool FU_GetDouble(FU_FileReader* reader, double* dest) {
    return FU_GetBytes(reader, (void*) dest, sizeof(double));
}

bool FU_GetFloat(FU_FileReader* reader, float* dest) {
    return FU_GetBytes(reader, (void*) dest, sizeof(float));
}

bool FU_GetString(FU_FileReader* reader, char* dest, FU_s32 destCap, FU_s32* size) {
    if (FU_GetInt32(reader, size, 0, destCap-1)) {
        if (FU_GetBytes(reader, dest, *size)) {
            dest[*size] = 0;
            return true;
        } else {
            reader->error = true;
            return false;
        }
    } else {
        reader->error = true;
        return false;
    }
}

bool FU_GetString(FU_FileReader* reader, char* dest, FU_s32 destCap) {
    FU_s32 size = 0;
    return FU_GetString(reader, dest, destCap, &size);
}

bool FU_GetUTF8String(FU_FileReader* reader, char* dest, FU_s32 destCap, FU_s32* size) {
    if (FU_GetString(reader, dest, destCap, size)) {
        if (FU_IsUTF8Valid(dest)) {
            return true;
        } else {
            reader->error = true;
            return false;
        }
    } else {
        return false;
    }
}

bool FU_GetUTF8String(FU_FileReader* reader, char* dest, FU_s32 destCap) {
    FU_s32 size;
    return FU_GetUTF8String(reader, dest, destCap, &size);
}

double getNextDoubleBinary(FU_FileReader* reader) {
    double value = *((double*)&reader->content[reader->at]);
    reader->at += sizeof(double);
    return value;
}

float getNextFloatBinary(FU_FileReader* reader) {
    float value = *((float*)&reader->content[reader->at]);
    reader->at += sizeof(float);
    return value;
}

void getNextBytes(FU_FileReader* reader, void* buffer, int byteCount) {
    if (buffer) {
        memcpy(buffer, &reader->content[reader->at], byteCount);
    }
    reader->at += byteCount;
}

int getNextStringBinary(FU_FileReader* reader, char* buffer=0) {
    int size = getNextIntegerBinary(reader);
    if (buffer) memset(buffer, 0, size+1);
    getNextBytes(reader, buffer, size);
    return size;
}

// FILE WRITER IMPLEMENTATION
// ----------------------------
struct FileWriter {
    char* buffer;
    unsigned long long at;
    unsigned long long bufferSize;
};

void writeInFile(FileWriter* fileWriter, void* p, int byteCount) {
    DD_Assert1(fileWriter->at + byteCount <= fileWriter->bufferSize, "Attempt to write past buffer.");
    memcpy(fileWriter->buffer + fileWriter->at, p, byteCount);
    fileWriter->at += byteCount;
}

void writeInFile(FileWriter* fileWriter, int value) {
    writeInFile(fileWriter, (void*) &value, sizeof(value));
    //memcpy(&fileWriter->buffer[fileWriter->at], &value, sizeof(int));
    //fileWriter->at += sizeof(int);
}

void writeInFile(FileWriter* fileWriter, unsigned int value) {
    writeInFile(fileWriter, (void*) &value, sizeof(value));
    //memcpy(&fileWriter->buffer[fileWriter->at], &value, sizeof(unsigned int));
    //fileWriter->at += sizeof(unsigned int);
}

void writeInFile(FileWriter* fileWriter, long long int value) {
    writeInFile(fileWriter, (void*) &value, sizeof(value));
    //memcpy(&fileWriter->buffer[fileWriter->at], &value, sizeof(long long int));
    //fileWriter->at += sizeof(long long int);
}

void writeInFile(FileWriter* fileWriter, long int value) {
    writeInFile(fileWriter, (void*) &value, sizeof(value));
    //memcpy(&fileWriter->buffer[fileWriter->at], &value, sizeof(long int));
    //fileWriter->at += sizeof(long int);
}

void writeInFile(FileWriter* fileWriter, double value) {
    writeInFile(fileWriter, (void*) &value, sizeof(value));
    //memcpy(&fileWriter->buffer[fileWriter->at], &value, sizeof(double));
    //fileWriter->at += sizeof(double);
}

void writeInFile(FileWriter* fileWriter, float value) {
    writeInFile(fileWriter, (void*) &value, sizeof(value));
    //memcpy(&fileWriter->buffer[fileWriter->at], &value, sizeof(float));
    //fileWriter->at += sizeof(float);
}

void writeInFile(FileWriter* fileWriter, char* string, int length) {
    writeInFile(fileWriter, (void*) &length, sizeof(int));
    writeInFile(fileWriter, (void*) string, length);
    //writeInFile(fileWriter, length);
    //memcpy(&fileWriter->buffer[fileWriter->at], string, length);
    //fileWriter->at += length;
}

void writeInFile(FileWriter* fileWriter, const char* cstr) {
    if (cstr != 0) {
        writeInFile(fileWriter, (char*) cstr, strlen(cstr));
    } else {
        writeInFile(fileWriter, 0);
    }
}

void writeTextInFileVA(FileWriter* writer, const char* formatString, va_list argList) {
    writer->at += vsprintf(&writer->buffer[writer->at], formatString, argList);
}

void writeTextInFile(FileWriter* writer, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    writeTextInFileVA(writer, formatString, argList);
    va_end(argList);
}

void saveFile(FileWriter writer, const char* formatString, ...) {
    char path[1024];
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    FILE* file = fopen(path, "w");
    fwrite(writer.buffer, 1, writer.at, file);
    fclose(file);
}

void appendFile(FileWriter writer, const char* formatString, ...) {
    char path[1024];
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    FILE* file = fopen(path, "a");
    fwrite(writer.buffer, 1, writer.at, file);
    fclose(file);
}

// Misc
///------

bool fileExists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

            
#endif





















