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
#include "scanner.h"

#include "priest.lua.h"

#include "render.h"
#include <d3d9.h>

uint32_t TICK_ADDR_BASE = 0x217B464;
// 0x0217B3D4  1.3

uint8_t          tzf::FrameRateFix::old_speed_reset_code2   [7];
uint8_t          tzf::FrameRateFix::old_limiter_instruction [6];
int32_t          tzf::FrameRateFix::tick_scale               = 2; // 30 FPS

CRITICAL_SECTION tzf::FrameRateFix::alter_speed_cs           = { 0 };

bool             tzf::FrameRateFix::stutter_fix_installed    = false;
bool             tzf::FrameRateFix::variable_speed_installed = false;

uint32_t         tzf::FrameRateFix::target_fps               = 30;

HMODULE          tzf::FrameRateFix::bink_dll                 = 0;
HMODULE          tzf::FrameRateFix::kernel32_dll             = 0;


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

    dll_log.Log ( L" %% Caught D3D9 Swapchain :: Fullscreen=%s "
                  L" (%lux%lu@%lu Hz) "
                  L" [Device Window: 0x%04X]",
                    pparams->Windowed ? L"False" :
                                        L"True",
                      pparams->BackBufferWidth,
                        pparams->BackBufferHeight,
                          pparams->FullScreen_RefreshRateInHz,
                            pparams->hDeviceWindow );
  
    tzf::RenderFix::hWndDevice = pparams->hDeviceWindow;

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
  if (dwThreadId != tzf::RenderFix::dwRenderThreadID || (! render_sleep0)) {
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

typedef void (__stdcall *BinkOpen_t)(const char* filename, DWORD unknown0);
BinkOpen_t BinkOpen_Original = nullptr;

void
__stdcall
BinkOpen_Detour ( const char* filename,
                  DWORD       unknown0 )
{
  dll_log.Log (L" * Disabling TargetFPS -- Bink Video Opened");

  tzf::RenderFix::bink = true;
  tzf::FrameRateFix::Begin30FPSEvent ();

  // Optionally play some other video...
  if (! stricmp (filename, "RAW\\MOVIE\\AM_TOZ_OP_001.BK2")) {
    dll_log.Log ( L" >> Bypassing Opening Movie with %ws",
                   config.system.intro_video.c_str ());

    static char szBypassName [MAX_PATH] = { '\0' };

    sprintf (szBypassName, "%ws", config.system.intro_video.c_str ());

    return BinkOpen_Original (szBypassName, unknown0);
  }

  return BinkOpen_Original (filename, unknown0);
}

typedef void (__stdcall *BinkClose_t)(DWORD unknown);
BinkClose_t BinkClose_Original = nullptr;

void
__stdcall
BinkClose_Detour (DWORD unknown)
{
  BinkClose_Original (unknown);

  dll_log.Log (L" * Restoring TargetFPS -- Bink Video Closed");

  tzf::RenderFix::bink = false;
  tzf::FrameRateFix::End30FPSEvent ();
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

LPVOID pLuaReturn = nullptr;

void
__declspec(naked)
TZF_LuaHook (void)
{
  char   *name;
  size_t *pSz;
  char  **pBuffer;

  __asm
  {
    pushad
    pushfd

    // save Lua loader's stack frame because we need some stuff on it
    mov  ebx, ebp

    push ebp
    mov  ebp, esp
    sub  esp, __LOCAL_SIZE

    // I don't think this is actually luaL_loadbuffer, name shouldn't be passed in eax,
    // but we can get it here 100% of the time anyway
    // more insight definitely welcome
    mov  name, eax

    mov  eax, ebx
    add  eax, 0xC
    mov  pSz, eax
    mov  eax, ebx
    add  eax, 0x8
    mov  pBuffer, eax
  }

#if 0
  dll_log.Log (L"Lua script loaded: \"%S\"", name);
#endif

  if (! strcmp (name, "MEP_100_130_010_PF_Script")) {
    dll_log.Log (L" * Replacing priest script...");

    *pSz     = lua_bytecode_priest_size;
    *pBuffer = lua_bytecode_priest;
  }

  __asm
  {
    mov esp, ebp
    pop ebp

    popfd
    popad

    // overwritten instructions from original function
    mov ecx, [ebp + 0x8]
    mov [esp + 0x4], ecx

    jmp pLuaReturn
  }
}

void
tzf::FrameRateFix::Init (void)
{
  CommandProcessor* comm_proc = CommandProcessor::getInstance ();

  InitializeCriticalSectionAndSpinCount (&alter_speed_cs, 1000UL);

  target_fps = config.framerate.target;

  TZF_CreateDLLHook ( L"d3d9.dll", "BMF_SetPresentParamsD3D9",
                      BMF_SetPresentParamsD3D9_Detour, 
           (LPVOID *)&BMF_SetPresentParamsD3D9_Original,
                     &pfnBMF_SetPresentParamsD3D9 );

  bink_dll = LoadLibrary (L"bink2w32.dll");

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

  if (true) {
    if (*((DWORD *)config.framerate.speedresetcode_addr) != 0x428CB08D) {
      uint8_t sig [] = { 0x8D, 0xB0, 0x8C, 0x42,
                         0x00, 0x00, 0x8D, 0xBB,
                         0x8C, 0x42, 0x00, 0x00,
                         0xB9, 0x0B, 0x00, 0x00 };
      intptr_t addr = (intptr_t)TZF_Scan (sig, 16);

      if (addr != NULL) {
        dll_log.Log (L"Scanned SpeedResetCode Address: %06Xh", addr);
        config.framerate.speedresetcode_addr = addr;
      }
      else {
        dll_log.Log (L" >> ERROR: Unable to find SpeedResetCode memory!");
      }
    }

    if (*((DWORD *)config.framerate.speedresetcode3_addr) != 0x02) {
      uint8_t sig [] = { 0x0F, 0x95, 0xC0, 0x3A,
                         0xC3, 0x74, 0x17, 0xB8,
                         0x02, 0x00, 0x00, 0x00 };
      intptr_t addr = (intptr_t)TZF_Scan (sig, 12);

      if (addr != NULL && *((DWORD *)((uint8_t *)addr + 8)) == 0x2) {
        dll_log.Log (L"Scanned SpeedResetCode3 Address: %06Xh", addr + 8);
        config.framerate.speedresetcode3_addr = addr + 8;
      }
      else {
        dll_log.Log (L" >> ERROR: Unable to find SpeedResetCode3 memory!");
      }
    }

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
    VirtualProtect((LPVOID)config.framerate.speedresetcode_addr, 17, PAGE_EXECUTE_READWRITE, &dwOld);
               *((DWORD *)(config.framerate.speedresetcode_addr +  2)) += 8;
               *((DWORD *)(config.framerate.speedresetcode_addr +  8)) += 8;
               *((DWORD *)(config.framerate.speedresetcode_addr + 13)) -= 2;
    VirtualProtect((LPVOID)config.framerate.speedresetcode_addr, 17, dwOld, &dwOld);

    TZF_FlushInstructionCache ((LPCVOID)config.framerate.speedresetcode_addr, 17);

    uint8_t mask [] = { 0xff, 0xff, 0xff,            // cmp     [ebx+28h], eax
                        0,    0,                     // jz      short <...>
                        0xff, 0xff, 0xff,            // cmp     eax, 2
                        0,    0,                     // jl      short <...>
                        0xff, 0,    0,     0, 0,     // mov     <tickaddr>,  eax
                        0xff, 0,    0,     0, 0,     // mov     <tickaddr2>, eax
                        0xff, 0xff, 0xff             // mov     [ebx+28h],   eax
                      };

    uint8_t sig [] = { 0x39, 0x43, 0x28,             // cmp     [ebx+28h], eax
                       0x74, 0x12,                   // jz      short <...>
                       0x83, 0xF8, 0x02,             // cmp     eax, 2
                       0x7C, 0x0D,                   // jl      short <...>
                       0xA3, 0x64, 0xB4, 0x17, 0x02, // mov     <tickaddr>,  eax
                       0xA3, 0x68, 0xB4, 0x17, 0x02, // mov     <tickaddr2>, eax
                       0x89, 0x43, 0x28              // mov     [ebx+28h],   eax
                      };

    if (*((DWORD *)config.framerate.speedresetcode2_addr) != 0x0F8831274) {
      intptr_t addr = (intptr_t)TZF_Scan (sig, 23, mask);

      if (addr != NULL) {
        config.framerate.speedresetcode2_addr = addr + 3;

        dll_log.Log (L"Scanned SpeedResetCode2 Address: %06Xh", addr + 3);

        TICK_ADDR_BASE = *(DWORD *)((uint8_t *)(addr + 11));

        dll_log.Log (L" >> TICK_ADDR_BASE: %06Xh", TICK_ADDR_BASE);
      }
      else {
        dll_log.Log (L" >> ERROR: Unable to find SpeedResetCode2 memory!");
      }
    }

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
    uint8_t new_code [7] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90 };

    TZF_InjectByteCode ( (LPVOID)config.framerate.speedresetcode2_addr,
                           new_code,
                             7,
                               PAGE_EXECUTE_READWRITE,
                                 old_speed_reset_code2 );

    variable_speed_installed = true;

    // mov eax, 02 to mov eax, 01
    char scale [32];
    sprintf ( scale,
                "TickScale %li",
                  CalcTickScale (1000.0f * (1.0f / target_fps)) );
    command.ProcessCommandLine (scale);

    dll_log.LogEx ( false, L"Field=%lu FPS, Battle=%lu FPS (%s), Cutscene=%lu FPS\n",
                      target_fps,
                        config.framerate.battle_target,
                          config.framerate.battle_adaptive ? L"Adaptive" : L"Fixed",
                            config.framerate.cutscene_target );
  }

  variable_speed_installed = true;

  if (config.framerate.disable_limiter) {
    DWORD dwOld;

    // Replace the original jump (jb) with an unconditional jump (jmp)
    uint8_t new_code [6] = { 0xE9, 0x8B, 0x00, 0x00, 0x00, 0x90 };

    if (*(DWORD *)config.framerate.limiter_branch_addr != 0x8A820F) {
      uint8_t sig [] = { 0x53,                           // push    ebx
                         0x56,                           // push    esi
                         0x57,                           // push    edi
                         0x0F, 0x82, 0x8A, 0x0, 0x0, 0x0 // jb      <dontcare>
                       };
      intptr_t addr = (intptr_t)TZF_Scan (sig, 9);

      if (addr != NULL) {
        config.framerate.limiter_branch_addr = addr + 3;

        dll_log.Log (L"Scanned Limiter Branch Address: %06Xh", addr + 3);
      }
      else {
        dll_log.Log (L" >> ERROR: Unable to find LimiterBranchAddr memory!");
      }
    }

    TZF_InjectByteCode ( (LPVOID)config.framerate.limiter_branch_addr,
                           new_code,
                             6,
                               PAGE_EXECUTE_READWRITE );
  }

  if (config.lua.fix_priest) {
    uint8_t sig[14] = {  0x8B, 0x4D, 0x08,        // mov ecx, [ebp+08]
                         0x89, 0x4C, 0x24, 0x04,  // mov [esp+04], ecx
                         0x8B, 0x4D, 0x0C,        // mov ecx, [ebp+0C]
                         0x89, 0x4C, 0x24, 0x08   // mov [esp+08], ecx
                      };
    uint8_t new_code[7] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90 };

    void *addr = TZF_Scan (sig, 14);

    if (addr != NULL) {
      dll_log.Log (L"Scanned Lua Loader Address: %06Xh", addr);

      DWORD hookOffset = (PtrToUlong (&TZF_LuaHook) - (intptr_t)addr - 5);
      memcpy (new_code + 1, &hookOffset, 4);
      TZF_InjectByteCode (addr, new_code, 7, PAGE_EXECUTE_READWRITE);
      pLuaReturn = (LPVOID)((intptr_t)addr + 7);
    }
    else {
      dll_log.Log (L" >> ERROR: Unable to find Lua loader address! Priest bug will occur.");
    }
  }

  command.AddVariable ("TargetFPS",      new eTB_VarStub <int>  ( (int *)&config.framerate.target));
  command.AddVariable ("BattleFPS",      new eTB_VarStub <int>  ( (int *)&config.framerate.battle_target));
  command.AddVariable ("BattleAdaptive", new eTB_VarStub <bool> ((bool *)&config.framerate.battle_adaptive));
  command.AddVariable ("CutsceneFPS",    new eTB_VarStub <int>  ( (int *)&config.framerate.cutscene_target));

  // No matter which technique we use, these things need to be options
  command.AddVariable ("MinimizeLatency",   new eTB_VarStub <bool>  (&config.framerate.minimize_latency));

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
  FreeLibrary (kernel32_dll);
  FreeLibrary (bink_dll);

  ////TZF_DisableHook (pfnBMF_SetPresentParamsD3D9);

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

  ////TZF_DisableHook (pfnSleep);

  if (! config.framerate.stutter_fix)
    return;

  ////TZF_DisableHook (pfnQueryPerformanceCounter);

  stutter_fix_installed = false;
}

