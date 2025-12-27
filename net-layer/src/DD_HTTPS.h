#ifndef DD_HTTPS_H
#define DD_HTTPS_H

#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include "libwebsockets.h"
#include "DD_SQLite.h"
#include "DD_JSON.h"

#define HS_KILO_BYTES(x) (1024*x)
#define HS_MEGA_BYTES(x) (1024*1024*x)
#define HS_GIGA_BYTES(x) (1024*1024*1024*x)

#undef HS_Assert
#undef HS_Assert1
#undef HS_Assert2

#define HS_Assert(expression) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d)\n", __FILE__, __LINE__);\
        *((int*) 0) = 0;\
    }

#define HS_Assert1(expression, msg) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d): ", __FILE__, __LINE__);\
        fprintf(stderr, msg);\
        fprintf(stderr, "\n");\
        *((int*) 0) = 0;\
    }

#define HS_Assert2(expression, format, ...) \
    if (!(expression)) {\
        fprintf(stderr, "Assertion failed at %s (%d): ", __FILE__, __LINE__);\
        fprintf(stderr, format, __VA_ARGS__);\
        fprintf(stderr, "\n");\
        *((int*) 0) = 0;\
    }
    
#define HS__VHostsArrayCap 10
#define HS__ProtocolsArrayCap 8
#define HS__HostNameCap 64
#define HS__URIMapCap 32
#define HS__CacheBustCap 48
#define HS__CacheControlMapCap 32
#define HS__NeedsSSIParsingCap 32
#define HS__RedirectMapCap 16
#define HS__RootDirMapCap 16
#define HS__FileMapCap 256
#define HS__URICap 2000
#define HS__FilePathCap 2048
#define HS__PostEndpointsCap 8
#define HS__AllowedOriginsArrayCap 8
#define HS__PostBufferSize HS_KILO_BYTES(8)
#define HS__ResponseHandlersArrayCap 8
#define HS__PluginNameCap 64
#define HS__PluginArrayCap 4
#define HS__CertTrustStoreCap 8

// Base64 Helpers
//-----------------
double HS_Ceil(double x) {
    long long i = (long long)x;
    if (x > 0 && x != (double)i)
        return (double)(i + 1);
    return (double)i;
}

int HS_EstimateDecodedBase64Size(const char* encodedB64) {
    float encodedSize = strlen(encodedB64);
    int result = HS_Ceil(encodedSize*(3.f/4.f));
    return result;
}

int HS_DecodeBase64(const char* input, int inputSize, char* output) {
    int padding = (inputSize % 4);
    int bufSize = inputSize + padding + 1;
    char* buf = (char*) calloc(1, bufSize);
    memcpy(buf, input, inputSize);
    sprintf(buf + inputSize, "%.*s", padding, "===");
    
    int len = EVP_DecodeBlock((uint8_t*) output, (uint8_t*) buf, strlen(buf));
    free(buf);
    
    int result = len - padding;
    output[result] = 0;
    
    return result;
}

typedef int (*HS_LWSCallback)(lws* socket, lws_callback_reasons reason, void* userData, void* in, size_t len);

#define HS_CALLBACK(func, args) \
func(HS_CallbackArgs* args); \
int HS_LWSCallback_ ## func (lws* socket, lws_callback_reasons reason, void* userData, void* in, size_t len) { \
    HS_CallbackArgs args = {}; \
    args.socket = socket; \
    args.reason = reason; \
    args.userData = userData; \
    args.in = in; \
    args.len = len; \
    return func(&args); \
}\
int func(HS_CallbackArgs* args)

#define HS_ImplementCallback(userCallback) \
int HS_LWSCallback_ ## userCallback (lws* socket, lws_callback_reasons reason, void* userData, void* in, size_t len) { \
    HS_CallbackArgs args = {}; \
    args.socket = socket; \
    args.reason = reason; \
    args.userData = userData; \
    args.in = in; \
    args.len = len; \
    return userCallback(&args); \
}

#define HS_Min(a,b) (a < b ? a:b)

struct HS_CallbackArgs {
    lws* socket;
    lws_callback_reasons reason;
    void* userData;
    void* in;
    size_t len;
    
    int result;
};

typedef int (*HS_CallbackFunc)(HS_CallbackArgs* args);

struct HS_FileMapEntry {
    char uri[HS__URICap];
    char filePath[HS__FilePathCap];
    
    char* fileBuffer;
    char* fileContent;
    int   fileSize;
    
    const char* mimeType;
    char* cacheControl;
    int cacheControlSize;
    int clientsReading;
};

struct HS_URIMapEntry {
    char uri[HS__URICap];
    int  uriSize;
    char resource[HS__FilePathCap];
};

struct HS_RedirectMapEntry {
    char uri[HS__URICap];
    char destination[HS__URICap];
};

struct HS_RootDirMapEntry {
    char uriPrefix[HS__URICap];
    char path[HS__URICap];
};

struct HS_CacheControlMapEntry {
    char uri[HS__URICap];
    int  uriSize;
    char cacheString[128];
};

struct HS_AllowedOrigin {
    char dest[HS__URICap];
    char origin[HS__URICap];
};

typedef int (*HS_UserCallback)(HS_CallbackArgs* args);

struct HS_ResponseHandler {
    char hostName[HS__URICap];
    HS_CallbackFunc handler;
};

struct HS_HTTPClient;

struct HS_Plugin {
    char name[HS__PluginNameCap];
    void* data;
    bool (*wantsToHandleRequest)(HS_HTTPClient* client);
    HS_UserCallback requestHandler;
    int sessionDataSize;
};

struct HS_Server;

struct HS_VHost {
    JS_JSON* jConfig;
    JS_JSON* gkConfig;
    
    char gkDatabasePath[PATH_MAX];
    sqlite3* gkdb;
    
    char hostName[HS__HostNameCap];
    int port;
    char host[HS__HostNameCap+16];
    
    char name[HS__HostNameCap];
    HS_UserCallback callback;
    
    char sslPublicKeyPath[PATH_MAX];
    char sslPrivateKeyPath[PATH_MAX];
    char sslCABundlePath[PATH_MAX];
    
    // internal
    int verbosity;
    
    lws_context_creation_info lwsContextInfo;
    lws_vhost* lwsVHost;
    
    lws_protocols lwsProtocolDefaults;
    
    lws_protocols lwsProtocols[HS__ProtocolsArrayCap];
    int lwsProtocolsCount;
    
    lws_http_mount lwsMounts[8];
    int lwsMountsSize;
    
    // http
    HS_UserCallback httpGetHandler;
    HS_UserCallback httpPostHandler;
    HS_UserCallback httpDeleteHandler;
    HS_UserCallback httpGetEndpointChecker;
    HS_UserCallback httpPostEndpointChecker;
    HS_UserCallback httpDeleteEndpointChecker;
    
    char postEndpoints[HS__PostEndpointsCap][HS__URICap];
    int postEndpointsSize;
    
    char deleteEndpoints[HS__PostEndpointsCap][HS__URICap];
    int deleteEndpointsSize;
    
    int nextHTTPClientId;
    int h2MaxFrameSize;
    
    char* frameBuffer;
    char* frameStart;
    
    int sessionDataSize;
    
    const char* certTrustStore[HS__CertTrustStoreCap];
    int certTrustStoreCount;
    
    // file server
    char servedFilesRootDir[HS__FilePathCap];
    char error404File[HS__URICap];
    
    HS_FileMapEntry* loadedFiles;
    int              loadedFilesCount;
    bool disableFileCache;
    int  memCacheMaxSizeMB;
    
    HS_URIMapEntry uriMap[HS__URIMapCap];
    int         uriMapSize;
    
    HS_CacheControlMapEntry cacheControlMap[HS__CacheControlMapCap];
    int                     cacheControlMapSize;
    
    char cacheBust[HS__CacheBustCap][HS__FilePathCap];
    int  cacheBustSize;
    
    char cacheBustVersion[32];
    char defaultContentLanguage[32];
    
    char needsSSIParsing[HS__NeedsSSIParsingCap][HS__FilePathCap];
    int  needsSSIParsingSize;
    
    HS_RedirectMapEntry redirectMap[HS__RedirectMapCap];
    int                 redirectMapSize;

    HS_RootDirMapEntry  rootDirMap[HS__RootDirMapCap];
    int                 rootDirMapSize;
    
    HS_AllowedOrigin allowedOrigins[HS__AllowedOriginsArrayCap];
    int  allowedOriginsCount;
    
    // requester
    HS_ResponseHandler responseHandlers[HS__ResponseHandlersArrayCap];
    int responseHandlersCount;
    
    // plugin
    HS_Plugin plugins[HS__PluginArrayCap];
    int pluginCount;
};

typedef void (*HS_PeriodicTask)(HS_Server* server);
typedef void (*HS_Task)(lws_sorted_usec_list_t* _server);
typedef lws_sorted_usec_list_t HS_ScheduledTask;

struct HS_SchedulerPeriodicEntry {
    lws_sorted_usec_list_t entry;
    uint32_t nanoSeconds;
    HS_Server* server;
    HS_PeriodicTask task;
};

struct HS_Server {
    bool isRunning;
    int verbosity;
    
    HS_VHost vhosts[HS__VHostsArrayCap];
    int vhostsCount;
    
    void* userData;
    
    // internal
    lws_context_creation_info lwsContextInfo;
    lws_context* lwsContext;
    
    int defaultConnectionFlags;
    
    HS_SchedulerPeriodicEntry schedulerPeriodicEntries[8];
    int schedulerPeriodicEntriesCount;
    
    // FIXME: we need a proper way to schedule multiple single time tasks.
    // With a single lws_sorted_usec_list_t on HS_Server, that's not possible.
    // It is necessary that we have one lws_sorted_usec_list_t for each task.
    lws_sorted_usec_list_t schedulerEntry;
};

struct HS_Replacement {
    const char* replaced;
    const char* replacement;
    int replacedSize;
    int replacementSize;
};

int HS_Replace(char* outBuffer, char* inBuffer, int inLength, HS_Replacement* reps) {
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

void HS_AddPlugin(HS_VHost* vhost, HS_Plugin plugin) {
    vhost->plugins[vhost->pluginCount++] = plugin;
}

HS_Plugin* HS_GetPlugin(HS_VHost* vhost, const char* name) {
    for (int i = 0; i < vhost->pluginCount; ++i) {
        if (strcmp(vhost->plugins[i].name, name)==0) {
            return &vhost->plugins[i];
        }
    }
    return 0;
}

void* HS_GetPluginData(HS_VHost* vhost, const char* name) {
    HS_Plugin* plugin = HS_GetPlugin(vhost, name);
    if (plugin) {
        return plugin->data;
    }
    return 0;
}

bool HS_IsFileReadable(const char* formatString, ...) {
    char path[PATH_MAX];
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    
    FILE* f = fopen(path, "r");
    bool result = f;
    if (f) fclose(f);
    return result;
}

void HS_ToLower(char* output, const char* string) {
    int i;
    for(i = 0; string[i]; i++){
        output[i] = tolower(string[i]);
    }
    output[i] = 0;
}

void HS_ToLower(char* string) {
    HS_ToLower(string, (const char*) string);
}

int HS_CallbackStub(HS_CallbackArgs* args) {return 0;}
int HS_LWSCallbackStub(lws* socket, lws_callback_reasons reason, void* userData, void* in, size_t len) {return 0;}

HS_VHost* HS_GetVHost(lws* socket) {
    lws_vhost* vh = lws_get_vhost(socket);
    if (vh) {
        return (HS_VHost*) lws_vhost_user(vh);
    } else {
        return 0;
    }
}

HS_VHost* HS_GetVHost(HS_CallbackArgs* args) {
    return HS_GetVHost(args->socket);
}

HS_Server* HS_GetServer(HS_CallbackArgs* args) {
    return (HS_Server*) lws_context_user(lws_get_context(args->socket));
}

void HS_PeriodicSchedulerCallback(lws_sorted_usec_list_t* entry) {
    HS_SchedulerPeriodicEntry* schedulerEntry = (HS_SchedulerPeriodicEntry*) entry;
    HS_Server* server = schedulerEntry->server;
    
    schedulerEntry->task(server);
    
    lws_sul_schedule(server->lwsContext, 0, &schedulerEntry->entry, HS_PeriodicSchedulerCallback, schedulerEntry->nanoSeconds);
}

void HS_SchedulePeriodicTask(HS_Server* server, HS_PeriodicTask task, uint64_t ms) {
    HS_SchedulerPeriodicEntry& schedulerEntry = server->schedulerPeriodicEntries[server->schedulerPeriodicEntriesCount++];
    schedulerEntry.server = server;
    schedulerEntry.task = task;
    schedulerEntry.nanoSeconds = ms*1000;
}

void HS_InitPeriodicTasks(HS_Server* server) {
    for (int i = 0; i < server->schedulerPeriodicEntriesCount; ++i) {
        HS_SchedulerPeriodicEntry& schedulerEntry = server->schedulerPeriodicEntries[i];
        lws_sul_schedule(server->lwsContext, 0, &schedulerEntry.entry, HS_PeriodicSchedulerCallback, schedulerEntry.nanoSeconds);
    }
}

#define HS_GetClientData(clientType, args) ((clientType*) (args)->userData)
#define HS_GetServerData(serverType, args) ((serverType*) HS_GetServer(args)->userData)

void HS_GetIpv4(HS_CallbackArgs* args, char* result) {
    lws_get_peer_simple(args->socket, result, 16);
}

HS_VHost* HS_GetVHost(HS_Server* server, const char* name) {
    for (int i = 0; i < server->vhostsCount; ++i) {
        if (strcmp(server->vhosts[i].name, name) == 0) {
            return &server->vhosts[i];
        }
    }
    return 0;
}

void HS_PushPostEndpoint(HS_Server* server, const char* vhostName, const char* endpoint) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    char* entry = vhost->postEndpoints[vhost->postEndpointsSize++];
    strcpy(entry, endpoint);
}

void HS_PushCacheBust(HS_Server* server, const char* vhostName, const char* resource) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    char* entry = vhost->cacheBust[vhost->cacheBustSize++];
    strcpy(entry, resource);
}

void HS_PushSSITarget(HS_Server* server, const char* vhostName, const char* resource) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    char* entry = vhost->needsSSIParsing[vhost->needsSSIParsingSize++];
    strcpy(entry, resource);
}

void HS_PushCacheControlMapping(HS_Server* server, const char* vhostName, const char* uri, const char* cacheString) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    HS_CacheControlMapEntry& entry = vhost->cacheControlMap[vhost->cacheControlMapSize++];
    strcpy(entry.uri, uri);
    entry.uriSize = strlen(uri);
    strcpy(entry.cacheString, cacheString);
}

void HS_PushRedirectMapping(HS_Server* server, const char* vhostName, const char* uri, const char* destination) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    HS_RedirectMapEntry& entry = vhost->redirectMap[vhost->redirectMapSize++];
    strcpy(entry.uri, uri);
    strcpy(entry.destination, destination);
}

