#ifndef DD_LOG_UTILS_H
#define DD_LOG_UTILS_H

#include <stdio.h>
#include <stdarg.h>

#define LU_ColorReset         "\x1b[0m"
#define LU_ColorCodeRed       "\x1b[31m"
#define LU_ColorCodeGreen     "\x1b[32m"
#define LU_ColorCodeYellow    "\x1b[33m"
#define LU_ColorCodeBlue      "\x1b[34m"
#define LU_ColorCodeMagenta   "\x1b[35m"
#define LU_ColorCodeCyan      "\x1b[36m"

enum LU_LogLevel {
    LU_Verbose,
    LU_Info,
    LU_Debug,
    LU_Important
};

typedef void(*LU_LogVAFunction) (FILE* logFile, int level, const char* formatString, va_list argList);

struct LU_LogFile {
    FILE* handle;
    char path[2048];
    int level;
    LU_LogVAFunction logFunc;
    LU_LogVAFunction logFuncStdout;
    LU_LogVAFunction logFuncStderr;
};

void LU_PrintTimestamp(FILE* file) {
    timespec tspec;
    clock_gettime(CLOCK_REALTIME, &tspec);
    tm tt = *localtime(&tspec.tv_sec);
    fprintf(
        file, 
        "%d-%02d-%02d %02d:%02d:%07.4f | ",
        tt.tm_year + 1900, 
        tt.tm_mon + 1, 
        tt.tm_mday, 
        tt.tm_hour, 
        tt.tm_min, 
        (float)tt.tm_sec + ((float)tspec.tv_nsec)/1.0e9
    );
}

void LU_LogVAStub(FILE* logFile, int level, const char* formatString, va_list args) {}

void LU_LogVA(FILE* logFile, int level, const char* formatString, va_list argList) {
    vfprintf(logFile, formatString, argList);
    fprintf(logFile, "\n");
}

void LU_TimedLogVA(FILE* file, int level, const char* formatString, va_list argList) {
    LU_PrintTimestamp(file);
    vfprintf(file, formatString, argList);
    fprintf(file, "\n");
}

void LU_ColoredTimedLogVA(FILE* file, int level, const char* formatString, va_list argList) {
    bool colored = true;
    if (level == LU_Important) {
        fprintf(file, LU_ColorCodeRed);
    } else {
        colored = false;
    }
    
    LU_PrintTimestamp(file);
    vfprintf(file, formatString, argList);
    fprintf(file, "\n");
    
    if (colored) {
        fprintf(file, LU_ColorReset);
    }
}

void LU_Disable(LU_LogFile* logFile) {
    logFile->logFunc = LU_LogVAStub;
}

void LU_DisableStdout(LU_LogFile* logFile) {
    logFile->logFuncStdout = LU_LogVAStub;
}

void LU_DisableStderr(LU_LogFile* logFile) {
    logFile->logFuncStderr = LU_LogVAStub;
}

void LU_Enable(LU_LogFile* logFile) {
    logFile->logFunc = LU_TimedLogVA;
    setbuf(logFile->handle, 0);
}

void LU_EnableStdout(LU_LogFile* logFile) {
    logFile->logFuncStdout = LU_ColoredTimedLogVA;
    setbuf(stdout, 0);
}

void LU_EnableStderr(LU_LogFile* logFile) {
    logFile->logFuncStderr = LU_TimedLogVA;
    setbuf(stderr, 0);
}

bool LU_IsStdoutEnabled(LU_LogFile logFile) {
    return logFile.logFuncStdout != LU_LogVAStub;
}

bool LU_IsStderrEnabled(LU_LogFile logFile) {
    return logFile.logFuncStderr != LU_LogVAStub;
}

void LU_Reopen(LU_LogFile* logFile) {
    if (logFile->handle) fclose(logFile->handle);
    logFile->handle = fopen(logFile->path, "a");
    if (logFile->handle) {
        LU_Enable(logFile);
    } else {
        LU_Disable(logFile);
    }
}

LU_LogFile LU_OpenLogFile(const char* formatString, ...) {
    LU_LogFile result = {};
    
    va_list argList;
    va_start(argList, formatString);
    vsprintf(result.path, formatString, argList);
    va_end(argList);
    
    result.handle = fopen(result.path, "a");
    
    if (result.handle) {
        LU_Enable(&result);
    } else {
        LU_Disable(&result);
    }
    
    LU_DisableStdout(&result);
    LU_DisableStderr(&result);
    
    return result;
}

