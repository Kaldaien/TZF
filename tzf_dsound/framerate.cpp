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
#include <d3d9.h>

bool stutter_fix_installed = false;
bool half_speed_installed  = false;
byte old_speed_reset_code2 [7];

bool     tzf::FrameRateFix::fullscreen         = false;
bool     tzf::FrameRateFix::driver_limit_setup = false;
uint32_t tzf::FrameRateFix::target_fps         = 30;

typedef D3DPRESENT_PARAMETERS* (__stdcall *BMF_SetPresentParamsD3D9_t)
  (IDirect3DDevice9*      device,
   D3DPRESENT_PARAMETERS* pparams);
BMF_SetPresentParamsD3D9_t BMF_SetPresentParamsD3D9_Original = nullptr;

COM_DECLSPEC_NOTHROW
D3DPRESENT_PARAMETERS*
__stdcall
BMF_SetPresentParamsD3D9_Detour (IDirect3DDevice9*      device,
                                 D3DPRESENT_PARAMETERS* pparams)
{
  D3DPRESENT_PARAMETERS present_params;

  tzf::RenderFix::pDevice             = device;
  tzf::RenderFix::pPostProcessSurface = nullptr;

  if (pparams != nullptr) {
    memcpy (&present_params, pparams, sizeof D3DPRESENT_PARAMETERS);

    dll_log.Log ( L" %% Caught D3D9 Swapchain :: Fullscreen=%s",
                    pparams->Windowed ? L"False" :
                                        L"True" );

    if (config.framerate.stutter_fix) {
      // On NVIDIA hardware, we can setup a framerate limiter at the driver
      //   level to handle windowed mode or improperly setup VSYNC
      static HMODULE hD3D9 = LoadLibrary (L"d3d9.dll");

      typedef BOOL (__stdcall *BMF_NvAPI_IsInit_t)
        (void);
      typedef void (__stdcall *BMF_NvAPI_SetAppFriendlyName_t)
        (const wchar_t* wszFriendlyName);
      typedef BOOL (__stdcall *BMF_NvAPI_SetFramerateLimit_t)
        (uint32_t limit);

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

        if (! BMF_NvAPI_SetFramerateLimit (30UL)) {
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


// We need to use TLS for this so... but, TLS in DLLs is tricky; so this hack works
#include <unordered_map>
std::unordered_map <DWORD, DWORD>         per_thread_sleep;
std::unordered_map <DWORD, LARGE_INTEGER> per_thread_perfCount;

// This solution sort of worked, but it was causing minor dialog issues
//static __declspec (thread) int           last_sleep     =   1;
//static __declspec (thread) LARGE_INTEGER last_perfCount = { 0 };


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

typedef void (WINAPI *Sleep_t)(DWORD dwMilliseconds);
Sleep_t Sleep_Original = nullptr;

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  per_thread_sleep [GetCurrentThreadId ()] = dwMilliseconds;

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

  DWORD&         last_sleep     = per_thread_sleep     [GetCurrentThreadId ()];
  LARGE_INTEGER& last_perfCount = per_thread_perfCount [GetCurrentThreadId ()];

  if (last_sleep != 0 || (! (tzf::FrameRateFix::fullscreen ||
                             tzf::FrameRateFix::driver_limit_setup ||
                             config.framerate.allow_windowed_mode))) {
    memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER) );
    
    return ret;
  }

  // A Sleep (0) call always precedes timing issues that were causing stuttering.
  else {
    const float fudge_factor = config.framerate.fudge_factor;

    last_sleep = -1;

    // Mess with the numbers slightly to prevent hiccups
    lpPerformanceCount->QuadPart += (lpPerformanceCount->QuadPart - last_perfCount.QuadPart) * 
                                     fudge_factor *
                             ((float)tzf::FrameRateFix::target_fps / 30.0f);
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
  // Poke a hole in the BMF design and hackishly query the TargetFPS ...
  //   this needs to be re-designed pronto.
  static HMODULE hD3D9 = LoadLibrary (L"d3d9.dll");

  typedef uint32_t (__stdcall *BMF_Config_GetTargetFPS_t)(void);
  static BMF_Config_GetTargetFPS_t BMF_Config_GetTargetFPS =
    (BMF_Config_GetTargetFPS_t)GetProcAddress ( hD3D9,
                                                  "BMF_Config_GetTargetFPS" );

  target_fps = BMF_Config_GetTargetFPS ();


  TZF_CreateDLLHook ( L"d3d9.dll", "BMF_SetPresentParamsD3D9",
                      BMF_SetPresentParamsD3D9_Detour, 
           (LPVOID *)&BMF_SetPresentParamsD3D9_Original,
                     &pfnBMF_SetPresentParamsD3D9 );


  if (target_fps >= 45) {
    dll_log.Log (L" * Target FPS is %lu, halving simulation speed to compensate...",
        target_fps);

    DWORD dwOld;

    //
    // original code:
    //
    // >> lea esi, [eax+0000428C]
    // >> lea edi, [ebx+0000428C]
    // >> mov ecx, 11
    // rep movsd
    // 
    // we want to skip the first two dwords
    //
    VirtualProtect((LPVOID)config.framerate.speedresetcode_addr, 17, PAGE_EXECUTE_READWRITE, &dwOld);
    *((DWORD *)(config.framerate.speedresetcode_addr +  2)) += 8;
    *((DWORD *)(config.framerate.speedresetcode_addr +  8)) += 8;
    *((DWORD *)(config.framerate.speedresetcode_addr + 13)) -= 2;
    VirtualProtect((LPVOID)config.framerate.speedresetcode_addr, 17, dwOld, &dwOld);

    //
    // original code:
    //
    // ...
    // cmp [ebx+28], eax
    // >> jz after_set
    // >> cmp eax, 2
    // >> jl after_set
    // mov 0217B3D4, eax
    // mov 0217B3D8, eax
    // mov [ebx+28], eax
    // after_set:
    // ...
    //
    // we just want this to be 1 always
    //
    // new code:
    // mov eax, 01
    // nop
    // nop
    //
    byte new_code [7] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90 };
    VirtualProtect ((LPVOID)config.framerate.speedresetcode2_addr, 7, PAGE_EXECUTE_READWRITE, &dwOld);
    memcpy (&old_speed_reset_code2, &new_code, 7);
    memcpy ((LPVOID)config.framerate.speedresetcode2_addr, &new_code, 7);
    VirtualProtect ((LPVOID)config.framerate.speedresetcode2_addr, 7, dwOld, &dwOld);

    // mov eax, 02 to mov eax, 01
    VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
    *((DWORD *)config.framerate.speedresetcode3_addr) = 1;
    VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, dwOld, &dwOld);

    half_speed_installed = true;
  }


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

  command.AddVariable ("FudgeFactor",       new eTB_VarStub <float> (&config.framerate.fudge_factor));
  command.AddVariable ("AllowFakeSleep",    new eTB_VarStub <bool>  (&config.framerate.allow_fake_sleep));
  command.AddVariable ("YieldProcessor",    new eTB_VarStub <bool>  (&config.framerate.yield_processor));
  command.AddVariable ("AllowWindowedMode", new eTB_VarStub <bool>  (&config.framerate.allow_windowed_mode));
  command.AddVariable ("MinimizeLatency",   new eTB_VarStub <bool>  (&config.framerate.minimize_latency));

  stutter_fix_installed = true;
}