// TODO: This reasons list is outdated
//-------------------------------------
const char* HS_ToString(lws_callback_reasons reason) {
    switch(reason) {
      case LWS_CALLBACK_PROTOCOL_INIT                            : return "LWS_CALLBACK_PROTOCOL_INIT";
      case LWS_CALLBACK_PROTOCOL_DESTROY                         : return "LWS_CALLBACK_PROTOCOL_DESTROY";
      case LWS_CALLBACK_WSI_CREATE                               : return "LWS_CALLBACK_WSI_CREATE";
      case LWS_CALLBACK_WSI_DESTROY                              : return "LWS_CALLBACK_WSI_DESTROY";
      case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS   : return "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS";
      case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS   : return "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS";
      case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION : return "LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION";
      //case LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY     : return "LWS_CALLBACK_OPENSSL_CONTEXT_REQUIRES_PRIVATE_KEY";
      case LWS_CALLBACK_SSL_INFO                                 : return "LWS_CALLBACK_SSL_INFO";
      case LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION : return "LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION";
      case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED           : return "LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED";
      case LWS_CALLBACK_HTTP                                     : return "LWS_CALLBACK_HTTP";
      case LWS_CALLBACK_HTTP_BODY                                : return "LWS_CALLBACK_HTTP_BODY";
      case LWS_CALLBACK_HTTP_BODY_COMPLETION                     : return "LWS_CALLBACK_HTTP_BODY_COMPLETION";
      case LWS_CALLBACK_HTTP_FILE_COMPLETION                     : return "LWS_CALLBACK_HTTP_FILE_COMPLETION";
      case LWS_CALLBACK_HTTP_WRITEABLE                           : return "LWS_CALLBACK_HTTP_WRITEABLE";
      case LWS_CALLBACK_CLOSED_HTTP                              : return "LWS_CALLBACK_CLOSED_HTTP";
      case LWS_CALLBACK_FILTER_HTTP_CONNECTION                   : return "LWS_CALLBACK_FILTER_HTTP_CONNECTION";
      case LWS_CALLBACK_ADD_HEADERS                              : return "LWS_CALLBACK_ADD_HEADERS";
      case LWS_CALLBACK_CHECK_ACCESS_RIGHTS                      : return "LWS_CALLBACK_CHECK_ACCESS_RIGHTS";
      case LWS_CALLBACK_PROCESS_HTML                             : return "LWS_CALLBACK_PROCESS_HTML";
      case LWS_CALLBACK_HTTP_BIND_PROTOCOL                       : return "LWS_CALLBACK_HTTP_BIND_PROTOCOL";
      case LWS_CALLBACK_HTTP_DROP_PROTOCOL                       : return "LWS_CALLBACK_HTTP_DROP_PROTOCOL";
      case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE                     : return "LWS_CALLBACK_HTTP_CONFIRM_UPGRADE";
      case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP                  : return "LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP";
      case LWS_CALLBACK_CLOSED_CLIENT_HTTP                       : return "LWS_CALLBACK_CLOSED_CLIENT_HTTP";
      case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ                 : return "LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ";
      case LWS_CALLBACK_RECEIVE_CLIENT_HTTP                      : return "LWS_CALLBACK_RECEIVE_CLIENT_HTTP";
      case LWS_CALLBACK_COMPLETED_CLIENT_HTTP                    : return "LWS_CALLBACK_COMPLETED_CLIENT_HTTP";
      case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE                    : return "LWS_CALLBACK_CLIENT_HTTP_WRITEABLE";
//      case LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL                : return "LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL";
//      case LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL                : return "LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL";
      case LWS_CALLBACK_ESTABLISHED                              : return "LWS_CALLBACK_ESTABLISHED";
      case LWS_CALLBACK_CLOSED                                   : return "LWS_CALLBACK_CLOSED";
      case LWS_CALLBACK_SERVER_WRITEABLE                         : return "LWS_CALLBACK_SERVER_WRITEABLE";
      case LWS_CALLBACK_RECEIVE                                  : return "LWS_CALLBACK_RECEIVE";
      case LWS_CALLBACK_RECEIVE_PONG                             : return "LWS_CALLBACK_RECEIVE_PONG";
      case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE                  : return "LWS_CALLBACK_WS_PEER_INITIATED_CLOSE";
      case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION               : return "LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION";
      case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY                   : return "LWS_CALLBACK_CONFIRM_EXTENSION_OKAY";
//      case LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL                  : return "LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL";
//      case LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL                  : return "LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL";
      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR                  : return "LWS_CALLBACK_CLIENT_CONNECTION_ERROR";
      case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH              : return "LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH";
      case LWS_CALLBACK_CLIENT_ESTABLISHED                       : return "LWS_CALLBACK_CLIENT_ESTABLISHED";
      case LWS_CALLBACK_CLIENT_CLOSED                            : return "LWS_CALLBACK_CLIENT_CLOSED";
      case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER           : return "LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER";
      case LWS_CALLBACK_CLIENT_RECEIVE                           : return "LWS_CALLBACK_CLIENT_RECEIVE";
      case LWS_CALLBACK_CLIENT_RECEIVE_PONG                      : return "LWS_CALLBACK_CLIENT_RECEIVE_PONG";
      case LWS_CALLBACK_CLIENT_WRITEABLE                         : return "LWS_CALLBACK_CLIENT_WRITEABLE";
      case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED       : return "LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED";
      case LWS_CALLBACK_WS_EXT_DEFAULTS                          : return "LWS_CALLBACK_WS_EXT_DEFAULTS ";
      case LWS_CALLBACK_FILTER_NETWORK_CONNECTION                : return "LWS_CALLBACK_FILTER_NETWORK_CONNECTION";
//      case LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL                  : return "LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL  ";
//      case LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL                  : return "LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL  ";
      case LWS_CALLBACK_GET_THREAD_ID                            : return "LWS_CALLBACK_GET_THREAD_ID";
      case LWS_CALLBACK_ADD_POLL_FD                              : return "LWS_CALLBACK_ADD_POLL_FD";
      case LWS_CALLBACK_DEL_POLL_FD                              : return "LWS_CALLBACK_DEL_POLL_FD";
      case LWS_CALLBACK_CHANGE_MODE_POLL_FD                      : return "LWS_CALLBACK_CHANGE_MODE_POLL_FD";
      case LWS_CALLBACK_LOCK_POLL                                : return "LWS_CALLBACK_LOCK_POLL";
      case LWS_CALLBACK_UNLOCK_POLL                              : return "LWS_CALLBACK_UNLOCK_POLL";
      case LWS_CALLBACK_CGI                                      : return "LWS_CALLBACK_CGI";
      case LWS_CALLBACK_CGI_TERMINATED                           : return "LWS_CALLBACK_CGI_TERMINATED";
      case LWS_CALLBACK_CGI_STDIN_DATA                           : return "LWS_CALLBACK_CGI_STDIN_DATA";
      case LWS_CALLBACK_CGI_STDIN_COMPLETED                      : return "LWS_CALLBACK_CGI_STDIN_COMPLETED";
      case LWS_CALLBACK_CGI_PROCESS_ATTACH                       : return "LWS_CALLBACK_CGI_PROCESS_ATTACH";
      case LWS_CALLBACK_SESSION_INFO                             : return "LWS_CALLBACK_SESSION_INFO";
      case LWS_CALLBACK_GS_EVENT                                 : return "LWS_CALLBACK_GS_EVENT";
      case LWS_CALLBACK_HTTP_PMO                                 : return "LWS_CALLBACK_HTTP_PMO";
//      case LWS_CALLBACK_RAW_PROXY_CLI_RX                         : return "LWS_CALLBACK_RAW_PROXY_CLI_RX         ";
//      case LWS_CALLBACK_RAW_PROXY_SRV_RX                         : return "LWS_CALLBACK_RAW_PROXY_SRV_RX         ";
//      case LWS_CALLBACK_RAW_PROXY_CLI_CLOSE                      : return "LWS_CALLBACK_RAW_PROXY_CLI_CLOSE      ";
//      case LWS_CALLBACK_RAW_PROXY_SRV_CLOSE                      : return "LWS_CALLBACK_RAW_PROXY_SRV_CLOSE      ";
//      case LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE                  : return "LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE  ";
//      case LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE                  : return "LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE  ";
//      case LWS_CALLBACK_RAW_PROXY_CLI_ADOPT                      : return "LWS_CALLBACK_RAW_PROXY_CLI_ADOPT      ";
//      case LWS_CALLBACK_RAW_PROXY_SRV_ADOPT                      : return "LWS_CALLBACK_RAW_PROXY_SRV_ADOPT      ";
//      case LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL              : return "LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL";
//      case LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL              : return "LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL";
//      case LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL              : return "LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL";
//      case LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL              : return "LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL";
      case LWS_CALLBACK_RAW_RX                                   : return "LWS_CALLBACK_RAW_RX";
      case LWS_CALLBACK_RAW_CLOSE                                : return "LWS_CALLBACK_RAW_CLOSE";
      case LWS_CALLBACK_RAW_WRITEABLE                            : return "LWS_CALLBACK_RAW_WRITEABLE";
      case LWS_CALLBACK_RAW_ADOPT                                : return "LWS_CALLBACK_RAW_ADOPT";
//      case LWS_CALLBACK_RAW_CONNECTED                            : return "LWS_CALLBACK_RAW_CONNECTED";
//      case LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL                    : return "LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL";
//      case LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL                    : return "LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL";
      case LWS_CALLBACK_RAW_ADOPT_FILE                           : return "LWS_CALLBACK_RAW_ADOPT_FILE";
      case LWS_CALLBACK_RAW_RX_FILE                              : return "LWS_CALLBACK_RAW_RX_FILE";
      case LWS_CALLBACK_RAW_WRITEABLE_FILE                       : return "LWS_CALLBACK_RAW_WRITEABLE_FILE";
      case LWS_CALLBACK_RAW_CLOSE_FILE                           : return "LWS_CALLBACK_RAW_CLOSE_FILE";
//      case LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL                   : return "LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL";
//      case LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL                   : return "LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL";
      case LWS_CALLBACK_TIMER                                    : return "LWS_CALLBACK_TIMER";
      case LWS_CALLBACK_EVENT_WAIT_CANCELLED                     : return "LWS_CALLBACK_EVENT_WAIT_CANCELLED";
      case LWS_CALLBACK_CHILD_CLOSING                            : return "LWS_CALLBACK_CHILD_CLOSING";
      case LWS_CALLBACK_VHOST_CERT_AGING                         : return "LWS_CALLBACK_VHOST_CERT_AGING";
      case LWS_CALLBACK_VHOST_CERT_UPDATE                        : return "LWS_CALLBACK_VHOST_CERT_UPDATE";
//      case LWS_CALLBACK_CHILD_WRITE_VIA_PARENT                   : return "LWS_CALLBACK_CHILD_WRITE_VIA_PARENT";
      case LWS_CALLBACK_USER                                     : return "LWS_CALLBACK_USER";
      default: return "Unknown lws_callback_reasons";
    }
}

lws_protocols* HS_GetProtocol(HS_VHost* vhost, const char* protocolName) {
    for (int i = 0; i < vhost->lwsProtocolsCount; ++i) {
        if (strcmp(protocolName, vhost->lwsProtocols[i].name)==0) {
            return &vhost->lwsProtocols[i];
        }
    }
    return 0;
}

#define HS_GetProtocolConfig(vhost, protocolName, configName) HS_GetProtocol(vhost, protocolName)->configName

int HS_GetH2FrameMaxSize(HS_VHost* vhost) {
    lws_protocols& protocol = vhost->lwsProtocols[0];
    
    int result = HS_KILO_BYTES(4);
    
    if (protocol.tx_packet_size) {
        result = protocol.tx_packet_size;
    } else if (protocol.rx_buffer_size) {
        result = protocol.rx_buffer_size;
    }
    
    return result;
}

struct HS_Date {
    int year;  // [1900...]
    int month; // [1..12]
    int day;   // [1..31]
    
    int hour;   // [0..23]
    int minute; // [0..59]
    int second; // [0..59]
};

HS_Date HS_GetDateNow() {
    HS_Date date = {};
    time_t t = time(0);
    tm tt = *localtime(&t);
    date.year = tt.tm_year + 1900;
    date.month = tt.tm_mon + 1;
    date.day = tt.tm_mday;
    date.hour = tt.tm_hour;
    date.minute = tt.tm_min;
    date.second = tt.tm_sec;
    return date;
}

//---------------------------
// FILE SERVER
// FILE SERVER
// FILE SERVER
//---------------------------
struct HS_UTMParams {
    char source[256];
    char medium[256];
    char campaign[256];
    char content[256];
    char term[256];
};

struct HS_HTTPClient {
    int id;
    lws* socket;
    
    char httpMethod[16];
    bool requestProcessed;
    
    char ipAddress[16];
    char host[256];
    char referer[1024];
    char acceptLanguage[1024];
    char userAgent[1024];
    char contentType[256];
    char origin[HS__HostNameCap];
    
    HS_UTMParams utm;
    
    int  contentLength;
    
    char* receivedBuffer;
    int   receivedCap;
    int   receivedSize;
    
    char uri[HS__URICap];
    int  uriSize;
    
    char  headerBuffer[LWS_PRE + 2048];
    char* headerAt;
    char* headerBegin;
    char* headerEnd;
    int   headerSize;
    
    char filePath[HS__FilePathCap];
    
    char* fileBuffer;
    char* fileContent;
    int   fileSize;
    int   at; // file writing progress
    char  contentLanguage[16];
    
    HS_FileMapEntry* fileEntry;
    
    bool closeConnection;
    http_status closeStatus;
    
    void* sessionData;
    bool delayBodyFree;
};

void HS_DelayBodyFree(HS_HTTPClient* client) {
    client->delayBodyFree = true;
}

void HS_CloseConnection(HS_HTTPClient* client, int closeStatus) {
    client->closeConnection = true;
    client->closeStatus = (http_status) closeStatus;
    lws_callback_on_writable(client->socket);
}

void HS_CloseConnection(HS_HTTPClient* client) {
    HS_CloseConnection(client, client->closeStatus);
}

