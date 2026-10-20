#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../interfaces/host.h"

void* HostWindowHandle();

namespace {

struct SessionDesc {
    unsigned long dwSize;
    unsigned long dwFlags;
    host_guid guidInstance;
    host_guid guidApplication;
    unsigned long dwMaxPlayers;
    unsigned long dwCurrentPlayers;
    char* lpszSessionName;
    char* lpszPassword;
    unsigned long dwReserved1;
    unsigned long dwReserved2;
    unsigned long dwUser1;
    unsigned long dwUser2;
    unsigned long dwUser3;
    unsigned long dwUser4;
};

struct GameRec {
    int unknown_0;
    int user1;
    int user2;
    int user3;
    int user4;
    int maxPlayers;
    char name[0x20];
    host_guid guidInstance;
    char unknown_48[0xc];
};

struct Conn {
    void* data;
    unsigned long size;
};

struct DPName {
    unsigned long dwSize;
    unsigned long dwFlags;
    char* lpszShortNameA;
    char* lpszLongNameA;
};

struct Connection {
    char unknown_0[8];
    SessionDesc* desc;
};

class DirectPlay {
public:
    virtual int __stdcall QueryInterface(void* iid, void** out);
    virtual unsigned long __stdcall AddRef();
    virtual unsigned long __stdcall Release();
    virtual int __stdcall AddPlayerToGroup(unsigned long group, unsigned long player);
    virtual int __stdcall Close();
    virtual int __stdcall Slot05();
    virtual int __stdcall CreatePlayer(unsigned long* id, DPName* name, void* event,
                                       void* data, unsigned long size, unsigned long flags);
    virtual int __stdcall Slot07();
    virtual int __stdcall Slot08();
    virtual int __stdcall DestroyPlayer(unsigned long id);
    virtual int __stdcall Slot10();
    virtual int __stdcall Slot11();
    virtual int __stdcall EnumPlayers(void* session, void* callback, void* context, unsigned long flags);
    virtual int __stdcall EnumSessions(void* desc, unsigned long timeout, void* callback, void* context, unsigned long flags);
    virtual int __stdcall Slot14();
    virtual int __stdcall Slot15();
    virtual int __stdcall Slot16();
    virtual int __stdcall Slot17();
    virtual int __stdcall GetPlayerAddress(unsigned long player, void* data, unsigned long* size);
    virtual int __stdcall Slot19();
    virtual int __stdcall GetPlayerData(unsigned long player, void* data, unsigned long* size, unsigned long flags);
    virtual int __stdcall GetPlayerName(unsigned long id, void* data, unsigned long* size);
    virtual int __stdcall GetSessionDesc(void* data, unsigned long* size);
    virtual int __stdcall Slot23();
    virtual int __stdcall Slot24(void* desc, unsigned long flags);
    virtual int __stdcall Receive(unsigned long* from, unsigned long* to, unsigned long flags, void* data, unsigned long* size);
    virtual int __stdcall Send(unsigned long from, unsigned long to, unsigned long flags, void* data, unsigned long size);
    virtual int __stdcall Slot27();
    virtual int __stdcall Slot28();
    virtual int __stdcall SetPlayerData(unsigned long player, void* data, unsigned long size, unsigned long flags);
    virtual int __stdcall SetPlayerName(unsigned long id, void* name, unsigned long flags);
    virtual int __stdcall SetSessionDesc(void* desc, unsigned long flags);
    virtual int __stdcall Slot32();
    virtual int __stdcall Slot33();
    virtual int __stdcall Slot34();
    virtual int __stdcall EnumConnections(host_guid* application, void* callback, void* context, unsigned long flags);
    virtual int __stdcall Slot36();
    virtual int __stdcall Slot37();
    virtual int __stdcall Slot38(int val, int flag);
};

class Lobby {
public:
    virtual int __stdcall QueryInterface(GUID* riid, void** ppvObject);
    virtual unsigned long __stdcall AddRef();
    virtual unsigned long __stdcall Release();
    virtual int __stdcall Connect(unsigned long flags, void** dp, void* outer);
    virtual int __stdcall CreateAddress(GUID* sp, GUID* device, const void* address,
                                        unsigned long addressSize, void* dest,
                                        unsigned long* destSize);
    virtual int __stdcall EnumAddress(void* callback, const void* address,
                                      unsigned long length, void* context);
    virtual int __stdcall EnumAddressTypes(void* callback, GUID* sp, void* context,
                                           unsigned long flags);
    virtual int __stdcall EnumLocalApplications(void* callback, void* context,
                                                unsigned long flags);
    virtual int __stdcall GetConnectionSettings(unsigned long appID, void* connection,
                                                unsigned long* dataSize);
    virtual int __stdcall Slot09();
    virtual int __stdcall Slot10();
    virtual int __stdcall Slot11();
    virtual int __stdcall SetConnectionSettings(unsigned long a, unsigned long b, Connection* conn);
    virtual int __stdcall Slot13();
    virtual int __stdcall CreateCompoundAddress(void* elements, unsigned long count, void* address,
                                                unsigned long* size);
};

}

