#include <sys/un.h>

#include "DD_Assert.h"
#include "DD_HTTPS.h"
#include "DD_LogUtils.h"
#include "DD_SignalUtils.h"

#define MIN(a, b) (a < b ? a : b)

extern "C" {

struct LT_Client {
    int id;

    HS_PacketQueue writeQueue;

    char* readBuffer;
    int   readCap;
    int   readSize;

    void* statePtr;
    JS_JSON* jState;

    pthread_mutex_t* mutex;
};

enum LT_NetEventType {
    LT_NetEventType_None,
    LT_NetEventType_NewClient,
    LT_NetEventType_NewPayload,
    LT_NetEventType_ServerLoopInterrupted
};

struct LT_NetEvent {
    LT_NetEventType type;
    int clientId;
    char* payload;
    int payloadSize;
};

enum LT_AppEventType {
    LT_AppEventType_None,
    LT_AppEventType_NewPayload,
};

struct LT_AppEvent {
    LT_AppEventType type;
    int clientId;
    char* payload;
    int payloadSize;
};

struct LT_Global {
    pthread_t threadId;
    char socketPath[PATH_MAX];
    char litPackageRootPath[PATH_MAX];
    int fdSocket;

    char projectPath[PATH_MAX];
    char appHostName[PATH_MAX];
    int appPort;
    bool serveDocs;
    bool devMode;

    HS_Server hserver;

    size_t appStateSize;
    void (*appInit)();
    void (*appNewClient)(void* statePtr);
    void (*appUpdate)(void* statePtr);

    LT_NetEvent* netEvents;
    LT_AppEvent* appEvents;

    pthread_mutex_t netEventsMutex;
    pthread_mutex_t appEventsMutex;

    LT_Client** clients;
    int nextClientId;
};

LT_Global g;

LT_Client* LT_GetClient(int id) {
    for (int i = 0; i < arrcount(g.clients); ++i) {
        if (g.clients[i]->id == id)
            return g.clients[i];
    }
    return 0;
}

// NOTE: Net events are created and pushed by the network layer and poped and
// destroyed by the app layer.
LT_NetEvent LT_CreateNetEvent(LT_NetEventType type, int clientId, char* payload, int payloadSize) {
    LT_NetEvent ev = {
        .type=type,
        .clientId=clientId,
        .payload=payload,
        .payloadSize=payloadSize,
    };

    if (payload) {
        ev.payload = arrstring(payload, payloadSize);
    }

    return ev;
}

void LT_DestroyNetEvent(LT_NetEvent ev) {
    if (ev.payload) {
        arrfree(ev.payload);
    }
}

void LT_PushNetEvent(LT_NetEvent ev) {
    pthread_mutex_lock(&g.netEventsMutex);
    arradd(g.netEvents, ev);
    pthread_mutex_unlock(&g.netEventsMutex);
}

LT_NetEvent LT_PopNetEvent() {
    pthread_mutex_lock(&g.netEventsMutex);

    LT_NetEvent ev = {};
    if (arrcount(g.netEvents)) {
        ev = g.netEvents[0];
        arrremove(g.netEvents, 0);
    }

    pthread_mutex_unlock(&g.netEventsMutex);

    return ev;
}

// NOTE: App events are created and pushed by the app layer and poped and
// destroyed by the net layer.
LT_AppEvent LT_CreateAppEvent(LT_AppEventType type, int clientId, char* payload, int payloadSize) {
    LT_AppEvent ev = {
        .type=type,
        .clientId=clientId,
        .payload=payload,
        .payloadSize=payloadSize,
    };

    if (payload) {
        // NOTE: This allocs memory to be used with HS_Packet and HS_SendPacket,
        // so we don't have to allocate more memory when we are ready to send
        // the payload. That's why we need LWS_PRE.
        //
        // This memory is free'd by DD_HTTPS, after sending the payload.
        int bufferSize = LWS_PRE + payloadSize;
        char* buffer = (char*) calloc(1, bufferSize);
        char* payloadBegin = buffer + LWS_PRE;
        memcpy(payloadBegin, payload, payloadSize);

        ev.payload = buffer;
        ev.payloadSize = bufferSize;
    }

    return ev;
}

void LT_DestroyAppEvent(LT_AppEvent ev) {
    // NOTE: The payload memory is free'd by DD_HTTPS, after sending the payload.
    // So I guess we don't have to do anything here...
}

void LT_PushAppEvent(LT_AppEvent ev) {
    pthread_mutex_lock(&g.appEventsMutex);
    arradd(g.appEvents, ev);
    pthread_mutex_unlock(&g.appEventsMutex);
}

LT_AppEvent LT_PopAppEvent() {
    pthread_mutex_lock(&g.appEventsMutex);

    LT_AppEvent ev = {};
    if (arrcount(g.appEvents)) {
        ev = g.appEvents[0];
        arrremove(g.appEvents, 0);
    }

    pthread_mutex_unlock(&g.appEventsMutex);

    return ev;
}

void LT_LockClient(int clientId) {
    LT_Client* wcClient = LT_GetClient(clientId);
    if (wcClient) {
        pthread_mutex_lock(wcClient->mutex);
    }
}

void LT_UnlockClient(int clientId) {
    LT_Client* wcClient = LT_GetClient(clientId);
    if (wcClient) {
        pthread_mutex_unlock(wcClient->mutex);
    }
}

int LT_ProcessIncomingMessage(HS_CallbackArgs* args) {
    LT_Client* wcClient = HS_GetClientData(LT_Client, args);

    LU_Log(LU_Debug, "IncomingMessage | Bytes: %d | Payload: %.*s", wcClient->readSize, wcClient->readSize, wcClient->readBuffer);

    LT_NetEvent ev = LT_CreateNetEvent(
        LT_NetEventType_NewPayload,
        wcClient->id,
        wcClient->readBuffer,
        wcClient->readSize
    );

    LT_PushNetEvent(ev);

    write(g.fdSocket, "", 1);

    return 0;
}

int HS_CALLBACK(handleEvent, args) {
    HS_VHost* vhost = HS_GetVHost(args->socket);
    LT_Client* wcClient = HS_GetClientData(LT_Client, args);

    LU_Log(LU_Debug, "HandleLWSEvent | %s", HS_ToString(args->reason));

    switch (args->reason) {
        case LWS_CALLBACK_RECEIVE: {
            HS_ReceiveMessageFragment(args, &wcClient->readBuffer, &wcClient->readSize, &wcClient->readCap, LT_ProcessIncomingMessage);
        } break;

        case LWS_CALLBACK_SERVER_WRITEABLE: {
            HS_WriteNextPacket(&wcClient->writeQueue);
        } break;

        case LWS_CALLBACK_ESTABLISHED: {
            wcClient->id = g.nextClientId++;
            wcClient->writeQueue = HS_CreatePacketQueue(args->socket, 128);
            wcClient->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(wcClient->mutex, 0);
            arradd(g.clients, wcClient);

            LT_PushNetEvent({
                .type = LT_NetEventType_NewClient,
                .clientId = wcClient->id,
            });

            write(g.fdSocket, "", 1);
        } break;

        case LWS_CALLBACK_PROTOCOL_INIT: {
            lws_sock_file_fd_type fd = {g.fdSocket};
            lws* wsi = lws_adopt_descriptor_vhost(vhost->lwsVHost, LWS_ADOPT_RAW_FILE_DESC, fd, "ws", 0);
        } break;

        case LWS_CALLBACK_CLOSED: {
            pthread_mutex_lock(wcClient->mutex);
            arrremovematch(g.clients, wcClient);
            free(wcClient->mutex);
        } break;

        case LWS_CALLBACK_RAW_RX_FILE: {
            char buf[1024] = {};
            read(g.fdSocket, buf, sizeof(buf));

            while (arrcount(g.appEvents)) {
                LT_AppEvent ev = LT_PopAppEvent();
                wcClient = LT_GetClient(ev.clientId);

                if (wcClient) {
                    if (ev.type == LT_AppEventType_NewPayload) {
                        LU_Log(LU_Debug, "AppEventType_NewPayload | %d | %.*s", ev.clientId, MIN(ev.payloadSize-LWS_PRE, 256), ev.payload+LWS_PRE);

                        HS_Packet packet = {
                            .buffer = ev.payload,
                            .bufferSize = ev.payloadSize,
                            .body = ev.payload+LWS_PRE,
                            .bodySize = ev.payloadSize-LWS_PRE,
                        };

                        HS_SendPacket(&wcClient->writeQueue, packet);
                        LT_DestroyAppEvent(ev);
                    } else {
                        DD_Assert2(0, "Unknown event %d", ev.type);
                    }
                } else {
                    // TODO: What to do if wcClient is no longer online?
                }
            }
        } break;

        default: break;
    }

    return 0;
}

void LT_PushURIMapping(const char* uri, int uriSize, const char* filePath, int filePathSize) {
    char uriBuf[PATH_MAX] = {};
    char filePathBuf[PATH_MAX] = {};
    strncpy(uriBuf, uri, uriSize);
    strncpy(filePathBuf, filePath, filePathSize);
    HS_PushURIMapping(&g.hserver, "lit-app", uriBuf, filePathBuf);
}

void LT_ClearURIMapping() {
    HS_ClearURIMapping(&g.hserver, "lit-app");
}

void LT_SetStateSize(size_t size) {
    g.appStateSize = size;
}

void LT_HandleSigInt(void* data) {
    HS_Stop(&g.hserver);

    LT_NetEvent ev = LT_CreateNetEvent(LT_NetEventType_ServerLoopInterrupted, 0, 0, 0);
    LT_PushNetEvent(ev);
    write(g.fdSocket, "", 1);
    close(g.fdSocket);

    printf("\r  \n");
    LU_Log(LU_Debug, "ServerLoopInterrupted");
}

void* LT_RunServer(void*) {
    if (!g.devMode) {
        HS_SetLogLevel(0);
    }

    g.nextClientId = 1;
    g.clients = arralloc(LT_Client*, 100);

    g.netEvents = arralloc(LT_NetEvent, 100);
    g.appEvents = arralloc(LT_AppEvent, 100);

    bool disableSSL = !HS_IsDirectory(".Lit/certs");

    g.hserver = HS_CreateServer(0, disableSSL);
    HS_InitServer(&g.hserver, true);
    HS_AddVHost(&g.hserver, "lit-app");
    HS_SetLWSVHostConfig(&g.hserver, "lit-app", pt_serv_buf_size, HS_KILO_BYTES(12));
    HS_SetLWSProtocolConfig(&g.hserver, "lit-app", "HTTP", rx_buffer_size, HS_KILO_BYTES(12));
    HS_SetHTTPGetHandler(&g.hserver, "lit-app", HS_GetFileByURI);
    HS_SetVHostHostName(&g.hserver, "lit-app", g.appHostName);
    HS_SetVHostPort(&g.hserver, "lit-app", g.appPort);
    HS_AddProtocol(&g.hserver, "lit-app", "ws", handleEvent, LT_Client);
    HS_PushCacheControlMapping(&g.hserver, "lit-app", "*.html", "no-cache, no-store, must-revalidate");
    HS_PushCacheControlMapping(&g.hserver, "lit-app", "/*", "max-age=2592000");
    if (!disableSSL) {
        HS_SetCertificate(&g.hserver, "lit-app", ".Lit/certs/certificate.crt", ".Lit/certs/private.key");
    }

    char tempBuffer[2*PATH_MAX];

    snprintf(tempBuffer, sizeof(tempBuffer), "%s/%s", g.projectPath, ".Lit/served-files");
    HS_SetServedFilesRootDir(&g.hserver, "lit-app", tempBuffer);

    snprintf(tempBuffer, sizeof(tempBuffer), "%s/%s", g.litPackageRootPath, "served-files");
    HS_AddServedFilesDir(&g.hserver, "lit-app", "/Lit.jl", tempBuffer);

    if (g.devMode) {
        HS_SetVHostVerbosity(&g.hserver, "lit-app", 1);
        HS_DisableFileCache(&g.hserver, "lit-app");
    }

    if (g.serveDocs) {
        snprintf(tempBuffer, sizeof(tempBuffer), "%s/%s", g.litPackageRootPath, "docs");
        HS_AddServedFilesDir(&g.hserver, "lit-app", "/docs", tempBuffer);
    }

    if (HS_IsRegularFile(".Lit/companion-host.json")) {
        HS_AddVHost(&g.hserver, "lit-companion");
        HS_SetLWSVHostConfig(&g.hserver, "lit-companion", pt_serv_buf_size, HS_KILO_BYTES(12));
        HS_SetLWSProtocolConfig(&g.hserver, "lit-companion", "HTTP", rx_buffer_size, HS_KILO_BYTES(12));
        HS_InitFileServer(&g.hserver, "lit-companion", ".Lit/companion-host.json");
        if (g.devMode) {
            HS_SetVHostVerbosity(&g.hserver, "lit-companion", 1);
        }
    }

    SG_RegisterHandler(SIGINT, LT_HandleSigInt, 0);

    g.fdSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    sockaddr_un socketAddr = {};
    socketAddr.sun_family = AF_UNIX;
    strncpy(socketAddr.sun_path, g.socketPath, sizeof(socketAddr.sun_path) - 1);
    connect(g.fdSocket, (sockaddr*)&socketAddr, sizeof(socketAddr));

    HS_RunForever(&g.hserver, true);
    HS_Destroy(&g.hserver);
    return 0;
}

void LT_InitNetLayer(const char* hostName, int hostNameSize, int port, bool serveDocs, const char* socketPath, int socketPathSize, const char* litPackageRootPath, int litPackageRootPathSize, bool devMode) {
    LU_Disable(&LU_GlobalLogFile);
    LU_EnableStdout(&LU_GlobalLogFile);
    LU_DisableStderr(&LU_GlobalLogFile);

    if (devMode) {
        LU_SetLogLevel(LU_Verbose);
    } else {
        LU_SetLogLevel(LU_Important);
    }

    strncpy(g.appHostName, hostName, hostNameSize);
    g.appPort = port;
    g.serveDocs = serveDocs;
    g.devMode = devMode;

    strncpy(g.socketPath, socketPath, socketPathSize);
    strncpy(g.litPackageRootPath, litPackageRootPath, litPackageRootPathSize);
    getcwd(g.projectPath, sizeof(g.projectPath));

    pthread_mutex_init(&g.netEventsMutex, 0);
    pthread_mutex_init(&g.appEventsMutex, 0);

    int result = pthread_create(&g.threadId, 0, LT_RunServer, 0);
}

bool LT_ServerIsRunning() {
    return g.hserver.isRunning;
}

int LT_DoServiceWork() {
    return lws_service(g.hserver.lwsContext, 0);
}

void LT_StopServer() {
    lws_cancel_service(g.hserver.lwsContext);
    g.hserver.isRunning = false;
}

} // extern "C"