static int scale_before = 2;
static int fps_before   = 60;
bool       forced_30    = false;

void
tzf::FrameRateFix::Begin30FPSEvent (void)
{
  EnterCriticalSection (&alter_speed_cs);

  if (variable_speed_installed/* && (! forced_30)*/) {
    forced_30    = true;
    fps_before   = target_fps;
    target_fps   = 30;
    scale_before = tick_scale;
    command.ProcessCommandLine ("TickScale 2");
  }

  LeaveCriticalSection (&alter_speed_cs);
}

void
tzf::FrameRateFix::End30FPSEvent (void)
{
  EnterCriticalSection (&alter_speed_cs);

  if (variable_speed_installed/* && (forced_30)*/) {
    forced_30  = false;
    target_fps = fps_before;
    char szRescale [32];
    sprintf (szRescale, "TickScale %i", scale_before);
    command.ProcessCommandLine (szRescale);
  }

  LeaveCriticalSection (&alter_speed_cs);
}

void
tzf::FrameRateFix::SetFPS (int fps)
{
  EnterCriticalSection (&alter_speed_cs);

  if (variable_speed_installed && (target_fps != fps)) {
    target_fps = fps;
    char szRescale [32];
    sprintf (szRescale, "TickScale %i", CalcTickScale (1000.0f * (1.0f / fps)));
    command.ProcessCommandLine (szRescale);
  }

  LeaveCriticalSection (&alter_speed_cs);
}




