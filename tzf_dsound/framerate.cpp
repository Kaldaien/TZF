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

#define TICK_ADDR_BASE 0x0217B3D4

uint8_t          tzf::FrameRateFix::old_speed_reset_code2   [7];
uint8_t          tzf::FrameRateFix::old_limiter_instruction [6];
int32_t          tzf::FrameRateFix::tick_scale               = 2; // 30 FPS

CRITICAL_SECTION tzf::FrameRateFix::alter_speed_cs           = { 0 };

bool             tzf::FrameRateFix::fullscreen               = false;
bool             tzf::FrameRateFix::driver_limit_setup       = false;
bool             tzf::FrameRateFix::stutter_fix_installed    = false;
bool             tzf::FrameRateFix::variable_speed_installed = false;

uint32_t         tzf::FrameRateFix::target_fps               = 30;


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

#if 0
  // Ignore D3D9Ex devices, I don't know what they are even for, but they will
  //   totally screw things up if we don't.
  IDirect3DDevice9Ex* pDeviceEx = nullptr;
  if ( SUCCEEDED ( device->QueryInterface ( __uuidof (IDirect3DDevice9Ex),
                                              (void **)&pDeviceEx )
                 ) 
     ) {
    pDeviceEx->Release ();
    return BMF_SetPresentParamsD3D9_Original (device, pparams);
  }