void HS_Redirect(HS_HTTPClient* client, const char* dest, int httpStatus=301) {
    lws_http_redirect(client->socket, httpStatus, (uint8_t*) dest, strlen(dest), (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
}

bool HS_AddHTTPHeader(HS_HTTPClient* client, lws_token_indexes header, const char* value) {
    return 0 == lws_add_http_header_by_token(client->socket, header, (uint8_t*) value, strlen(value), (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
}

bool HS_AddHTTPHeader(HS_HTTPClient* client, const char* header, const char* value) {
    char buf[128] = {};
    sprintf(buf, "%s:", header);
    return 0 == lws_add_http_header_by_name(client->socket, (uint8_t*) buf, (uint8_t*) value, strlen(value), (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
}

bool HS_AddHTTPHeader(HS_HTTPClient* client, lws_token_indexes header, int value) {
    char buf[64] = {};
    sprintf(buf, "%d", value);
    return HS_AddHTTPHeader(client, header, buf);
}

bool HS_AddHTTPHeaderStatus(HS_HTTPClient* client, int status) {
    client->closeStatus = (http_status) status;
    return 0 == lws_add_http_header_status(client->socket, status, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
}

bool HS__FinalizeHTTPHeader(HS_HTTPClient* client) {
    int r = lws_finalize_http_header(client->socket, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
    client->headerSize = client->headerAt - client->headerBegin;
    return r == 0;
}

void HS_WriteHeaders(HS_HTTPClient* client) {
    //HS__FinalizeHTTPHeader(client);
    //lws_write(client->socket, (uint8_t*) client->headerBegin, client->headerSize, LWS_WRITE_HTTP_HEADERS);
    lws_finalize_write_http_header(client->socket, (uint8_t*) client->headerBegin, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
}

void HS_WriteBody(HS_HTTPClient* client) {
    lws_callback_on_writable(client->socket);
}

void HS_WriteResponse(HS_HTTPClient* client) {
    HS_WriteHeaders(client);
    HS_WriteBody(client);
}

#define HS_PrintIntoResponse(client, ...) client->fileSize += sprintf(client->fileContent + client->fileSize, __VA_ARGS__)

HS_FileMapEntry* HS_GetFileEntryByURI(HS_FileMapEntry* entries, int entryCount, char* uri) {
    for (int i = 0; i < entryCount; ++i) {
        if (strcmp(uri, entries[i].uri)==0) {
            return &entries[i];
        }
    }
    return 0;
}

bool HS_GetCookieValue(lws* socket, const char* key, char* resultBuffer, int resultBufferSize) {
    char cookie[HS_KILO_BYTES(4)] = {};
    lws_hdr_copy(socket, cookie, sizeof(cookie), WSI_TOKEN_HTTP_COOKIE);
    
    char keyWithEqual[HS__URICap] = {};
    int keyWithEqualSize = sprintf(keyWithEqual, "%s=", key);
    
    char* at = strtok(cookie, ";");
    while (at && *at == ' ') ++at;
    
    while (at && *at) {
        int entryLen = strlen(at);
        
        if (entryLen >= keyWithEqualSize && memcmp(at, keyWithEqual, keyWithEqualSize)==0) {
            char* value = at + keyWithEqualSize;
            strncpy(resultBuffer, value, resultBufferSize);
            return true;
        } else {
            at = strtok(0, ";");
            while (at && *at == ' ') ++at;
        }
    }
    
    return false;
}

bool HS_GetQueryStringValue(lws* socket, const char* key, char* resultBuffer, int resultBufferSize) {
    char keyWithEqual[HS__URICap] = {};
    char valueBuffer[HS__URICap] = {};
    sprintf(keyWithEqual, "%s=", key);
    
    if (lws_get_urlarg_by_name(socket, keyWithEqual, valueBuffer, sizeof(valueBuffer))) {
        strncpy(resultBuffer, valueBuffer, resultBufferSize);
        return true;
    }
    
    memset(resultBuffer, 0, resultBufferSize);
    return false;
}

bool HS_GetQueryStringValue(HS_HTTPClient* client, const char* key, char* resultBuffer, int resultBufferSize) {
    return HS_GetQueryStringValue(client->socket, key, resultBuffer, resultBufferSize);
}

bool HS_GetQueryStringValue(HS_HTTPClient* client, const char* key, int* result) {
    char buf[64] = {};
    
    if (HS_GetQueryStringValue(client->socket, key, buf, sizeof(buf))) {
        *result = atoi(buf);
        return true;
    }
    
    return false;
}

void HS_InitResponseBuffer(HS_HTTPClient* client, size_t size) {
    client->fileBuffer = (char*) calloc(1, LWS_PRE + size);
    client->fileContent = client->fileBuffer + LWS_PRE;
}

void HS_AppendBytesToResponse(HS_HTTPClient* client, void* bytes, int count) {
    memcpy(client->fileContent, bytes, count);
    client->fileSize += count;
}

void HS_AppendStringToResponse(HS_HTTPClient* client, const char* string) {
    HS_AppendBytesToResponse(client, (void*) string, strlen(string));
}

bool HS_GetUTMParams(lws* socket, HS_UTMParams* result) {
    *result = {};
    HS_GetQueryStringValue(socket, "utm_source", result->source, sizeof(result->source));
    HS_GetQueryStringValue(socket, "utm_medium", result->medium, sizeof(result->medium));
    HS_GetQueryStringValue(socket, "utm_campaign", result->campaign, sizeof(result->campaign));
    HS_GetQueryStringValue(socket, "utm_content", result->content, sizeof(result->content));
    HS_GetQueryStringValue(socket, "utm_term", result->term, sizeof(result->term));
    
    return result->source[0] || result->medium[0] || result->campaign[0] || result->content[0] || result->term[0];
}

bool HS_ContainsUTM(HS_HTTPClient* client) {
    return client->utm.source[0] || client->utm.medium[0] || client->utm.campaign[0] || client->utm.content[0] || client->utm.term[0];
}

void HS_GetURIQueryStringWithExceptions(lws* socket, char* resultBuffer, const char** exceptions, int exceptionsCount) {
    int resultSize = 0;
    int fragIndex = 0;
    char fragBuffer[4096] = {};
    int copyResult = lws_hdr_copy_fragment(socket, fragBuffer, sizeof(fragBuffer), WSI_TOKEN_HTTP_URI_ARGS, fragIndex);
    
    while (copyResult != -1) {
        char key[256] = {};
        int at = 0;
        while (fragBuffer[at] && fragBuffer[at] != '=') {
            key[at] = fragBuffer[at];
            ++at;
        }
        key[at] = 0;
        
        bool isException = false;
        for (int i = 0; i < exceptionsCount; ++i) {
            if (strcmp(exceptions[i], key)==0) {
                isException = true;
                break;
            }
        }
        
        if (!isException) {
            resultSize += sprintf(resultBuffer + resultSize, "%s&", fragBuffer);
        }
        
        ++fragIndex;
        copyResult = lws_hdr_copy_fragment(socket, fragBuffer, sizeof(fragBuffer), WSI_TOKEN_HTTP_URI_ARGS, fragIndex);
    }
    
    if (resultSize) {
        resultBuffer[--resultSize] = 0;
    }
}

void HS_GetURIQueryStringWithoutUTMParams(lws* socket, char* resultBuffer) {
    const char* exceptions[] = {
        "utm_source",
        "utm_medium",
        "utm_campaign",
        "utm_content",
        "utm_term"
    };
    
    HS_GetURIQueryStringWithExceptions(socket, resultBuffer, exceptions, 5);
}

void HS_GetURIQueryString(lws* socket, char* resultBuffer) {
    HS_GetURIQueryStringWithExceptions(socket, resultBuffer, 0, 0);
}

void HS_GetHTTPSURLWithoutUTMParams(lws* socket, char* host, char* resultBuffer, int resultBufferSize) {
    char uri[4096] = {};
    char searchString[4096] = {};
    lws_hdr_copy(socket, uri, sizeof(uri), WSI_TOKEN_GET_URI);
    HS_GetURIQueryStringWithoutUTMParams(socket, searchString);
    
    int at = snprintf(resultBuffer, resultBufferSize, "https://%s%s", host, uri);
    if (searchString[0]) {
        at += sprintf(resultBuffer + at, "?%s", searchString);
    }
}

int HS_GetURLParam(lws* socket, const char* paramName, char* result) {
    // TODO: not tested
    char buf[4096] = {};
    int resultSize = lws_get_urlarg_by_name_safe(socket, paramName, buf, sizeof(buf));
    if (resultSize >= 0) {
        strcpy(result, buf + strlen(paramName) + 1);
    }
    return resultSize;
}

HS_HTTPClient* HS_GetHTTPClientData(HS_CallbackArgs* args) {
    return HS_GetClientData(HS_HTTPClient, args);
}

bool HS_Consume(char* begin, char* end, int* at, const char* consumable) {
    if (begin + *at >= end) return false;
    
    char* string = begin + *at;
    int stringSize = end - string;
    int consumableSize = strlen(consumable);
    if (stringSize >= consumableSize) {
        if (memcmp(string, consumable, consumableSize)==0) {
            *at += consumableSize;
            return true;
        }
    }
    return false;
}

bool HS_Consume(char* begin, char* end, int* at, char consumable) {
    if (begin + *at >= end) return false;
    
    char consumableString[2] = {consumable, 0};
    return HS_Consume(begin, end, at, consumableString);
}

bool HS_ConsumeDigit(char* begin, char* end, int* at) {
    if (begin + *at >= end) return false;
    
    char c = begin[*at];
    if ('0' <= c && c <= '9') {
        *at += 1;
        return true;
    }
    
    return false;
}

bool HS_ConsumeDigits(char* begin, char* end, int* at, int digitsCount) {
    int atOriginal = *at;
    
    for (int i = 0; i < digitsCount; ++i) {
        if (!HS_ConsumeDigit(begin, end, at)) {
            *at = atOriginal;
            return false;
        }
    }
    
    return true;
}

bool HS_EndsWith(char* string, int stringSize, const char* termination, int terminationSize) {
    if (stringSize >= terminationSize) {
        if (memcmp(string+stringSize-terminationSize, termination, terminationSize)==0) {
            return true;
        }
    }
    return false;
}

bool HS_EndsWith(char* string, const char* termination) {
    int stringSize = strlen(string);
    int terminationSize = strlen(termination);
    return HS_EndsWith(string, stringSize, termination, terminationSize);
}

bool HS_EndsWith(char* string, const char* termination, int terminationSize) {
    int stringSize = strlen(string);
    return HS_EndsWith(string, stringSize, termination, terminationSize);
}

bool HS_StartsWith(char* string, int stringSize, const char* begining, int beginingSize) {
    if (stringSize >= beginingSize) {
        if (memcmp(string, begining, beginingSize)==0) {
            return true;
        }
    }
    return false;
}

bool HS_StartsWith(char* string, const char* begining) {
    int stringSize = strlen(string);
    int beginingSize = strlen(begining);
    return HS_StartsWith(string, stringSize, begining, beginingSize);
}

bool HS_StartsWith(char* string, const char* begining, int beginingSize) {
    int stringSize = strlen(string);
    return HS_StartsWith(string, stringSize, begining, beginingSize);
}

bool HS_EndsWithVersionString(char* uri, int uriSize) {
    // Pattern: -v0000.00.00.00.00.00
    int versionStringSize = 21;
    if (uriSize > versionStringSize) {
        char* begin = uri + uriSize - versionStringSize;
        char* end = uri + uriSize;
        int at = 0;
        
        if (HS_Consume(begin, end, &at, "-v") 
         && HS_ConsumeDigits(begin, end, &at, 4)
         && HS_Consume(begin, end, &at, '.')
         && HS_ConsumeDigits(begin, end, &at, 2)
         && HS_Consume(begin, end, &at, '.')
         && HS_ConsumeDigits(begin, end, &at, 2)
         && HS_Consume(begin, end, &at, '.')
         && HS_ConsumeDigits(begin, end, &at, 2)
         && HS_Consume(begin, end, &at, '.')
         && HS_ConsumeDigits(begin, end, &at, 2)
         && HS_Consume(begin, end, &at, '.')
         && HS_ConsumeDigits(begin, end, &at, 2)
        ) {
            return true;
        }
    }
    return false;
}

void HS_RemoveVersionString(char* string, int* size) {
    // Pattern: -v0000.00.00.00.00.00
    int versionStringSize = 21;
    string[*size - versionStringSize] = 0;
    *size -= versionStringSize;
}

bool HS_IsDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

bool HS_IsRegularFile(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) return false;
    return S_ISREG(path_stat.st_mode);
}

void HS_SystemCall(const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    char command[2048] = {};
    vsprintf(command, formatString, argList);
    va_end(argList);
    
    system(command);
}

void HS_CreateFilePath(const char* root, const char* path) {
    char pathBuffer[HS__FilePathCap] = {};
    strcpy(pathBuffer, path);
    char dirPath[HS__FilePathCap] = {};
    strcpy(dirPath, root);
    
    char* pathComponent = strtok(pathBuffer, "/");
    char* nextPathComponent = strtok(0, "/");
    
    while (nextPathComponent) {
        strcat(dirPath, "/");
        strcat(dirPath, pathComponent);
        if (!HS_IsDirectory(dirPath)) {
            mkdir(dirPath, 0777);
        }
        pathComponent = nextPathComponent;
        nextPathComponent = strtok(0, "/");
    }
}

int HS_GetFileSize(FILE* file) {
    if (fseek(file, 0, SEEK_END) == -1) {
        // TODO: handle error
    }
    
    int fsize = ftell(file);
    
    if (fseek(file, 0, SEEK_SET) == -1) {
        // TODO: handle error
    }
    
    return fsize;
}

void HS_SaveFile(char* content, int size, const char* formatString, ...) {
    char path[HS__FilePathCap] = {};
    va_list argList;
    va_start(argList, formatString);
    vsprintf(path, formatString, argList);
    va_end(argList);
    
    FILE* file = fopen(path, "w");
    if (file) {
        fwrite(content, 1, size, file);
        fclose(file);
    } else {
        // TODO: handle error
    }
}

int HS_Replace(char* outBuffer, char* inBuffer, int inLength, const char* searchedString, int searchedLength, const char* replacementString, int replacementLength) {
    int i = 0;
    int j = 0;
    while (i < inLength) {
        if (i < inLength - searchedLength && memcmp(&inBuffer[i], searchedString, searchedLength) == 0) {
            memcpy(&outBuffer[j], replacementString, replacementLength);
            i += searchedLength;
            j += replacementLength;
        } else {
            outBuffer[j] = inBuffer[i];
            ++i;
            ++j;
        }
    }
    outBuffer[j] = 0;
    return j;
}

int HS_Replace(char* outBuffer, char* inBuffer, int inLength, const char* searchedString, const char* replacementString) {
    int searchedLength = strlen(searchedString);
    int replacementLength = strlen(replacementString);
    return HS_Replace(outBuffer, inBuffer, inLength, searchedString, searchedLength, replacementString, replacementLength);
}

void HS_PushURIMapping(HS_Server* server, const char* vhostName, const char* uri, const char* resource) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    HS_URIMapEntry& entry = vhost->uriMap[vhost->uriMapSize++];
    strcpy(entry.uri, uri);
    entry.uriSize = strlen(uri);
    strcpy(entry.resource, resource);

    if (HS_StartsWith(entry.resource, "/$lang/") && !vhost->defaultContentLanguage[0]) {
        fprintf(stderr, "[WARN] %s: localized URI mapping is being used, but 'default-content-language' was not set\n", vhostName);
    }
}

void HS_ClearURIMapping(HS_Server* server, const char* vhostName) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    vhost->uriMapSize = 0;
}

struct HS_FileChunk {
    char* begin;
    int   size;
};

void HS_DoSSI(FILE* file, const char* rootDir, char* savePath) {
    int   fileSize = HS_GetFileSize(file);
    char* fileContent = (char*) calloc(1, fileSize);
    fread(fileContent, fileSize, 1, file);
    
    char* mainFileEnd = fileContent + fileSize;
    
    int at = 0;
    
    char* parsedContent = 0;
    int   parsedContentSize = fileSize;
    
    HS_FileChunk mainFileChunks[16] = {};
    int mainFileChunksCount = 0;
    
    HS_FileChunk includedFiles[16] = {};
    int includedFilesCount = 0;
    
    while (at < fileSize) {
        HS_FileChunk chunk = {fileContent + at};
        int at0 = at;
        const char* ssiPrefix = "<!--#include virtual=\"";
        
        while (at < fileSize && !HS_Consume(fileContent, mainFileEnd, &at, ssiPrefix)) ++at;
        
        if (at < fileSize) {
            char* includedFilePathBegin = fileContent + at;
            while (at < fileSize && fileContent[at] != '\"') ++at;
            int includedFilePathSize = (fileContent + at) - includedFilePathBegin;
            char includedFilePath[HS__FilePathCap] = {};
            memcpy(includedFilePath, includedFilePathBegin, includedFilePathSize);
            
            if (includedFilePathSize && at < fileSize) {
                while (at < fileSize && !HS_Consume(fileContent, mainFileEnd, &at, "-->")) ++at;
                
                if (at < fileSize) {
                    char includedFileFullPath[HS__FilePathCap] = {};
                    sprintf(includedFileFullPath, "%s%.*s", rootDir, includedFilePathSize, includedFilePath);
                    FILE* includedFile = fopen(includedFileFullPath, "rb");
                    
                    if (includedFile) {
                        HS_FileChunk incChunk = {};
                        incChunk.size = HS_GetFileSize(includedFile);
                        incChunk.begin = (char*) calloc(1, incChunk.size);
                        fread(incChunk.begin, incChunk.size, 1, includedFile);
                        fclose(includedFile);
                        
                        parsedContentSize += incChunk.size;
                        includedFiles[includedFilesCount++] = incChunk;
                    } else {
                        // TODO: failed to open included file
                    }
                } else {
                    // TODO: Include directive termination '-->' not found
                }
                
            }
        }
        
        chunk.size = at - at0;
        mainFileChunks[mainFileChunksCount++] = chunk;
    }
    
    parsedContent = (char*) calloc(1, parsedContentSize);
    
    int caret = 0;
    
    for (int i = 0; i < includedFilesCount; ++i) {
        HS_FileChunk& mainChunk = mainFileChunks[i];
        HS_FileChunk& incChunk = includedFiles[i];
        
        memcpy(parsedContent + caret, mainChunk.begin, mainChunk.size);
        caret += mainChunk.size;
        memcpy(parsedContent + caret, incChunk.begin, incChunk.size);
        caret += incChunk.size;
        
        free(incChunk.begin);
    }
    
    memcpy(parsedContent + caret, mainFileChunks[mainFileChunksCount-1].begin, mainFileChunks[mainFileChunksCount-1].size);
    
    HS_SaveFile(parsedContent, parsedContentSize, savePath);
    
    free(parsedContent);
    free(fileContent);
}

HS_FileMapEntry* HS_GetFileByPath(HS_FileMapEntry* map, int mapSize, char* path) {
    for (int i = 0; i < mapSize; ++i) {
        if (strcmp(path, map[i].filePath)==0) {
            return map+i;
        }
    }
    return 0;
}

int HS_Return404(HS_VHost* vhost, HS_HTTPClient* client) {
    snprintf(client->filePath, PATH_MAX, "%s%s", vhost->servedFilesRootDir, vhost->error404File);
    strcpy(client->uri, vhost->error404File);
    
    FILE* file = fopen(client->filePath, "rb");
    client->fileSize = HS_GetFileSize(file);
    client->fileBuffer = (char*) calloc(1, LWS_PRE + client->fileSize);
    client->fileContent = client->fileBuffer + LWS_PRE;
    fread(client->fileContent, client->fileSize, 1, file);
    fclose(file);
    
    // Write headers
    //-----------------
    if (   lws_add_http_header_status(client->socket, HTTP_STATUS_NOT_FOUND, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd)
        || lws_add_http_header_content_length(client->socket, client->fileSize, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd)
        || lws_add_http_header_by_token(client->socket, WSI_TOKEN_HTTP_CONTENT_TYPE, (uint8_t*) "text/html", strlen("text/html"), (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd)
        || lws_add_http_header_by_token(client->socket, WSI_TOKEN_HTTP_CACHE_CONTROL, (uint8_t*) "max-age=2592000", strlen("max-age=2592000"), (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd)
        //|| lws_add_http_header_by_token(client->socket, WSI_TOKEN_CONNECTION, (uint8_t*) "keep-alive", 10, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd)
        || lws_finalize_http_header(client->socket, (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd)
    ) {
        return -1;
    }
    
    client->headerSize = client->headerAt - client->headerBegin;
    lws_write(client->socket, (uint8_t*) client->headerBegin, client->headerSize, LWS_WRITE_HTTP_HEADERS);
    
    lws_callback_on_writable(client->socket);
    return 0;
}

void HS_SetServedFilesRootDir(HS_Server* server, const char* vhostName, const char* path) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    realpath(path, vhost->servedFilesRootDir);
}

void HS_SetVHostVerbosity(HS_Server* server, const char* vhostName, int verbosity) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    vhost->verbosity = verbosity;
}

void HS_UpdateVHostHostString(HS_Server* server, const char* vhostName) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    if (vhost->port == 443) {
        strcpy(vhost->host, vhost->hostName);
    } else {
        sprintf(vhost->host, "%s:%d", vhost->hostName, vhost->port);
    }
}

void HS_SetVHostHostName(HS_Server* server, const char* vhostName, const char* hostName) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    strcpy(vhost->hostName, hostName);
    vhost->lwsContextInfo.vhost_name = vhost->hostName;
    HS_UpdateVHostHostString(server, vhostName);
}

void HS_SetVHostPort(HS_Server* server, const char* vhostName, int port) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    vhost->port = port;
    vhost->lwsContextInfo.port = vhost->port;
    HS_UpdateVHostHostString(server, vhostName);
}

void HS_SetHTTPSessionDataSize(HS_Server* server, const char* vhostName, int sessionDataSize) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    vhost->sessionDataSize = sessionDataSize;
}

void HS_Set404File(HS_Server* server, const char* vhostName, const char* pathRelative) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    if (pathRelative[0] != '/') {
        sprintf(vhost->error404File, "/%s", pathRelative);
    } else {
        strcpy(vhost->error404File, pathRelative);
    }
}

bool HS_IsOriginAllowed(char* allowedOriginTemplate, char* origin, char* allowedOrigin) {
    bool exactMatch = strcmp(allowedOriginTemplate, origin)==0;
    int allowedOriginTemplateSize = strlen(allowedOriginTemplate);
    if (exactMatch || (HS_EndsWith(allowedOriginTemplate, "*") && HS_StartsWith(origin, allowedOriginTemplate, allowedOriginTemplateSize-1))) {
        if (origin[0]) {
            strcpy(allowedOrigin, origin);
        } else {
            strcpy(allowedOrigin, "*");
        }
        return true;
    }
    
    return false;
}

char* HS_GetAllowedOriginTemplate(HS_VHost* vhost, char* dest) {
    for (int i = 0; i < vhost->allowedOriginsCount; ++i) {
        HS_AllowedOrigin& entry = vhost->allowedOrigins[i];
        
        if (strcmp(entry.dest, dest)==0 || (HS_EndsWith(entry.dest, "*") && HS_StartsWith(dest, entry.dest, strlen(entry.dest)-1))) {
            return entry.origin;
        }
    }
    
    return 0;
}

bool HS_ShouldAddAllowOriginHeader(HS_HTTPClient* client, char* allowedOrigin) {
    lws_vhost* vh = lws_get_vhost(client->socket);
    HS_VHost* vhost = (HS_VHost*) lws_vhost_user(vh);
    
    char* allowedOriginTemplate = HS_GetAllowedOriginTemplate(vhost, client->uri);
    
    if (allowedOriginTemplate && HS_IsOriginAllowed(allowedOriginTemplate, client->origin, allowedOrigin)) {
        return true;
    }
    
    return false;
}

bool HS_MaybeAddAllowOriginHeader(HS_HTTPClient* client) {
    char allowedOrigin[HS__URICap];
    if (HS_ShouldAddAllowOriginHeader(client, allowedOrigin)) {
        HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_ACCESS_CONTROL_ALLOW_ORIGIN, allowedOrigin);
        return true;
    }
    
    return false;
}
    
int HS_GetFileByURI(HS_CallbackArgs* args) {
    HS_VHost* server = HS_GetVHost(args);
    HS_HTTPClient* client = HS_GetHTTPClientData(args);
    
    int callbackResult = 0;
    
    http_status httpStatus = HTTP_STATUS_OK;
    const char* mimeType = 0;
    char defaultCacheControl[] = "no-cache, no-store, must-revalidate";
    
    char* cacheControl = (char*) defaultCacheControl;
    int cacheControlSize = sizeof(defaultCacheControl)-1;
    
    if (HS_GetUTMParams(args->socket, &client->utm)) {
        char redirURL[HS__URICap] = {};
        HS_GetHTTPSURLWithoutUTMParams(args->socket, server->host, redirURL, sizeof(redirURL));
        HS_Redirect(client, redirURL, 307);
        
        if (server->verbosity) {
            printf("UTMParams | utm_source=%s | utm_medium=%s | utm_campaign=%s | utm_content=%s | utm_term=%s", client->utm.source, client->utm.medium, client->utm.campaign, client->utm.content, client->utm.term);
        }
        
        return 0;
    }
    
    HS_FileMapEntry* fileEntry = HS_GetFileEntryByURI(server->loadedFiles, server->loadedFilesCount, client->uri);
    
    if (!server->disableFileCache || !fileEntry) {
        // Strip version string
        if (HS_EndsWithVersionString(client->uri, client->uriSize)) {
            HS_RemoveVersionString(client->uri, &client->uriSize);
        }

        // Redirect?
        //------------
        bool redirected = false;
        
        for (int i = 0; i < server->redirectMapSize; ++i) {
            if (strcmp(server->redirectMap[i].uri, client->uri)==0) {
                char* redirURL = server->redirectMap[i].destination;
                lws_http_redirect(args->socket, 301, (uint8_t*) redirURL, strlen(redirURL), (uint8_t**) &client->headerAt, (uint8_t*) client->headerEnd);
                return -1;
            }
        }
        
        // URI processing
        //-----------------
        for (int i = 0; i < server->uriMapSize; ++i) {
            if (HS_EndsWith(server->uriMap[i].uri, "*")) {
                if (HS_StartsWith(client->uri, server->uriMap[i].uri, server->uriMap[i].uriSize-1)) {
                    strcpy(client->uri, server->uriMap[i].resource);
                    break;
                }
            } else if (HS_StartsWith(server->uriMap[i].uri, "*")) {
                char* ending = server->uriMap[i].uri+1;
                if (HS_EndsWith(client->uri, ending)) {
                    strcpy(client->uri, server->uriMap[i].resource);
                    break;
                }
            } else if (strcmp(server->uriMap[i].uri, client->uri)==0) {
                strcpy(client->uri, server->uriMap[i].resource);
                break;
            }
        }

        // Root directory
        //------------------
        const char* rootDir = server->servedFilesRootDir;
        const char* unprefixedURI = client->uri;

        for (int i = 0; i < server->rootDirMapSize; ++i) {
            const char* uriPrefix = server->rootDirMap[i].uriPrefix;
            if (HS_StartsWith(client->uri, uriPrefix)) {
                rootDir = server->rootDirMap[i].path;
                unprefixedURI = client->uri + strlen(uriPrefix) - 1;
            }
        }

        if (!rootDir[0]) {
            HS_CloseConnection(client, HTTP_STATUS_NOT_FOUND);
            return -1;
        }

        // Localization
        //--------------
        char tempBuffer[4096] = {};
        
        if (HS_StartsWith(client->uri, "/$lang/")) {
            char acceptLanguage[256] = {};
            strcpy(acceptLanguage, client->acceptLanguage);
            
            char* cell = strtok(acceptLanguage, ",");
            
            char locURI[2048] = {};
            char filePath[4096] = {};
            
            while (cell) {
                char lang[32] = {};
                char* langStart = cell;
                
                // Trim spaces
                while (*langStart && *langStart == ' ') ++langStart;
                
                int at = 0;
                
                while (langStart[at] && langStart[at] != ';' && langStart[at] != ' ') {
                    lang[at] = langStart[at];
                    ++at;
                }
                
                HS_ToLower(lang);
                
                sprintf(locURI, "/%s/%s", lang, client->uri + 7);
                
                sprintf(tempBuffer, "%s%s", rootDir, locURI);
                realpath(tempBuffer, filePath);
                
                if (HS_IsRegularFile(filePath)) {
                    strcpy(client->uri, locURI);
                    strcpy(client->contentLanguage, lang);
                    break;
                }
                
                cell = strtok(0, ",");
            }
            
            if (!client->contentLanguage[0]) {
                sprintf(locURI, "/%s/%s", server->defaultContentLanguage, client->uri + 7);
                strcpy(client->uri, locURI);
                strcpy(client->contentLanguage, server->defaultContentLanguage);
            }
        }
        
        // Infer file path
        //-----------------------
        sprintf(tempBuffer, "%s%s", rootDir, unprefixedURI);
        realpath(tempBuffer, client->filePath);
        
        if (!HS_StartsWith(client->filePath, rootDir)) {
            HS_CloseConnection(client, HTTP_STATUS_NOT_FOUND);
        } else if (!HS_IsRegularFile(client->filePath)) {
            if (HS_IsDirectory(client->filePath)) {
                strcat(client->filePath, "/index.html");
            }

            if (!HS_IsRegularFile(client->filePath)) {
                if (server->error404File[0]) {
                    snprintf(client->filePath, PATH_MAX, "%s%s", rootDir, server->error404File);
                    httpStatus = HTTP_STATUS_NOT_FOUND;
                    strcpy(client->uri, server->error404File);
                } else {
                    HS_CloseConnection(client, HTTP_STATUS_NOT_FOUND);
                }
            }
        }
        
        // Get mimetype
        //----------------
        mimeType = lws_get_mimetype(client->filePath, 0);

        if (!mimeType) {
            if (HS_EndsWith(client->filePath, ".wasm")) {
                mimeType = "application/wasm";
            } else if (HS_EndsWith(client->filePath, ".woff2")) {
                mimeType = "font/woff2";
            } else if (HS_EndsWith(client->filePath, ".map")) {
                mimeType = "application/json";
            } else if (HS_EndsWith(client->filePath, ".zip")) {
                mimeType = "application/zip";
            } else if (HS_EndsWith(client->filePath, ".pdf")) {
                mimeType = "application/pdf";
            } else if (HS_EndsWith(client->filePath, ".txt")) {
                mimeType = "text/plain";
            } else {
                mimeType = "text/html";
            }
        }
        
        // Cache busting
        //---------------
        bool needsCacheBusting = false;
        if (strcmp(mimeType, "text/html")==0 || strcmp(mimeType, "text/javascript")==0) {
            for (int i = 0; i < server->cacheBustSize; ++i) {
                if (HS_EndsWith(server->cacheBust[i], "*")) {
                    if (HS_StartsWith(client->uri, server->cacheBust[i], strlen(server->cacheBust[i])-1)) {
                        needsCacheBusting = true;
                        break;
                    }
                } else if (strcmp(client->uri, server->cacheBust[i])==0) {
                    needsCacheBusting = true;
                    break;
                }
            }
        }
        
        if (needsCacheBusting) {
            char transformedPath[HS__FilePathCap] = {};
            char transformedFullPath[HS__FilePathCap*2] = {};
            sprintf(transformedPath, "/.cache-bust%s", client->uri);
            sprintf(transformedFullPath, "%s%s", rootDir, transformedPath);
            
            if (!HS_IsRegularFile(transformedFullPath)) {
                HS_CreateFilePath(rootDir, transformedPath);
                
                FILE* file = fopen(client->filePath, "rb");
                char* fileContent = 0;
                int   fileSize = 0;
                
                if (file) {
                    fileSize = HS_GetFileSize(file);
                    fileContent = (char*) calloc(1, fileSize+1);
                    fread(fileContent, fileSize, 1, file);
                    fclose(file);
                    
                    HS_Replace(fileContent, fileContent, fileSize, "-v0000.00.00.00.00.00", server->cacheBustVersion);
                    HS_SaveFile(fileContent, fileSize, transformedFullPath);
                    free(fileContent);
                } else {
                    //TODO error
                }
            }
            
            strcpy(client->filePath, transformedFullPath);
        }
        
        // Check if needs SSI Parsing
        //-----------------------------
        bool needsSSIParsing = false;
        if (strcmp(mimeType, "text/html")==0) {
            for (int i = 0; i < server->needsSSIParsingSize; ++i) {
                if (HS_EndsWith(server->needsSSIParsing[i], "*")) {
                    if (HS_StartsWith(client->uri, server->needsSSIParsing[i], strlen(server->needsSSIParsing[i])-1)) {
                        needsSSIParsing = true;
                        break;
                    }
                } else if (strcmp(server->needsSSIParsing[i], client->uri)==0) {
                    needsSSIParsing = true;
                    break;
                }
            }
        }
        
        if (needsSSIParsing) {
            char transformedPath[HS__FilePathCap] = {};
            char transformedFullPath[HS__FilePathCap*2] = {};
            sprintf(transformedPath, "/.ssi-parsed%s", client->uri);
            sprintf(transformedFullPath, "%s%s", rootDir, transformedPath);
            
            if (!HS_IsRegularFile(transformedFullPath)) {
                HS_CreateFilePath(rootDir, transformedPath);
                FILE* file = fopen(client->filePath, "rb");
                
                if (file) {
                    HS_DoSSI(file, rootDir, transformedFullPath);
                    fclose(file);
                } else {
                    // TODO: handle error
                }
            }
            
            strcpy(client->filePath, transformedFullPath);
        }

        // Cache control
        //---------------
        if (!server->disableFileCache) {
            for (int i = 0; i < server->cacheControlMapSize; ++i) {
                if (HS_EndsWith(server->cacheControlMap[i].uri, "*")) {
                    if (HS_StartsWith(client->uri, server->cacheControlMap[i].uri, server->cacheControlMap[i].uriSize-1)) {
                        cacheControl = server->cacheControlMap[i].cacheString;
                        cacheControlSize = strlen(server->cacheControlMap[i].cacheString);
                        break;
                    }
                } else if (server->cacheControlMap[i].uri[0] == '*') {
                    char* ending = server->cacheControlMap[i].uri+1;
                    if (HS_EndsWith(client->uri, ending)) {
                        cacheControl = server->cacheControlMap[i].cacheString;
                        cacheControlSize = strlen(server->cacheControlMap[i].cacheString);
                        break;
                    }
                } else if (strcmp(server->cacheControlMap[i].uri, client->uri)==0) {
                    cacheControl = server->cacheControlMap[i].cacheString;
                    cacheControlSize = strlen(server->cacheControlMap[i].cacheString);
                    break;
                }
            }
        }
        
        fileEntry = HS_GetFileByPath(server->loadedFiles, server->loadedFilesCount, client->filePath);
        
        if (fileEntry) {
            client->fileBuffer = fileEntry->fileBuffer;
            client->fileContent = fileEntry->fileContent;
            client->fileSize = fileEntry->fileSize;
            ++fileEntry->clientsReading;
            client->fileEntry = fileEntry;
        } else {
            // Load resource
            //---------------
            FILE* file = fopen(client->filePath, "rb");
            if (file) {
                client->fileSize = HS_GetFileSize(file);
                client->fileBuffer = (char*) calloc(1, LWS_PRE + client->fileSize);
                client->fileContent = client->fileBuffer + LWS_PRE;
                fread(client->fileContent, client->fileSize, 1, file);
                fclose(file);
            } else {
                // TODO: ERROR
            }
        }

        if (server->disableFileCache || (server->memCacheMaxSizeMB > 0 && (int)client->fileSize > HS_MEGA_BYTES(server->memCacheMaxSizeMB))) {
            // Don't cache
            HS_SystemCall("rm -rf %s/.cache-bust", rootDir);
            HS_SystemCall("rm -rf %s/.ssi-parsed", rootDir);
        } else if (!fileEntry) {
            fileEntry = &server->loadedFiles[server->loadedFilesCount++];
            strcpy(fileEntry->uri, client->uri);
            strcpy(fileEntry->filePath, client->filePath);
            fileEntry->fileBuffer = client->fileBuffer;
            fileEntry->fileContent = client->fileContent;
            fileEntry->fileSize = client->fileSize;
            fileEntry->mimeType = mimeType;
            fileEntry->cacheControl = cacheControl;
            fileEntry->cacheControlSize = cacheControlSize;
            fileEntry->clientsReading = 1;

            client->fileEntry = fileEntry;
        }
    } else {
        client->fileBuffer = fileEntry->fileBuffer;
        client->fileContent = fileEntry->fileContent;
        client->fileSize = fileEntry->fileSize;
        
        mimeType = fileEntry->mimeType;
        cacheControl = fileEntry->cacheControl;
        cacheControlSize = fileEntry->cacheControlSize;
        ++fileEntry->clientsReading;
        
        client->fileEntry = fileEntry;
    }

    // Write headers
    //-----------------
    HS_AddHTTPHeaderStatus(client, httpStatus);
    HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_LENGTH, client->fileSize);
    HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_TYPE, mimeType);
    HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CACHE_CONTROL, cacheControl);
    
    // TODO: Connection: keep-alive is not allowed in http 2. I couldn't find
    // a way to detect if the connection is using h1 or h2, so I'm removing
    // this header from all responses.
    // HS_AddHTTPHeader(client, WSI_TOKEN_CONNECTION, "keep-alive");
    
    HS_MaybeAddAllowOriginHeader(client);
    
    if (client->contentLanguage[0]) {
        HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_LANGUAGE, client->contentLanguage);
    }
    
    HS_WriteResponse(client);
    
    return callbackResult;
}