bool use_accumulator = false;
bool floating_target = true;

int  max_latency     = 1;//2
bool wait_for_vblank = false;

tzf::FrameRateFix::CommandProcessor* tzf::FrameRateFix::CommandProcessor::pCommProc;

tzf::FrameRateFix::CommandProcessor::CommandProcessor (void)
{
  tick_scale_ = new eTB_VarStub <int> (&tick_scale, this);

  command.AddVariable ("TickScale", tick_scale_);

  command.AddVariable ("UseAccumulator",  new eTB_VarStub <bool> (&use_accumulator));
  command.AddVariable ("MaxFrameLatency", new eTB_VarStub <int>  (&max_latency));
  command.AddVariable ("WaitForVBLANK",   new eTB_VarStub <bool> (&wait_for_vblank));
}

bool
tzf::FrameRateFix::CommandProcessor::OnVarChange (eTB_Variable* var, void* val)
{
  DWORD dwOld;

  if (var == tick_scale_) {
    DWORD original0 = *((DWORD *)(TICK_ADDR_BASE    ));
    DWORD original1 = *((DWORD *)(TICK_ADDR_BASE + 4));

    if (val != nullptr) {
      VirtualProtect ((LPVOID)TICK_ADDR_BASE, 8, PAGE_READWRITE, &dwOld);

      if (variable_speed_installed) {
        // Battle Tickrate
        *(DWORD *)(TICK_ADDR_BASE) = *(DWORD *)val;
      }

      if (variable_speed_installed) {
        // World Tickrate
        *(DWORD *)(TICK_ADDR_BASE + 4) = *(DWORD *)val;
      }
      *(DWORD *)val = *(DWORD *)(TICK_ADDR_BASE + 4);

      VirtualProtect ((LPVOID)TICK_ADDR_BASE, 8, dwOld, &dwOld);

      if (variable_speed_installed) {
        // mov eax, 02 to mov eax, <val>
        VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, PAGE_EXECUTE_READWRITE, &dwOld);
                      *(DWORD *)config.framerate.speedresetcode3_addr = *(DWORD *)val;
        VirtualProtect ((LPVOID)config.framerate.speedresetcode3_addr, 4, dwOld, &dwOld);

        uint8_t new_code [7] = { 0xB8, (uint8_t)*(DWORD *)val, 0x00, 0x00, 0x00, 0x90, 0x90 };

        TZF_InjectByteCode ( (LPVOID)config.framerate.speedresetcode2_addr,
                               new_code,
                                 7,
                                   PAGE_EXECUTE_READWRITE,
                                     old_speed_reset_code2 );
      }
      //InterlockedExchange ((DWORD *)val, *(DWORD *)config.framerate.speedresetcode3_addr);

      tick_scale      = *(int32_t *)val;
    }
  }

  return true;
}



