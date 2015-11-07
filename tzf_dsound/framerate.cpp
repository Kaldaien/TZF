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

#include "framerate.h"
#include "config.h"
#include "log.h"
#include "hook.h"

#include "render.h"


bool stutter_fix_installed = false;


#include <d3d9.h>
D3DPRESENT_PARAMETERS present_params;

bool tzf::FrameRateFix::fullscreen         = false;
bool tzf::FrameRateFix::driver_limit_setup = false;

typedef D3DPRESENT_PARAMETERS* (__stdcall *BMF_SetPresentParamsD3D9_t)
  (IDirect3DDevice9*      device,
   D3DPRESENT_PARAMETERS* pparams);
BMF_SetPresentParamsD3D9_t BMF_SetPresentParamsD3D9_Original = nullptr;

D3DPRESENT_PARAMETERS*
__stdcall
BMF_SetPresentParamsD3D9_Detour (IDirect3DDevice9*      device,
                                 D3DPRESENT_PARAMETERS* pparams)
{
  tzf::RenderFix::pDevice = device;

  if (pparams != nullptr) {
    memcpy (&present_params, pparams, sizeof D3DPRESENT_PARAMETERS);

    dll_log.Log ( L" %% Caught D3D9 Swapchain :: Fullscreen=%s",
                    pparams->Windowed ? L"False" :
                                        L"True" );

    dll_log.Log ( L" * Flags=0x%04X", pparams->Flags);

    if (config.framerate.stutter_fix) {
      // On NVIDIA hardware, we can setup a framerate limiter at the driver
      //   level to handle windowed mode or improperly setup VSYNC
      static HMODULE hD3D9 = LoadLibrary (L"d3d9.dll");

      typedef BOOL (__stdcall *BMF_NvAPI_IsInit_t)
        (void);
      typedef void (__stdcall *BMF_NvAPI_SetAppFriendlyName_t)
        (const wchar_t* wszFriendlyName);
      typedef BOOL (__stdcall *BMF_NvAPI_SetFramerateLimit_t)
        (const wchar_t* wszName, uint32_t limit);

      static BMF_NvAPI_IsInit_t             BMF_NvAPI_IsInit             =
        (BMF_NvAPI_IsInit_t)GetProcAddress
        (hD3D9, "BMF_NvAPI_IsInit");
      static BMF_NvAPI_SetAppFriendlyName_t BMF_NvAPI_SetAppFriendlyName = 
        (BMF_NvAPI_SetAppFriendlyName_t)GetProcAddress
        (hD3D9, "BMF_NvAPI_SetAppFriendlyName");
      static BMF_NvAPI_SetFramerateLimit_t BMF_NvAPI_SetFramerateLimit   = 
        (BMF_NvAPI_SetFramerateLimit_t)GetProcAddress
        (hD3D9, "BMF_NvAPI_SetFramerateLimit");

      if ((! tzf::FrameRateFix::driver_limit_setup) && BMF_NvAPI_IsInit ()) {
        BMF_NvAPI_SetAppFriendlyName (L"Tales of Zestiria");

        DWORD   dwProcessSize = MAX_PATH;
        wchar_t wszProcessName [MAX_PATH];

        HANDLE hProc = GetCurrentProcess ();

        QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

        // Shorten the name of the .exe, fully-qualified paths are known to
        //   create NvAPI errors, but just the .exe by itself works well.
        wchar_t* pwszShortName = wszProcessName + lstrlenW (wszProcessName);

        while (  pwszShortName      >  wszProcessName &&
          *(pwszShortName - 1) != L'\\')
          --pwszShortName;

        if (! BMF_NvAPI_SetFramerateLimit (pwszShortName, 30UL)) {
          MessageBox ( NULL,
                         L"Stutter Fix:\t\t"
                         L"Optimal NVIDIA Driver Framerate Limit Setup",
                           L"Game Must Restart",
                             MB_OK | MB_TOPMOST | MB_SETFOREGROUND |
                             MB_ICONINFORMATION );
          exit (0);
        }

        tzf::FrameRateFix::driver_limit_setup = true;
      }
    }

    if (! pparams->Windowed)
      tzf::FrameRateFix::fullscreen = true;
    else {
      tzf::FrameRateFix::fullscreen = false;
    }

    tzf::RenderFix::width  = present_params.BackBufferWidth;
    tzf::RenderFix::height = present_params.BackBufferHeight;

    // Change the Aspect Ratio
    char szAspectCommand [64];
    sprintf (szAspectCommand, "AspectRatio %f", (float)tzf::RenderFix::width / (float)tzf::RenderFix::height);

    command.ProcessCommandLine (szAspectCommand);
  }

  return BMF_SetPresentParamsD3D9_Original (device, pparams);
}


