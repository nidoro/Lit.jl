#include <pthread.h>
#include "DD_Assert.h"
#include "DD_HTTPS.h"
#include "DD_LogUtils.h"
#include "DD_SignalUtils.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/un.h>
    #include <unistd.h>
    #include <netinet/tcp.h>
    typedef int socket_t;
#endif

#define MIN(a, b) (a < b ? a : b)

#ifdef _WIN32
#define MG_API __declspec(dllexport)
#else
#define MG_API
#endif

extern "C" {

struct MG_Client {
    int id;

    HS_PacketQueue writeQueue;

    char* readBuffer;
    int   readCap;
    int   readSize;

    void* statePtr;
    JS_JSON* jState;

    pthread_mutex_t* mutex;
};

enum MG_NetEventType {
    MG_NetEventType_None,
    MG_NetEventType_NewClient,
    MG_NetEventType_ClientLeft,
    MG_NetEventType_NewPayload,
    MG_NetEventType_ServerLoopInterrupted
};

struct MG_NetEvent {
    MG_NetEventType type;
    int clientId;
    char* payload;
    int payloadSize;
};

enum MG_AppEventType {
    MG_AppEventType_None,
    MG_AppEventType_NewPayload,
};

struct MG_AppEvent {
    MG_AppEventType type;
    int clientId;
    char* payload;
    int payloadSize;
};

struct MG_Global {
    pthread_t threadId;
    int ipcPort;
    char magicPackageRootPath[PATH_MAX];

#ifdef _WIN32
    SOCKET fdSocket;
#else
    int fdSocket;
#endif

    char projectPath[PATH_MAX];
    char appHostName[PATH_MAX];
    int appPort;
    char docsPath[PATH_MAX];
    int  docsPathSize;
    bool verbose;
    bool devMode;

    HS_Server hserver;

    size_t appStateSize;
    void (*appInit)();
    void (*appNewClient)(void* statePtr);
    void (*appUpdate)(void* statePtr);

    MG_NetEvent* netEvents;
    MG_AppEvent* appEvents;

    pthread_mutex_t netEventsMutex;
    pthread_mutex_t appEventsMutex;

    MG_Client** clients;
    int nextClientId;
};

MG_Global g;

MG_API void MG_WakeUpAppLayer() {
#ifdef _WIN32
    int sent = send(g.fdSocket, "x", 1, 0);
    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        LU_Log(LU_Debug, "Send error: %d", err);
    }
#else
    ssize_t sent = write(g.fdSocket, "x", 1);
    if (sent < 0) {
        LU_Log(LU_Debug, "Write error: %s", strerror(errno));
    }
#endif
}

MG_API MG_Client* MG_GetClient(int id) {
    for (int i = 0; i < arrcount(g.clients); ++i) {
        if (g.clients[i]->id == id)
            return g.clients[i];
    }
    return 0;
}

