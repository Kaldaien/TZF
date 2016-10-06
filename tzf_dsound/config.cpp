/*
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

#include "config.h"
#include "parameter.h"
#include "ini.h"
#include "log.h"

#include "DLL_VERSION.H"

extern HMODULE hInjectorDLL;

std::wstring TZF_VER_STR = TZF_VERSION_STR_W;
std::wstring DEFAULT_BK2 = L"RAW\\MOVIE\\AM_TOZ_OP_001.BK2";

static
  iSK_INI*   dll_ini       = nullptr;
tzf_config_t config;

tzf::ParameterFactory g_ParameterFactory;

struct {
  tzf::ParameterInt*     sample_rate;
  tzf::ParameterInt*     channels;      // OBSOLETE
  tzf::ParameterBool*    compatibility;
  tzf::ParameterBool*    enable_fix;
} audio;

struct {
  tzf::ParameterBool*    allow_fake_sleep;
  tzf::ParameterBool*    yield_processor;
  tzf::ParameterBool*    minimize_latency;
  tzf::ParameterInt*     speedresetcode_addr;
  tzf::ParameterInt*     speedresetcode2_addr;
  tzf::ParameterInt*     speedresetcode3_addr;
  tzf::ParameterInt*     limiter_branch_addr;
  tzf::ParameterBool*    disable_limiter;
  tzf::ParameterBool*    auto_adjust;
  tzf::ParameterInt*     target;
  tzf::ParameterInt*     battle_target;
  tzf::ParameterBool*    battle_adaptive;
  tzf::ParameterInt*     cutscene_target;
} framerate;

struct {
  tzf::ParameterInt*     fovy_addr;
  tzf::ParameterInt*     aspect_addr;
  tzf::ParameterFloat*   fovy;
  tzf::ParameterFloat*   aspect_ratio;
  tzf::ParameterBool*    aspect_correct_vids;
  tzf::ParameterBool*    aspect_correction;
  tzf::ParameterInt*     rescale_shadows;
  tzf::ParameterInt*     rescale_env_shadows;
  tzf::ParameterFloat*   postproc_ratio;
  tzf::ParameterBool*    clear_blackbars;
} render;

struct {
  tzf::ParameterBool*    remaster;
  tzf::ParameterBool*    cache;
  tzf::ParameterBool*    dump;
  tzf::ParameterInt*     cache_size;
  tzf::ParameterInt*     worker_threads;
} textures;


struct {
  tzf::ParameterBool*    allow_broadcasts;
} steam;


struct {
  tzf::ParameterStringW* swap_keys;
} keyboard;


struct {
  tzf::ParameterBool*    fix_priest;
} lua;

struct {
  tzf::ParameterStringW* intro_video;
  tzf::ParameterStringW* version;
  tzf::ParameterStringW* injector;
} sys;

typedef const wchar_t* (__stdcall *SK_GetConfigPath_pfn)(void);
static SK_GetConfigPath_pfn SK_GetConfigPath = nullptr;

bool
TZF_LoadConfig (std::wstring name)
{
  SK_GetConfigPath =
    (SK_GetConfigPath_pfn)
      GetProcAddress (
        hInjectorDLL,
          "SK_GetConfigPath"
      );

  // Load INI File
  wchar_t wszFullName [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW (wszFullName, SK_GetConfigPath ());
  lstrcatW (wszFullName,       name.c_str ());
  lstrcatW (wszFullName,             L".ini");
  dll_ini = TZF_CreateINI (wszFullName);


  bool empty = dll_ini->get_sections ().empty ();

  //
  // Create Parameters
  //

  audio.channels =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Audio Channels")
      );
  audio.channels->register_to_ini (
    dll_ini,
      L"TZFIX.Audio",
        L"Channels" );

  audio.sample_rate =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Sample Rate")
      );
  audio.sample_rate->register_to_ini (
    dll_ini,
      L"TZFIX.Audio",
        L"SampleRate" );

  audio.compatibility =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Compatibility Mode")
      );
  audio.compatibility->register_to_ini (
    dll_ini,
      L"TZFIX.Audio",
        L"CompatibilityMode" );

  audio.enable_fix =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Enable Fix")
      );
  audio.enable_fix->register_to_ini (
    dll_ini,
      L"TZFIX.Audio",
        L"EnableFix" );


  framerate.allow_fake_sleep =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Allow Fake Sleep")
      );
  framerate.allow_fake_sleep->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"AllowFakeSleep" );

  framerate.yield_processor =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Yield Instead of Sleep")
      );
  framerate.yield_processor->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"YieldProcessor" );

  framerate.minimize_latency =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Minimize Input Latency")
      );
  framerate.minimize_latency->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"MinimizeLatency" );

  framerate.speedresetcode_addr =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Simulation Speed Reset Code Memory Address")
      );
  framerate.speedresetcode_addr->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"SpeedResetCode_Address" );

  framerate.speedresetcode2_addr =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Simulation Speed Reset Code 2 Memory Address")
      );
  framerate.speedresetcode2_addr->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"SpeedResetCode2_Address" );

  framerate.speedresetcode3_addr =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Simulation Speed Reset Code 3 Memory Address")
      );
  framerate.speedresetcode3_addr->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"SpeedResetCode3_Address" );

  framerate.limiter_branch_addr =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Framerate Limiter Branch Instruction")
      );
  framerate.limiter_branch_addr->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"LimiterBranch_Address" );

   framerate.disable_limiter =
     static_cast <tzf::ParameterBool *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Disable Namco's Limiter")
       );
   framerate.disable_limiter->register_to_ini (
     dll_ini,
       L"TZFIX.FrameRate",
         L"DisableNamcoLimiter" );

   framerate.auto_adjust =
     static_cast <tzf::ParameterBool *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Automatically Adjust TickScale")
       );
   framerate.auto_adjust->register_to_ini (
     dll_ini,
       L"TZFIX.FrameRate",
         L"AutoAdjust" );

   framerate.target =
     static_cast <tzf::ParameterInt *>
       (g_ParameterFactory.create_parameter <int> (
         L"Target FPS")
       );
   framerate.target->register_to_ini (
     dll_ini,
       L"TZFIX.FrameRate",
         L"Target" );

   framerate.battle_target =
     static_cast <tzf::ParameterInt *>
       (g_ParameterFactory.create_parameter <int> (
         L"Battle Target FPS")
       );
   framerate.battle_target->register_to_ini (
     dll_ini,
       L"TZFIX.FrameRate",
         L"BattleTarget" );

   framerate.battle_adaptive =
     static_cast <tzf::ParameterBool *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Battle Adaptive Scaling")
       );
   framerate.battle_adaptive->register_to_ini (
     dll_ini,
       L"TZFIX.FrameRate",
         L"BattleAdaptive" );

   framerate.cutscene_target =
     static_cast <tzf::ParameterInt *>
       (g_ParameterFactory.create_parameter <int> (
         L"Cutscene Target FPS")
       );
   framerate.cutscene_target->register_to_ini (
     dll_ini,
       L"TZFIX.FrameRate",
         L"CutsceneTarget" );


  render.aspect_ratio =
    static_cast <tzf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Aspect Ratio")
      );
  render.aspect_ratio->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"AspectRatio" );

  render.fovy =
    static_cast <tzf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Field of View Vertical")
      );
  render.fovy->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"FOVY" );

  render.aspect_addr =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Aspect Ratio Memory Address")
      );
  render.aspect_addr->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"AspectRatio_Address" );

  render.fovy_addr =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Field of View Vertical Address")
      );
  render.fovy_addr->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"FOVY_Address" );

   render.aspect_correct_vids =
     static_cast <tzf::ParameterBool *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Aspect Ratio Correct Videos")
       );
   render.aspect_correct_vids->register_to_ini (
     dll_ini,
       L"TZFIX.Render",
         L"AspectCorrectVideos" );

   render.aspect_correction =
     static_cast <tzf::ParameterBool *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Aspect Ratio Correct EVERYTHING")
       );
   render.aspect_correction->register_to_ini (
     dll_ini,
       L"TZFIX.Render",
         L"AspectCorrection" );

  render.rescale_shadows =
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Shadow Rescale Factor")
      );
  render.rescale_shadows->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"RescaleShadows" );

  render.postproc_ratio =
    static_cast <tzf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Postprocess Resolution Ratio")
      );
  render.postproc_ratio->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"PostProcessRatio" );

   render.rescale_env_shadows =
     static_cast <tzf::ParameterInt *>
       (g_ParameterFactory.create_parameter <int> (
         L"Rescale Enviornmental Shadows")
       );
   render.rescale_env_shadows->register_to_ini (
     dll_ini,
       L"TZFIX.Render",
         L"RescaleEnvShadows" );

   render.clear_blackbars =
     static_cast <tzf::ParameterBool *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Clear Blackbar Regions (Aspect Ratio Correction)")
       );
   render.clear_blackbars->register_to_ini (
     dll_ini,
       L"TZFIX.Render",
         L"ClearBlackbars" );


  textures.cache =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Cache Textures to Speed Up Menus (and Remaster)")
      );
  textures.cache->register_to_ini (
    dll_ini,
      L"TZFIX.Textures",
        L"Cache" );

  textures.dump =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Dump Textures as they are loaded")
      );
  textures.dump->register_to_ini (
    dll_ini,
      L"TZFIX.Textures",
        L"Dump" );

  textures.remaster =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Various Fixes to Eliminate Texture Aliasing")
      );
  textures.remaster->register_to_ini (
    dll_ini,
      L"TZFIX.Textures",
        L"Remaster" );

  textures.cache_size = 
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Size of Texture Cache")
      );
  textures.cache_size->register_to_ini (
    dll_ini,
      L"TZFIX.Textures",
        L"MaxCacheInMiB" );

  textures.worker_threads = 
    static_cast <tzf::ParameterInt *>
      (g_ParameterFactory.create_parameter <int> (
        L"Number of Worker Threads")
      );
  textures.worker_threads->register_to_ini (
    dll_ini,
      L"TZFIX.Textures",
        L"WorkerThreads" );


  keyboard.swap_keys =
    static_cast <tzf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Swap SDL Scancodes")
      );
  keyboard.swap_keys->register_to_ini (
    dll_ini,
      L"TZFIX.Keyboard",
        L"SwapKeys" );


  steam.allow_broadcasts =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Allow Steam Broadcasting")
      );
  steam.allow_broadcasts->register_to_ini(
    dll_ini,
      L"TZFIX.Steam",
        L"AllowBroadcasts" );


  lua.fix_priest =   
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Fix Lastonbell Priest")
      );
  lua.fix_priest->register_to_ini (
    dll_ini,
      L"TZFIX.Lua",
        L"FixPriest" );


  sys.version =
    static_cast <tzf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Software Version")
      );
  sys.version->register_to_ini (
    dll_ini,
      L"TZFIX.System",
        L"Version" );

  sys.intro_video =
    static_cast <tzf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"Intro Video")
      );
  sys.intro_video->register_to_ini (
    dll_ini,
      L"TZFIX.System",
        L"IntroVideo" );

  sys.injector =
    static_cast <tzf::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (
        L"DLL That Injected Us")
      );
  sys.injector->register_to_ini (
    dll_ini,
      L"TZFIX.System",
        L"Injector" );

  //
  // Load Parameters
  //
  audio.channels->load      ( (int &)config.audio.channels      );
  audio.sample_rate->load   ( (int &)config.audio.sample_hz     );
  audio.compatibility->load (        config.audio.compatibility );
  audio.enable_fix->load    (        config.audio.enable_fix    );

  framerate.allow_fake_sleep->load    (config.framerate.allow_fake_sleep);
  framerate.yield_processor->load     (config.framerate.yield_processor);
  framerate.minimize_latency->load    (config.framerate.minimize_latency);

  framerate.speedresetcode_addr->load  (
             (int &)config.framerate.speedresetcode_addr   );
  framerate.speedresetcode2_addr->load (
             (int &)config.framerate.speedresetcode2_addr  );
  framerate.speedresetcode3_addr->load (
              (int &)config.framerate.speedresetcode3_addr );
  framerate.limiter_branch_addr->load  (
              (int &)config.framerate.limiter_branch_addr  );

  framerate.disable_limiter->load (config.framerate.disable_limiter);
  framerate.auto_adjust->load     (config.framerate.auto_adjust);
  framerate.target->load          (config.framerate.target);
  framerate.battle_target->load   (config.framerate.battle_target);
  framerate.battle_adaptive->load (config.framerate.battle_adaptive);
  framerate.cutscene_target->load (config.framerate.cutscene_target);


  render.aspect_addr->load  ((int &)config.render.aspect_addr);
  render.fovy_addr->load    ((int &)config.render.fovy_addr);

  render.aspect_ratio->load (config.render.aspect_ratio);
  render.fovy->load         (config.render.fovy);

  render.aspect_correct_vids->load (config.render.blackbar_videos);
  render.aspect_correction->load   (config.render.aspect_correction);
  render.clear_blackbars->load     (config.render.clear_blackbars);

  // The video aspect ratio correction option is scheduled for removal anyway, so
  //   this hack is okay...
  if (config.render.clear_blackbars && config.render.aspect_correction)
    config.render.blackbar_videos = true;

  render.postproc_ratio->load      (config.render.postproc_ratio);
  render.rescale_shadows->load     (config.render.shadow_rescale);
  render.rescale_env_shadows->load (config.render.env_shadow_rescale);

  textures.remaster->load       (config.textures.remaster);
  textures.cache->load          (config.textures.cache);
  textures.dump->load           (config.textures.dump);
  textures.cache_size->load     (config.textures.max_cache_in_mib);
  textures.worker_threads->load (config.textures.worker_threads);

  steam.allow_broadcasts->load  (config.steam.allow_broadcasts);

  keyboard.swap_keys->load (config.keyboard.swap_keys);

  lua.fix_priest->load (config.lua.fix_priest);

  sys.version->load     (config.system.version);
  sys.intro_video->load (config.system.intro_video);
  sys.injector->load    (config.system.injector);

  if (empty)
    return false;

  return true;
}

void
TZF_SaveConfig (std::wstring name, bool close_config)
{
//audio.channels->store      (config.audio.channels);  // OBSOLETE

  audio.sample_rate->store   (config.audio.sample_hz);
  audio.compatibility->store (config.audio.compatibility);
  audio.enable_fix->store    (config.audio.enable_fix);

  framerate.allow_fake_sleep->store     (config.framerate.allow_fake_sleep);
  framerate.yield_processor->store      (config.framerate.yield_processor);
  framerate.minimize_latency->store     (config.framerate.minimize_latency);
  framerate.speedresetcode_addr->store  (config.framerate.speedresetcode_addr);
  framerate.speedresetcode2_addr->store (config.framerate.speedresetcode2_addr);
  framerate.speedresetcode3_addr->store (config.framerate.speedresetcode3_addr);
  framerate.limiter_branch_addr->store  (config.framerate.limiter_branch_addr);
  framerate.disable_limiter->store      (config.framerate.disable_limiter);
  framerate.auto_adjust->store          (config.framerate.auto_adjust);

  //
  // Don't store changes to this preference made while the game is running
  //
  //framerate.target->store          (config.framerate.target);
  //framerate.battle_target->store   (config.framerate.battle_target);
  //framerate.battle_adaptive->store (config.framerate.battle_adaptive);
  //framerate.cutscene_target->store (config.framerate.cutscene_target);

  render.aspect_addr->store   (config.render.aspect_addr);
  render.fovy_addr->store     (config.render.fovy_addr);
  render.aspect_ratio->store  (config.render.aspect_ratio);
  render.fovy->store          (config.render.fovy);

//render.aspect_correct_vids->store (config.render.blackbar_videos);
  render.aspect_correction->store   (config.render.aspect_correction);
  render.clear_blackbars->store     (config.render.clear_blackbars);

  render.postproc_ratio->store      (config.render.postproc_ratio);
  render.rescale_shadows->store     (config.render.shadow_rescale);
  render.rescale_env_shadows->store (config.render.env_shadow_rescale);

  textures.remaster->store       (config.textures.remaster);
  textures.cache->store          (config.textures.cache);
  textures.dump->store           (config.textures.dump);
  textures.cache_size->store     (config.textures.max_cache_in_mib);
  textures.worker_threads->store (config.textures.worker_threads);

  steam.allow_broadcasts->store  (config.steam.allow_broadcasts);

  lua.fix_priest->store          (config.lua.fix_priest);

  sys.version->store             (TZF_VER_STR);
  sys.intro_video->store         (config.system.intro_video);
  sys.injector->store            (config.system.injector);

  wchar_t wszFullName [ MAX_PATH + 2 ] = { L'\0' };

  lstrcatW ( wszFullName,
               SK_GetConfigPath () );
  lstrcatW ( wszFullName,
               name.c_str () );
  lstrcatW ( wszFullName,
               L".ini" );

  dll_ini->write (wszFullName);

  if (close_config) {
    if (dll_ini != nullptr) {
      delete dll_ini;
      dll_ini = nullptr;
    }
  }
}