LU_LogFile LU_StdoutLog() {
    LU_LogFile result = {};
    LU_Disable(&result);
    LU_EnableStdout(&result);
    LU_DisableStderr(&result);
    return result;
}

LU_LogFile LU_StderrLog() {
    LU_LogFile result = {};
    LU_Disable(&result);
    LU_DisableStdout(&result);
    LU_EnableStderr(&result);
    return result;
}

LU_LogFile LU_Stub() {
    LU_LogFile result = {};
    LU_Disable(&result);
    LU_DisableStdout(&result);
    LU_DisableStderr(&result);
    return result;
}

void LU_LogVA(LU_LogFile logFile, int level, const char* formatString, va_list argList) {
    va_list argList1;
    va_list argList2;
    va_copy(argList1, argList);
    va_copy(argList2, argList);
    if (level >= logFile.level) {
        logFile.logFunc(logFile.handle, level, formatString, argList);
        logFile.logFuncStdout(stdout, level, formatString, argList1);
        logFile.logFuncStderr(stderr, level, formatString, argList2);
    }
    va_end(argList1);
    va_end(argList2);
}

void LU_LogColored(const char* color, LU_LogFile logFile, const char* formatString, va_list argList) {
    if (LU_IsStdoutEnabled(logFile)) fprintf(stdout, "%s", color);
    if (LU_IsStderrEnabled(logFile)) fprintf(stderr, "%s", color);
    
    LU_LogVA(logFile, 0, formatString, argList);
    
    if (LU_IsStdoutEnabled(logFile)) fprintf(stdout, "%s", LU_ColorReset);
    if (LU_IsStderrEnabled(logFile)) fprintf(stderr, "%s", LU_ColorReset);
}

#define LU_COLORED_LOG_FUNC_IMPLEMENTATION(color) \
    void LU_Log ## color (LU_LogFile logFile, const char* formatString, ...) { \
        va_list argList; \
        va_start(argList, formatString); \
        LU_LogColored(LU_ColorCode ## color, logFile, formatString, argList); \
        va_end(argList); \
    } \

LU_COLORED_LOG_FUNC_IMPLEMENTATION(Red);
LU_COLORED_LOG_FUNC_IMPLEMENTATION(Green);
LU_COLORED_LOG_FUNC_IMPLEMENTATION(Blue);
LU_COLORED_LOG_FUNC_IMPLEMENTATION(Yellow);
LU_COLORED_LOG_FUNC_IMPLEMENTATION(Magenta);
LU_COLORED_LOG_FUNC_IMPLEMENTATION(Cyan);

void LU_Log(LU_LogFile logFile, int level, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    LU_LogVA(logFile, level, formatString, argList);
    va_end(argList);
}

void LU_Log(LU_LogFile logFile, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    LU_LogVA(logFile, 0, formatString, argList);
    va_end(argList);
}

void LU_Log(LU_LogFile logFile, int level, LU_LogVAFunction logFunc, const char* formatString, ...) {
    if (logFile.logFunc != LU_LogVAStub) {
        logFile.logFunc = logFunc;
    }
    if (logFile.logFuncStdout != LU_LogVAStub) {
        logFile.logFuncStdout = logFunc;
    }
    if (logFile.logFuncStderr != LU_LogVAStub) {
        logFile.logFuncStderr = logFunc;
    }
    
    va_list argList;
    va_start(argList, formatString);
    LU_LogVA(logFile, level, formatString, argList);
    va_end(argList);
}

void LU_SetLogLevel(LU_LogFile* logFile, int level) {
    logFile->level = level;
}

bool LU_IsInitialized(LU_LogFile logFile) {
    return logFile.logFunc || logFile.logFuncStdout || logFile.logFuncStderr;
}

// Global log file
//------------------
LU_LogFile LU_GlobalLogFile;

void LU_Log(int level, const char* formatString, ...) {
    if (!LU_IsInitialized(LU_GlobalLogFile)) return;
    
    va_list argList;
    va_start(argList, formatString);
    LU_LogVA(LU_GlobalLogFile, level, formatString, argList);
    va_end(argList);
}

void LU_Log(const char* formatString, ...) {
    if (!LU_IsInitialized(LU_GlobalLogFile)) return;
    
    va_list argList;
    va_start(argList, formatString);
    LU_LogVA(LU_GlobalLogFile, 0, formatString, argList);
    va_end(argList);
}

void LU_SetLogLevel(int level) {
    LU_GlobalLogFile.level = level;
}

#endif