#pragma pack(push, 1)
struct host_net {
    char name[0x10];
    char unknown_10;
    char longName[16];
    char shortName[16];
    char unknown_31[0x431 - 0x31];
    host_guid* guids;
    Conn* conns;
    char* names;
    host_guid application;
    host_guid sp;
    SessionDesc desc;
    int hwnd;
    char unknown_4b1[4];
    unsigned long from;
    unsigned long to;
    GameRec* games;
    DirectPlay* dp1;
    DirectPlay* dp;
    Lobby* created;
    Lobby* lobby;
    Connection* connection;
    int connection_size;
    char compressionOff[4];
    int maxPlayers;
    int enumTimeout;
    int providerCount;
    int count;
};

struct PlayerData {
    char name[17];
    short versionMajor;
    short versionMinor;
};
#pragma pack(pop)

struct host_net_pair {
    DirectPlay* dp;
    DirectPlay* dp3;
};

typedef BOOL (__stdcall* EnumSpCallback)(host_guid* sp, char* name, DWORD major, DWORD minor, host_net* context);

extern "C" HRESULT __stdcall DirectPlayCreate(GUID* sp, DirectPlay** dp, void* unknown);
extern "C" HRESULT __stdcall DirectPlayLobbyCreateA(GUID* lpGUID,
                                                    Lobby** lplpDPL,
                                                    void* lpUnkOuter, void* lpReserved,
                                                    unsigned long dwFlags);
extern "C" HRESULT __stdcall DirectPlayEnumerateA(EnumSpCallback callback, void* context);

