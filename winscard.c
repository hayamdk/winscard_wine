#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <wchar.h>

#include <wintypes.h>
#include <pcsclite.h>

#include "mymalloc.h"

//#define dbg(...) printf(__VA_ARGS__)
#define dbg(...) /* do nothing */

#ifdef __x86_64__
#define WINAPI /* nothings */
#else
#define WINAPI __attribute__((stdcall))
#endif

#define LOAD_SYM(sym) if( !(_ ## sym = dlsym(hdl, #sym)) ) { dbg("Load error: " #sym "\n" ); return;}

typedef wchar_t WCHAR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;

static void* hdl;

static LONG (*_SCardEstablishContext)(DWORD, LPVOID, LPVOID, LPSCARDCONTEXT) = NULL;
static LONG (*_SCardListReaders)(SCARDCONTEXT, LPCSTR, LPSTR, LPDWORD) = NULL;
static LONG (*_SCardConnect)(SCARDCONTEXT, LPCSTR, DWORD, DWORD, LPSCARDHANDLE, LPDWORD) = NULL;
static LONG (*_SCardTransmit)(SCARDHANDLE, const SCARD_IO_REQUEST*, LPCBYTE, DWORD, SCARD_IO_REQUEST*, LPBYTE pbRecvBuffer, LPDWORD) = NULL;
static LONG (*_SCardDisconnect)(SCARDHANDLE, DWORD) = NULL;
static LONG (*_SCardReleaseContext)(SCARDCONTEXT) = NULL;
static LONG (*_SCardFreeMemory)(SCARDCONTEXT, LPCVOID) = NULL;
static LONG (*_SCardGetStatusChange)(SCARDCONTEXT, DWORD, LPSCARD_READERSTATE, DWORD) = NULL;
static LONG (*_SCardIsValidContext)(SCARDCONTEXT) = NULL;
static LONG (*_SCardStatus)(SCARDHANDLE, LPSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD) = NULL;

static SCARD_IO_REQUEST *_g_rgSCardT0Pci = NULL;
static SCARD_IO_REQUEST *_g_rgSCardT1Pci = NULL;
static SCARD_IO_REQUEST *_g_rgSCardRawPci = NULL;
const SCARD_IO_REQUEST g_rgSCardT0Pci = { SCARD_PROTOCOL_T0, sizeof(SCARD_IO_REQUEST) };
const SCARD_IO_REQUEST g_rgSCardT1Pci = { SCARD_PROTOCOL_T1, sizeof(SCARD_IO_REQUEST) };
const SCARD_IO_REQUEST g_rgSCardRawPci = { SCARD_PROTOCOL_RAW, sizeof(SCARD_IO_REQUEST) };

static void __attribute__ ((constructor)) init(void)
{
    dbg("construct!\n");
    hdl = dlopen("libpcsclite.so", RTLD_NOW | RTLD_LOCAL);
    if(hdl == NULL) {
        dbg("dlopen error: %s\n", dlerror());
        return;
    }
    LOAD_SYM(g_rgSCardT0Pci);
    LOAD_SYM(g_rgSCardT1Pci);
    LOAD_SYM(g_rgSCardRawPci);
    LOAD_SYM(SCardEstablishContext);
    LOAD_SYM(SCardListReaders);
    LOAD_SYM(SCardConnect);
    LOAD_SYM(SCardTransmit);
    LOAD_SYM(SCardDisconnect);
    LOAD_SYM(SCardReleaseContext);
    LOAD_SYM(SCardFreeMemory);
    LOAD_SYM(SCardGetStatusChange);
    LOAD_SYM(SCardIsValidContext);
    LOAD_SYM(SCardStatus);
}

static void __attribute__ ((destructor)) finish(void)
{
    dlclose(hdl);
    my_free_all();
    dbg("destruct!\n");
}

static const SCARD_IO_REQUEST* convert_pci(const SCARD_IO_REQUEST* pci)
{
    if(pci == &g_rgSCardT0Pci) {
        return _g_rgSCardT0Pci;
    } else if(pci == &g_rgSCardT1Pci) {
        return _g_rgSCardT1Pci;
    } else if(pci == &g_rgSCardRawPci) {
        return _g_rgSCardRawPci;
    }
    return pci;
}