bool HS_GetHeader(HS_HTTPClient* client, lws_token_indexes header, char* buffer, int bufferSize) {
    return lws_hdr_copy(client->socket, buffer, bufferSize, header) > 0;
}

void HS_GetPathNodes(char* path, char** nodes) {
    // TODO: This function can crash very easily!!1
    // We don't check if the passed buffers for each
    // path node has sufficient space to receive the data.
    char buffer[PATH_MAX] = {};
    strcpy(buffer, path);
    char* entry = strtok(buffer, "/");
    
    if (entry[0] == 0) {
        entry = strtok(0, "/");
    }
    
    int i = 0;
    while (nodes[i]) {
        if (!entry || !entry[0]) {
            break;
        }
        
        strcpy(nodes[i], entry);
        entry = strtok(0, "/");
        ++i;
    }
}

JS_JSON* HS_GetGatedArea(JS_JSON* gkConfig, const char* areaId) {
    JS_Iterator it = JS_ForEach(JS_Get(gkConfig, "gated-areas"));
    
    while (JS_Next(&it)) {
        JS_JSON* j = JS_Unwrap(it);
        if (strcmp(JS_GetString(j, "id"), areaId)==0) {
            return j;
        }
    }
    
    return 0;
}

int HS_GatekeeprGetRequestHandler(HS_CallbackArgs* args) {
    HS_HTTPClient* client = HS_GetHTTPClientData(args);
    HS_VHost* vhost = HS_GetVHost(args);
    JS_JSON* gkConfig = vhost->gkConfig;
    
    char defaultCacheControl[] = "no-cache, no-store, must-revalidate";
    
    int result = 0;
    
    char urlPrefix[1024] = {};
    char gatedAreaId[256] = {};
    char* nodes[] = {urlPrefix, gatedAreaId, 0};
    
    HS_GetPathNodes(client->uri, nodes);
    
    if (strcmp(urlPrefix, "gk")==0 && gatedAreaId[0]) {
        JS_JSON* jArea = HS_GetGatedArea(gkConfig, gatedAreaId);
        
        if (jArea) {
            JS_JSON* jConfig = JS_Create();
            JS_Set(jConfig, "server", vhost->host);
            JS_Set(jConfig, "googleClientId", JS_GetString(gkConfig, "google-client-id"));
            JS_Set(jConfig, "gatedArea", JS_Copy(jArea));
            
            char config[HS_KILO_BYTES(2)] = {};
            JS_DumpCompact(jConfig, config, sizeof(config));
            
            char tmplPath[PATH_MAX] = {};
            sprintf(tmplPath, "%s/gatekeepr/gatekeepr.html", vhost->servedFilesRootDir);
            FILE* tmplFile = fopen(tmplPath, "r");
            
            if (tmplFile) {
                int tmplFileSize = HS_GetFileSize(tmplFile);
                char* tmpl = (char*) calloc(1, tmplFileSize);
                fread(tmpl, tmplFileSize, 1, tmplFile);
                fclose(tmplFile);
                
                HS_InitResponseBuffer(client, tmplFileSize + HS_KILO_BYTES(2));
                
                HS_Replacement reps[] = {
                    {"<!--TITLE GOES HERE-->", JS_GetString(jArea, "name")},
                    {"/*GATEKEEPR CONFIG GOES HERE*/", config},
                    {"LOGO PATH GOES HERE", JS_Get(jArea, "image") ? JS_GetString(jArea, "image") : ""},
                    {"AREA ID GOES HERE", gatedAreaId},
                    {"HOME PATH GOES HERE", JS_Get(jArea, "home") ? JS_GetString(jArea, "home") : ""},
                    {"TERMS URL GOES HERE", JS_Get(jArea, "terms") ? JS_GetString(jArea, "terms") : ""},
                    {"-v0000.00.00.00.00.00", vhost->cacheBustVersion},
                    {}
                };
                
                client->fileSize = HS_Replace(client->fileContent, tmpl, tmplFileSize, reps);
                
                HS_AddHTTPHeaderStatus(client, 200);
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_LENGTH, client->fileSize);
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CONTENT_TYPE, "text/html");
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_CACHE_CONTROL, defaultCacheControl);
                HS_WriteResponse(client);
                
                free(tmpl);
            } else {
                // TODO: gatekeepr.html not found!
            }
            
            JS_Free(jConfig);
        } else {
            HS_AddHTTPHeaderStatus(client, 404);
            HS_WriteResponse(client);
        }
    } else {
        HS_AddHTTPHeaderStatus(client, 404);
        HS_WriteResponse(client);
    }
    
    return result;
}

