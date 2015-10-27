#include <Windows.h>

#include "config.h"
#include "log.h"

#include <string>

#include <dsound.h>

#include <comdef.h>

#include <initguid.h>
#include <Mmdeviceapi.h>
#include <Audiopolicy.h>
#include <Audioclient.h>

#pragma comment (lib, "kernel32.lib")

HMODULE backend_dll = nullptr;

//
// Create and reuse a single DirectSound Instance to minimize issues with Bink
//
IDirectSound* g_pDS       = nullptr;
bool          new_session = false;

void
LoadBackend (void)
{
  if (backend_dll != nullptr)
    return;

  dll_log.LogEx (true, L"Loading Default dsound.dll: ");

  wchar_t wszBackendDLL [MAX_PATH] = { L'\0' };
#ifdef _WIN64
  GetSystemDirectory (wszBackendDLL, MAX_PATH);
#else
  BOOL bWOW64;
  IsWow64Process (GetCurrentProcess (), &bWOW64);

  if (bWOW64)
    GetSystemWow64Directory (wszBackendDLL, MAX_PATH);
  else
    GetSystemDirectory (wszBackendDLL, MAX_PATH);
#endif

  lstrcatW (wszBackendDLL, L"\\dsound.dll");

  dll_log.LogEx (false, L"%s... ", wszBackendDLL);

  backend_dll = LoadLibrary (wszBackendDLL);

  if (backend_dll != nullptr)
     dll_log.LogEx (false, L"done!\n");
  else
    dll_log.LogEx (false, L"failed!\n");
}

#define __PTR_SIZE   sizeof LPCVOID
#define __PAGE_PRIVS PAGE_EXECUTE_READWRITE

#define DSOUND_VIRTUAL_OVERRIDE(_Base,_Index,_Name,_Override,_Original,_Type){\
  void** vftable = *(void***)*_Base;                                          \
                                                                              \
  if (vftable [_Index] != _Override) {                                        \
    DWORD dwProtect;                                                          \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, __PAGE_PRIVS, &dwProtect); \
                                                                              \
  /*dll_log.Log (L" Old VFTable entry for %s: %08Xh  (Memory Policy: %s)",*/  \
               /*L##_Name, vftable [_Index],                              */  \
               /*TZF_DescribeVirtualProtectFlags (dwProtect));            */  \
                                                                              \
    if (_Original == NULL)                                                    \
      _Original = (##_Type)vftable [_Index];                                  \
                                                                              \
    /*dll_log.Log (L"  + %s: %08Xh", L#_Original, _Original);*/               \
                                                                              \
    vftable [_Index] = _Override;                                             \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, dwProtect, &dwProtect);    \
                                                                              \
  /*dll_log.Log (L" New VFTable entry for %s: %08Xh  (Memory Policy: %s)\n",*/\
                /*L##_Name, vftable [_Index],                               */\
                /*TZF_DescribeVirtualProtectFlags (dwProtect));             */\
  }                                                                           \
}

#define DSOUND_STUB(_Return, _Name, _Proto, _Args)                        \
  _Return WINAPI                                                          \
  _Name##_Detour _Proto {                                                 \
    LoadBackend ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_t) _Proto;            \
    static passthrough_t _default_impl = nullptr;                         \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_t)GetProcAddress (backend_dll, szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
          dll_log.Log (                                                   \
            L"Unable to locate symbol  %s in dsound.dll",                 \
            L#_Name);                                                     \
        return E_NOTIMPL;                                                 \
      }                                                                   \
    }                                                                     \
                                                                          \
    dll_log.Log (L"[!] %s %s - "                                          \
               L"[Calling Thread: 0x%04x]",                               \
        L#_Name, L#_Proto, GetCurrentThreadId ());                        \
                                                                          \
    return _default_impl _Args;                                           \
}

const wchar_t*
TZF_DescribeHRESULT (HRESULT result)
{
  switch (result)
  {
    /* Generic (SUCCEEDED) */

  case S_OK:
    return L"S_OK";

  case S_FALSE:
    return L"S_FALSE";


   /* DirectSound */
  case DS_NO_VIRTUALIZATION:
    return L"DS_NO_VIRTUALIZATION";

  case DSERR_ALLOCATED:
    return L"DSERR_ALLOCATED";

  case DSERR_CONTROLUNAVAIL:
    return L"DSERR_CONTROLUNAVAIL";

  case DSERR_INVALIDPARAM:
    return L"DSERR_INVALIDPARAM";

  case DSERR_INVALIDCALL:
    return L"DSERR_INVALIDCALL";

  case DSERR_GENERIC:
    return L"DSERR_GENERIC";

  case DSERR_PRIOLEVELNEEDED:
    return L"DSERR_PRIOLEVELNEEDED";

  case DSERR_OUTOFMEMORY:
    return L"DSERR_OUTOFMEMORY";

  case DSERR_BADFORMAT:
    return L"DSERR_BADFORMAT";

  case DSERR_UNSUPPORTED:
    return L"DSERR_UNSUPPORTED";

  case DSERR_NODRIVER:
    return L"DSERR_NODRIVER";

  case DSERR_ALREADYINITIALIZED:
    return L"DSERR_ALREADYINITIALIZED";

  case DSERR_NOAGGREGATION:
    return L"DSERR_NOAGGREGATION";

  case DSERR_BUFFERLOST:
    return L"DSERR_BUFFERLOST";

  case DSERR_OTHERAPPHASPRIO:
    return L"DSERR_OTHERAPPHASPRIO";

  case DSERR_UNINITIALIZED:
    return L"DSERR_UNINITIALIZED";

  case DSERR_NOINTERFACE:
    return L"DSERR_NOINTERFACE";

  case DSERR_ACCESSDENIED:
    return L"DSERR_ACCESSDENIED";

  case DSERR_BUFFERTOOSMALL:
    return L"DSERR_BUFFERTOOSMALL";

  case DSERR_DS8_REQUIRED:
    return L"DSERR_DS8_REQUIRED";

  case DSERR_SENDLOOP:
    return L"DSERR_SENDLOOP";

  case DSERR_BADSENDBUFFERGUID:
    return L"DSERR_BADSENDBUFFERGUID";

  case DSERR_OBJECTNOTFOUND:
    return L"DSERR_OBJECTNOTFOUND";

  case DSERR_FXUNAVAILABLE:
    return L"DSERR_FXUNAVAILABLE";


#if 0
    /* Generic (FAILED) */

  case E_FAIL:
    return L"E_FAIL";

  case E_INVALIDARG:
    return L"E_INVALIDARG";

  case E_OUTOFMEMORY:
    return L"E_OUTOFMEMORY";

  case E_NOTIMPL:
    return L"E_NOTIMPL";
#endif


  default:
    dll_log.Log (L" *** Encountered unknown HRESULT: (0x%08X)",
      (unsigned long)result);
    return L"UNKNOWN";
  }
}