/* ASCII文字以外は正しく変換できない簡易版（カードリーダー名にそれは含まれないと仮定） */
static void char2wchar(WCHAR *dst, const char *src, size_t size)
{
    int i;
    char *p;
    for(i=0; i<size-1 && src[i] != '\0'; i++) {
        p = (char*)(&dst[i]);
        p[0] = src[i];
        p[1] = 0;
    }
    dst[i] = L'\0';
}

/* ASCII文字以外は正しく変換できない簡易版（カードリーダー名にそれは含まれないと仮定） */
static void wchar2char(char *dst, const WCHAR *src, size_t size)
{
    int i;
    const char *p;
    for(i=0; i<size-1 && src[i] != L'\0'; i++) {
        p = (char*)(&src[i]);
        dst[i] = p[0];
    }
    dst[i] = '\0';
}

LONG WINAPI SCardEstablishContext(DWORD dwScope, LPVOID pvReserved1, LPVOID pvReserved2, LPSCARDCONTEXT phContext)
{
    dbg("SCardEstablishContext(%p)\n", _SCardEstablishContext);
    return _SCardEstablishContext(dwScope, pvReserved1, pvReserved2, phContext);
}

LONG WINAPI SCardListReadersA(SCARDCONTEXT hContext, LPCSTR mszGroups, LPSTR mszReaders, LPDWORD pcchReaders)
{
    dbg("SCardListReadersA(%p)\n", _SCardListReaders);
    return _SCardListReaders(hContext, mszGroups, mszReaders, pcchReaders);
}

LONG WINAPI SCardListReadersW(SCARDCONTEXT hContext, LPCWSTR mszGroups, LPWSTR mszReaders, LPDWORD pcchReaders)
{
    LPCSTR mszGroupsA = NULL; /* not used */
    char mszReadersA[256];
    DWORD len = 256;
    LONG ret;
    ret = _SCardListReaders(hContext, mszGroupsA, mszReadersA, &len);
    if(ret == SCARD_S_SUCCESS) {
        dbg("SCardListReadersW(%p): reader = %s\n", _SCardListReaders, mszReadersA);
        if(mszReaders) {
            if(*pcchReaders == SCARD_AUTOALLOCATE) {
                *(LPWSTR*)mszReaders = my_malloc(sizeof(WCHAR)*len);
                char2wchar(*(LPWSTR*)mszReaders, mszReadersA, len);
            } else if(*pcchReaders < len) {
                char2wchar(mszReaders, mszReadersA, *pcchReaders);
            } else {
                char2wchar(mszReaders, mszReadersA, len);
            }
        }
        *pcchReaders = len;
    }
    dbg("SCardListReadersW(%p): return = %lx\n", _SCardListReaders, ret);
    return ret;
}

LONG WINAPI SCardConnectA(SCARDCONTEXT hContext, LPCSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
    LONG ret = _SCardConnect(hContext, szReader, dwShareMode, dwPreferredProtocols, phCard, pdwActiveProtocol);
    dbg("SCardConnectA(%p) *phCard = %lx return = %lx\n", _SCardConnect, *phCard, ret);
    return ret;
}

LONG WINAPI SCardConnectW(SCARDCONTEXT hContext, LPCWSTR szReader, DWORD dwShareMode, DWORD dwPreferredProtocols, LPSCARDHANDLE phCard, LPDWORD pdwActiveProtocol)
{
    char szReaderA[256];
    wchar2char(szReaderA, szReader, 256);
    LONG ret = _SCardConnect(hContext, szReaderA, dwShareMode, dwPreferredProtocols, phCard, pdwActiveProtocol);
    dbg("SCardConnectW(%p) *phCard = %lx return = %lx\n", _SCardConnect, *phCard, ret);
    return ret;
}

LONG WINAPI SCardTransmit(SCARDHANDLE hCard, LPCSCARD_IO_REQUEST pioSendPci, LPCBYTE pbSendBuffer, DWORD cbSendLength, LPSCARD_IO_REQUEST pioRecvPci, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength)
{
    LPCSCARD_IO_REQUEST _pioSendPci = convert_pci(pioSendPci);
    LONG ret = _SCardTransmit(hCard, _pioSendPci, pbSendBuffer, cbSendLength, pioRecvPci, pbRecvBuffer, pcbRecvLength);
    dbg("SCardTransmit(%p) ioSendPci=%p(%ld,%ld) sendsize = %ld recvsize = %ld return = %lx\n", _SCardTransmit, _pioSendPci, _pioSendPci->dwProtocol, _pioSendPci->cbPciLength, cbSendLength, *pcbRecvLength, ret);
    return ret;
}