#endif

  //
  // TODO: Figure out what the hell is doing this when RTSS is allowed to use
  //         custom D3D libs. 1x1@0Hz is obviously NOT for rendering!
  //
  if ( pparams->BackBufferWidth            == 1 &&
       pparams->BackBufferHeight           == 1 &&
       pparams->FullScreen_RefreshRateInHz == 0 ) {
    dll_log.Log (L" * Fake D3D9Ex Device Detected... Ignoring!");
    return BMF_SetPresentParamsD3D9_Original (device, pparams);
  }


  tzf::RenderFix::pDevice             = device;
  tzf::RenderFix::pPostProcessSurface = nullptr;

  if (pparams != nullptr) {
    memcpy (&present_params, pparams, sizeof D3DPRESENT_PARAMETERS);

    dll_log.Log ( L" %% Caught D3D9 Swapchain :: Fullscreen=%s  (%lux%lu@%lu Hz)",
                    pparams->Windowed ? L"False" :
                                        L"True",
                      pparams->BackBufferWidth,
                        pparams->BackBufferHeight,
                          pparams->FullScreen_RefreshRateInHz );

    extern HWND hDeviceWindow;
    hDeviceWindow = pparams->hDeviceWindow;

    if (config.framerate.stutter_fix) {
      // On NVIDIA hardware, we can setup a framerate limiter at the driver
      //   level to handle windowed mode or improperly setup VSYNC
      static HMODULE hD3D9 = GetModuleHandle (L"d3d9.dll");

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

        if (! BMF_NvAPI_SetFramerateLimit (0UL)) {
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

bool          render_sleep0  = false;
LARGE_INTEGER last_perfCount = { 0 };

void
WINAPI
Sleep_Detour (DWORD dwMilliseconds)
{
  if (GetCurrentThreadId () == tzf::RenderFix::dwRenderThreadID) {
    if (dwMilliseconds == 0)
      render_sleep0 = true;
    else
      render_sleep0 = false;
  }

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

  DWORD dwThreadId = GetCurrentThreadId ();

  //
  // Handle threads that aren't render-related NORMALLY as well as the render
  //   thread when it DID NOT just voluntarily relinquish its scheduling
  //     timeslice
  //
  if (dwThreadId != tzf::RenderFix::dwRenderThreadID || (! render_sleep0) ||
                              (! (tzf::FrameRateFix::fullscreen           ||
                                  tzf::FrameRateFix::driver_limit_setup   ||
                                  config.framerate.allow_windowed_mode))) {

    if (dwThreadId == tzf::RenderFix::dwRenderThreadID)
      memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER));

    return ret;
  }

  //
  // At this point, we're fixing up the thread that throttles the swapchain.
  //
  const float fudge_factor = config.framerate.fudge_factor;

  render_sleep0 = false;

  LARGE_INTEGER freq;
  QueryPerformanceFrequency (&freq);

  // Mess with the numbers slightly to prevent scheduling from wreaking havoc
  lpPerformanceCount->QuadPart += (lpPerformanceCount->QuadPart - last_perfCount.QuadPart) * 
    fudge_factor *
    ((float)tzf::FrameRateFix::target_fps / 30.0f);
  memcpy (&last_perfCount, lpPerformanceCount, sizeof (LARGE_INTEGER));

  return ret;
}

typedef void (__stdcall *BinkOpen_t)(DWORD unknown0, DWORD unknown1);
BinkOpen_t BinkOpen_Original = nullptr;

void
__stdcall
BinkOpen_Detour ( DWORD unknown0,
                  DWORD unknown1 )
{
  dll_log.Log (L" * Disabling 60 FPS -- Bink Video Opened");

  tzf::RenderFix::bink = true;
  tzf::FrameRateFix::Disallow60FPS ();

  return BinkOpen_Original (unknown0, unknown1);
}

typedef void (__stdcall *BinkClose_t)(DWORD unknown);
BinkClose_t BinkClose_Original = nullptr;

void
__stdcall
BinkClose_Detour (DWORD unknown)
{
  BinkClose_Original (unknown);

  dll_log.Log (L" * Restoring 60 FPS -- Bink Video Closed");

  tzf::RenderFix::bink = false;
  tzf::FrameRateFix::Allow60FPS ();
}


LPVOID pfnQueryPerformanceCounter  = nullptr;
LPVOID pfnSleep                    = nullptr;
LPVOID pfnBMF_SetPresentParamsD3D9 = nullptr;

// Hook these to properly synch audio subtitles during FMVs
LPVOID pfnBinkOpen                 = nullptr;
LPVOID pfnBinkClose                = nullptr;

void
TZF_FlushInstructionCache ( LPCVOID base_addr,
                            size_t  code_size )
{
  FlushInstructionCache ( GetCurrentProcess (),
                            base_addr,
                              code_size );
}

void
TZF_InjectByteCode ( LPVOID   base_addr,
                     uint8_t* new_code,
                     size_t   code_size,
                     DWORD    permissions,
                     uint8_t* old_code = nullptr )
{
  DWORD dwOld;

  VirtualProtect (base_addr, code_size, permissions, &dwOld);
  {
    if (old_code != nullptr)
      memcpy (old_code, base_addr, code_size);

    memcpy (base_addr, new_code, code_size);
  }
  VirtualProtect (base_addr, code_size, dwOld, &dwOld);

  TZF_FlushInstructionCache (base_addr, code_size);
}

void
tzf::FrameRateFix::Init (void)
{
  CommandProcessor* comm_proc = CommandProcessor::getInstance ();

  // Poke a hole in the BMF design and hackishly query the TargetFPS ...
  //   this needs to be re-designed pronto.
  static HMODULE hD3D9 = GetModuleHandle (L"d3d9.dll");

  typedef uint32_t (__stdcall *BMF_Config_GetTargetFPS_t)(void);
  static BMF_Config_GetTargetFPS_t BMF_Config_GetTargetFPS =
    (BMF_Config_GetTargetFPS_t)GetProcAddress ( hD3D9,
                                                  "BMF_Config_GetTargetFPS" );

  target_fps = BMF_Config_GetTargetFPS ();

  TZF_CreateDLLHook ( L"d3d9.dll", "BMF_SetPresentParamsD3D9",
                      BMF_SetPresentParamsD3D9_Detour, 
           (LPVOID *)&BMF_SetPresentParamsD3D9_Original,
                     &pfnBMF_SetPresentParamsD3D9 );

  TZF_CreateDLLHook ( L"bink2w32.dll", "_BinkOpen@8",
                      BinkOpen_Detour, 
           (LPVOID *)&BinkOpen_Original,
                     &pfnBinkOpen );

  TZF_CreateDLLHook ( L"bink2w32.dll", "_BinkClose@4",
                      BinkClose_Detour, 
           (LPVOID *)&BinkClose_Original,
                     &pfnBinkClose );

  TZF_EnableHook (pfnBMF_SetPresentParamsD3D9);

  TZF_EnableHook (pfnBinkOpen);
  TZF_EnableHook (pfnBinkClose);

  InitializeCriticalSectionAndSpinCount (&alter_speed_cs, 1000UL);

  if (true) {//target_fps != 30) {
    dll_log.LogEx (true, L" * Installing variable rate simulation... ");

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
#if 0
    VirtualProtect((LPVOID)config.framerate.speedresetcode_addr, 17, PAGE_EXECUTE_READWRITE, &dwOld);
               *((DWORD *)(config.framerate.speedresetcode_addr +  2)) += 8;
               *((DWORD *)(config.framerate.speedresetcode_addr +  8)) += 8;
               *((DWORD *)(config.framerate.speedresetcode_addr + 13)) -= 2;
    VirtualProtect((LPVOID)config.framerate.speedresetcode_addr, 17, dwOld, &dwOld);
#endif

    TZF_FlushInstructionCache ((LPCVOID)config.framerate.speedresetcode_addr, 17);

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
#if 0
    uint8_t new_code [7] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90 };

    TZF_InjectByteCode ( (LPVOID)config.framerate.speedresetcode2_addr,
                           new_code,
                             7,
                               PAGE_EXECUTE_READWRITE,
                                 old_speed_reset_code2 );
#endif

    variable_speed_installed = true;

    // mov eax, 02 to mov eax, 01
    command.ProcessCommandLine ("TickScale 1");

    VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
    *(DWORD *)config.framerate.speedresetcode3_addr = 1;
    VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, dwOld, &dwOld);

    dll_log.LogEx (false, L" Target=%lu FPS\n",
      target_fps);
  }

  variable_speed_installed = true;

  if (config.framerate.disable_limiter) {
    DWORD dwOld;

    // Replace the original jump with an unconditional jump
    uint8_t new_code [6] = { 0xE9, 0x8B, 0x00, 0x00, 0x00, 0x90 };

    TZF_InjectByteCode ( (LPVOID)config.framerate.limiter_branch_addr,
                           new_code,
                             6,
                               PAGE_EXECUTE_READWRITE );
  }

  command.AddVariable ("TargetFPS", new eTB_VarStub <int> ((int *)&target_fps));

  // No matter which technique we use, these things need to be options
  command.AddVariable ("MinimizeLatency",   new eTB_VarStub <bool>  (&config.framerate.minimize_latency));
  command.AddVariable ("AllowWindowedMode", new eTB_VarStub <bool>  (&config.framerate.allow_windowed_mode));

  // Hook this no matter what, because it lowers the _REPORTED_ CPU usage,
  //   and some people would object if we suddenly changed this behavior :P
  TZF_CreateDLLHook ( L"kernel32.dll", "Sleep",
                      Sleep_Detour, 
           (LPVOID *)&Sleep_Original,
           (LPVOID *)&pfnSleep );
  TZF_EnableHook (pfnSleep);

  command.AddVariable ("AllowFakeSleep",    new eTB_VarStub <bool>  (&config.framerate.allow_fake_sleep));
  command.AddVariable ("YieldProcessor",    new eTB_VarStub <bool>  (&config.framerate.yield_processor));

  command.AddVariable ("AutoAdjust", new eTB_VarStub <bool> (&config.framerate.auto_adjust));

  // Stuff below here is ONLY needed for the hacky frame pacing fix
  if (! config.framerate.stutter_fix)
    return;

  TZF_CreateDLLHook ( L"kernel32.dll", "QueryPerformanceCounter",
                      QueryPerformanceCounter_Detour, 
           (LPVOID *)&QueryPerformanceCounter_Original,
           (LPVOID *)&pfnQueryPerformanceCounter );
  TZF_EnableHook (pfnQueryPerformanceCounter);

  // Only make these in-game options if the stutter fix code is enabled
  command.AddVariable ("FudgeFactor",       new eTB_VarStub <float> (&config.framerate.fudge_factor));

  stutter_fix_installed = true;
}

