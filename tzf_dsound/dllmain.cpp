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

#include "config.h"
#include "log.h"

#include "sound.h"
#include "framerate.h"
#include "general_io.h"
#include "keyboard.h"
#include "steam.h"
#include "render.h"
#include "scanner.h"

#include "command.h"

#include "hook.h"

#include <process.h>

#pragma comment (lib, "kernel32.lib")

HMODULE hDLLMod      = { 0 }; // Handle to SELF
HMODULE hInjectorDLL = { 0 }; // Handle to Special K

std::wstring injector_dll;

typedef void (__stdcall *SK_SetPluginName_pfn)(std::wstring name);
SK_SetPluginName_pfn SK_SetPluginName = nullptr;

extern void TZF_InitCompatBlacklist (void);

unsigned int
WINAPI
DllThread (LPVOID user)
{
  std::wstring plugin_name = L"Tales of Zestiria \"Fix\" v " + TZF_VER_STR;

  dll_log = TZF_CreateLog (L"logs/tzfix.log");

  dll_log->LogEx ( false, L"------- [Tales of Zestiria  \"Fix\"] "
                          L"-------\n" ); // <--- I was bored ;)
  dll_log->Log   (        L"tzfix.dll Plug-In\n"
                          L"=========== (Version: v %s) "
                          L"===========",
                            TZF_VER_STR.c_str () );

  DWORD speedresetcode_addr  = 0x0046C0F9; //0x0046C529;
  DWORD speedresetcode2_addr = 0x0056EB41; //0x0056E441;  0x217B464
  DWORD speedresetcode3_addr = 0x0056E03E; //0x0056D93F;
  DWORD limiter_branch_addr  = 0x00990F53; //0x00990873;
  DWORD aspect_addr          = 0x00D52388; //0x00D52398;
  DWORD fovy_addr            = 0x00D5238C; //0x00D5239C;

  if (! TZF_LoadConfig ()) {
    config.audio.channels                 = 8;
    config.audio.sample_hz                = 48000;
    config.audio.compatibility            = false;
    config.audio.enable_fix               = true;

    config.framerate.allow_fake_sleep     = false;
    config.framerate.yield_processor      = true;
    config.framerate.minimize_latency     = false;
    config.framerate.speedresetcode_addr  = 0x0046C0F9;
    config.framerate.speedresetcode2_addr = 0x0056EB41;
    config.framerate.speedresetcode3_addr = 0x0056E03E;
    config.framerate.limiter_branch_addr  = 0x00990873;
    config.framerate.disable_limiter      = true;
    config.framerate.auto_adjust          = false;
    config.framerate.target               = 60;
    config.framerate.battle_target        = 60;
    config.framerate.battle_adaptive      = false;
    config.framerate.cutscene_target      = 30;

    config.file_io.capture                = false;

    config.steam.allow_broadcasts         = false;

    config.lua.fix_priest                 = true;

    config.render.aspect_ratio            = 1.777778f;
    config.render.fovy                    = 0.785398f;

    config.render.aspect_addr             = 0x00D56494;
    config.render.fovy_addr               = 0x00D56498;
    config.render.blackbar_videos         = true;
    config.render.aspect_correction       = true;
    config.render.postproc_ratio          =  1.0f;
    config.render.shadow_rescale          = -2;
    config.render.env_shadow_rescale      =  0;
    config.render.clear_blackbars         = true;

    config.textures.remaster              = true;
    config.textures.dump                  = false;
    config.textures.cache                 = true;

    config.system.injector = injector_dll;

    // Save a new config if none exists
    TZF_SaveConfig ();
  }

  config.system.injector = injector_dll;

  SK_SetPluginName = 
    (SK_SetPluginName_pfn)
      GetProcAddress (hInjectorDLL, "SK_SetPluginName");
  SK_GetCommandProcessor =
    (SK_GetCommandProcessor_pfn)
      GetProcAddress (hInjectorDLL, "SK_GetCommandProcessor");

  //
  // If this is NULL, the injector system isn't working right!!!
  //
  if (SK_SetPluginName != nullptr)
    SK_SetPluginName (plugin_name);

  // Locate the gamestate address; having this as the first thing in the log
  //   file is tremendously handy in identifying which client version a user
  //     is running.
  {
    uint8_t  sig [] = { 0x74, 0x42, 0xB1, 0x01, 0x38, 0x1D };
    uintptr_t addr  = (uintptr_t)TZF_Scan (sig, 6);

    if (addr != NULL) {
      game_state.base_addr = (BYTE *)(*(DWORD *)(addr + 6) - 0x13);
      dll_log->Log (L"[ Sig Scan ] Scanned Gamestate Address: %06Xh", game_state.base_addr);
    }
  }

  if (TZF_Init_MinHook () == MH_OK) {
    CoInitialize (nullptr);

    tzf::SoundFix::Init     ();
    tzf::FileIO::Init       ();
    tzf::SteamFix::Init     ();
    tzf::RenderFix::Init    ();
    tzf::FrameRateFix::Init ();
    tzf::KeyboardFix::Init  ();
  }

  return 0;
}

__declspec (dllexport)
BOOL
WINAPI
SKPlugIn_Init (HMODULE hModSpecialK)
{
  wchar_t wszSKFileName [MAX_PATH];
          wszSKFileName [MAX_PATH - 1] = L'\0';

  GetModuleFileName (hModSpecialK, wszSKFileName, MAX_PATH - 1);

  injector_dll = wszSKFileName;

  hInjectorDLL = hModSpecialK;

#if 1
  DllThread (nullptr);
#else
  _beginthreadex ( nullptr, 0, DllThread, nullptr, 0x00, nullptr );
#endif

  return TRUE;
}

BOOL
APIENTRY
DllMain (HMODULE hModule,
         DWORD    ul_reason_for_call,
         LPVOID  /* lpReserved */)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      hDLLMod = hModule;
    } break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
    {
      if (dll_log != nullptr) {
        tzf::SoundFix::Shutdown     ();
        tzf::FileIO::Shutdown       ();
        tzf::SteamFix::Shutdown     ();
        tzf::RenderFix::Shutdown    ();
        tzf::FrameRateFix::Shutdown ();
        tzf::KeyboardFix::Shutdown  ();

        TZF_UnInit_MinHook ();
        TZF_SaveConfig     ();


        dll_log->LogEx ( false, L"=========== (Version: v %s) "
                                L"===========\n",
                                  TZF_VER_STR.c_str () );
        dll_log->LogEx ( true,  L"End TZFix Plug-In\n" );
        dll_log->LogEx ( false, L"------- [Tales of Zestiria  \"Fix\"] "
                                L"-------\n" );

        dll_log->close ();
      }
    } break;
  }

  return TRUE;
}