LONG WINAPI SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition)
{
    dbg("SCardDisconnect(%p)\n", _SCardDisconnect);
    return _SCardDisconnect(hCard, dwDisposition);
}

LONG WINAPI SCardReleaseContext(SCARDCONTEXT hContext)
{
    dbg("SCardReleaseContext(%p)\n", _SCardReleaseContext);
    return _SCardReleaseContext(hContext);
}

LONG WINAPI SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem)
{
    dbg("SCardFreeMemory(%p)\n", _SCardFreeMemory);
    if( my_free(pvMem) == 0 ) {
        return SCARD_S_SUCCESS;
    }
    return _SCardFreeMemory(hContext, pvMem);
}

LONG WINAPI SCardGetStatusChangeA(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATE rgReaderStates, DWORD cReaders)
{
    dbg("SCardGetStatusChangeA(%p)\n", _SCardGetStatusChange);
    return _SCardGetStatusChange(hContext, dwTimeout, rgReaderStates, cReaders);
}

LONG WINAPI SCardGetStatusChangeW(SCARDCONTEXT hContext, DWORD dwTimeout, LPSCARD_READERSTATE rgReaderStates, DWORD cReaders)
{
    SCARD_READERSTATE rgReaderStatesA;
    char readerA[256];
    const char *t;
    memcpy( &rgReaderStatesA, rgReaderStates, sizeof(SCARD_READERSTATE) );
    if(rgReaderStates->szReader == NULL) {
        rgReaderStatesA.szReader = NULL;
    } else {
        rgReaderStatesA.szReader = readerA;
        wchar2char(readerA, (WCHAR*)rgReaderStates->szReader, 256);
    }
    LONG ret = _SCardGetStatusChange(hContext, dwTimeout, &rgReaderStatesA, cReaders);
    dbg("SCardGetStatusChangeW(%p): return = %ld\n", _SCardGetStatusChange, ret);
    if(ret == SCARD_S_SUCCESS) {
        t = rgReaderStates->szReader;
        memcpy( rgReaderStates, &rgReaderStatesA, sizeof(SCARD_READERSTATE) );
        rgReaderStates->szReader = t;
    }
    return ret;
}

LONG WINAPI SCardIsValidContext(SCARDCONTEXT hContext)
{
    LONG ret = _SCardIsValidContext(hContext);
    dbg("SCardIsValidContext(%p): return = %ld\n", _SCardIsValidContext, ret);
    return ret;
}

LONG WINAPI SCardStatusA(SCARDHANDLE hCard, LPSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
    dbg("SCardStatusA(%p)\n", _SCardStatus);
    return _SCardStatus(hCard, szReaderName, pcchReaderLen, pdwState, pdwProtocol, pbAtr, pcbAtrLen);
}

LONG WINAPI SCardStatusW(SCARDHANDLE hCard, LPWSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
    dbg("SCardStatusW(%p)\n", _SCardStatus);
    char szReaderNameA[256];
    DWORD len = 256;
    LONG ret = _SCardStatus(hCard, szReaderNameA, &len, pdwState, pdwProtocol, pbAtr, pcbAtrLen);
    if(ret == SCARD_S_SUCCESS) {
        dbg("SCardStatusW(%p): reader = %s\n", _SCardStatus, szReaderNameA);
        if(szReaderName) {
            if(*pcchReaderLen == SCARD_AUTOALLOCATE) {
                *(LPWSTR*)szReaderName = my_malloc(sizeof(WCHAR)*len);
                char2wchar(*(LPWSTR*)szReaderName, szReaderNameA, len);
            } else if(*pcchReaderLen < len) {
                char2wchar(szReaderName, szReaderNameA, *pcchReaderLen);
            } else {
                char2wchar(szReaderName, szReaderNameA, len);
            }
        }
        *pcchReaderLen = len;
    }
    return ret;
}