long
tzf::FrameRateFix::CalcTickScale (double elapsed_ms)
{
  const double tick_ms  = (1.0 / 60.0) * 1000.0;
  const double inv_rate =  1.0 / target_fps;

  long scale = min (max (elapsed_ms / tick_ms, 1), 7);

  if (scale > 6)
    scale = inv_rate / (1.0 / 60.0);

  return scale;
}

class FramerateLimiter
{
public:
  FramerateLimiter (double target = 60.0) {
     init (target);
   }
  ~FramerateLimiter (void) {
   }

  void init (double target) {
    ms  = 1000.0 / target;
    fps = target;

    frames = 0;

    IDirect3DDevice9Ex* d3d9ex = nullptr;
    if (tzf::RenderFix::pDevice != nullptr) {
      tzf::RenderFix::pDevice->QueryInterface ( 
                         __uuidof (IDirect3DDevice9Ex),
                           (void **)&d3d9ex );
    }

    QueryPerformanceFrequency (&freq);

    // Align the start to VBlank for minimum input latency
    if (d3d9ex != nullptr) {
      d3d9ex->SetMaximumFrameLatency (max_latency);
      d3d9ex->WaitForVBlank          (0);
      d3d9ex->Release                ();
    }

    QueryPerformanceCounter   (&start);

    next.QuadPart = 0ULL;
    time.QuadPart = 0ULL;
    last.QuadPart = 0ULL;

    last.QuadPart = start.QuadPart - (ms / 1000.0) * freq.QuadPart;
  }

