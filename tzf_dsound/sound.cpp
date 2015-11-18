/**
 * This file is part of Tales of Zestiria "Fix".
 *
 * Tales of Zestiria "Fix" is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Tales of Zestiria "Fix" is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Zestiria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/
#include <Windows.h>

#include "log.h"
#include "config.h"
#include "sound.h"

#include <dsound.h>

#include <comdef.h>

#include <initguid.h>
#include <Mmdeviceapi.h>
#include <Audiopolicy.h>
#include <Audioclient.h>
#include <audiomediatype.h>

//
// Create and reuse a single DirectSound Instance to minimize issues with Bink
//
IDirectSound* g_pDS       = nullptr;
bool          new_session = false;

bool          tzf::SoundFix::wasapi_init = false;

WAVEFORMATEX tzf::SoundFix::snd_core_fmt;
WAVEFORMATEX tzf::SoundFix::snd_bink_fmt;
WAVEFORMATEX tzf::SoundFix::snd_device_fmt;

WAVEFORMATEXTENSIBLE g_DeviceFormat;

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

typedef HRESULT (WINAPI *DirectSoundCreate_t)
( _In_opt_   LPCGUID        pcGuidDevice,
  _Outptr_   LPDIRECTSOUND *ppDS,
  _Pre_null_ LPUNKNOWN      pUnkOuter);

static DirectSoundCreate_t DirectSoundCreate_Original = nullptr;

_Check_return_
HRESULT
WINAPI
DirectSoundCreate_Detour (_In_opt_   LPCGUID        pcGuidDevice,
                          _Outptr_   LPDIRECTSOUND *ppDS,
                          _Pre_null_ LPUNKNOWN      pUnkOuter)
{
  new_session                  = false;
  g_DeviceFormat.Format.cbSize = 0;

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

  HRESULT ret = DirectSoundCreate_Original (pcGuidDevice, ppDS, pUnkOuter);

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
            memcpy (&tzf::SoundFix::snd_device_fmt, pMixFormat, sizeof (WAVEFORMATEX));

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

        pMixFormat->nAvgBytesPerSec = (pMixFormat->nSamplesPerSec * pMixFormat->nChannels * pMixFormat->wBitsPerSample) >> 3;

        g_DeviceFormat.Format.cbSize = 22;
        g_DeviceFormat.Format.nSamplesPerSec  = pMixFormat->nSamplesPerSec;
        g_DeviceFormat.Format.nChannels       = pMixFormat->nChannels;
        g_DeviceFormat.Format.nBlockAlign     = pMixFormat->nBlockAlign;
        g_DeviceFormat.Format.nAvgBytesPerSec = pMixFormat->nAvgBytesPerSec;
        g_DeviceFormat.Format.wBitsPerSample  = pMixFormat->wBitsPerSample;
        //g_DeviceFormat.Format.wBitsPerSample  = ((PWAVEFORMATEXTENSIBLE)pMixFormat)->Samples.wValidBitsPerSample;

        // We may have gotten an extensible wave format, but... we need to
        //   truncate this sucker to a plain old WAVEFORMATEX
        IAudioClient_GetMixFormat_Original (This, ppDeviceFormat);
        memcpy (*ppDeviceFormat, pMixFormat, (*ppDeviceFormat)->cbSize);

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

  if (g_DeviceFormat.Format.cbSize == 22) {
    pClosestMatch->nChannels       = g_DeviceFormat.Format.nChannels;
    pClosestMatch->nSamplesPerSec  = g_DeviceFormat.Format.nSamplesPerSec;
    //pClosestMatch->nBlockAlign     = g_DeviceFormat.Format.nBlockAlign;
    //pClosestMatch->nAvgBytesPerSec = g_DeviceFormat.Format.nAvgBytesPerSec;
    //pClosestMatch->wBitsPerSample  = g_DeviceFormat.Format.wBitsPerSample;
  }

  HRESULT ret =
    IAudioClient_Initialize_Original (This,           AUDCLNT_SHAREMODE_SHARED,
                                      StreamFlags,    hnsBufferDuration,
                                      hnsPeriodicity, pClosestMatch,
                                      AudioSessionGuid);

  if (new_session)
    memcpy (&tzf::SoundFix::snd_core_fmt, pFormat, sizeof (WAVEFORMATEX));

  _com_error error (ret);

#if 0
  if (pClosestMatch != &format)
    CoTaskMemFree (pClosestMatch);
#endif

  dll_log.Log ( L"   Result: 0x%04X (%s)\n", ret - AUDCLNT_ERR (0x0000),
                  error.ErrorMessage () );

  //new_session = false;
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
        tzf::SoundFix::wasapi_init = true;
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

    if (SUCCEEDED (hr2) && pDev != nullptr) {
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



#include "hook.h"

LPVOID pfnCoCreateInstance  = nullptr;
LPVOID pfnDirectSoundCreate = nullptr;

void
tzf::SoundFix::Init (void)
{
  CommandProcessor* pCmdProc = CommandProcessor::getInstance ();

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

  if (! config.audio.enable_fix)
    return;

  // Not setup yet
  g_DeviceFormat.Format.cbSize = 0;

  TZF_CreateDLLHook ( L"dsound.dll", "DirectSoundCreate",
                      DirectSoundCreate_Detour,
           (LPVOID *)&DirectSoundCreate_Original,
           (LPVOID *)&pfnDirectSoundCreate );

  TZF_EnableHook (pfnDirectSoundCreate);

  // We need DSSCL_EXCLUSIVE for Bink to work, or at least we did
  //   when the code was originally written -- test this in the future.
  DirectSoundCreate_Detour   (NULL, &g_pDS, NULL);
  g_pDS->SetCooperativeLevel (NULL, DSSCL_EXCLUSIVE);

  new_session = true;

  TZF_CreateDLLHook ( L"Ole32.dll", "CoCreateInstance",
                      CoCreateInstance_Detour,
           (LPVOID *)&CoCreateInstance_Original,
           (LPVOID *)&pfnCoCreateInstance );

  TZF_EnableHook (pfnCoCreateInstance);
}

void
tzf::SoundFix::Shutdown (void)
{
  if (! config.audio.enable_fix)
    return;

  TZF_RemoveHook (pfnCoCreateInstance);
  TZF_RemoveHook (pfnDirectSoundCreate);
}


class SndInfoCmd : public eTB_Command {
public:
  virtual eTB_CommandResult execute (const char* szArgs) {
    char info_str [2048];
    sprintf (info_str, "\n"
                       " (SoundCore)  SampleRate: %6lu Channels: %lu  Format: 0x%04X  BytesPerSec: %7lu  BitsPerSample: %lu\n"
                       "  ( Device )  SampleRate: %6lu Channels: %lu  Format: 0x%04X  BytesPerSec: %7lu  BitsPerSample: %lu\n",
                         tzf::SoundFix::snd_core_fmt.nSamplesPerSec,
                         tzf::SoundFix::snd_core_fmt.nChannels,
                         tzf::SoundFix::snd_core_fmt.wFormatTag,
                         tzf::SoundFix::snd_core_fmt.nAvgBytesPerSec,
                         tzf::SoundFix::snd_core_fmt.wBitsPerSample,

                         tzf::SoundFix::snd_device_fmt.nSamplesPerSec,
                         tzf::SoundFix::snd_device_fmt.nChannels,
                         tzf::SoundFix::snd_device_fmt.wFormatTag,
                         tzf::SoundFix::snd_device_fmt.nAvgBytesPerSec,
                         tzf::SoundFix::snd_device_fmt.wBitsPerSample);

    return eTB_CommandResult ("SoundInfo", "", info_str, 1);
  }
};

tzf::SoundFix::CommandProcessor::CommandProcessor (void)
{
  sample_rate_   = new eTB_VarStub <int>  ((int *)&config.audio.sample_hz);
  channels_      = new eTB_VarStub <int>  ((int *)&config.audio.channels);
  enable_        = new eTB_VarStub <bool> (&config.audio.enable_fix);
  compatibility_ = new eTB_VarStub <bool> (&config.audio.compatibility);

  command.AddVariable ("SampleRate",            sample_rate_);
  command.AddVariable ("Channels",              channels_);
  command.AddVariable ("SoundFixCompatibility", compatibility_);
  command.AddVariable ("EnableSoundFix",        enable_);

  SndInfoCmd* sndinfo = new SndInfoCmd ();
  command.AddCommand ("SoundInfo", sndinfo);
}

bool
tzf::SoundFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  return true;
}


tzf::SoundFix::CommandProcessor* tzf::SoundFix::CommandProcessor::pCommProc;