JS_JSON* HS_GetGatedAreaFromURI(JS_JSON* gkConfig, char* uri) {
    JS_Iterator it = JS_ForEach(JS_Get(gkConfig, "gated-areas"));
    
    while (JS_Next(&it)) {
        JS_JSON* j = JS_Unwrap(it);
        if (HS_StartsWith(uri, JS_GetString(j, "prefix"))) {
            return j;
        }
    }
    
    return 0;
}

int HS_HandleGetRequestToGatedArea(HS_CallbackArgs* args) {
    HS_HTTPClient* client = HS_GetHTTPClientData(args);
    HS_VHost* vhost = HS_GetVHost(args);
    
    int result = 0;
    
    JS_JSON* jArea = HS_GetGatedAreaFromURI(vhost->gkConfig, client->uri);
    
    char* gatedAreaId = JS_GetString(jArea, "id");
    char sessionCookie[512] = {};
    char cookieName[256] = {};
    sprintf(cookieName, "gatekeepr_%s", gatedAreaId);
    
    if (HS_GetCookieValue(client->socket, cookieName, sessionCookie, sizeof(sessionCookie))) {
        // Cleanup database
        time_t now = time(0);
        SQ_DeleteRows(vhost->gkdb, "sessions", "%lld >= expirationDate OR (%lld-lastUseDate) >= 3600", now, now);
        
        if (SQ_GetAggregateFunctionResultInt(vhost->gkdb, "SELECT COUNT(*) FROM sessions JOIN users ON sessions.userId=users.id WHERE gatedArea='%s' AND users.id || '.' || sessions.id = '%s'", gatedAreaId, sessionCookie)) {
            // Authentication successful
            SQ_ExecuteCommand(vhost->gkdb, "UPDATE sessions SET lastUseDate=%lld WHERE '%s' LIKE '%%' || id", now, sessionCookie);
            return HS_GetFileByURI(args);
        }
    }
    
    // Authentication needed. Redirect to gatekeepr
    //-----------------------------------------------
    sprintf(sessionCookie, "%s=; Expires=Thu, 01 Jan 1970 00:00:01 GMT; Path=/;", cookieName);

    char redirParam[HS_KILO_BYTES(2)] = {};
    if (!HS_GetQueryStringValue(client, "redirect", redirParam, sizeof(redirParam))) {
        lws_urlencode(redirParam, client->uri, sizeof(redirParam));
    }

    char redirURL[HS_KILO_BYTES(2)] = {};
    int _ = snprintf(redirURL, sizeof(redirURL), "https://%s/gk/%s?redirect=%s", vhost->host, JS_GetString(jArea, "id"), redirParam);
    
    HS_AddHTTPHeaderStatus(client, 302);
    HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_LOCATION, redirURL);
    HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_SET_COOKIE, sessionCookie);
    HS_WriteResponse(client);
    
    return result;
}

int HS_GetFileByURIOrAuthenticate(HS_CallbackArgs* args) {
    HS_HTTPClient* client = HS_GetHTTPClientData(args);
    HS_VHost* vhost = HS_GetVHost(args);
    
    char cookie[HS_KILO_BYTES(4)] = {};
    HS_GetHeader(client, WSI_TOKEN_HTTP_COOKIE, cookie, sizeof(cookie));
    
    if (HS_StartsWith(client->uri, "/gk/")) {
        return HS_GatekeeprGetRequestHandler(args);
    }
    
    if (HS_GetGatedAreaFromURI(vhost->gkConfig, client->uri)) {
        return HS_HandleGetRequestToGatedArea(args);
    }
    
    return HS_GetFileByURI(args);
}
  
