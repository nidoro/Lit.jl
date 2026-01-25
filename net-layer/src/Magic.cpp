#include <pthread.h>
#include "DD_Assert.h"
#include "DD_HTTPS.h"
#include "DD_LogUtils.h"
#include "DD_SignalUtils.h"
#include "DD_StringUtils.h"
#include "DD_FileUtils.h"
#include "DD_RandomUtils.h" // TODO: DD_RandomUtils is not cross-platform!

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

#define MG_SESSION_ID_SIZE 32-1
#define MG_FILE_ID_SIZE 32-1

extern "C" {

struct MG_Client {
    int id;
    char sessionId[MG_SESSION_ID_SIZE+1];

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
    char sessionId[MG_SESSION_ID_SIZE+1];
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

void MG_GenSessionId(char* output) {
    bool unique = false;

    while (!unique) {
        RU_GenerateRandomString(output, MG_SESSION_ID_SIZE, RU_CHAR_SET_ALPHA_NUM);
        memcpy(output, "session_", 8);

        unique = true;
        for (int i = 0; i < arrcount(g.clients); ++i) {
            if (strcmp(g.clients[i]->sessionId, output)==0) {
                unique = false;
                break;
            }
        }
    }
}

void MG_GenFileId(char* output) {
    RU_GenerateRandomString(output, MG_FILE_ID_SIZE, RU_CHAR_SET_ALPHA_NUM);
    memcpy(output, "file_", 5);
}

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
        if (g.clients[i]->id == id) {
            return g.clients[i];
        }
    }
    return 0;
}

MG_Client* MG_GetClientBySessionId(char* sessionId) {
    for (int i = 0; i < arrcount(g.clients); ++i) {
        if (SU_AreStringsEqual(g.clients[i]->sessionId, sessionId)) {
            return g.clients[i];
        }
    }
    return 0;
}

// NOTE: Net events are created and pushed by the network layer and poped and
// destroyed by the app layer.
MG_API MG_NetEvent MG_CreateNetEvent(MG_NetEventType type, int clientId, char* sessionId, char* payload, int payloadSize) {
    MG_NetEvent ev = {
        .type=type,
        .clientId=clientId,
        .payload=payload,
        .payloadSize=payloadSize,
    };

    if (sessionId) {
        strcpy(ev.sessionId, sessionId);
    }

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
    MG_Client* mgClient = MG_GetClient(clientId);
    if (mgClient) {
        pthread_mutex_lock(mgClient->mutex);
    }
}

MG_API void MG_UnlockClient(int clientId) {
    MG_Client* mgClient = MG_GetClient(clientId);
    if (mgClient) {
        pthread_mutex_unlock(mgClient->mutex);
    }
}

MG_API int MG_ProcessIncomingMessage(HS_CallbackArgs* args) {
    MG_Client* mgClient = HS_GetClientData(MG_Client, args);

    LU_Log(LU_Debug, "IncomingMessage | Bytes: %d | Payload: %.*s", mgClient->readSize, mgClient->readSize, mgClient->readBuffer);

    MG_NetEvent ev = MG_CreateNetEvent(
        MG_NetEventType_NewPayload,
        mgClient->id,
        mgClient->sessionId,
        mgClient->readBuffer,
        mgClient->readSize
    );

    MG_PushNetEvent(ev);

    MG_WakeUpAppLayer();

    return 0;
}

MG_API int HS_CALLBACK(MG_PostRequestChecker, args) {
    HS_HTTPClient* client = HS_GetHTTPClientData(args);

    LU_Log(LU_Debug, "MG_PostRequestChecker");

    char ignore[PATH_MAX] = {};
    char sessionId[PATH_MAX] = {};

    if (SU_StartsWith(client->uri, "/.Magic/uploaded-files/")) {
        char* nodes[] = {ignore, ignore, sessionId};
        HS_GetPathNodes(client->uri, nodes);
        MG_Client* mgClient = MG_GetClientBySessionId(sessionId);

        if (mgClient) {
            return 1;
        } else {
            return 0;
        }
    }

    return 0;
}

const char* MG_GetExtension(const char* fileName, bool withDot=true) {
    const char* dot = strrchr(fileName, '.');
    if (!dot || dot == fileName) {
        return "";
    }
    if (withDot) return dot;
    return dot + 1;
}

MG_API int HS_CALLBACK(MG_PostRequestHandler, args) {
    HS_HTTPClient* client = HS_GetHTTPClientData(args);

    char ignore[PATH_MAX] = {};
    char sessionId[PATH_MAX] = {};
    char fileId[MG_FILE_ID_SIZE+1] = {};
    char filePath[PATH_MAX] = {};
    char fileName[PATH_MAX] = {};

    if (SU_StartsWith(client->uri, "/.Magic/uploaded-files/")) {
        char* nodes[] = {ignore, ignore, sessionId};
        HS_GetPathNodes(client->uri, nodes);
        MG_Client* mgClient = MG_GetClientBySessionId(sessionId);

        if (mgClient) {
            HS_GetQueryStringValue(client, "file_name", fileName, sizeof(fileName));
            if (!SU_IsEmpty(fileName)) {
                LU_Log(LU_Debug, "MG_PostRequestHandler | Session=%s | PayloadSize=%d", sessionId, client->receivedSize);

                MG_GenFileId(fileId);
                const char* ext = MG_GetExtension(fileName);
                sprintf(filePath, "%s/%s%s", client->uri+1, fileId, ext);
                FU_WriteEntireFile(filePath, client->receivedBuffer, client->receivedSize);

                HS_InitResponseBuffer(client, 512);
                client->fileSize = sprintf(client->fileContent, R"({"file_id": "%s"})", fileId);
                HS_AddHTTPHeaderStatus(client, 200);
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_LENGTH, client->fileSize);
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_TYPE, "application/json");
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CACHE_CONTROL, "no-cache, no-store, must-revalidate");
                HS_WriteResponse(client);

                return 0;
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    }

    return 1;
}

