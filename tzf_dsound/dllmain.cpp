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

#pragma comment (lib, "kernel32.lib")

DWORD
WINAPI
DllThread (LPVOID user)
{
  if (TZF_Init_MinHook () == MH_OK) {
    tzf::SoundFix::Init     ();
    tzf::FileIO::Init       ();
    tzf::SteamFix::Init     ();
    tzf::RenderFix::Init    ();
    tzf::FrameRateFix::Init ();
    tzf::KeyboardFix::Init  ();
  }

  return 0;
}

HMODULE hDLLMod = { 0 };

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

    dll_log.init ("logs/tzfix.log", "w");
    dll_log.Log  (L"tzfix.log created");

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
      config.framerate.battle_adaptive      = true;
      config.framerate.cutscene_target      = 30;

      config.file_io.capture                = false;

      config.steam.allow_broadcasts         = false;

      config.lua.fix_priest                 = true;

      config.render.aspect_ratio            = 1.777778f;
      config.render.fovy                    = 0.785398f;

      config.render.aspect_addr             = 0x00D56494;
      config.render.fovy_addr               = 0x00D56498;
      config.render.blackbar_videos         = true;
      config.render.aspect_correction       = false;
      config.render.complete_mipmaps        = false;
      config.render.postproc_ratio          =  1.0f;
      config.render.shadow_rescale          = -2;
      config.render.env_shadow_rescale      =  0;
      config.render.clear_blackbars         = true;

      // Save a new config if none exists
      TZF_SaveConfig ();
    }

    // Locate the gamestate address; having this as the first thing in the log
    //   file is tremendously handy in identifying which client version a user
    //     is running.
    {
      uint8_t  sig [] = { 0x74, 0x42, 0xB1, 0x01, 0x38, 0x1D };
      intptr_t addr   = (intptr_t)TZF_Scan (sig, 6);

      if (addr != NULL) {
        game_state.base_addr = (BYTE *)(*(DWORD *)(addr + 6) - 0x13);
        dll_log.Log (L"Scanned Gamestate Address: %06Xh", game_state.base_addr);
      }
    }

    HANDLE hThread = CreateThread (NULL, NULL, DllThread, 0, 0, NULL);

    // Initialization delay
    if (hThread != 0)
      WaitForSingleObject (hThread, 250UL);
  } break;

  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    tzf::SoundFix::Shutdown     ();
    tzf::FileIO::Shutdown       ();
    tzf::SteamFix::Shutdown     ();
    tzf::RenderFix::Shutdown    ();
    tzf::FrameRateFix::Shutdown ();
    tzf::KeyboardFix::Shutdown  ();

    TZF_UnInit_MinHook ();
    TZF_SaveConfig     ();

    dll_log.LogEx      (true,  L"Closing log file... ");
    dll_log.close      ();
    break;
  }

  return TRUE;
}