int HS_HTTPCallback(lws* socket, lws_callback_reasons reason, void* userData, void* in, size_t len) {
    HS_CallbackArgs args = {};
    args.socket = socket;
    args.reason = reason;
    args.userData = userData;
    args.in = in;
    args.len = len;
    
    HS_VHost* server = HS_GetVHost(&args);
    HS_HTTPClient* client = HS_GetHTTPClientData(&args);
    
    int callbackResult = 0;
    
    if (reason != LWS_CALLBACK_GET_THREAD_ID
     && reason != LWS_CALLBACK_CHANGE_MODE_POLL_FD
     && reason != LWS_CALLBACK_VHOST_CERT_AGING
     && reason != LWS_CALLBACK_LOCK_POLL
     && reason != LWS_CALLBACK_UNLOCK_POLL) {
        if (server->verbosity) {
            printf("%s %s -- wsi %p\n", server->name, HS_ToString(reason), socket);
        }
    }

    switch (reason) {
      case LWS_CALLBACK_HTTP: {
#if 0
        printf("Loaded Files: (%d)\n", server->loadedFilesCount);
        for (int i = 0; i < server->loadedFilesCount; ++i) {
            printf("  %s | %s (clients reading: %d)\n", server->loadedFiles[i].uri, server->loadedFiles[i].filePath, server->loadedFiles[i].clientsReading);
        }
#endif

        char uri[HS__URICap] = {};
        if (len >= sizeof(uri)) {
            callbackResult = -1;
            break;
        }
        
        *client = {};
        client->socket = socket;
        client->id = server->nextHTTPClientId++;
            
        // Init header buffer
        client->headerBegin = client->headerBuffer + LWS_PRE;
        client->headerEnd = client->headerBuffer + sizeof(client->headerBuffer) - LWS_PRE - 1;
        client->headerAt = client->headerBegin;
    
        HS_GetIpv4(&args, client->ipAddress);
        
        char contentLength[64] = {};
        
        // Read headers
        //--------------
        lws_hdr_copy(socket, client->host, sizeof(client->host), WSI_TOKEN_HOST);
        lws_hdr_copy(socket, client->referer, sizeof(client->referer), WSI_TOKEN_HTTP_REFERER);
        lws_hdr_copy(socket, client->acceptLanguage, sizeof(client->acceptLanguage), WSI_TOKEN_HTTP_ACCEPT_LANGUAGE);
        lws_hdr_copy(socket, client->userAgent, sizeof(client->userAgent), WSI_TOKEN_HTTP_USER_AGENT);
        lws_hdr_copy(socket, client->origin, sizeof(client->origin), WSI_TOKEN_ORIGIN);
        lws_hdr_copy(socket, contentLength, sizeof(contentLength), WSI_TOKEN_HTTP_CONTENT_LENGTH);
        lws_hdr_copy(socket, client->contentType, sizeof(client->contentType), WSI_TOKEN_HTTP_CONTENT_TYPE);
        
        if (!client->host[0]) {
            strcpy(client->host, server->hostName);
        }
        
        if (contentLength[0]) {
            client->contentLength = atoi(contentLength);
        } else {
            client->contentLength = -1;
        }
        
        if (lws_hdr_total_length(socket, WSI_TOKEN_GET_URI)) {
            strcpy(client->httpMethod, "GET");
        } else if (lws_hdr_total_length(socket, WSI_TOKEN_POST_URI)) {
            strcpy(client->httpMethod, "POST");
        } else if (lws_hdr_total_length(socket, WSI_TOKEN_DELETE_URI)) {
            strcpy(client->httpMethod, "DELETE");
        } else if (lws_hdr_total_length(socket, WSI_TOKEN_OPTIONS_URI)) {
            strcpy(client->httpMethod, "OPTIONS");
        } else {
            strcpy(client->httpMethod, "OTHER");
        }
        
        sprintf(uri, "%.*s", (int)len, (char*)in);
        strcpy(client->uri, uri);
        client->uriSize = strlen(client->uri);
        
        if (server->verbosity) {
            printf("%s %s\n", client->httpMethod, uri);
        }
        
        // Plugins
        //---------
        for (int i = 0; i < server->pluginCount; ++i) {
            HS_Plugin& plugin = server->plugins[i];
            if (plugin.wantsToHandleRequest(client)) {
                client->sessionData = calloc(1, plugin.sessionDataSize);
                if (strcmp(client->httpMethod, "POST")==0) {
                    return 0; // Handle later, on write call
                } else {
                    return plugin.requestHandler(&args);
                }
            }
        }

        // Is GET method?
        //----------------
        if (strcmp(client->httpMethod, "GET")==0) {
            bool validEndpoint = true;
            if (server->httpGetEndpointChecker) {
                validEndpoint = server->httpGetEndpointChecker(&args);
            }

            if (validEndpoint) {
                if (server->sessionDataSize) {
                    client->sessionData = calloc(1, server->sessionDataSize);
                }

                if (server->httpGetHandler) {
                    server->httpGetHandler(&args);
                }
            }

        } else if (strcmp(client->httpMethod, "OPTIONS")==0) {
            char methods[256] = {};
            int methodsSize = sprintf(methods, "OPTIONS");
            if (server->httpGetHandler) methodsSize += sprintf(methods+methodsSize, ", GET");
            if (server->httpPostHandler) methodsSize += sprintf(methods+methodsSize, ", POST");
            if (server->httpDeleteHandler) methodsSize += sprintf(methods+methodsSize, ", DELETE");
            
            char allowedOrigin[HS__URICap];
            if (HS_ShouldAddAllowOriginHeader(client, allowedOrigin)) {
                HS_AddHTTPHeaderStatus(client, 200);
                HS_AddHTTPHeader(client, WSI_TOKEN_HTTP_ACCESS_CONTROL_ALLOW_ORIGIN, allowedOrigin);
                HS_AddHTTPHeader(client, "Access-Control-Allow-Headers", "Authorization, Content-Type, Content-Length, Cache-Control");
                HS_AddHTTPHeader(client, "Access-Control-Allow-Methods", methods);
                HS_AddHTTPHeader(client, "Access-Control-Allow-Credentials", "true");
                HS_AddHTTPHeader(client, "Access-Control-Max-Age", "600");
            } else {
                HS_AddHTTPHeaderStatus(client, 400);
            }
            
            HS_WriteResponse(client);
        } else {
            // Other methods: POST / DELETE
            bool validEndpoint = false;
            if (strcmp(client->httpMethod, "POST")==0 && server->httpPostHandler) {
                if (server->httpPostEndpointChecker) {
                    validEndpoint = server->httpPostEndpointChecker(&args);
                } else if (server->postEndpointsSize) {
                    for (int i = 0; i < server->postEndpointsSize; ++i) {
                        if (HS_StartsWith(uri, server->postEndpoints[i])) {
                            validEndpoint = true;
                            break;
                        }
                    }
                } else {
                    validEndpoint = true;
                }
                
                if (validEndpoint) {
                    if (server->sessionDataSize) {
                        client->sessionData = calloc(1, server->sessionDataSize);
                    }
                } else {
                    return -1;
                }
            } else if (strcmp(client->httpMethod, "DELETE")==0 && server->httpDeleteHandler) {
                if (server->httpDeleteEndpointChecker) {
                    validEndpoint = server->httpDeleteEndpointChecker(&args);
                } else if (server->deleteEndpointsSize) {
                    for (int i = 0; i < server->deleteEndpointsSize; ++i) {
                        if (strcmp(uri, server->deleteEndpoints[i])==0) {
                            validEndpoint = true;
                            break;
                        }
                    }
                } else {
                    validEndpoint = true;
                }
                
                if (validEndpoint) {
                    if (server->sessionDataSize) {
                        client->sessionData = calloc(1, server->sessionDataSize);
                    }
                    callbackResult = server->httpDeleteHandler(&args);
                } else {
                    return -1;
                }
            }
        }
      } break;
      
      case LWS_CALLBACK_HTTP_WRITEABLE: {
        if (strcmp(client->httpMethod, "POST")==0 && !client->requestProcessed) {
            // Plugins
            //---------
            for (int i = 0; i < server->pluginCount; ++i) {
                HS_Plugin& plugin = server->plugins[i];
                if (plugin.wantsToHandleRequest(client)) {
                    client->requestProcessed = true;
                    callbackResult = plugin.requestHandler(&args);
                }
            }
            
            if (!client->requestProcessed && server->httpPostHandler) {
                callbackResult = server->httpPostHandler(&args);
                client->requestProcessed = true;
            }
            
            if (!client->delayBodyFree) {
                free(client->receivedBuffer);
                client->receivedBuffer = 0;
            }
            break;
        }
        
        if (client->closeConnection) {
            lws_return_http_status(socket, client->closeStatus, 0);
        } else if (client->fileBuffer) {
            int remaining = client->fileSize - client->at;
            
            if (remaining) {
                int amount = HS_Min(remaining, server->h2MaxFrameSize);
                bool finalWrite = client->at + amount >= client->fileSize;
                lws_write_protocol writeProtocol = finalWrite ? LWS_WRITE_HTTP_FINAL : LWS_WRITE_HTTP;
                memcpy(server->frameStart, ((uint8_t*) client->fileContent) + client->at, amount);
                lws_write(socket, (uint8_t*) server->frameStart, amount, writeProtocol);
                
                client->at += amount;
                
                if (finalWrite) {
                    client->closeStatus = (http_status) 0;
                    callbackResult = -1;
                } else {
                    lws_set_timeout(socket, PENDING_TIMEOUT_HTTP_CONTENT, 20);
                    lws_callback_on_writable(socket);
                }
            } else {
                lws_write(socket, (uint8_t*) server->frameStart, 0, LWS_WRITE_HTTP_FINAL);
                client->closeStatus = (http_status) 0;
                callbackResult = -1;
                
                if (!client->fileEntry) {
                    free(client->fileBuffer);
                    client->fileBuffer = 0;
                }
            }
        } else if (client->closeStatus) {
            lws_write(socket, (uint8_t*) server->frameStart, 0, LWS_WRITE_HTTP_FINAL);
            client->closeStatus = (http_status) 0;
            callbackResult = -1;
        }
      } break;
      
      case LWS_CALLBACK_HTTP_BODY: {
        if (strcmp(client->httpMethod, "POST")==0) {
            if (!client->receivedBuffer && len) {
                client->receivedCap = client->contentLength >= 0 ? client->contentLength+1 : HS__PostBufferSize+1;
                client->receivedBuffer = (char*) calloc(1, client->receivedCap);
            }
            
            if (len) {
                if (client->receivedSize + len < (size_t) client->receivedCap) {
                    memcpy(client->receivedBuffer + client->receivedSize, in, len);
                    client->receivedSize += len;
                    client->receivedBuffer[client->receivedSize] = 0;
                    //
                    // NOTE: Sometimes LWS_CALLBACK_HTTP_BODY_COMPLETION is never trigered,
                    // even though we have received the entirety of the request body
                    // (no idea why). So I'm now calling lws_callback_on_writable here and
                    // there, to make sure that the writable event is generated in order to
                    // handle the post request.
                    //
                    // See note on LWS_CALLBACK_HTTP_BODY_COMPLETION
                    //

                    if (client->contentLength && client->receivedSize == client->contentLength) {
                        lws_callback_on_writable(socket);
                    }
                } else {
                    return -1;
                }
            }
        } else {
            return -1;
        }
      } break;
      
      case LWS_CALLBACK_HTTP_BODY_COMPLETION: {
        if (strcmp(client->httpMethod, "POST")==0) {
            // NOTE: For a while, I thought I could handle the POST here:
            //
            //     callbackResult = server->httpPostHandler(&args);
            //
            // However, after expriencing strange differences in behavior in Firefox and Chrome when
            // sending POST requests with no body, the only way I've found to work arround
            // that inconsistent behavior was to postpone the handling to a write callback. This is
            // because sometimes we are called-back TWICE with _HTTP and _BODY_COMPLETION,
            // and the second time is called with a completely different per-session data,
            // and, on top of that, the previous per-session data is completely lost.
            //
            // Related: https://github.com/warmcat/libwebsockets/issues/2865
            lws_callback_on_writable(socket);
        } else if (strcmp(client->httpMethod, "DELETE")==0 && server->httpDeleteHandler) {
            callbackResult = server->httpDeleteHandler(&args);
        } else {
            callbackResult = -1;
        }
      } break;
      
      //case LWS_CALLBACK_WSI_DESTROY: {
      case LWS_CALLBACK_HTTP_DROP_PROTOCOL: {
        if (client) {
            if (client->fileEntry) {
                --client->fileEntry->clientsReading;
            } else if (client->fileBuffer) {
                free(client->fileBuffer);
            }
            
            if (client->sessionData) {
                free(client->sessionData);
            }
            
            if (client->receivedBuffer) {
                free(client->receivedBuffer);
            }
        }
      } break;
      
      case LWS_CALLBACK_PROTOCOL_INIT: {
        HS_SystemCall("rm -rf %s/.cache-bust", server->servedFilesRootDir);
        HS_SystemCall("rm -rf %s/.ssi-parsed", server->servedFilesRootDir);

        for (int i = 0; i < server->rootDirMapSize; ++i) {
            const char* path = server->rootDirMap[i].path;
            HS_SystemCall("rm -rf %s/.cache-bust", path);
            HS_SystemCall("rm -rf %s/.ssi-parsed", path);
        }
        
        server->h2MaxFrameSize = HS_GetH2FrameMaxSize(server);
        
        if (!server->disableFileCache) {
            server->loadedFiles = (HS_FileMapEntry*) calloc(1, HS__FileMapCap*sizeof(HS_FileMapEntry));
        }
        
        server->frameBuffer = (char*) calloc(1, LWS_PRE + server->h2MaxFrameSize);
        server->frameStart = server->frameBuffer + LWS_PRE;
        
        HS_Date dateNow = HS_GetDateNow();
        sprintf(server->cacheBustVersion, "-v%d.%02d.%02d.%02d.%02d.%02d", dateNow.year, dateNow.month, dateNow.day, dateNow.hour, dateNow.minute, dateNow.second);
      } break;
      
      case LWS_CALLBACK_PROTOCOL_DESTROY: {
        if (server->loadedFiles) free(server->loadedFiles);
        if (server->frameBuffer) free(server->frameBuffer);
      } break;
      
      default: break;
    }

    return callbackResult;
}

struct HS_Request {
    lws* socket;
    lws* originSocket;
    
    // Plugins should explicitly set this handler function,
    // so that they don't interfere with user code that sets up
    // handlers with HS_AddResponseHandler(...)
    HS_CallbackFunc responseHandler;
    
    char endpoint[4096];
    char hostName[256];
    int  port;
    char method[16];
    char origin[256];
    char path[2048];
    char* body;
    int bodyCap;
    int bodySize;
    
    // This `type` is for user convenience, so that they can
    // easily identify the request type using enums or something,
    // without having to inspect other request fields (endpoint,
    // body, etc) or `userData` to figure it out. The same goes
    // for `intent`, so to make it easier to chain requests
    // together.
    int type;
    int intent;
    
    char headerSection[2048];
    int headerSectionSize;
    