  void wait (void) {
    frames++;

    QueryPerformanceCounter (&time);

    last.QuadPart = time.QuadPart;
    next.QuadPart = (start.QuadPart + frames * (ms / 1000.0) * freq.QuadPart);

    if (next.QuadPart > 0ULL) {
      // If available (Windows 7+), wait on the swapchain
      IDirect3DDevice9Ex* d3d9ex = nullptr;
      if (tzf::RenderFix::pDevice != nullptr) {
        tzf::RenderFix::pDevice->QueryInterface ( 
                           __uuidof (IDirect3DDevice9Ex),
                             (void **)&d3d9ex );
      }

      while (time.QuadPart < next.QuadPart) {
        if (wait_for_vblank) {
          if (d3d9ex != nullptr)
            d3d9ex->WaitForVBlank (0);
        }
        QueryPerformanceCounter (&time);
      }

      if (d3d9ex != nullptr)
        d3d9ex->Release ();
    }

    else {
      start.QuadPart += -next.QuadPart;
    }
  }

  void change_limit (double target) {
    init (target);
  }

private:
    double ms, fps;

    LARGE_INTEGER start, last, next, time, freq;

    uint32_t frames;
} *limiter = nullptr;

bool loading = false;

void
tzf::FrameRateFix::RenderTick (void)
{
  //if (! forced_30) {
  if (config.framerate.cutscene_target != config.framerate.target)
    if (game_state.inCutscene ())
      SetFPS (config.framerate.cutscene_target);

  if (config.framerate.battle_target != config.framerate.target)
    if (game_state.inBattle ())
      SetFPS (config.framerate.battle_target);

  if (tzf::RenderFix::bink)
    SetFPS (30);
  else if (! (game_state.inBattle () || game_state.inCutscene ()))
    SetFPS (config.framerate.target);


  static long last_scale = 1;

  static LARGE_INTEGER last_time  = { 0 };
  static LARGE_INTEGER freq       = { 0 };

  LARGE_INTEGER time;

  QueryPerformanceFrequency (&freq);
  QueryPerformanceCounter   (&time);

  const double inv_rate = 1.0 / target_fps;
  const double epsilon  = 0.0;


  static uint32_t last_limit = target_fps;

  if (limiter == nullptr)
    limiter = new FramerateLimiter ();

  if (last_limit != target_fps) {
    limiter->change_limit (target_fps);
    last_limit = target_fps;
  }

  limiter->wait ();

  QueryPerformanceCounter (&time);


  //if (forced_30) {
    //last_time.QuadPart = time.QuadPart;
    //return;
  //}


  double dt = ((double)(time.QuadPart - last_time.QuadPart) / (double)freq.QuadPart) / (1.0 / 60.0);


#ifdef ACCUMULATOR
  static double accum = 0.0;

  if (use_accumulator) {
    accum +=  dt - floor (dt);

    time.QuadPart +=
      (dt - floor (dt)) * (1.0 / 60.0) * freq.QuadPart;

    dt    -= (dt - floor (dt));

    if (accum >= 1.0) {
      time.QuadPart -=
          (accum - (accum - floor (accum))) * (1.0 / 60.0) * freq.QuadPart;
      dt    +=  accum - (accum - floor (accum));
      accum -=  accum - (accum - floor (accum));
    }
  } else {
    accum = 0.0;
  }
#endif

  long scale = CalcTickScale (dt * (1.0 / 60.0) * 1000.0);

  static bool last_frame_battle   = false;
  static int  scale_before_battle = 0;

  if (scale != last_scale) {
    if (config.framerate.auto_adjust || (config.framerate.battle_adaptive && game_state.inBattle ())) {
      if (config.framerate.battle_adaptive && game_state.inBattle ()) {
        if (! last_frame_battle) {
          scale_before_battle = last_scale;
          last_frame_battle   = true;
        }

#if 0
        dll_log.Log ( L" ** Adjusting TickScale because of battle framerate change "
                      L"(Expected: ~%4.2f ms, Got: %4.2f ms)",
                        last_scale * 16.666667f, dt * (1.0 / 60.0) * 1000.0 );
#endif
      }

      char rescale [32];
      sprintf (rescale, "TickScale %li", scale);
      command.ProcessCommandLine (rescale);
    }
  }

  if (last_frame_battle && (! game_state.inBattle ())) {
    char rescale [32];
    sprintf (rescale, "TickScale %li", scale_before_battle);
    command.ProcessCommandLine (rescale);
  }

  if (! game_state.inBattle ())
    last_frame_battle = false;

  last_scale = scale;

  last_time.QuadPart = time.QuadPart;
}