namespace {

int g_enumSessionsResult;
int g_guaranteePackets = 1;

host_guid g_defaultSp = {{0, 0, 0, 0}};
host_guid g_skipped0 = {{0xd8d29744, 0x11d0208a, 0xa0009dbc, 0xb6672924}};
host_guid g_skipped1 = {{0xd1714f40, 0x11d05989, 0x4544849a, 0x00005453}};
host_guid g_skipped2 = {{0x25462ba0, 0x11cfce8d, 0xaa009b83, 0x4830b900}};
host_guid g_skipped3 = {{0x4595fd80, 0x11d11d4d, 0x6000fc99, 0x5abf6a97}};
host_guid* g_skippedProviders[4] = {&g_skipped0, &g_skipped1, &g_skipped2, &g_skipped3};

GUID g_iidDirectPlay3A = {0x133efe41, 0x32dc, 0x11d0, {0x9c, 0xfb, 0x00, 0xa0, 0xc9, 0x0a, 0x43, 0xcb}};
GUID g_iidDirectPlayLobby2A = {0x1bb4af80, 0xa303, 0x11d0, {0x9c, 0x4f, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e}};

// FUNCTION: 0x4c9740
void HapinetTrace(const char*)
{
}

char* SkipTextLines(char* text, int n)
{
    int i = 0;
    int lines = 0;
    while (lines < n) {
        if (text[i] == 0 || text[i] == '\n') {
            lines++;
        }
        i++;
    }
    return text + i;
}

// FUNCTION: 0x4c9c50
int __stdcall HAPINET_getgamescallback(SessionDesc* lpsd, int* lpdwTimeout, unsigned int dwFlags, host_net* ctx)
{
    HapinetTrace("HAPINET_getgamescallback\n");
    if ((dwFlags & 1) == 0)
    {
        GameRec* rec = &ctx->games[ctx->count];
        strcpy(rec->name, lpsd->lpszSessionName);
        rec->user1 = lpsd->dwUser1;
        rec->user2 = lpsd->dwUser2;
        rec->user3 = lpsd->dwUser3;
        rec->user4 = lpsd->dwUser4;
        rec->guidInstance = lpsd->guidInstance;
        rec->maxPlayers = lpsd->dwMaxPlayers;
        ctx->count++;
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9d10
int __stdcall HAPINET_justone(int p1, int p2, int p3, int p4, void* p5)
{
    HapinetTrace("HAPINET_justone\n");
    int* addr = (int*)p5;
    addr[0x12] = p1;
    return 0;
}

// FUNCTION: 0x4c9d30
int HAPINET_setplayerdatabuffer(host_net* net, unsigned long player, void* data, unsigned long size)
{
    HapinetTrace("HAPINET_setplayerdatabuffer\n");
    if (net->dp != 0 && net->dp->SetPlayerData(player, data, size, 2) == 0) {
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9d80
int HAPINET_getplayerdatabuffer(host_net* net, unsigned long player, void* data, unsigned long* size)
{
    HapinetTrace("HAPINET_getplayerdatabuffer\n");
    if (net->dp != 0 && net->dp->GetPlayerData(player, data, size, 0) == 0) {
        return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9dd0
int HAPINET_getcurrentplayers(host_net* net)
{
    int hr;
    HapinetTrace("HAPINET_getcurrentplayers\n");
    int players = 0;
    if (net->dp != 0) {
        unsigned long size = 0;
        hr = net->dp->GetSessionDesc(0, &size);
        if (hr == (int)0x8877001e) {
            SessionDesc* desc = (SessionDesc*)malloc(size);
            hr = net->dp->GetSessionDesc(desc, &size);
            if (hr == 0)
                players = desc->dwCurrentPlayers;
            free(desc);
        }
    }
    return players;
}

// FUNCTION: 0x4c9800
int HAPINET_sendpacketguaranteed(host_net* net, unsigned long from, unsigned long to, void* data, unsigned long size)
{
    HapinetTrace("HAPINET_sendpacketguaranteed\n");
    int result = 0x887700aa;
    if (net->dp != 0) {
        result = net->dp->Send(from, to, 1, data, size);
    }
    return result;
}

// FUNCTION: 0x4ca100
int __stdcall HAPINET_enumconnections(host_guid* guid, void* connection, unsigned long size,
                                      DPName* name, unsigned long flags, host_net* net)
{
    char local[200];
    HapinetTrace("HAPINET_enumconnections\n");
    for (unsigned int i = 0; i < 4; i++) {
        if (memcmp(guid, g_skippedProviders[i], sizeof(host_guid)) == 0)
            return 1;
    }
    net->guids[net->providerCount] = *guid;
    net->conns[net->providerCount].data = malloc(size);
    if (net->conns[net->providerCount].data == 0)
        return 0;
    memcpy(net->conns[net->providerCount].data, connection, size);
    net->conns[net->providerCount].size = size;
    sprintf(local, "%s", name->lpszShortNameA);
    char* dest = SkipTextLines(net->names, net->providerCount);
    strcpy(dest, local);
    net->providerCount++;
    return 1;
}

// FUNCTION: 0x4ca330
int __stdcall HAPINET_enumproviders(host_guid* guid, char* name, unsigned long major,
                                    unsigned long minor, host_net* net)
{
    char local[200];
    HapinetTrace("HAPINET_enumproviders\n");
    net->guids[net->providerCount] = *guid;
    sprintf(local, "%s %d.%d", name, major, minor);
    char* slot = SkipTextLines(net->names, net->providerCount);
    strcpy(slot, local);
    net->providerCount++;
    return 1;
}

// FUNCTION: 0x4ca400
int HAPINET_getproviders(host_net* net, int a, int c)
{
    HapinetTrace("HAPINET_getproviders\n");
    net->providerCount = 0;
    net->guids = (host_guid*)a;
    net->names = (char*)c;
    DirectPlayEnumerateA(HAPINET_enumproviders, net);
    return 1;
}

// FUNCTION: 0x4ca5d0
int HAPINET_initmultiplay(host_net* net, host_guid* sp, host_guid* application)
{
    HapinetTrace("HAPINET_initmultiplay\n");
    g_guaranteePackets = 0;
    net->hwnd = (int)(size_t)HostWindowHandle();
    net->application = *application;
    net->created = 0;
    net->lobby = 0;
    net->connection = 0;
    net->connection_size = 0;
    net->dp1 = 0;
    net->dp = 0;
    net->sp = *sp;
    int hr = DirectPlayCreate((GUID*)&net->sp, &net->dp1, 0);
    if (hr != 0)
        return 0;
    hr = net->dp1->QueryInterface(&g_iidDirectPlay3A, (void**)&net->dp);
    return hr == 0 ? 1 : 0;
}

}

// FUNCTION: 0x4c9530
extern "C" const char* HOST_CALL host_net_error_string(int error)
{
    HapinetTrace("HAPINET_GetDPErrorString\n");
    switch (error)
    {
    case 0x80004001:
        return "DPERR_UNSUPPORTED - The function is not available in this implementation. ";
    case 0x80004005:
        return "DPERR_GENERIC - An undefined error condition occurred. ";
    case 0x8007000e:
        return "DPERR_OUTOFMEMORY - There is insufficient memory to perform the requested operation. ";
    case 0x80070057:
        return "DPERR_INVALIDPARAMS - One or more of the parameters passed to the function are invalid.";
    case 0x88770005:
        return "DPERR_ALREADYINITIALIZED - This object is already initialized. ";
    case 0x8877000a:
        return "DPERR_ACCESSDENIED - The session is full or an incorrect password was supplied.";
    case 0x88770014:
        return "DPERR_ACTIVEPLAYERS - The requested operation cannot be performed because there are existing active players.";
    case 0x8877001e:
        return "DPERR_BUFFERTOOSMALL - The supplied buffer is not large enough to contain the requested data.";
    case 0x88770028:
        return "DPERR_CANTADDPLAYER - The player cannot be added to the session.";
    case 0x8877003c:
        return "DPERR_CANTCREATEPLAYER - A new player cannot be created. ";
    case 0x88770050:
        return "DPERR_CAPSNOTAVAILABLEYET - The capabilities of the DirectPlay object have not been determined yet.";
    case 0x8877005a:
        return "DPERR_EXCEPTION - An exception occurred when processing the request. ";
    case 0x88770078:
        return "DPERR_INVALIDFLAGS - The flags passed to this function are invalid. ";
    case 0x88770082:
        return "DPERR_INVALIDOBJECT - The DirectPlay object pointer is invalid. ";
    case 0x88770096:
        return "DPERR_INVALIDPLAYER - The player ID is not recognized as a valid player ID for this game session.";
    case 0x887700a0:
        return "DPERR_NOCAPS - The communication link underneath DirectPlay is not capable of this function.";
    case 0x887700aa:
        return "DPERR_NOCONNECTION - No communication link was established. ";
    case 0x887700be:
        return "DPERR_NOMESSAGES - There are no messages to be received. ";
    case 0x887700c8:
        return "DPERR_NONAMESERVERFOUND - No name server could be found or created. A name server must exist in order to create a player. ";
    case 0x887700d2:
        return "DPERR_NOPLAYERS - There are no active players in the session. ";
    case 0x887700dc:
        return "DPERR_NOSESSIONS - There are no existing sessions for this game. ";
    case 0x887700e6:
        return "DPERR_SENDTOOBIG - The message buffer passed to the IDirectPlay::Send method is larger than allowed. ";
    case 0x887700f0:
        return "DPERR_TIMEOUT - The operation could not be completed in the specified time. ";
    case 0x887700fa:
        return "DPERR_UNAVAILABLE - The requested service provider or session is not available. ";
    case 0x8877010e:
        return "DPERR_BUSY - The DirectPlay message queue is full. ";
    case 0x88770118:
        return "DPERR_USERCANCEL - The user canceled the connection process during a call to the IDirectPlay::Open method.";
    case 0x88770136:
        return "DPERR_SESSIONLOST - The session was lost. ";
    case 0x88770140:
        return "DPERR_UNINITIALIZED - An object was not initialized. ";
    case 0x00000000:
        return "DP_OK - The request completed successfully.";
    }
    return "DPERR_IHAVENOBLOODYIDEA - DirectPlay returned an undocumented return value.";
}

extern "C" int HOST_CALL host_net_enum_sessions_result(void)
{
    return g_enumSessionsResult;
}

extern "C" void HOST_CALL host_net_free(void* block)
{
    free(block);
}

// FUNCTION: 0x4c9790
extern "C" int HOST_CALL host_net_guarantee_packets(int enable)
{
    HapinetTrace("HAPINET_guaranteepackets\n");
    int result = g_guaranteePackets;
    g_guaranteePackets = enable;
    return result;
}

// FUNCTION: 0x4c97b0
extern "C" int HOST_CALL host_net_send_packet(host_net* net, unsigned long from, unsigned long to, void* data, unsigned long size)
{
    HapinetTrace("HAPINET_sendpacket\n");
    int result = 0x887700aa;
    if (net->dp != 0) {
        result = net->dp->Send(from, to, g_guaranteePackets != 0, data, size);
    }
    return result;
}

// FUNCTION: 0x4c9840
extern "C" int HOST_CALL host_net_receive_packet(host_net* net, void* data, unsigned long* size)
{
    HapinetTrace("HAPINET_receivepacket\n");
    int result = 0x887700aa;
    if (net->dp != 0) {
        result = net->dp->Receive(&net->from, &net->to, 1, data, size);
    }
    return result;
}

// FUNCTION: 0x4c9890
extern "C" int HOST_CALL host_net_update_game_info(host_net* net, char* name, char* data, int user1, int user2, int user3, int user4)
{
    HapinetTrace("HAPINET_updategameinfo\n");
    if (net->dp != 0) {
        strncpy(net->name, name, 0x10);
        net->desc.dwSize = 0x50;
        net->desc.dwUser4 = user4;
        net->desc.dwUser3 = user3;
        net->desc.dwUser2 = user2;
        net->desc.dwUser1 = user1;
        net->desc.lpszSessionName = name;
        if (net->dp->SetSessionDesc(&net->desc, 0) == 0)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9920
extern "C" int HOST_CALL host_net_create_new_game(host_net* net, char* name, char* password, int user1, int user2, int user3, int user4)
{
    HapinetTrace("HAPINET_createnewgame\n");
    if (net->dp != 0) {
        strncpy((char*)net, name, 0x10);
        memset(&net->desc, 0, 0x50);
        net->desc.dwMaxPlayers = net->maxPlayers;
        net->desc.guidApplication = net->application;
        net->desc.dwSize = 0x50;
        net->desc.lpszSessionName = name;
        net->desc.dwUser4 = user4;
        net->desc.dwUser3 = user3;
        net->desc.dwUser2 = user2;
        net->desc.dwUser1 = user1;
        net->desc.lpszPassword = password;
        int r = net->dp->Slot24(&net->desc, 2);
        unsigned long size = 0x50;
        int hr = net->dp->GetSessionDesc(0, &size);
        if (hr == (int)0x8877001e) {
            SessionDesc* buf = (SessionDesc*)malloc(size);
            if (buf != 0) {
                if (net->dp->GetSessionDesc(buf, &size) == 0)
                    net->desc.guidInstance = buf->guidInstance;
                free(buf);
            }
        }
        if (r == 0)
            return 1;
    }
    return 0;
}

// FUNCTION: 0x4c9a70
extern "C" int HOST_CALL host_net_create_or_join_lobby_game(host_net* net, char* password, unsigned long maxPlayers,
                                                            unsigned long user1, unsigned long user2, unsigned long user3,
                                                            unsigned long user4)
{
    HapinetTrace("HAPINET_createorjoinlobbygame\n");
    int hr = 0;
    if (password != 0 || maxPlayers != 0 || user1 != 0 || user2 != 0 || user3 != 0 || user4 != 0) {
        SessionDesc* d = net->connection->desc;
        if (password != 0) {
            d->lpszPassword[0] = 0;
            strncat(d->lpszPassword, password, 0x10);
        }
        if (d->dwMaxPlayers == 0) d->dwMaxPlayers = maxPlayers;
        if (d->dwUser1 == 0) d->dwUser1 = user1;
        if (d->dwUser2 == 0) d->dwUser2 = user2;
        if (d->dwUser3 == 0) d->dwUser3 = user3;
        if (d->dwUser4 == 0) d->dwUser4 = user4;
        hr = net->lobby->SetConnectionSettings(0, 0, net->connection);
    }
    if (hr >= 0) {
        net->desc = *net->connection->desc;
        hr = net->lobby->Connect(0, (void**)&net->dp, 0);
    }
    return hr >= 0;
}

// FUNCTION: 0x4c9b70
extern "C" void HOST_CALL host_net_uninit(host_net* net)
{
    HapinetTrace("HAPINET_uninitmultiplay\n");
    if (net->dp != 0) {
        net->dp->Close();
        net->dp->Release();
        net->dp = 0;
    }
    if (net->dp1 != 0) {
        net->dp1->Close();
        net->dp1->Release();
        net->dp1 = 0;
    }
    if (net->lobby != 0) {
        net->lobby->Release();
        net->lobby = 0;
    }
    if (net->created != 0) {
        net->created->Release();
        net->created = 0;
    }
    if (net->connection != 0) {
        free(net->connection);
        net->connection = 0;
        net->connection_size = 0;
    }
}

// FUNCTION: 0x4c9c20
extern "C" void HOST_CALL host_net_init_defaults(host_net* net)
{
    HapinetTrace("HAPINET_initmultiplaydefaults\n");
    net->maxPlayers = 0x10;
    net->enumTimeout = 0x5dc;
}

// FUNCTION: 0x4c9e50
extern "C" int HOST_CALL host_net_get_games(host_net* net, void* sessions, int (HOST_CALL* poll)(void))
{
    HapinetTrace("HAPINET_getgames\n");
    net->count = 0;
    if (net->dp == 0)
        return -1;
    SessionDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = 0x50;
    desc.guidApplication = net->application;
    desc.guidInstance = net->desc.guidInstance;
    net->games = (GameRec*)sessions;
    int result;
    do {
        result = net->dp->EnumSessions(&desc, net->enumTimeout, (void*)HAPINET_getgamescallback, net, 0x81);
        g_enumSessionsResult = result;
        host_pump_events(0);
        if (poll != 0 && poll() != 0)
            break;
    } while (result == 0x8877015e);
    if (result == 0)
        return net->count;
    return -1;
}

// FUNCTION: 0x4c9f90
extern "C" int HOST_CALL host_net_quit_game(host_net* net)
{
    HapinetTrace("HAPINET_quitgame\n");
    if (net->dp != 0 && net->dp->Close() != 0) {
        return 0;
    }
    return 1;
}

// FUNCTION: 0x4c9fd0
extern "C" int HOST_CALL host_net_join_game(host_net* net, const host_guid* guid)
{
    HapinetTrace("HAPINET_joingame\n");
    if (net->dp != 0) {
        memset(&net->desc, 0, 0x50);
        net->desc.dwSize = 0x50;
        net->desc.guidApplication = net->application;
        net->desc.guidInstance = *guid;
        if (net->dp->Slot24(&net->desc, 1) == 0) {
            unsigned long size = 0x50;
            if (net->dp->GetSessionDesc(0, &size) == (int)0x8877001e) {
                SessionDesc* p = (SessionDesc*)malloc(size);
                if (p != 0) {
                    if (net->dp->GetSessionDesc(p, &size) == 0) {
                        net->desc.guidInstance = p->guidInstance;
                        if (p->lpszSessionName != 0)
                            lstrcpynA(net->name, p->lpszSessionName, 0x11);
                    }
                    free(p);
                }
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: 0x4ca250
extern "C" int HOST_CALL host_net_get_connections(host_net* net, void* guids, void* conns, void* names, const host_guid* application)
{
    HapinetTrace("HAPINET_getconnections\n");
    if (net->dp == 0) {
        host_guid sp = g_defaultSp;
        host_guid app = *application;
        if (!HAPINET_initmultiplay(net, &sp, &app)) {
            return 0;
        }
    }
    net->guids = (host_guid*)guids;
    net->providerCount = 0;
    net->conns = (Conn*)conns;
    net->names = (char*)names;
    int hr = net->dp->EnumConnections((host_guid*)application, (void*)HAPINET_enumconnections, net, 1);
    return hr == 0 ? 1 : 0;
}

// FUNCTION: 0x4ca450
extern "C" unsigned int HOST_CALL host_net_password_required(host_net* net)
{
    HapinetTrace("HAPINET_passwordrequired\n");
    if (net->connection != 0)
        return net->connection->desc->dwFlags & 0x400;
    return net->desc.dwFlags & 0x400;
}

// FUNCTION: 0x4ca490
extern "C" int HOST_CALL host_net_init_lobbied_connection(host_net* net)
{
    HapinetTrace("HAPINET_initlobbiedconnection\n");
    if (net->lobby != 0)
        return 0;
    net->created = 0;
    net->lobby = 0;
    net->connection = 0;
    net->connection_size = 0;
    int hr = DirectPlayLobbyCreateA(0, &net->created, 0, 0, 0);
    if (hr >= 0) {
        hr = net->created->QueryInterface(&g_iidDirectPlayLobby2A, (void**)&net->lobby);
        if (hr >= 0) {
            unsigned long size = 0;
            hr = net->lobby->GetConnectionSettings(0, 0, &size);
            if (hr == (int)0x8877001e) {
                net->connection = (Connection*)malloc(size);
                if (net->connection != 0) {
                    hr = net->lobby->GetConnectionSettings(0, net->connection, &size);
                    if (hr >= 0)
                        net->connection_size = size;
                } else {
                    hr = (int)0x8007000e;
                }
            }
            HapinetTrace("HAPINET_guaranteepackets\n");
            g_guaranteePackets = 1;
        }
    }
    return hr >= 0;
}

// FUNCTION: 0x4ca590
extern "C" int HOST_CALL host_net_init_connection(host_net* net, int* connection)
{
    HapinetTrace("HAPINET_initconnection\n");
    return net->dp->Slot38(*connection, 0) >= 0;
}

// FUNCTION: 0x4ca6a0
extern "C" int HOST_CALL host_net_add_player(host_net* net, unsigned long* id, char* shortName, char* longName,
                                             char* name, short versionMajor, short versionMinor)
{
    HapinetTrace("HAPINET_addplayer\n");
    if (net->dp != 0) {
        DPName dpname;
        dpname.dwSize = sizeof(DPName);
        dpname.dwFlags = 0;
        dpname.lpszShortNameA = shortName;
        dpname.lpszLongNameA = longName;
        strncpy(net->longName, longName, 16);
        strncpy(net->shortName, shortName, 16);
        PlayerData data;
        memset(&data, 0, sizeof(data));
        strncpy(data.name, name, 16);
        data.versionMajor = versionMajor;
        data.versionMinor = versionMinor;
        int hr = net->dp->CreatePlayer(id, &dpname, 0, &data, sizeof(data), 0);
        return hr == 0 ? 1 : 0;
    }
    return 0;
}

// FUNCTION: 0x4ca780
extern "C" int HOST_CALL host_net_remove_player(host_net* net, unsigned long id)
{
    HapinetTrace("HAPINET_removeplayer\n");
    if (net->dp != 0) {
        int hr = net->dp->DestroyPlayer(id);
        return hr == 0 ? 1 : 0;
    }
    return 0;
}

// FUNCTION: 0x4ca7c0
extern "C" int HOST_CALL host_net_get_player_name(host_net* net, unsigned long id, void* data, unsigned long* size)
{
    HapinetTrace("HAPINET_getplayername\n");
    if (net->dp != 0) {
        return net->dp->GetPlayerName(id, data, size);
    }
    return 0x88770140;
}

// FUNCTION: 0x4ca800
extern "C" int HOST_CALL host_net_set_player_name(host_net* net, unsigned long id, void* name, unsigned long flags)
{
    HapinetTrace("HAPINET_setplayername\n");
    if (net->dp != 0) {
        return net->dp->SetPlayerName(id, name, flags);
    }
    return 0x88770140;
}

// FUNCTION: 0x4ca840
extern "C" int HOST_CALL host_net_enum_players(host_net* net, void* session, void* callback, void* context, unsigned long flags)
{
    HapinetTrace("HAPINET_enumplayers\n");
    if (net->dp != 0) {
        return net->dp->EnumPlayers(session, callback, context, flags);
    }
    return 0x88770140;
}

// FUNCTION: 0x4ca880
extern "C" int HOST_CALL host_net_create_compound_address(host_net* net, void* elements, unsigned long count, void* address,
                                                          unsigned long* size)
{
    HapinetTrace("HAPINET_createcompoundaddress\n");
    if (net->lobby != 0) {
        return net->lobby->CreateCompoundAddress(elements, count, address, size);
    }
    return 0x88770140;
}

// FUNCTION: 0x4ca8c0
extern "C" int HOST_CALL host_net_enum_address(host_net* net, void* callback, void* address, unsigned long size, void* context)
{
    HapinetTrace("HAPINET_enumaddress\n");
    if (net->lobby != 0) {
        return net->lobby->EnumAddress(callback, address, size, context);
    }
    return 0x88770140;
}

// FUNCTION: 0x4ca900
extern "C" int HOST_CALL host_net_create_play_interface(const host_guid* sp, host_net_pair* net)
{
    HapinetTrace("HAPINET_createdplayinterface\n");
    memset(net, 0, sizeof(host_net_pair));
    HRESULT hr = DirectPlayCreate((GUID*)sp, &net->dp, 0);
    if (hr >= 0) {
        hr = net->dp->QueryInterface(&g_iidDirectPlay3A, (void**)&net->dp3);
    }
    return hr;
}

// FUNCTION: 0x4ca940
extern "C" int HOST_CALL host_net_release_play_interface(host_net_pair* net)
{
    HapinetTrace("HAPINET_releasedplayinterface\n");
    int result = 0;
    if (net->dp3 != 0) {
        result = net->dp3->Release();
        net->dp3 = 0;
    }
    if (net->dp != 0) {
        net->dp->Release();
        net->dp = 0;
    }
    return result;
}

// FUNCTION: 0x4ca990
extern "C" int HOST_CALL host_net_get_player_address(host_net_pair* net, unsigned long player, void* data, unsigned long* size)
{
    HapinetTrace("HAPINET_getplayeraddress\n");
    if (net->dp3 != 0)
        return net->dp3->GetPlayerAddress(player, data, size);
    return 0x88770140;
}