    char* responseBuffer;
    int responseContentLength;
    int responseSize;
    int responseStatus;
    char contentType[256];
    
    bool delayBodyFree;
    
    int connectionFlags;
    lws_context* lwsContext;
    lws_vhost* vhost;
    char localProtocolName[256];
    char protocolName[256];
    
    void* userData;
};

void HS_PrintHex(const char *blob, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", (unsigned char)blob[i]);
    }
    printf("\n");
}

void HS_DumpRaw(HS_Request* request, bool bodyIsPrintable=false) {
    printf("--------------------------------\n");
    printf("%s %s HTTPS/1.1\n", request->method, request->path);
    printf("Host: %s:%d\n", request->hostName, request->port ? request->port : 443);
    printf("%s\n", request->headerSection);
    
    if (request->bodySize) {
        if (bodyIsPrintable) {
            printf("%s\n", request->body);
        } else {
            HS_PrintHex(request->body, request->bodySize);
        }
    }
    printf("--------------------------------\n");
}

void HS_DelayBodyFree(HS_Request* request) {
    request->delayBodyFree = true;
}

//#define HS_GetOriginalClient(type, request) (type*) lws_wsi_user((request)->originSocket)
#define HS_GetOriginalClient(request) (HS_HTTPClient*) lws_wsi_user((request)->originSocket)

bool HS_AddHTTPHeader(HS_Request* request, const char* formatString, ...) {
    va_list argList;
    va_start(argList, formatString);
    request->headerSectionSize += vsprintf(request->headerSection+request->headerSectionSize, formatString, argList);
    request->headerSectionSize += sprintf(request->headerSection+request->headerSectionSize, "\x0d\x0a");
    va_end(argList);
    return true;
}

void HS_SetEndpoint(HS_Request* request, const char* formatString, ...) {
    char* endpoint = request->endpoint;
    
    va_list argList;
    va_start(argList, formatString);
    vsprintf(endpoint, formatString, argList);
    va_end(argList);
    
    int i = 0;
    int k = 0;
    while (endpoint[k] && endpoint[k] != ':' && endpoint[k] != '/') {
        request->hostName[i++] = endpoint[k++];
    }
    
    strcpy(request->origin, request->hostName);
    
    if (endpoint[k] == ':') {
        i = 0;
        ++k;
        char portString[8] = {};
        while (endpoint[k] && endpoint[k] != '/') {
            portString[i++] = endpoint[k++];
        }
        
        request->port = atoi(portString);
    }
    
    if (endpoint[k]) {
        strcpy(request->path, endpoint+k);
    } else {
        request->path[0] = '/';
    }
}

void HS_AddResponseHandler(HS_Server* server, const char* hostName, HS_CallbackFunc responseHandler) {
    HS_VHost* vhost = HS_GetVHost(server, "hs-requester-vhost");
    HS_ResponseHandler& entry = vhost->responseHandlers[vhost->responseHandlersCount++];
    strcpy(entry.hostName, hostName);
    entry.handler = responseHandler;
}

HS_CallbackFunc HS_GetResponseHandler(HS_VHost* vhost, const char* hostName) {
    for (int i = 0; i < vhost->responseHandlersCount; ++i) {
        if (strcmp(vhost->responseHandlers[i].hostName, hostName)==0) {
            return vhost->responseHandlers[i].handler;
        }
    }
    
    return 0;
}

HS_Request* HS_SendRequest(HS_Request* request) {
    lws_client_connect_info connectInfo = {};
    connectInfo.context = request->lwsContext;
    connectInfo.ssl_connection = request->connectionFlags;
    connectInfo.address = request->hostName;
    connectInfo.port = request->port ? request->port : 443;
    connectInfo.host = request->hostName;
    connectInfo.origin = request->origin;
    if (request->method[0]) connectInfo.method = request->method;
    connectInfo.path = request->path;
    connectInfo.userdata = request;
    connectInfo.vhost = request->vhost;
    if (request->localProtocolName[0]) connectInfo.local_protocol_name = request->localProtocolName;
    connectInfo.protocol = request->protocolName;
    
    request->socket = lws_client_connect_via_info(&connectInfo);
    
    if (request->socket) {
        return request;
    } else {
        if (request->body) free(request->body);
        free(request);
        return 0;
    }
}

HS_Request* HS_InitRequest(HS_Server* server, int bodyCap) {
    HS_Request* request = (HS_Request*) calloc(1, sizeof(HS_Request));
    
    request->lwsContext = server->lwsContext;
    request->connectionFlags = server->defaultConnectionFlags;
    request->bodyCap = bodyCap;
    if (bodyCap) request->body = (char*) calloc(1, bodyCap);
    request->vhost = HS_GetVHost(server, "hs-requester-vhost")->lwsVHost;
    strcpy(request->localProtocolName, "requester");
    
    return request;
}

HS_Request* HS_InitPostRequest(HS_Server* server, int bodyCap) {
    HS_Request* request = HS_InitRequest(server, bodyCap);
    strcpy(request->method, "POST");
    return request;
}

HS_Request* HS_InitGetRequest(HS_Server* server) {
    HS_Request* request = HS_InitRequest(server, 0);
    strcpy(request->method, "GET");
    return request;
}

HS_Request* HS_InitDeleteRequest(HS_Server* server, int bodyCap) {
    HS_Request* request = HS_InitRequest(server, bodyCap);
    strcpy(request->method, "DELETE");
    return request;
}

HS_Request* HS_SendUpgradeRequest(HS_Server* server, const char* hostName, int port, const char* protocolName) {
    HS_Request* request = HS_InitRequest(server, 0);
    strcpy(request->protocolName, protocolName);
    strcpy(request->localProtocolName, protocolName);
    HS_SetEndpoint(request, "%s:%d", hostName, port);
    HS_SendRequest(request);
    return request;
}

#define HS_AppendToBody(request, fmt, ...) (request)->bodySize += sprintf((request)->body+(request)->bodySize, fmt, __VA_ARGS__)

int HS_RequesterCallback(lws* socket, lws_callback_reasons reason, void* userData, void* in, size_t len) {
    HS_CallbackArgs args = {};
    args.socket = socket;
    args.reason = reason;
    args.userData = userData;
    args.in = in;
    args.len = len;
    
    if (socket == 0) return 0;
    
    HS_VHost* server = HS_GetVHost(&args);
    HS_Request* request = (HS_Request*) args.userData;
    
    int callbackResult = 0;
    
    if (reason != LWS_CALLBACK_GET_THREAD_ID
     && reason != LWS_CALLBACK_CHANGE_MODE_POLL_FD
     && reason != LWS_CALLBACK_VHOST_CERT_AGING
     && reason != LWS_CALLBACK_LOCK_POLL
     && reason != LWS_CALLBACK_UNLOCK_POLL) {
        if (server->verbosity) {
            printf("%s %s -- wsi %p\n", server->name, HS_ToString(reason), socket);
        }
    }

    switch (reason) {
      case LWS_CALLBACK_COMPLETED_CLIENT_HTTP: {
        request->responseBuffer[request->responseSize] = 0;
        request->responseStatus = lws_http_client_http_response(socket);
        
        lws_hdr_copy(socket, request->contentType, sizeof(request->contentType), WSI_TOKEN_HTTP_CONTENT_TYPE);
        
        HS_CallbackFunc responseHandler = request->responseHandler;
        if (!responseHandler) {
            responseHandler = HS_GetResponseHandler(server, request->hostName);
        }
        
        if (responseHandler) responseHandler(&args);
      } break;
      
      case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
        if (request->headerSectionSize) {
            strncpy(*((char**) in), request->headerSection, request->headerSectionSize);
            *(char**)in += request->headerSectionSize;
        }
        
        if (request->body) lws_client_http_body_pending(socket, 1);
        lws_callback_on_writable(socket);
      } break;
      
      case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE: {
        if (request->body) {
            // TODO: Send body in chunks
            lws_write(socket, (unsigned char*) request->body, request->bodySize, LWS_WRITE_HTTP);
            lws_client_http_body_pending(socket, 0);
            
            if (!request->delayBodyFree) {
                free(request->body);
                request->body = 0;
            }
        }
      } break;
      
      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: {
        if (in) {
            printf("ConnectionError | %.*s", (int)len, (char*) in);
        } else {
            printf("ConnectionError | UnknownReason");
        }
      } break;
      
      case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP: {
        char contentLengthString[64] = {};
        lws_hdr_copy(socket, contentLengthString, sizeof(contentLengthString), WSI_TOKEN_HTTP_CONTENT_LENGTH);
        
        request->responseContentLength = atoi(contentLengthString);
        
        if (request->responseContentLength > 0) {
            if (request->responseContentLength >= HS_MEGA_BYTES(64)) {
                callbackResult = -1;
                //request->result = HTTPSPostResult_BufferTooSmall;
            } else {
                request->responseBuffer = (char*) calloc(1, request->responseContentLength+1);
            }
        } else {
            request->responseBuffer = (char*) calloc(1, HS_KILO_BYTES(512));
        }
      } break;
      
      case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: {
        memcpy(&request->responseBuffer[request->responseSize], in, len);
        request->responseSize += len;
      } break;
      
      case LWS_CALLBACK_RECEIVE_CLIENT_HTTP: {
        char buffer[LWS_PRE + 16000] = {};
        char* p = &buffer[LWS_PRE];
        int n = sizeof(buffer) - LWS_PRE;
        if (lws_http_client_read(socket, &p, &n) < 0) {
            // read failed for some reason
            callbackResult = -1;
        }
      } break;
      
      case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS: {
        for (int i = 0; i < server->certTrustStoreCount; ++i) {
            BIO* certBio = BIO_new(BIO_s_mem());
            BIO_puts(certBio, server->certTrustStore[i]);
            
            X509* x509 = PEM_read_bio_X509(certBio, 0, 0, 0);
            X509_STORE_add_cert(SSL_CTX_get_cert_store((SSL_CTX*) userData), x509);
        }
      } break;
      
      case LWS_CALLBACK_WSI_DESTROY: {
        if (request) {
            if (request->body) {
                free(request->body);
            }
            
            if (request->responseBuffer) {
                free(request->responseBuffer);
            }
            
            free(request);
        }
      } break;
      
      default: break;
    }
    
    return 0;
}

bool HS_InitServer(HS_Server* server, bool disableHTTP2=false) {
    server->lwsContextInfo = {};
    server->lwsContextInfo.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS | LWS_SERVER_OPTION_DISABLE_IPV6;
    server->lwsContextInfo.user = server;
    server->lwsContextInfo.alpn = disableHTTP2 ? "h1" : 0;
    server->lwsContextInfo.pt_serv_buf_size = HS_MEGA_BYTES(16);
    server->lwsContext = lws_create_context(&server->lwsContextInfo);
    return server->lwsContext;
}

HS_Server HS_CreateServer(void* userData, bool disableSSL=false) {
    HS_Server server = {};
    server.userData = userData;
    if (!disableSSL) {
        server.defaultConnectionFlags = LCCSCF_USE_SSL;
    }
    return server;
}

HS_Server HS_CreateServer() {
    return HS_CreateServer(0);
}

bool HS__AddProtocol(HS_Server* server, const char* vhostName, const char* protocolName, HS_LWSCallback callback, int clientSize) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    
    lws_protocols& protocol = v->lwsProtocols[v->lwsProtocolsCount++];
    protocol.name = protocolName;
    protocol.callback = callback;
    protocol.per_session_data_size = clientSize;
    protocol.rx_buffer_size = v->lwsProtocolDefaults.rx_buffer_size;
    
    return true;
}

#define HS_AddProtocol(server, vhostName, protocolName, userCallback, clientType) \
    HS__AddProtocol(server, vhostName, protocolName, HS_LWSCallback_ ## userCallback, sizeof(clientType))

#define HS_AddSimpleProtocol(server, vhostName, protocolName, userCallback) \
    HS__AddProtocol(server, vhostName, protocolName, HS_LWSCallback_ ## userCallback, 0)
    
lws_protocols* HS_GetProtocol(HS_Server* server, const char* vhostName, const char* protocolName) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    for (int i = 0; i < v->lwsProtocolsCount; ++i) {
        lws_protocols& p = v->lwsProtocols[i];
        if (strcmp(p.name, protocolName)==0) {
            return &p;
        }
    }
    return 0;
}

bool HS_AddVHost(HS_Server* server, const char* name) {
    HS_VHost& v = server->vhosts[server->vhostsCount++];
    
    strcpy(v.name, name);
    
    v.lwsContextInfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    //v.lwsContextInfo.ssl_cert_filepath = server->sslPublicKeyPath;
    //v.lwsContextInfo.ssl_private_key_filepath = server->sslPrivateKeyPath;
    //v.lwsContextInfo.pt_serv_buf_size = server->lwsContextInfo.pt_serv_buf_size;
    
    //v.lwsContextInfo.port = port;
    v.lwsContextInfo.user = &v;
    //v.lwsContextInfo.vhost_name  = hostName;
    
    //v.lwsContextInfo.error_document_404 = server->lwsContextInfo.error_document_404;
    //v.lwsContextInfo.ip_limit_ah = server->lwsContextInfo.ip_limit_ah; // 500;
    
    v.lwsContextInfo.protocols = v.lwsProtocols;
    v.lwsContextInfo.mounts = v.lwsMounts;
    
    v.verbosity = server->verbosity;
    HS__AddProtocol(server, name, "HTTP", HS_HTTPCallback, sizeof(HS_HTTPClient));
    
    return true;
}

#define HS_GetRequest(args) ((HS_Request*) args->userData);

bool HS_AddRequesterVHost(HS_Server* server, const char* clientSSLCAPath) {
    HS_AddVHost(server, "hs-requester-vhost");
    HS_VHost* v = HS_GetVHost(server, "hs-requester-vhost");
    v->lwsProtocolsCount = 0;
    v->lwsContextInfo.port = CONTEXT_PORT_NO_LISTEN;
    v->lwsContextInfo.client_ssl_ca_filepath = clientSSLCAPath;
    HS__AddProtocol(server, "hs-requester-vhost", "requester", HS_RequesterCallback, 0);
    return true;
}

void HS_AddCertToTrustStore(HS_Server* server, const char* cert) {
    HS_VHost* v = HS_GetVHost(server, "hs-requester-vhost");
    v->certTrustStore[v->certTrustStoreCount++] = cert;
}

void HS_IgnoreHTTP(HS_Server* server, const char* vhostName) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    lws_protocols& p = v->lwsProtocols[0];
    p.callback = HS_LWSCallbackStub;
    p.per_session_data_size = 0;
}

void HS_SetHTTPGetHandler(HS_Server* server, const char* vhostName, HS_UserCallback callback) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpGetHandler = callback;
}

void HS_SetHTTPPostHandler(HS_Server* server, const char* vhostName, HS_UserCallback callback) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpPostHandler = callback;
}

void HS_SetHTTPDeleteHandler(HS_Server* server, const char* vhostName, HS_UserCallback callback) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpDeleteHandler = callback;
}

void HS_SetHTTPRequestHandler(HS_Server* server, const char* vhostName, HS_UserCallback callback) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpGetHandler = callback;
    v->httpPostHandler = callback;
    v->httpDeleteHandler = callback;
}