void
tzf::FrameRateFix::Shutdown (void)
{
  TZF_RemoveHook (pfnBMF_SetPresentParamsD3D9);

  if (half_speed_installed) {
    DWORD dwOld;

    VirtualProtect ((LPVOID)config.framerate.speedresetcode_addr, 17, PAGE_EXECUTE_READWRITE, &dwOld);
    *((DWORD *)(config.framerate.speedresetcode_addr +  2)) -= 8;
    *((DWORD *)(config.framerate.speedresetcode_addr +  8)) -= 8;
    *((DWORD *)(config.framerate.speedresetcode_addr + 13)) += 2;
    VirtualProtect ((LPVOID)config.framerate.speedresetcode_addr, 17, dwOld, &dwOld);

    VirtualProtect ((LPVOID)config.framerate.speedresetcode2_addr, 7, PAGE_EXECUTE_READWRITE, &dwOld);
    memcpy ((LPVOID)config.framerate.speedresetcode2_addr, &old_speed_reset_code2, 7);
    VirtualProtect ((LPVOID)config.framerate.speedresetcode2_addr, 7, dwOld, &dwOld);

    VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
    *((DWORD *)config.framerate.speedresetcode3_addr) = 2;
    VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, dwOld, &dwOld);

    half_speed_installed = false;
  }

  if (! config.framerate.stutter_fix)
    return;

  TZF_RemoveHook (pfnQueryPerformanceCounter);
  TZF_RemoveHook (pfnSleep);

  stutter_fix_installed = false;
}