#define DSOUND_CALL(_Ret, _Call) {                                   \
  dll_log.LogEx (true, L"  Calling original function: ");            \
  (_Ret) = (_Call);                                                  \
  dll_log.LogEx (false, L"(ret=%s)\n\n", TZF_DescribeHRESULT (_Ret));\
}

typedef HRESULT (WINAPI *DSound_GetSpeakerConfig_t)
          (IDirectSound *This, 
     _Out_ LPDWORD       pdwSpeakerConfig);

DSound_GetSpeakerConfig_t DSound_GetSpeakerConfig_Original = nullptr;

HRESULT
WINAPI DSound_GetSpeakerConfig (IDirectSound *This, 
                          _Out_ LPDWORD       pdwSpeakerConfig)
{
  dll_log.Log (L"[!] %s (%08Xh, %08Xh) - "
    L"[Calling Thread: 0x%04x]",
    L"IDirectSound::GetSpeakerConfig", This, pdwSpeakerConfig,
    GetCurrentThreadId ()
  );

  HRESULT ret;
  DSOUND_CALL(ret,DSound_GetSpeakerConfig_Original (This, pdwSpeakerConfig));

  if (*pdwSpeakerConfig == DSSPEAKER_7POINT1_SURROUND ||
      *pdwSpeakerConfig == DSSPEAKER_7POINT1 ) {
    *pdwSpeakerConfig = DSSPEAKER_5POINT1_SURROUND;
    dll_log.Log ( L"IDirectSound::GetSpeakerConfig (...) : "
                  L"Reporting 7.1 system as 5.1\n" );
  }

  return ret;
}

typedef HRESULT (WINAPI *DSound_SetSpeakerConfig_t)
          (IDirectSound *This, 
     _In_  DWORD         dwSpeakerConfig);

DSound_SetSpeakerConfig_t DSound_SetSpeakerConfig_Original = nullptr;

HRESULT
WINAPI DSound_SetSpeakerConfig (IDirectSound *This, 
                          _In_  DWORD         dwSpeakerConfig)
{
  dll_log.Log (L"[!] %s (%08Xh, %08Xh) - "
               L"[Calling Thread: 0x%04x]",
                 L"IDirectSound::SetSpeakerConfig",
                   This, dwSpeakerConfig,
                     GetCurrentThreadId ()
  );

  HRESULT ret;
  DSOUND_CALL(ret,DSound_SetSpeakerConfig_Original (This, dwSpeakerConfig));

  if (dwSpeakerConfig == DSSPEAKER_7POINT1_SURROUND ||
      dwSpeakerConfig == DSSPEAKER_7POINT1 ) {
    dwSpeakerConfig = DSSPEAKER_5POINT1_SURROUND;
    dll_log.Log ( L"IDirectSound::SetSpeakerConfig (...) : "
                  L"Changing 7.1 system to 5.1\n" );
  }

  return ret;
}

typedef HRESULT (WINAPI *DSound_GetSpeakerConfig8_t)
         (IDirectSound8 *This, 
     _Out_ LPDWORD       pdwSpeakerConfig);

DSound_GetSpeakerConfig8_t DSound_GetSpeakerConfig8_Original = nullptr;

HRESULT
WINAPI DSound_GetSpeakerConfig8 (IDirectSound8 *This, 
                           _Out_ LPDWORD        pdwSpeakerConfig)
{
  dll_log.Log (L"[!] %s (%08Xh, %08Xh) - "
               L"[Calling Thread: 0x%04x]",
                 L"IDirectSound8::GetSpeakerConfig",
                   This, pdwSpeakerConfig,
                     GetCurrentThreadId ()
  );

  HRESULT ret;
  DSOUND_CALL(ret,DSound_GetSpeakerConfig8_Original (This, pdwSpeakerConfig));

  if (*pdwSpeakerConfig == DSSPEAKER_7POINT1_SURROUND ||
      *pdwSpeakerConfig == DSSPEAKER_7POINT1 ) {
    *pdwSpeakerConfig = DSSPEAKER_5POINT1_SURROUND;
    dll_log.Log ( L"IDirectSound8::GetSpeakerConfig (...) : "
                  L"Reporting 7.1 system as 5.1\n" );
  }

  return ret;
}

_Check_return_
HRESULT
WINAPI
DirectSoundCreate_Detour (_In_opt_   LPCGUID        pcGuidDevice,
                          _Outptr_   LPDIRECTSOUND *ppDS,
                          _Pre_null_ LPUNKNOWN      pUnkOuter)
{
  LoadBackend ();

  dll_log.Log ( L"[!] %s (%08Xh, %08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                  L"DirectSoundCreate",
                    pcGuidDevice, ppDS, pUnkOuter,
                      GetCurrentThreadId ()
  );

  if (g_pDS) {
    dll_log.Log ( L" @ Short-Circuiting and returning previous IDirectSound "
                  L"interface." );

    *ppDS = g_pDS;
    (*ppDS)->AddRef ();

    return S_OK;
  }

  typedef HRESULT (WINAPI *DirectSoundCreate_t)
         (_In_opt_ LPCGUID        pcGuidDevice,
          _Outptr_ LPDIRECTSOUND *ppDS,
        _Pre_null_ LPUNKNOWN      pUnkOuter);

  static DirectSoundCreate_t dsound_create =
    (DirectSoundCreate_t)GetProcAddress (backend_dll, "DirectSoundCreate");

  HRESULT ret = dsound_create (pcGuidDevice, ppDS, pUnkOuter);

  if (SUCCEEDED (ret)) {
    DSOUND_VIRTUAL_OVERRIDE ( ppDS, 8, "IDirectSound::GetSpeakerConfig",
                              DSound_GetSpeakerConfig,
                              DSound_GetSpeakerConfig_Original,
                              DSound_GetSpeakerConfig_t );

    DSOUND_VIRTUAL_OVERRIDE ( ppDS, 9, "IDirectSound::SetSpeakerConfig",
                              DSound_SetSpeakerConfig,
                              DSound_SetSpeakerConfig_Original,
                              DSound_SetSpeakerConfig_t );

    DWORD dwSpeaker;

    if (SUCCEEDED ((*ppDS)->GetSpeakerConfig (&dwSpeaker)))
      (*ppDS)->SetSpeakerConfig (dwSpeaker);
  }
  else {
    _com_error ce (ret);
    dll_log.Log ( L" > FAILURE %s - (%s)", TZF_DescribeHRESULT (ret),
                                            ce.ErrorMessage () );
  }

  return ret;
}