MG_API int HS_CALLBACK(MG_WSEventsHandler, args) {
    HS_VHost* vhost = HS_GetVHost(args->socket);
    MG_Client* mgClient = HS_GetClientData(MG_Client, args);

    LU_Log(LU_Debug, "HandleLWSEvent | %s", HS_ToString(args->reason));

    switch (args->reason) {
        case LWS_CALLBACK_RECEIVE: {
            HS_ReceiveMessageFragment(args, &mgClient->readBuffer, &mgClient->readSize, &mgClient->readCap, MG_ProcessIncomingMessage);
        } break;

        case LWS_CALLBACK_SERVER_WRITEABLE: {
            HS_WriteNextPacket(&mgClient->writeQueue);
        } break;

        case LWS_CALLBACK_ESTABLISHED: {
            mgClient->id = g.nextClientId++;
            MG_GenSessionId(mgClient->sessionId);
            mgClient->writeQueue = HS_CreatePacketQueue(args->socket, 128);
            mgClient->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(mgClient->mutex, 0);
            arradd(g.clients, mgClient);

            MG_NetEvent ev = MG_CreateNetEvent(MG_NetEventType_NewClient, mgClient->id, mgClient->sessionId, 0, 0);
            MG_PushNetEvent(ev);
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
                .clientId = mgClient->id,
            });

            MG_WakeUpAppLayer();

            pthread_mutex_lock(mgClient->mutex);
            arrremovematch(g.clients, mgClient);
            free(mgClient->mutex);
        } break;

        case LWS_CALLBACK_RAW_RX: {
            char buf[1024] = {};
            read(g.fdSocket, buf, sizeof(buf));

            while (arrcount(g.appEvents)) {
                MG_AppEvent ev = MG_PopAppEvent();
                mgClient = MG_GetClient(ev.clientId);

                if (mgClient) {
                    if (ev.type == MG_AppEventType_NewPayload) {
                        LU_Log(LU_Debug, "AppEventType_NewPayload | %d | %.*s", ev.clientId, MIN(ev.payloadSize-LWS_PRE, 256), ev.payload+LWS_PRE);

                        HS_Packet packet = {
                            .buffer = ev.payload,
                            .bufferSize = ev.payloadSize,
                            .body = ev.payload+LWS_PRE,
                            .bodySize = ev.payloadSize-LWS_PRE,
                        };

                        HS_SendPacket(&mgClient->writeQueue, packet);
                        MG_DestroyAppEvent(ev);
                    } else {
                        DD_Assert2(0, "Unknown event %d", ev.type);
                    }
                } else {
                    // TODO: What to do if mgClient is no longer online?
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

    MG_NetEvent ev = MG_CreateNetEvent(MG_NetEventType_ServerLoopInterrupted, 0, 0, 0, 0);
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
    HS_SetVHostHostName(&g.hserver, "magic-app", g.appHostName);
    HS_SetVHostPort(&g.hserver, "magic-app", g.appPort);
    HS_SetLWSVHostConfig(&g.hserver, "magic-app", pt_serv_buf_size, HS_KILO_BYTES(12));
    HS_SetLWSProtocolConfig(&g.hserver, "magic-app", "HTTP", rx_buffer_size, HS_KILO_BYTES(12));
    HS_SetHTTPGetHandler(&g.hserver, "magic-app", HS_GetFileByURI);
    HS_SetHTTPPostEndpointChecker(&g.hserver, "magic-app", MG_PostRequestChecker);
    HS_SetHTTPPostHandler(&g.hserver, "magic-app", MG_PostRequestHandler);
    HS_AddProtocol(&g.hserver, "magic-app", "ws", MG_WSEventsHandler, MG_Client);
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

    if (g.docsPath[0]) {
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

        // HACK: Create a lit.coisasdodavi.net vhost that redirects
        // to magic.coisasdodavi.net. This is temporary, just while the package
        // name change is recent.
        HS_AddRedirToHTTPSVHost(&g.hserver, "lit-redir", "lit.coisasdodavi.net", 443, "magic.coisasdodavi.net", 443);
        if (!disableSSL) {
            HS_SetCertificate(&g.hserver, "lit-redir", ".Magic/certs/certificate.crt", ".Magic/certs/private.key");
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
    // TODO: DD_RandomUtils is not cross-platform!
    RU_OpenURandomDevice();

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