static __declspec (thread) int           last_sleep     =   1;
static __declspec (thread) LARGE_INTEGER last_perfCount = { 0 };


typedef MMRESULT (WINAPI *timeBeginPeriod_t)(UINT uPeriod);
timeBeginPeriod_t timeBeginPeriod_Original = nullptr;

MMRESULT
WINAPI
timeBeginPeriod_Detour (UINT uPeriod)
{
  dll_log.Log ( L"[!] timeBeginPeriod (%d) - "
                L"[Calling Thread: 0x%04x]",
                  uPeriod,
                    GetCurrentThreadId () );


  return timeBeginPeriod_Original (uPeriod);
}

// We need to use TLS for this, I would imagine...


typedef void (WINAPI *Sleep_t)(DWORD dwMilliseconds);
Sleep_t Sleep_Original = nullptr;

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  last_sleep = dwMilliseconds;

  if (config.framerate.yield_processor && dwMilliseconds == 0)
    YieldProcessor ();

  if (dwMilliseconds != 0 || config.framerate.allow_fake_sleep) {
    Sleep_Original (dwMilliseconds);
  }
}

typedef BOOL (WINAPI *QueryPerformanceCounter_t)(_Out_ LARGE_INTEGER *lpPerformanceCount);
QueryPerformanceCounter_t QueryPerformanceCounter_Original = nullptr;

BOOL
WINAPI
QueryPerformanceCounter_Detour (_Out_ LARGE_INTEGER *lpPerformanceCount)
{
  BOOL ret = QueryPerformanceCounter_Original (lpPerformanceCount);

  if (last_sleep != 0 || (! (tzf::FrameRateFix::fullscreen ||
                             tzf::FrameRateFix::driver_limit_setup))) {
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );
    return ret;
  } else {
    const float fudge_factor = config.framerate.fudge_factor;

    last_sleep = -1;

    // Mess with the numbers slightly to prevent hiccups
    lpPerformanceCount->QuadPart += (lpPerformanceCount->QuadPart - last_perfCount.QuadPart) * 
                                     fudge_factor/* * freq.QuadPart*/;
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );

    return ret;
  }
}


LPVOID pfnQueryPerformanceCounter  = nullptr;
LPVOID pfnSleep                    = nullptr;
LPVOID pfnBMF_SetPresentParamsD3D9 = nullptr;

void
tzf::FrameRateFix::Init (void)
{
  if (! config.framerate.stutter_fix)
    return;

  TZF_CreateDLLHook ( L"kernel32.dll", "QueryPerformanceCounter",
                      QueryPerformanceCounter_Detour, 
           (LPVOID *)&QueryPerformanceCounter_Original,
           (LPVOID *)&pfnQueryPerformanceCounter );

  TZF_CreateDLLHook ( L"kernel32.dll", "Sleep",
                      Sleep_Detour, 
           (LPVOID *)&Sleep_Original,
           (LPVOID *)&pfnSleep );

  TZF_CreateDLLHook ( L"d3d9.dll", "BMF_SetPresentParamsD3D9",
                      BMF_SetPresentParamsD3D9_Detour, 
           (LPVOID *)&BMF_SetPresentParamsD3D9_Original,
                     &pfnBMF_SetPresentParamsD3D9 );

  command.AddVariable ("FudgeFactor",    new eTB_VarStub <float> (&config.framerate.fudge_factor));
  command.AddVariable ("AllowFakeSleep", new eTB_VarStub <bool>  (&config.framerate.allow_fake_sleep));
  command.AddVariable ("YieldProcessor", new eTB_VarStub <bool>  (&config.framerate.yield_processor));

  stutter_fix_installed = true;
}

void
tzf::FrameRateFix::Shutdown(void)
{
  if (! config.framerate.stutter_fix)
    return;

  TZF_RemoveHook (pfnQueryPerformanceCounter);
  TZF_RemoveHook (pfnSleep);
  TZF_RemoveHook (pfnBMF_SetPresentParamsD3D9);

  stutter_fix_installed = false;
}