void
tzf::FrameRateFix::Shutdown (void)
{
  TZF_RemoveHook (pfnBMF_SetPresentParamsD3D9);

  if (variable_speed_installed) {
    DWORD dwOld;

    VirtualProtect ((LPVOID)config.framerate.speedresetcode_addr, 17, PAGE_EXECUTE_READWRITE, &dwOld);
                *((DWORD *)(config.framerate.speedresetcode_addr +  2)) -= 8;
                *((DWORD *)(config.framerate.speedresetcode_addr +  8)) -= 8;
                *((DWORD *)(config.framerate.speedresetcode_addr + 13)) += 2;
    VirtualProtect ((LPVOID)config.framerate.speedresetcode_addr, 17, dwOld, &dwOld);

    TZF_FlushInstructionCache ((LPCVOID)config.framerate.speedresetcode_addr, 17);

    TZF_InjectByteCode ( (LPVOID)config.framerate.speedresetcode2_addr,
                           old_speed_reset_code2,
                             7,
                               PAGE_EXECUTE_READWRITE );

    command.ProcessCommandLine ("TickScale 2");

    variable_speed_installed = false;
  }

  TZF_RemoveHook (pfnSleep);

  if (! config.framerate.stutter_fix)
    return;

  TZF_RemoveHook (pfnQueryPerformanceCounter);

  stutter_fix_installed = false;
}