DSOUND_STUB(HRESULT, DirectSoundEnumerateA, (_In_ LPDSENUMCALLBACKA pDSEnumCallback, _In_opt_ LPVOID pContext),
                                            (                       pDSEnumCallback,                 pContext))

DSOUND_STUB(HRESULT, DirectSoundEnumerateW, (_In_ LPDSENUMCALLBACKW pDSEnumCallback, _In_opt_ LPVOID pContext),
                                            (                       pDSEnumCallback,                 pContext))

DSOUND_STUB(HRESULT, DirectSoundCaptureCreate, (_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUNDCAPTURE *ppDSC, _Pre_null_ LPUNKNOWN pUnkOuter),
                                               (                 pcGuidDevice,                                ppDSC,                      pUnkOuter))

DSOUND_STUB(HRESULT, DirectSoundCaptureEnumerateA, (_In_ LPDSENUMCALLBACKA pDSEnumCallback, _In_opt_ LPVOID pContext),
                                                   (                       pDSEnumCallback,                 pContext))

DSOUND_STUB(HRESULT, DirectSoundCaptureEnumerateW, (_In_ LPDSENUMCALLBACKW pDSEnumCallback, _In_opt_ LPVOID pContext),
                                                   (                       pDSEnumCallback,                 pContext))

HRESULT
WINAPI
DirectSoundCreate8_Detour ( _In_opt_ LPCGUID         pcGuidDevice,
                            _Outptr_ LPDIRECTSOUND8 *ppDS8,
                          _Pre_null_ LPUNKNOWN       pUnkOuter )
{
  LoadBackend ();

  dll_log.Log ( L"[!] %s (%08Xh, %08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                  L"DirectSoundCreate8",
                    pcGuidDevice, ppDS8, pUnkOuter,
                      GetCurrentThreadId ()
   );

  typedef HRESULT (WINAPI *DirectSoundCreate8_t)
         (_In_opt_ LPCGUID         pcGuidDevice,
          _Outptr_ LPDIRECTSOUND8 *ppDS,
        _Pre_null_ LPUNKNOWN       pUnkOuter);

  static DirectSoundCreate8_t dsound_create8 =
    (DirectSoundCreate8_t)GetProcAddress (backend_dll, "DirectSoundCreate8");

  HRESULT ret = dsound_create8 (pcGuidDevice, ppDS8, pUnkOuter);

  if (SUCCEEDED (ret)) {
    DSOUND_VIRTUAL_OVERRIDE ( ppDS8, 8, "IDirectSound8::GetSpeakerConfig",
                              DSound_GetSpeakerConfig8,
                              DSound_GetSpeakerConfig8_Original,
                              DSound_GetSpeakerConfig8_t );

    DWORD dwSpeaker;

    if (SUCCEEDED ((*ppDS8)->GetSpeakerConfig (&dwSpeaker)))
      (*ppDS8)->SetSpeakerConfig (dwSpeaker);
  } else {
    _com_error ce (ret);
    dll_log.Log ( L" > FAILURE %s - (%s)", TZF_DescribeHRESULT (ret),
                                            ce.ErrorMessage () );
  }

  return ret;
}

DSOUND_STUB(HRESULT, DirectSoundCaptureCreate8, (_In_opt_ LPCGUID pcGuidDevice, _Outptr_ LPDIRECTSOUNDCAPTURE8 *ppDSC, _Pre_null_ LPUNKNOWN pUnkOuter),
                                                (                 pcGuidDevice,                                 ppDSC,                      pUnkOuter))


HRESULT
WINAPI
DirectSoundFullDuplexCreate_Detour
(
  _In_opt_   LPCGUID                      pcGuidCaptureDevice,
  _In_opt_   LPCGUID                      pcGuidRenderDevice,
  _In_       LPCDSCBUFFERDESC             pcDSCBufferDesc,
  _In_       LPCDSBUFFERDESC              pcDSBufferDesc,
             HWND                         hWnd,
             DWORD                        dwLevel,
  _Outptr_   LPDIRECTSOUNDFULLDUPLEX     *ppDSFD,
  _Outptr_   LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
  _Outptr_   LPDIRECTSOUNDBUFFER8        *ppDSBuffer8,
  _Pre_null_ LPUNKNOWN                    pUnkOuter
  )
{
  LoadBackend ();

  dll_log.Log ( L"[!] %s (%08Xh, %08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                  L"DirectSoundFullDuplexCreate",
                    NULL, NULL, pUnkOuter,
                      GetCurrentThreadId ()
  );

  return S_OK;
}

DSOUND_STUB(HRESULT, GetDeviceID, (_In_opt_ LPCGUID pGuidSrc, _Out_ LPGUID pGuidDest),
                                  (                 pGuidSrc,              pGuidDest))

#pragma comment (lib, "MinHook/lib/libMinHook.x86.lib")
#include "MinHook/include/MinHook.h"


typedef DECLSPEC_IMPORT HMODULE (WINAPI *LoadLibraryA_t)(LPCSTR  lpFileName);
typedef DECLSPEC_IMPORT HMODULE (WINAPI *LoadLibraryW_t)(LPCWSTR lpFileName);

LoadLibraryA_t LoadLibraryA_Original = nullptr;
LoadLibraryW_t LoadLibraryW_Original = nullptr;

HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hMod = LoadLibraryA_Original (lpFileName);

  if (strstr (lpFileName, "steam_api") ||
      strstr (lpFileName, "SteamworksNative")) {
    //TZF::SteamAPI::Init (false);
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hMod = LoadLibraryW_Original (lpFileName);

  if (wcsstr (lpFileName, L"steam_api") ||
      wcsstr (lpFileName, L"SteamworksNative")) {
    //TZF::SteamAPI::Init (false);
  }

  return hMod;
}


MH_STATUS
WINAPI
TZF_CreateFuncHook ( LPCWSTR pwszFuncName,
  LPVOID  pTarget,
  LPVOID  pDetour,
  LPVOID *ppOriginal )
{
  MH_STATUS status =
    MH_CreateHook ( pTarget,
      pDetour,
      ppOriginal );

  if (status != MH_OK) {
    dll_log.Log ( L" [ MinHook ] Failed to Install Hook for '%s' "
      L"[Address: %04Xh]!  (Status: \"%hs\")",
      pwszFuncName,
      pTarget,
      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
TZF_CreateDLLHook ( LPCWSTR pwszModule, LPCSTR  pszProcName,
                    LPVOID  pDetour,    LPVOID *ppOriginal )
{
#if 1
  HMODULE hMod = GetModuleHandle (pwszModule);

  if (hMod == NULL) {
    if (LoadLibraryW_Original != nullptr) {
      hMod = LoadLibraryW_Original (pwszModule);
    } else {
      hMod = LoadLibraryW (pwszModule);
    }
  }

  LPVOID pFuncAddr =
    GetProcAddress (hMod, pszProcName);

  MH_STATUS status =
    MH_CreateHook ( pFuncAddr,
      pDetour,
      ppOriginal );
#else
  MH_STATUS status =
    MH_CreateHookApi ( pwszModule,
      pszProcName,
      pDetour,
      ppOriginal );
#endif

  if (status != MH_OK) {
    dll_log.Log ( L" [ MinHook ] Failed to Install Hook for: '%hs' in '%s'! "
                  L"(Status: \"%hs\")",
                    pszProcName,
                      pwszModule,
                        MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
TZF_EnableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_EnableHook (pTarget);

  if (status != MH_OK)
  {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log( L" [ MinHook ] Failed to Enable Hook with Address: %04Xh!"
                   L" (Status: \"%hs\")",
                     pTarget,
                       MH_StatusToString (status) );
    } else {
      dll_log.Log ( L" [ MinHook ] Failed to Enable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
WINAPI
TZF_DisableHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_DisableHook (pTarget);

  if (status != MH_OK)
  {
    if (pTarget != MH_ALL_HOOKS) {
      dll_log.Log ( L" [ MinHook ] Failed to Disable Hook with Address: %04Xh!"
                    L" (Status: \"%hs\")",
                      pTarget,
                        MH_StatusToString (status) );
    } else {
      dll_log.Log ( L" [ MinHook ] Failed to Disable All Hooks! "
                    L"(Status: \"%hs\")",
                      MH_StatusToString (status) );
    }
  }

  return status;
}

MH_STATUS
WINAPI
TZF_RemoveHook (LPVOID pTarget)
{
  MH_STATUS status =
    MH_RemoveHook (pTarget);

  if (status != MH_OK)
  {
    dll_log.Log ( L" [ MinHook ] Failed to Remove Hook with Address: %04Xh! "
                  L"(Status: \"%hs\")",
                    pTarget,
                      MH_StatusToString (status) );
  }

  return status;
}

MH_STATUS
WINAPI
TZF_Init_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Initialize ()) != MH_OK)
  {
    dll_log.Log ( L" [ MinHook ] Failed to Initialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
  }

#if 0
  //
  // Hook LoadLibrary so that we can watch for things like steam_api*.dll
  //
  TZF_CreateDLLHook ( L"kernel32.dll",
                       "LoadLibraryA",
                      LoadLibraryA_Detour,
           (LPVOID *)&LoadLibraryA_Original );

  TZF_CreateDLLHook ( L"kernel32.dll",
                       "LoadLibraryW",
                      LoadLibraryW_Detour,
           (LPVOID *)&LoadLibraryW_Original );
#endif

  //TZF_EnableHook (MH_ALL_HOOKS);

  return status;
}

MH_STATUS
WINAPI
TZF_UnInit_MinHook (void)
{
  MH_STATUS status;

  if ((status = MH_Uninitialize ()) != MH_OK) {
    dll_log.Log ( L" [ MinHook ] Failed to Uninitialize MinHook Library! "
                  L"(Status: \"%hs\")",
                    MH_StatusToString (status) );
  }

  return status;
}


typedef HRESULT (STDAPICALLTYPE *CoCreateInstance_t)(
                                               _In_     REFCLSID  rclsid,
                                               _In_opt_ LPUNKNOWN pUnkOuter,
                                               _In_     DWORD     dwClsContext,
                                               _In_     REFIID    riid,
_COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies)))
                                                         LPVOID FAR*
                                                                  ppv);

CoCreateInstance_t CoCreateInstance_Original = nullptr;

IMMDevice* g_pAudioDev;


typedef HRESULT (STDMETHODCALLTYPE *IAudioClient_GetMixFormat_t)
               (IAudioClient       *This,
         _Out_  WAVEFORMATEX      **ppDeviceFormat);

IAudioClient_GetMixFormat_t IAudioClient_GetMixFormat_Original = nullptr;

#include <comdef.h>
#include <audiomediatype.h>

HRESULT
STDMETHODCALLTYPE
IAudioClient_GetMixFormat_Detour (IAudioClient       *This,
                           _Out_  WAVEFORMATEX      **ppDeviceFormat)
{
  dll_log.Log (L" [!] IAudioClient::GetMixFormat (%08Xh)", This);

  if (new_session) {
    dll_log.Log (L" >> Overriding  -  "
                 L"Using Device Format rather than Mix Format <<");

    WAVEFORMATEX*   pMixFormat;
    IPropertyStore* pStore;

    if (SUCCEEDED (g_pAudioDev->OpenPropertyStore (STGM_READ, &pStore)))
    {
      PROPVARIANT property;

      if (SUCCEEDED (pStore->GetValue (
                        PKEY_AudioEngine_DeviceFormat, &property
                       )
                    )
         )
      {
        pMixFormat = (PWAVEFORMATEX)property.blob.pBlobData;
        if (pMixFormat->cbSize > sizeof (WAVEFORMATEX)) {
          if (pMixFormat->cbSize = 22) {
            std::wstring format_name;

            if (((PWAVEFORMATEXTENSIBLE)pMixFormat)->SubFormat ==
                  KSDATAFORMAT_SUBTYPE_PCM)
              format_name = L"PCM";
            if (((PWAVEFORMATEXTENSIBLE)pMixFormat)->SubFormat ==
                  KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
              format_name = L"IEEE Floating-Point";
            else
              format_name = L"Unknown";

            dll_log.Log (L" Extensible WAVEFORMAT Found...");
            dll_log.Log (L"  %lu Channels, %lu Samples Per Sec, %lu Bits"
                         L" Per Sample (%lu Actual)",
              ((PWAVEFORMATEXTENSIBLE)pMixFormat)->Format.nChannels,
              ((PWAVEFORMATEXTENSIBLE)pMixFormat)->Format.nSamplesPerSec,
              ((PWAVEFORMATEXTENSIBLE)pMixFormat)->Format.wBitsPerSample,
              ((PWAVEFORMATEXTENSIBLE)pMixFormat)->Samples.wValidBitsPerSample);
            dll_log.Log (L"  Format: %s", format_name.c_str ());

            pMixFormat->wBitsPerSample =
              ((PWAVEFORMATEXTENSIBLE)pMixFormat)->Samples.wValidBitsPerSample;

            DWORD dwChannels =
              ((PWAVEFORMATEXTENSIBLE)pMixFormat)->dwChannelMask;

            dll_log.LogEx (true, L" Speaker Geometry:");

            if (dwChannels & SPEAKER_FRONT_LEFT)
              dll_log.LogEx (false, L" FrontLeft");
            if (dwChannels & SPEAKER_FRONT_RIGHT)
              dll_log.LogEx (false, L" FrontRight");
            if (dwChannels & SPEAKER_FRONT_CENTER)
              dll_log.LogEx (false, L" FrontCenter");
            if (dwChannels & SPEAKER_LOW_FREQUENCY)
              dll_log.LogEx (false, L" LowFrequencyEmitter");
            if (dwChannels & SPEAKER_BACK_LEFT)
              dll_log.LogEx (false, L" BackLeft");
            if (dwChannels & SPEAKER_BACK_RIGHT)
              dll_log.LogEx (false, L" BackRight");
            if (dwChannels & SPEAKER_FRONT_LEFT_OF_CENTER)
              dll_log.LogEx (false, L" FrontLeftOfCenter");
            if (dwChannels & SPEAKER_FRONT_RIGHT_OF_CENTER)
              dll_log.LogEx (false, L" FrontRightOfCenter");
            if (dwChannels & SPEAKER_SIDE_LEFT)
              dll_log.LogEx (false, L" SideLeft");
            if (dwChannels & SPEAKER_SIDE_RIGHT)
              dll_log.LogEx (false, L" SideRight");

            dll_log.LogEx (false, L"\n");

            // Dolby Atmos? Neat!
//#define SPEAKER_TOP_CENTER              0x800
//#define SPEAKER_TOP_FRONT_LEFT          0x1000
//#define SPEAKER_TOP_FRONT_CENTER        0x2000
//#define SPEAKER_TOP_FRONT_RIGHT         0x4000
//#define SPEAKER_TOP_BACK_LEFT           0x8000
//#define SPEAKER_TOP_BACK_CENTER         0x10000
//#define SPEAKER_TOP_BACK_RIGHT          0x20000
          }
        }

        if (pMixFormat->nChannels > config.audio.channels) {
          dll_log.Log ( L"  ** Downmixing from %lu channels to %lu",
                         pMixFormat->nChannels, config.audio.channels );
          pMixFormat->nChannels = config.audio.channels;
        }

        #define TARGET_SAMPLE_RATE config.audio.sample_hz

        if (pMixFormat->nSamplesPerSec != TARGET_SAMPLE_RATE) {
          dll_log.Log ( L"  ** Resampling Audiostream from %lu Hz to %lu Hz",
                          pMixFormat->nSamplesPerSec, TARGET_SAMPLE_RATE );
          pMixFormat->nSamplesPerSec = TARGET_SAMPLE_RATE;
        }

        *ppDeviceFormat = (PWAVEFORMATEX)CoTaskMemAlloc (pMixFormat->cbSize);
        memcpy (*ppDeviceFormat, pMixFormat, pMixFormat->cbSize);

        pStore->Release ();

        return S_OK;
      }

      pStore->Release ();
    }
  }

  return IAudioClient_GetMixFormat_Original (This, ppDeviceFormat);
}


typedef HRESULT (STDMETHODCALLTYPE *IAudioClient_Initialize_t)
               (IAudioClient       *This,
          _In_  AUDCLNT_SHAREMODE   ShareMode,
          _In_  DWORD               StreamFlags,
          _In_  REFERENCE_TIME      hnsBufferDuration,
          _In_  REFERENCE_TIME      hnsPeriodicity,
          _In_  const WAVEFORMATEX *pFormat,
      _In_opt_  LPCGUID             AudioSessionGuid);

IAudioClient_Initialize_t IAudioClient_Initialize_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
IAudioClient_Initialize_Detour (IAudioClient       *This,
                          _In_  AUDCLNT_SHAREMODE   ShareMode,
                          _In_  DWORD               StreamFlags,
                          _In_  REFERENCE_TIME      hnsBufferDuration,
                          _In_  REFERENCE_TIME      hnsPeriodicity,
                          _In_  const WAVEFORMATEX *pFormat,
                      _In_opt_  LPCGUID             AudioSessionGuid)
{
  dll_log.Log (L" [!] IAudioClient::Initialize (...)");

  dll_log.Log (
    L"  {Share Mode: %lu - Stream Flags: 0x%04X}", ShareMode, StreamFlags );
  dll_log.Log (
    L"  >> Channels: %lu, Samples Per Sec: %lu, Bits Per Sample: %hu\n",
    pFormat->nChannels, pFormat->nSamplesPerSec, pFormat->wBitsPerSample );

#if 0
  WAVEFORMATEX format;

  format.cbSize          = pFormat->cbSize;
  format.nAvgBytesPerSec = pFormat->nAvgBytesPerSec;
  format.nBlockAlign     = pFormat->nBlockAlign;
  format.nChannels       = pFormat->nChannels;
  format.nSamplesPerSec  = pFormat->nSamplesPerSec;
  format.wBitsPerSample  = pFormat->wBitsPerSample;
  format.wFormatTag      = pFormat->wFormatTag;

  WAVEFORMATEX *pClosestMatch =
    (PWAVEFORMATEX)CoTaskMemAlloc (sizeof (WAVEFORMATEXTENSIBLE));

  if (This->IsFormatSupported (AUDCLNT_SHAREMODE_SHARED, pFormat, &pClosestMatch) == S_OK) {
    CoTaskMemFree (pClosestMatch);
    pClosestMatch = &format;
  }
#endif

  WAVEFORMATEX *pClosestMatch = (WAVEFORMATEX *)pFormat;

  #define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM      0x80000000
  #define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000

  // These two flags were observed when using WinMM to play a sound that had a
  //   different format than the output device. Like the SRC_DEFAULT_QUALITY
  //     flag above, these are undocumented.
  #define AUDCLNT_STREAMFLAGS_UNKNOWN4000000 0x4000000
  #define AUDCLNT_STREAMFLAGS_UNKNOWN0100000 0x0100000

  // This is REQUIRED or we cannot get the shared stream to work this way!
  StreamFlags |= ( AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                   AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY );

  // Since these two flags are even LESS documented than the ones above (which
  //   are needed, make the use of these optional).
  if (! config.audio.compatibility) {
    StreamFlags |= ( AUDCLNT_STREAMFLAGS_UNKNOWN0100000 |
                     AUDCLNT_STREAMFLAGS_UNKNOWN4000000 );
  }

  HRESULT ret =
    IAudioClient_Initialize_Original (This,           AUDCLNT_SHAREMODE_SHARED,
                                      StreamFlags,    hnsBufferDuration,
                                      hnsPeriodicity, pClosestMatch,
                                      AudioSessionGuid);

  _com_error error (ret);

#if 0
  if (pClosestMatch != &format)
    CoTaskMemFree (pClosestMatch);
#endif

  dll_log.Log ( L"   Result: 0x%04X (%s)\n", ret - AUDCLNT_ERR (0x0000),
                  error.ErrorMessage () );

  new_session = false;
  //DSOUND_VIRTUAL_OVERRIDE ( ppAudioClient, 8, "IAudioClient::GetMixFormat",
    //IAudioClient_GetMixFormat_Original,
    //IAudioClient_GetMixFormat_Original,
    //IAudioClient_GetMixFormat_t );

  return ret;
}

typedef HRESULT (STDMETHODCALLTYPE *IMMDevice_Activate_t)
                     (IMMDevice    *This,
                _In_  REFIID        iid,
                _In_  DWORD         dwClsCtx,
            _In_opt_  PROPVARIANT  *pActivationParams,
               _Out_  void        **ppInterface);

IMMDevice_Activate_t IMMDevice_Activate_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
IMMDevice_Activate_Detour (IMMDevice    *This,
                     _In_  REFIID        iid,
                     _In_  DWORD         dwClsCtx,
                 _In_opt_  PROPVARIANT  *pActivationParams,
                    _Out_  void        **ppInterface)
{
  if (iid == __uuidof (IAudioSessionManager2)) {
    dll_log.Log ( L" >> IMMDevice Activating IAudioSessionManager2 Interface "
                  L" (new_session = true)" );
    new_session = true;
  }
  
  else if (iid == __uuidof (IAudioClient)) {
    dll_log.Log (L" >> IMMDevice Activating IAudioClient Interface");
  }
  
  else {
    std::wstring iid_str;
    wchar_t*     pwszIID;

    if (SUCCEEDED (StringFromIID (iid, (LPOLESTR *)&pwszIID)))
    {
           iid_str = pwszIID;
      CoTaskMemFree (pwszIID);
    }

    dll_log.Log (L"IMMDevice::Activate (%s, ...)\n", iid_str.c_str ());
  }

  HRESULT ret =
    IMMDevice_Activate_Original ( This,
                                    iid,
                                      dwClsCtx,
                                        pActivationParams,
                                          ppInterface );

  if (SUCCEEDED (ret)) {
    if (iid == __uuidof (IAudioClient)) {
      IAudioClient** ppAudioClient = (IAudioClient **)ppInterface;

      // vtbl (ppAudioClient)
      // --------------------
      // 3   Initialize
      // 4   GetBufferSize
      // 5   GetStreamLatency
      // 6   GetCurrentPadding
      // 7   IsFormatSupported
      // 8   GetMixFormat
      // 9   GetDevicePeriod
      // 10  Start
      // 11  Stop
      // 12  Reset
      // 13  SetEventHandle
      // 14  GetService

      DSOUND_VIRTUAL_OVERRIDE ( ppAudioClient, 3, "IAudioClient::Initialize",
                                IAudioClient_Initialize_Detour,
                                IAudioClient_Initialize_Original,
                                IAudioClient_Initialize_t );

      DSOUND_VIRTUAL_OVERRIDE ( ppAudioClient, 8, "IAudioClient::GetMixFormat",
                                IAudioClient_GetMixFormat_Detour,
                                IAudioClient_GetMixFormat_Original,
                                IAudioClient_GetMixFormat_t );

      g_pAudioDev = This;

      // Stop listening to device enumeration and so forth after the first
      //   audio session is created... this is behavior specific to this game.
      if (new_session) {
#if 0
    TZF_CreateDLLHook ( L"kernel32.dll", "SleepConditionVariableCS", 
                        SleepConditionVariableCS_Detour,
             (LPVOID *)&SleepConditionVariableCS_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "Sleep", 
                        Sleep_Detour,
             (LPVOID *)&Sleep_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "SuspendThread", 
                        SuspendThread_Detour,
             (LPVOID *)&SuspendThread_Original );
#endif

#if 0
        TZF_CreateDLLHook ( L"msvcr100.dll", "sprintf", 
                        sprintf_Detour,
             (LPVOID *)&sprintf_Original );

       TZF_CreateDLLHook ( L"msvcr100.dll", "sprintf_s", 
                        sprintf_s_Detour,
             (LPVOID *)&sprintf_s_Original );

       TZF_CreateDLLHook ( L"msvcr100.dll", "_snprintf", 
                        _snprintf_Detour,
             (LPVOID *)&_snprintf_Original );

       TZF_CreateDLLHook ( L"msvcr100.dll", "_snprintf_s", 
                        _snprintf_s_Detour,
             (LPVOID *)&_snprintf_s_Original );
#endif

//        TZF_EnableHook (MH_ALL_HOOKS);
      }
    }
  }

  return ret;
}

HRESULT
STDAPICALLTYPE
CoCreateInstance_Detour (_In_     REFCLSID    rclsid,
                         _In_opt_ LPUNKNOWN   pUnkOuter,
                         _In_     DWORD       dwClsContext,
                         _In_     REFIID      riid,
  _COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies)))
                                  LPVOID FAR* ppv)
{
  HRESULT hr =
    CoCreateInstance_Original (rclsid, pUnkOuter, dwClsContext, riid, ppv);

  if (SUCCEEDED (hr) && riid == __uuidof (IMMDeviceEnumerator)) {
    IMMDeviceEnumerator** ppEnum = (IMMDeviceEnumerator **)ppv;

    IMMDevice* pDev;
    HRESULT hr2 =
      (*ppEnum)->GetDefaultAudioEndpoint (eRender, eConsole, &pDev);

    // vtbl (&pDev)
    // ------------
    // 3 Activate

    if (SUCCEEDED (hr2)) {
      DSOUND_VIRTUAL_OVERRIDE ( (void **)&pDev, 3, "IMMDevice::Activate",
                                IMMDevice_Activate_Detour,
                                IMMDevice_Activate_Original,
                                IMMDevice_Activate_t );
      pDev->Release ();
    }

    // vtbl (ppEnum)
    // -------------
    // 3  EnumAudioEndpoints
    // 4  GetDefaultAudioEndpoint
    // 5  GetDevice
  }

  return hr;
}


