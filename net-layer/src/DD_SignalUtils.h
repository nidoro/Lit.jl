#include <signal.h>

// According to the wikipedia, the C standard defines only 6 signals:
// SIGABRT - "abort", abnormal termination.
// SIGFPE - floating point exception.
// SIGILL - "illegal", invalid instruction.
// SIGINT - "interrupt", interactive attention request sent to the program.
// SIGSEGV - "segmentation violation", invalid memory access.
// SIGTERM - "terminate", termination request sent to the program.

typedef void (*SG_SignalHandlerFunction)(void* data);

struct SG_SignalHandler {
    int signal;
    SG_SignalHandlerFunction function;
    void* data;
};

SG_SignalHandler SG_SignalHandlers[32] = {};

void SG_SignalReceiver(int sig) {
    if (sig < 32) {
        SG_SignalHandlers[sig].function(SG_SignalHandlers[sig].data);
    }
}

#ifdef _WIN32
#include <windows.h>
BOOL WINAPI WindowsCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        // Find SIGINT handler
        if (SG_SignalHandlers[SIGINT].function) {
            SG_SignalHandlers[SIGINT].function(SG_SignalHandlers[SIGINT].data);
        }
        return TRUE;  // Signal handled
    }
    return FALSE;
}

void SG_RegisterHandler(int sig, SG_SignalHandlerFunction function, void* data) {
    SG_SignalHandlers[sig] = {sig, function, data};

    if (sig == SIGINT) {
        SetConsoleCtrlHandler(WindowsCtrlHandler, TRUE);
    } else {
        signal(sig, SG_SignalReceiver);
    }
}

#else
void SG_RegisterHandler(int sig, SG_SignalHandlerFunction function, void* data) {
    SG_SignalHandlers[sig] = {sig, function, data};
    signal(sig, SG_SignalReceiver);
}
#endif