void
tzf::FrameRateFix::Disallow60FPS (void)
{
  EnterCriticalSection (&alter_speed_cs);

  if (variable_speed_installed) {
    command.ProcessCommandLine ("TickScale 2");
  }

  LeaveCriticalSection (&alter_speed_cs);
}

void
tzf::FrameRateFix::Allow60FPS (void)
{
  EnterCriticalSection (&alter_speed_cs);

  if (variable_speed_installed) {
    command.ProcessCommandLine ("TickScale 1");
  }

  LeaveCriticalSection (&alter_speed_cs);
}





tzf::FrameRateFix::CommandProcessor* tzf::FrameRateFix::CommandProcessor::pCommProc;

tzf::FrameRateFix::CommandProcessor::CommandProcessor (void)
{
  tick_scale_ = new eTB_VarStub <int> (&tick_scale, this);

  command.AddVariable ("TickScale", tick_scale_);
}

bool
tzf::FrameRateFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  DWORD dwOld;

  if (var == tick_scale_) {
    DWORD original0 = *((DWORD *)(TICK_ADDR_BASE    ));
    DWORD original1 = *((DWORD *)(TICK_ADDR_BASE + 4));

    if (val != nullptr) {
#if 1
      VirtualProtect ((LPVOID)TICK_ADDR_BASE, 8, PAGE_READWRITE, &dwOld);

#if 0
      if (variable_speed_installed) {
        *(DWORD *)(TICK_ADDR_BASE) = *(DWORD *)val;
      }
#endif

      if (variable_speed_installed) {
        *(DWORD *)(TICK_ADDR_BASE + 4) = *(DWORD *)val;
      }
      *(DWORD *)val = *(DWORD *)(TICK_ADDR_BASE + 4);

      VirtualProtect ((LPVOID)TICK_ADDR_BASE, 8, dwOld, &dwOld);

      if (variable_speed_installed) {
        // mov eax, 02 to mov eax, 01
        VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
        *(DWORD *)config.framerate.speedresetcode3_addr = *(DWORD *)val;
        VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, dwOld, &dwOld);
      }
      //InterlockedExchange ((DWORD *)val, *(DWORD *)config.framerate.speedresetcode3_addr);
#endif

      tick_scale      = *(int32_t *)val;
    }
  }

  return true;
}



void
tzf::FrameRateFix::RenderTick (void)
{
  static LARGE_INTEGER last_time = { 0 };
  static LARGE_INTEGER freq      = { 0 };

  LARGE_INTEGER time;

  QueryPerformanceFrequency (&freq);
  QueryPerformanceCounter   (&time);

  const double inv_rate = 1.0 / tzf::FrameRateFix::target_fps;
  const double epsilon  = 0.0;

  // Busy-wait because we are rendering too fast...
  while (time.QuadPart < (last_time.QuadPart + freq.QuadPart * inv_rate - epsilon)) {
    QueryPerformanceCounter (&time);
  }

  double  dt = ((double)(time.QuadPart - last_time.QuadPart) / (double)freq.QuadPart) / (1.0 / 60.0);

#if 0
  static double accum = 0.0;

  accum +=  dt - floor (dt);
  dt    -= (dt - floor (dt));

  if (accum >= 1.0) {
    dt    +=  accum - (accum - floor (accum));
    accum -=  accum - (accum - floor (accum));
  }
#endif

  long scale = min (max (dt, 1), 7);

  if (scale > 6) {
    scale = inv_rate / (1.0 / 60.0);
  }

  if (config.framerate.auto_adjust) {
    char rescale [32];
    sprintf (rescale, "TickScale %li", scale);
    command.ProcessCommandLine (rescale);
  }

  last_time.QuadPart = time.QuadPart;
}