void HS_SetHTTPGetEndpointChecker(HS_Server* server, const char* vhostName, HS_UserCallback checker) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpGetEndpointChecker = checker;
}

void HS_SetHTTPPostEndpointChecker(HS_Server* server, const char* vhostName, HS_UserCallback checker) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpPostEndpointChecker = checker;
}

void HS_SetHTTPDeleteEndpointChecker(HS_Server* server, const char* vhostName, HS_UserCallback checker) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpDeleteEndpointChecker = checker;
}

void HS_SetHTTPEndpointChecker(HS_Server* server, const char* vhostName, HS_UserCallback checker) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->httpGetEndpointChecker = checker;
    v->httpPostEndpointChecker = checker;
    v->httpDeleteEndpointChecker = checker;
}

void HS_SetCertificate(HS_Server* server, const char* vhostName, const char* sslPublicKeyPath, const char* sslPrivateKeyPath) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    strcpy(v->sslPublicKeyPath, sslPublicKeyPath);
    strcpy(v->sslPrivateKeyPath, sslPrivateKeyPath);

    v->lwsContextInfo.ssl_cert_filepath = v->sslPublicKeyPath;
    v->lwsContextInfo.ssl_private_key_filepath = v->sslPrivateKeyPath;
    v->lwsContextInfo.ssl_ca_filepath = v->sslPrivateKeyPath;
}

void HS_SetLogLevel(int level) {
    // LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG;
    lws_set_log_level(level, 0);
}

#define HS_SetLWSContextConfig(server, configName, value) server->lwsContextInfo.configName = value
#define HS_SetLWSVHostConfig(server, vhostName, configName, value) HS_GetVHost(server, vhostName)->lwsContextInfo.configName = value
#define HS_SetLWSDefaultProtocolConfig(server, vhostName, configName, value) HS_GetVHost(server, vhostName)->lwsProtocolDefaults.configName = value
#define HS_SetLWSProtocolConfig(server, vhostName, protocolName, configName, value) HS_GetProtocol(server, vhostName, protocolName)->configName = value

bool HS_InitVHosts(HS_Server* server) {
    bool result = true;
    for (int i = 0; i < server->vhostsCount; ++i) {
        HS_VHost& v = server->vhosts[i];
        v.lwsVHost = lws_create_vhost(server->lwsContext, &v.lwsContextInfo);
        result = v.lwsVHost ? result : false;
    }
    return result;
}

void HS_DisableFileCache(HS_Server* server, const char* vhostName) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->disableFileCache = true;
}

void HS_EnableFileCache(HS_Server* server, const char* vhostName) {
    HS_VHost* v = HS_GetVHost(server, vhostName);
    v->disableFileCache = false;
}

bool HS_RunForever(HS_Server* server, bool disableHTTP2=false) {
    if (!server->lwsContext) HS_InitServer(server, disableHTTP2);
    HS_InitVHosts(server);
    HS_InitPeriodicTasks(server);
    
    server->isRunning = true;
    int serviceReturn = 0;
    
    while (serviceReturn >= 0 && server->isRunning) {
        serviceReturn = lws_service(server->lwsContext, 0);
    }
    
    lws_cancel_service(server->lwsContext);
    server->isRunning = false;
    return true;
}

void HS_Stop(HS_Server* server) {
    server->isRunning = false;
    lws_cancel_service(server->lwsContext);
}

void HS_Destroy(HS_Server* server) {
    lws_context_destroy(server->lwsContext);
}

void HS_AddRedirToHTTPSVHost(HS_Server* server, const char* vhostName, const char* fromHostname, int fromPort, const char* toHostname, int toPort) {
    HS_AddVHost(server, vhostName);
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    
    vhost->lwsContextInfo.options = 0;
    vhost->lwsContextInfo.vhost_name = fromHostname;
    vhost->lwsContextInfo.port = fromPort;
    vhost->lwsContextInfo.protocols = {};
    
    // Redir Mount
    //-------------------
    lws_http_mount& redirectMount = vhost->lwsMounts[vhost->lwsMountsSize++];
    redirectMount.mountpoint = "/";
    redirectMount.mountpoint_len = 1;
    redirectMount.origin_protocol = LWSMPRO_REDIR_HTTPS;
    redirectMount.origin = (char*) calloc(1, HS__HostNameCap+8);
    sprintf((char*) redirectMount.origin, "%s:%d", toHostname, toPort);
}

//-------------------------
// Packet stuff
//-------------------------
struct HS_Packet {
    char* buffer;
    int   bufferSize;
    
    char* body;
    int   bodySize;
    
    int   closeStatus;
    int*  refCount;
};

struct HS_PacketQueue {
    HS_Packet* packets;
    int first;
    int end;
    int size;
    int capacity;
    lws* socket;
    lws_write_protocol writeProtocol;
};

HS_PacketQueue HS_CreatePacketQueue(lws* socket, int capacity, lws_write_protocol writeProtocol=LWS_WRITE_TEXT) {
    HS_PacketQueue queue = {};
    queue.packets = (HS_Packet*) calloc(1, capacity * sizeof(HS_Packet));
    queue.capacity = capacity;
    queue.socket = socket;
    queue.writeProtocol = writeProtocol;
    return queue;
}

void HS_Free(HS_PacketQueue queue) {
    free(queue.packets);
}

HS_Packet HS_Dequeue(HS_PacketQueue* queue) {
    HS_Assert(queue->size > 0);
    HS_Packet packet = queue->packets[queue->first];
    queue->first = (queue->first + 1) % queue->capacity;
    --queue->size;
    return packet;
}

void HS_Enqueue(HS_PacketQueue* queue, HS_Packet packet) {
    HS_Assert(queue->size < queue->capacity);
    queue->packets[queue->end] = packet;
    queue->end = (queue->end+1) % queue->capacity;
    ++queue->size;
}

bool HS_IsEmpty(HS_PacketQueue queue) {
    return queue.size == 0;
}

void HS_SendPacket(HS_PacketQueue* sendQueue, HS_Packet packet) {
    HS_Enqueue(sendQueue, packet);
    lws_callback_on_writable(sendQueue->socket);
}

HS_Packet HS_CreatePacket(int bufferSize=HS_KILO_BYTES(4)) {
    HS_Packet result = {};
    result.bufferSize = LWS_PRE + bufferSize;
    result.buffer = (char*) calloc(1, result.bufferSize);
    result.body = result.buffer + LWS_PRE;
    return result;
}

void HS_Free(HS_Packet packet) {
    free(packet.buffer);
}

int HS_WriteNextPacket(HS_PacketQueue* queue) {
    // Call from LWS_CALLBACK_SERVER_WRITEABLE
    if (!HS_IsEmpty(*queue)) {
        HS_Packet packet = HS_Dequeue(queue);
        
        if (packet.closeStatus) {
            lws_close_reason(queue->socket, (lws_close_status) packet.closeStatus, 0, 0);
            return -1;
        } else {
            int packetRawSize = LWS_PRE + packet.bodySize;
            
            lws_write(queue->socket, (unsigned char*) packet.body, packet.bodySize, queue->writeProtocol);
            
            HS_Free(packet);
            
            if (!HS_IsEmpty(*queue)) {
                lws_callback_on_writable(queue->socket);
            }
        }
    }
    
    return 0;
}

void HS_WriteBytes(HS_Packet* packet, void* bytes, int count) {
    HS_Assert1(LWS_PRE + packet->bodySize + count <= packet->bufferSize, "Attempt to write past buffer.");
    memcpy(packet->body + packet->bodySize, bytes, count);
    packet->bodySize += count;
}

void HS_WriteInt32(HS_Packet* packet, int value) {
    HS_WriteBytes(packet, (void*) &value, sizeof(value));
}

void HS_WriteString(HS_Packet* packet, const char* string, int size) {
    HS_WriteInt32(packet, size);
    HS_WriteBytes(packet, (void*) string, size);
}

void HS_WriteString(HS_Packet* packet, const char* string) {
    int size = strlen(string);
    HS_WriteString(packet, string, size);
}

void HS_CloseConnection(HS_PacketQueue* queue, int closeStatus) {
    HS_Packet packet = {};
    packet.closeStatus = closeStatus;
    lws_callback_on_writable(queue->socket);
}

bool HS_ReceiveMessageFragment(HS_CallbackArgs* args, char** receivedBuffer, int* receivedSize, int* receivedCap, HS_CallbackFunc processCompleteMessage) {
    // Call this from LWS_CALLBACK_RECEIVE
    
    if (*receivedSize + (int) args->len > *receivedCap) {
        if (*receivedBuffer) {
            *receivedCap = 2*(*receivedSize + args->len);
            char* newBuffer = (char*) calloc(1, *receivedCap);
            memcpy(newBuffer, *receivedBuffer, *receivedSize);
            free(*receivedBuffer);
            *receivedBuffer = newBuffer;
        } else {
            *receivedCap = args->len+1;
            *receivedBuffer = (char*) calloc(1, *receivedCap);
        }
    }

    memcpy((*receivedBuffer)+(*receivedSize), args->in, args->len);
    *receivedSize += args->len;
    
    if (lws_is_final_fragment(args->socket)) {
        (*receivedBuffer)[*receivedSize] = 0;
        processCompleteMessage(args);
        
        free(*receivedBuffer);
        *receivedBuffer = 0;
        *receivedCap = 0;
        *receivedSize = 0;
    }
    
    return false;
}

void HS_AddServedFilesDir(HS_Server* server, const char* vhostName, const char* uriPrefix, const char* path) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    HS_RootDirMapEntry& entry = vhost->rootDirMap[vhost->rootDirMapSize++];
    strcpy(entry.uriPrefix, uriPrefix);
    realpath(path, entry.path);

    if (!HS_EndsWith(entry.uriPrefix, "/")) {
        strcat(entry.uriPrefix, "/");
    }
}

bool HS_InitFileServer(HS_Server* server, const char* vhostName, const char* configFilePath) {
    HS_VHost* vhost = HS_GetVHost(server, vhostName);
    char servedFilesRootDir[HS__FilePathCap] = {};
    
    JS_Reader jreader[] = {
        {"hostname", JS_Type_String, vhost->hostName},
        {"port", JS_Type_Integer, &vhost->port},
        {"served-files-dir", JS_Type_String, servedFilesRootDir},
        {"served-files-dir-map", JS_Type_Dict},
        {"error-404-file", JS_Type_String, vhost->error404File},
        {"ssl-public-key-path", JS_Type_String, vhost->sslPublicKeyPath},
        {"ssl-private-key-path", JS_Type_String, vhost->sslPrivateKeyPath},
        {"ssl-ca-bundle-path", JS_Type_String, vhost->sslCABundlePath},
        {"mem-cache-max-size-mb", JS_Type_Integer, &vhost->memCacheMaxSizeMB},
        {"default-content-language", JS_Type_String, &vhost->defaultContentLanguage},
        {"allowed-origins", JS_Type_Dict},
        {"gatekeepr", JS_Type_Dict},
        {"gatekeepr/database", JS_Type_String, &vhost->gkDatabasePath},
        {}
    };
    
    JS_JSON* jConfig = JS_ParseFile(configFilePath);
    vhost->jConfig = jConfig;
    bool result = JS_Parse(jreader, jConfig);
    
    if (result) {
        HS_SetVHostHostName(server, vhostName, vhost->hostName);
        HS_SetVHostPort(server, vhostName, vhost->port);

        vhost->lwsContextInfo.ssl_cert_filepath = vhost->sslPublicKeyPath;
        vhost->lwsContextInfo.ssl_private_key_filepath = vhost->sslPrivateKeyPath;
        vhost->lwsContextInfo.ssl_ca_filepath = vhost->sslPrivateKeyPath;
        
        realpath(servedFilesRootDir, vhost->servedFilesRootDir);
        
        JS_JSON* j = JS_Get(jConfig, "uri-map");
        if (j) {
            for (int i = 0; i < j->size; ++i) {
                HS_PushURIMapping(server, vhostName, j->pairs[i].key, j->pairs[i].value->string);
            }
        }
        
        j = JS_Get(jConfig, "cache-control-map");
        if (j) {
            for (int i = 0; i < j->size; ++i) {
                HS_CacheControlMapEntry& entry = vhost->cacheControlMap[vhost->cacheControlMapSize++];
                strcpy(entry.uri, j->pairs[i].key);
                entry.uriSize = strlen(entry.uri);
                strcpy(entry.cacheString, j->pairs[i].value->string);
            }
        }
        
        j = JS_Get(jConfig, "redirect-map");
        if (j) {
            for (int i = 0; i < j->size; ++i) {
                HS_RedirectMapEntry& entry = vhost->redirectMap[vhost->redirectMapSize++];
                strcpy(entry.uri, j->pairs[i].key);
                strcpy(entry.destination, j->pairs[i].value->string);
            }
        }

        j = JS_Get(jConfig, "served-files-dir-map");
        if (j) {
            for (int i = 0; i < j->size; ++i) {
                HS_AddServedFilesDir(server, vhostName, j->pairs[i].key, j->pairs[i].value->string);
            }
        }

        j = JS_Get(jConfig, "cache-bust");
        if (j) {
            for (int i = 0; i < j->size; ++i) {
                char* entry = vhost->cacheBust[vhost->cacheBustSize++];
                strcpy(entry, j->array[i]->string);
            }
        }
        
        j = JS_Get(jConfig, "needs-ssi-parsing");
        if (j) {
            for (int i = 0; i < j->size; ++i) {
                char* entry = vhost->needsSSIParsing[vhost->needsSSIParsingSize++];
                strcpy(entry, j->array[i]->string);
            }
        }
        
        j = JS_Get(jConfig, "allowed-origins");
        if (j) {
            JS_Iterator it = JS_ForEach(j);
            while (JS_Next(&it)) {
                HS_AllowedOrigin& entry = vhost->allowedOrigins[vhost->allowedOriginsCount++];
                strcpy(entry.dest, it.key);
                strcpy(entry.origin, it.value->string);
            }
        }
        
        j = JS_Get(jConfig, "gatekeepr");
        if (j) {
            if (!vhost->gkDatabasePath[0]) {
                strcpy(vhost->gkDatabasePath, "gatekeepr.db");
            }
            
            if (HS_IsFileReadable(vhost->gkDatabasePath)) {
                vhost->gkdb = SQ_OpenDB(vhost->gkDatabasePath);
            }
            
            vhost->gkConfig = j;
        }
    
        if (vhost->port == 443) {
            char redirName[HS__HostNameCap+12] = {};
            sprintf(redirName, "%s-80-to-443", vhost->hostName);
            HS_AddRedirToHTTPSVHost(server, redirName, vhost->hostName, 80, vhost->hostName, vhost->port);
        }
        
        if (vhost->gkConfig) {
            vhost->httpGetHandler = HS_GetFileByURIOrAuthenticate;
        } else {
            vhost->httpGetHandler = HS_GetFileByURI;
        }
    } else {
        fprintf(stderr, "ERROR: Failed to load/parse configuration file\n");
    }
    
    return result;
}

#endif