//#define LOG_FILE_IO
#ifdef LOG_FILE_IO
typedef BOOL (WINAPI *ReadFile_t)(_In_        HANDLE       hFile,
                                  _Out_       LPVOID       lpBuffer,
                                  _In_        DWORD        nNumberOfBytesToRead,
                                  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
                                  _Inout_opt_ LPOVERLAPPED lpOverlapped);

ReadFile_t ReadFile_Original = nullptr;

#include <memory>

BOOL
GetFileNameFromHandle (HANDLE hFile, WCHAR *pszFileName, const unsigned int uiMaxLen)
{
  pszFileName[0]=0;

  std::unique_ptr <WCHAR> ptrcFni (new WCHAR[_MAX_PATH + sizeof(FILE_NAME_INFO)]);
  FILE_NAME_INFO *pFni = reinterpret_cast <FILE_NAME_INFO *>(ptrcFni.get ());

  BOOL b = GetFileInformationByHandleEx (hFile, 
    FileNameInfo,
    pFni,
    sizeof(FILE_NAME_INFO) + (_MAX_PATH * sizeof (WCHAR)) );
  if ( b )
  {
    wcsncpy_s(pszFileName, 
      min(uiMaxLen, (pFni->FileNameLength / sizeof(pFni->FileName[0])) + 1 ), 
      pFni->FileName, 
      _TRUNCATE);
  }
  return b;
}