// NOTE: Net events are created and pushed by the network layer and poped and
// destroyed by the app layer.
MG_API MG_NetEvent MG_CreateNetEvent(MG_NetEventType type, int clientId, char* payload, int payloadSize) {
    MG_NetEvent ev = {
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

MG_API void MG_DestroyNetEvent(MG_NetEvent ev) {
    if (ev.payload) {
        arrfree(ev.payload);
    }
}

MG_API void MG_PushNetEvent(MG_NetEvent ev) {
    pthread_mutex_lock(&g.netEventsMutex);
    arradd(g.netEvents, ev);
    pthread_mutex_unlock(&g.netEventsMutex);
}

MG_API MG_NetEvent MG_PopNetEvent() {
    pthread_mutex_lock(&g.netEventsMutex);

    MG_NetEvent ev = {};
    if (arrcount(g.netEvents)) {
        ev = g.netEvents[0];
        arrremove(g.netEvents, 0);
    }

    pthread_mutex_unlock(&g.netEventsMutex);

    return ev;
}

// NOTE: App events are created and pushed by the app layer and poped and
// destroyed by the net layer.
MG_API MG_AppEvent MG_CreateAppEvent(MG_AppEventType type, int clientId, char* payload, int payloadSize) {
    MG_AppEvent ev = {
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

MG_API void MG_DestroyAppEvent(MG_AppEvent ev) {
    // NOTE: The payload memory is free'd by DD_HTTPS, after sending the payload.
    // So I guess we don't have to do anything here...
}

MG_API void MG_PushAppEvent(MG_AppEvent ev) {
    pthread_mutex_lock(&g.appEventsMutex);
    arradd(g.appEvents, ev);
    pthread_mutex_unlock(&g.appEventsMutex);
}

MG_API MG_AppEvent MG_PopAppEvent() {
    pthread_mutex_lock(&g.appEventsMutex);

    MG_AppEvent ev = {};
    if (arrcount(g.appEvents)) {
        ev = g.appEvents[0];
        arrremove(g.appEvents, 0);
    }

    pthread_mutex_unlock(&g.appEventsMutex);

    return ev;
}

MG_API void MG_LockClient(int clientId) {
    MG_Client* wcClient = MG_GetClient(clientId);
    if (wcClient) {
        pthread_mutex_lock(wcClient->mutex);
    }
}

MG_API void MG_UnlockClient(int clientId) {
    MG_Client* wcClient = MG_GetClient(clientId);
    if (wcClient) {
        pthread_mutex_unlock(wcClient->mutex);
    }
}

MG_API int MG_ProcessIncomingMessage(HS_CallbackArgs* args) {
    MG_Client* wcClient = HS_GetClientData(MG_Client, args);

    LU_Log(LU_Debug, "IncomingMessage | Bytes: %d | Payload: %.*s", wcClient->readSize, wcClient->readSize, wcClient->readBuffer);

    MG_NetEvent ev = MG_CreateNetEvent(
        MG_NetEventType_NewPayload,
        wcClient->id,
        wcClient->readBuffer,
        wcClient->readSize
    );

    MG_PushNetEvent(ev);

    MG_WakeUpAppLayer();

    return 0;
}

MG_API int HS_CALLBACK(handleEvent, args) {
    HS_VHost* vhost = HS_GetVHost(args->socket);
    MG_Client* wcClient = HS_GetClientData(MG_Client, args);

    LU_Log(LU_Debug, "HandleLWSEvent | %s", HS_ToString(args->reason));

    switch (args->reason) {
        case LWS_CALLBACK_RECEIVE: {
            HS_ReceiveMessageFragment(args, &wcClient->readBuffer, &wcClient->readSize, &wcClient->readCap, MG_ProcessIncomingMessage);
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

            MG_PushNetEvent({
                .type = MG_NetEventType_NewClient,
                .clientId = wcClient->id,
            });

            MG_WakeUpAppLayer();
        } break;

        case LWS_CALLBACK_PROTOCOL_INIT: {
#ifdef _WIN32
            lws_sock_file_fd_type fd = {.sockfd=(long long unsigned int) g.fdSocket};
#else
            lws_sock_file_fd_type fd = {.sockfd=g.fdSocket};
#endif
            lws* wsi = lws_adopt_descriptor_vhost(vhost->lwsVHost, LWS_ADOPT_SOCKET, fd, "ws", 0);
        } break;

        case LWS_CALLBACK_CLOSED: {
            MG_PushNetEvent({
                .type = MG_NetEventType_ClientLeft,
                .clientId = wcClient->id,
            });

            MG_WakeUpAppLayer();

            pthread_mutex_lock(wcClient->mutex);
            arrremovematch(g.clients, wcClient);
            free(wcClient->mutex);
        } break;

        case LWS_CALLBACK_RAW_RX: {
            char buf[1024] = {};
            read(g.fdSocket, buf, sizeof(buf));

            while (arrcount(g.appEvents)) {
                MG_AppEvent ev = MG_PopAppEvent();
                wcClient = MG_GetClient(ev.clientId);

                if (wcClient) {
                    if (ev.type == MG_AppEventType_NewPayload) {
                        LU_Log(LU_Debug, "AppEventType_NewPayload | %d | %.*s", ev.clientId, MIN(ev.payloadSize-LWS_PRE, 256), ev.payload+LWS_PRE);

                        HS_Packet packet = {
                            .buffer = ev.payload,
                            .bufferSize = ev.payloadSize,
                            .body = ev.payload+LWS_PRE,
                            .bodySize = ev.payloadSize-LWS_PRE,
                        };

                        HS_SendPacket(&wcClient->writeQueue, packet);
                        MG_DestroyAppEvent(ev);
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

MG_API void MG_PushURIMapping(const char* uri, int uriSize, const char* filePath, int filePathSize) {
    char uriBuf[PATH_MAX] = {};
    char filePathBuf[PATH_MAX] = {};
    strncpy(uriBuf, uri, uriSize);
    strncpy(filePathBuf, filePath, filePathSize);
    HS_PushURIMapping(&g.hserver, "magic-app", uriBuf, filePathBuf);
}

MG_API void MG_ClearURIMapping() {
    HS_ClearURIMapping(&g.hserver, "magic-app");
}

MG_API void MG_SetStateSize(size_t size) {
    g.appStateSize = size;
}

MG_API void MG_HandleSigInt(void* data) {
    HS_Stop(&g.hserver);

    MG_NetEvent ev = MG_CreateNetEvent(MG_NetEventType_ServerLoopInterrupted, 0, 0, 0);
    MG_PushNetEvent(ev);
    MG_WakeUpAppLayer();
    close(g.fdSocket);

    printf("\r  \n");
    LU_Log(LU_Debug, "ServerLoopInterrupted");
}

MG_API void MG_StartIPC(void) {
#ifdef _WIN32
    static int wsa_initialized = 0;
    if (!wsa_initialized) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
            return;
        wsa_initialized = 1;
    }
#endif

    g.fdSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    DD_Assert(g.fdSocket != INVALID_SOCKET);
#else
    DD_Assert(g.fdSocket >= 0);
#endif

    struct sockaddr_in socketAddr = {0};
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons((uint16_t)g.ipcPort);
    inet_pton(AF_INET, "127.0.0.1", &socketAddr.sin_addr);

    int result = connect(g.fdSocket, (struct sockaddr *)&socketAddr, sizeof(socketAddr));

#ifdef _WIN32
    DD_Assert(result != SOCKET_ERROR);
#else
    DD_Assert(result >= 0);
#endif
}

MG_API void* MG_RunServer(void*) {
    if (g.verbose) {
        HS_SetLogLevel(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG);
    } else {
        HS_SetLogLevel(LLL_ERR | LLL_WARN);
    }

    g.nextClientId = 1;
    g.clients = arralloc(MG_Client*, 100);

    g.netEvents = arralloc(MG_NetEvent, 100);
    g.appEvents = arralloc(MG_AppEvent, 100);

    bool disableSSL = !HS_IsDirectory(".Magic/certs");

    g.hserver = HS_CreateServer(0, disableSSL);
    HS_InitServer(&g.hserver, true);
    HS_AddVHost(&g.hserver, "magic-app");
    HS_SetLWSVHostConfig(&g.hserver, "magic-app", pt_serv_buf_size, HS_KILO_BYTES(12));
    HS_SetLWSProtocolConfig(&g.hserver, "magic-app", "HTTP", rx_buffer_size, HS_KILO_BYTES(12));
    HS_SetHTTPGetHandler(&g.hserver, "magic-app", HS_GetFileByURI);
    HS_SetVHostHostName(&g.hserver, "magic-app", g.appHostName);
    HS_SetVHostPort(&g.hserver, "magic-app", g.appPort);
    HS_AddProtocol(&g.hserver, "magic-app", "ws", handleEvent, MG_Client);
    HS_PushCacheBust(&g.hserver, "magic-app", "*.html");
    HS_PushCacheControlMapping(&g.hserver, "magic-app", "*.html", "no-cache, no-store, must-revalidate");
    HS_PushCacheControlMapping(&g.hserver, "magic-app", "/*", "max-age=2592000");
    if (!disableSSL) {
        HS_SetCertificate(&g.hserver, "magic-app", ".Magic/certs/certificate.crt", ".Magic/certs/private.key");
    }

    char tempBuffer[2*PATH_MAX];

    snprintf(tempBuffer, sizeof(tempBuffer), "%s/%s", g.projectPath, ".Magic/served-files");
    HS_SetServedFilesRootDir(&g.hserver, "magic-app", tempBuffer);
    HS_Set404File(&g.hserver, "magic-app", "/generated/app/pages/404.html");

    snprintf(tempBuffer, sizeof(tempBuffer), "%s/%s", g.magicPackageRootPath, "served-files");
    HS_AddServedFilesDir(&g.hserver, "magic-app", "/Magic.jl", tempBuffer);

    if (g.verbose) {
        HS_SetVHostVerbosity(&g.hserver, "magic-app", 1);
    }

    if (g.devMode) {
        HS_DisableFileCache(&g.hserver, "magic-app");
    }

    if (g.docsPath) {
        HS_AddServedFilesDir(&g.hserver, "magic-app", "/docs", g.docsPath);
    }

    if (HS_IsRegularFile(".Magic/companion-host.json")) {
        HS_AddVHost(&g.hserver, "magic-companion");
        HS_SetLWSVHostConfig(&g.hserver, "magic-companion", pt_serv_buf_size, HS_KILO_BYTES(12));
        HS_SetLWSProtocolConfig(&g.hserver, "magic-companion", "HTTP", rx_buffer_size, HS_KILO_BYTES(12));
        HS_InitFileServer(&g.hserver, "magic-companion", ".Magic/companion-host.json");
        if (g.verbose) {
            HS_SetVHostVerbosity(&g.hserver, "magic-companion", 1);
        }
    }

    SG_RegisterHandler(SIGINT, MG_HandleSigInt, 0);

    MG_StartIPC();

    HS_RunForever(&g.hserver, true);
    HS_Destroy(&g.hserver);
    return 0;
}

MG_API void MG_InitNetLayer(
    const char* hostName,
    int hostNameSize,
    int port,
    const char* docsPath,
    int docsPathSize,
    int ipcPort,
    const char* magicPackageRootPath,
    int magicPackageRootPathSize,
    bool verbose,
    bool devMode
) {
    LU_Disable(&LU_GlobalLogFile);
    LU_EnableStdout(&LU_GlobalLogFile);
    LU_DisableStderr(&LU_GlobalLogFile);

    if (verbose) {
        LU_SetLogLevel(LU_Verbose);
    } else {
        LU_SetLogLevel(LU_Important);
    }

    strncpy(g.appHostName, hostName, hostNameSize);
    strncpy(g.docsPath, docsPath, docsPathSize);
    g.appPort = port;
    g.verbose = verbose;
    g.devMode = devMode;
    g.ipcPort = ipcPort;

    strncpy(g.magicPackageRootPath, magicPackageRootPath, magicPackageRootPathSize);
    getcwd(g.projectPath, sizeof(g.projectPath));

    pthread_mutex_init(&g.netEventsMutex, 0);
    pthread_mutex_init(&g.appEventsMutex, 0);

    int result = pthread_create(&g.threadId, 0, MG_RunServer, 0);
}

MG_API bool MG_ServerIsRunning() {
    return g.hserver.isRunning;
}

MG_API int MG_DoServiceWork() {
    return lws_service(g.hserver.lwsContext, 0);
}

MG_API void MG_StopServer() {
    lws_cancel_service(g.hserver.lwsContext);
    g.hserver.isRunning = false;
}

} // extern "C"

