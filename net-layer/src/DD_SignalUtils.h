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

void SG_RegisterHandler(int sig, SG_SignalHandlerFunction function, void* data) {
    SG_SignalHandlers[sig] = {sig, function, data};
    signal(sig, SG_SignalReceiver);
}