BOOL
WINAPI
ReadFile_Detour (_In_        HANDLE       hFile,
                 _Out_       LPVOID       lpBuffer,
                 _In_        DWORD        nNumberOfBytesToRead,
                 _Out_opt_   LPDWORD      lpNumberOfBytesRead,
                 _Inout_opt_ LPOVERLAPPED lpOverlapped)
{
  wchar_t wszFileName [MAX_PATH] = { L'\0' };

  GetFileNameFromHandle (hFile, wszFileName, MAX_PATH);

  dll_log.Log (L"Reading: %s ...", wszFileName);

  return ReadFile_Original (hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

typedef HANDLE (WINAPI *CreateFileW_t)(_In_     LPCTSTR               lpFileName,
                                       _In_     DWORD                 dwDesiredAccess,
                                       _In_     DWORD                 dwShareMode,
                                       _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                       _In_     DWORD                 dwCreationDisposition,
                                       _In_     DWORD                 dwFlagsAndAttributes,
                                       _In_opt_ HANDLE                hTemplateFile);

CreateFileW_t CreateFileW_Original = nullptr;

HANDLE
WINAPI
CreateFileW_Detour (_In_     LPCWSTR               lpFileName,
                    _In_     DWORD                 dwDesiredAccess,
                    _In_     DWORD                 dwShareMode,
                    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                    _In_     DWORD                 dwCreationDisposition,
                    _In_     DWORD                 dwFlagsAndAttributes,
                    _In_opt_ HANDLE                hTemplateFile)
{
  dll_log.Log (L" [!] CreateFile (%s, ...)", lpFileName);

  if (! _wcsicmp (lpFileName, L"raw\\movie\\am_toz_op_001.bk2")) {
    dll_log.Log (L"Opening Intro Movie!\n");
  }

  return CreateFileW_Original (lpFileName, dwDesiredAccess, dwShareMode,
                               lpSecurityAttributes, dwCreationDisposition,
                               dwFlagsAndAttributes, hTemplateFile);
}

typedef HANDLE (WINAPI *CreateFileA_t)(_In_     LPCSTR                lpFileName,
                                       _In_     DWORD                 dwDesiredAccess,
                                       _In_     DWORD                 dwShareMode,
                                       _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                       _In_     DWORD                 dwCreationDisposition,
                                       _In_     DWORD                 dwFlagsAndAttributes,
                                       _In_opt_ HANDLE                hTemplateFile);

CreateFileA_t CreateFileA_Original = nullptr;

HANDLE
WINAPI
CreateFileA_Detour (_In_     LPCSTR                lpFileName,
                    _In_     DWORD                 dwDesiredAccess,
                    _In_     DWORD                 dwShareMode,
                    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                    _In_     DWORD                 dwCreationDisposition,
                    _In_     DWORD                 dwFlagsAndAttributes,
                    _In_opt_ HANDLE                hTemplateFile)
{
  dll_log.Log (L" [!] CreateFile (%hs, ...)", lpFileName);

  if (! stricmp (lpFileName, "raw\\movie\\am_toz_op_001.bk2")) {
    dll_log.Log (L"Opening Intro Movie!\n");
  }

  return CreateFileA_Original (lpFileName, dwDesiredAccess, dwShareMode,
                               lpSecurityAttributes, dwCreationDisposition,
                               dwFlagsAndAttributes, hTemplateFile);
}


typedef HFILE (WINAPI *OpenFile_t)(_In_    LPCSTR     lpFileName,
                                   _Inout_ LPOFSTRUCT lpReOpenBuff,
                                   _In_    UINT       uStyle);

OpenFile_t OpenFile_Original = nullptr;

HFILE
WINAPI
OpenFile_Detour (_In_    LPCSTR     lpFileName,
                 _Inout_ LPOFSTRUCT lpReOpenBuff,
                 _In_    UINT       uStyle)
{
  dll_log.Log (L" [!] OpenFile (%hs, ...)", lpFileName);

  if (! stricmp (lpFileName, "raw\\movie\\am_toz_op_001.bk2")) {
    dll_log.Log (L"Opening Intro Movie!\n");
  }

  return OpenFile_Original (lpFileName, lpReOpenBuff, uStyle);
}
#endif


typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
} D3DMATRIX;

typedef D3DMATRIX* (WINAPI *D3DXMatrixMultiply_t)(_Inout_    D3DMATRIX *pOut,
                                                  _In_ const D3DMATRIX *pM1,
                                                  _In_ const D3DMATRIX *pM2);

D3DXMatrixMultiply_t D3DXMatrixMultiply_Original = nullptr;

D3DMATRIX*
WINAPI
D3DXMatrixMultiply_Detour (_Inout_       D3DMATRIX *pOut,
                           _In_          D3DMATRIX *pM1,
                           _In_          D3DMATRIX *pM2)
{
//  if (pM2->_44 == 0.0f ||
      //pM1->_44 == 0.0f) {
//    dll_log.Log (L" [!] D3DMatrixMultiply (...) ");

    if (pM2->_44 == 0.0f &&
        pM2->_34 == -1.0f) {
      dll_log.Log (L"Right-Handed Perspective Projection Matrix(R) Detected: FOVY=%f\n",
                   tan (pM2->_22 * 2.0f));
    }

    if (pM2->_44 == 0.0f &&
        pM2->_34 == 1.0f) {
      dll_log.Log (L"Left-Handed Perspective Projection Matrix(R) Detected: FOVY=%f\n",
                   tan (pM2->_22 * 2.0f));
    }

    if (pM1->_44 == 0.0f &&
        pM1->_34 == -1.0f) {
      dll_log.Log (L"Right-Handed Perspective Projection Matrix(L) Detected: FOVY=%f\n",
                   tan (pM1->_22 * 2.0f));
    }

    if (pM1->_44 == 0.0f &&
        pM1->_34 == 1.0f) {
      dll_log.Log (L"Left-Handed Perspective Projection Matrix(L) Detected: FOVY=%f\n",
                   tan (pM1->_22 * 2.0f));
    }


    //if (tan (pM1->_22 * 2.0f) > 52.5f &&
        //tan (pM1->_22 * 2.0f) < 53.0f) {
    float fovy = tan ((pM1->_22 * 2.0f))  * (3.14159265f / 2.0f);
    if (fovy > 104 && fovy < 107)
      fovy = 70;

    pM1->_22 = atan (fovy / 2.0f * 2.0f * 3.14159265f);
    //dll_log.Log (L"FOVY1=%f\n",
                 //);

    //::tanpM1->_22 * 2.0f
  //}
//    dll_log.Log (L"FOVY2=%f\n",
//                 tan ((pM2->_22 * 2.0f)) * (3.14159265f / 2.0f));
  //}

  return D3DXMatrixMultiply_Original (pOut, pM1, pM2);
}

typedef bool (__cdecl *SteamAPI_Init_t)(void);
SteamAPI_Init_t SteamAPI_Init_Original = nullptr;

bool
__cdecl
SteamAPI_Init_Detour (void)
{
  dll_log.Log (L" [!] SteamAPI_Init (...) ");

  return true;
}

DWORD
WINAPI DllThread (LPVOID user)
{
  if (TZF_Init_MinHook() == MH_OK) {
    TZF_CreateDLLHook ( L"Ole32.dll", "CoCreateInstance",
                        CoCreateInstance_Detour,
             (LPVOID *)&CoCreateInstance_Original );

    //TZF_CreateDLLHook ( L"D3DX9_43.DLL", "D3DXMatrixMultiply", 
                        //D3DXMatrixMultiply_Detour,
             //(LPVOID *)&D3DXMatrixMultiply_Original );

#ifdef LOG_FILE_IO
    TZF_CreateDLLHook ( L"kernel32.dll", "OpenFile",
                        OpenFile_Detour,
             (LPVOID *)&OpenFile_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "CreateFileW",
                        CreateFileW_Detour,
             (LPVOID *)&CreateFileW_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "CreateFileA",
                        CreateFileA_Detour,
             (LPVOID *)&CreateFileA_Original );

    TZF_CreateDLLHook ( L"kernel32.dll", "ReadFile",
                        ReadFile_Detour,
             (LPVOID *)&ReadFile_Original );
#endif

    TZF_EnableHook    (MH_ALL_HOOKS);
  }

  DirectSoundCreate_Detour   (NULL, &g_pDS, NULL);
  g_pDS->SetCooperativeLevel (NULL, DSSCL_EXCLUSIVE);

  return 0;
}

BOOL
APIENTRY
DllMain (HMODULE /* hModule */,
         DWORD    ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    dll_log.init ("dsound.log", "w");
    dll_log.Log  (L"dsound.log created");

    if (! TZF_LoadConfig ()) {
      config.audio.channels      = 6;
      config.audio.sample_hz     = 44100;
      config.audio.compatibility = false;

      // Save a new config if none exists
      TZF_SaveConfig ();
    }

    // Create a thread to load the REAL dsound.dll,
    //  do this from a thread so we do not deadlock!
    CreateThread (NULL, NULL, DllThread, 0, 0, NULL);
    break;

  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    //TZF_SaveConfig ();
    //dll_log.Log (L"closing log file");
    //dll_log.close ();
    break;
  }

  return TRUE;
}
