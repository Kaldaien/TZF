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

std::wstring TZF_VER_STR = L"1.1.0";

static tzf::INI::File*  dll_ini = nullptr;

tzf_config_t config;

tzf::ParameterFactory g_ParameterFactory;

struct {
  tzf::ParameterInt*     sample_rate;
  tzf::ParameterInt*     channels;
  tzf::ParameterBool*    compatibility;
  tzf::ParameterBool*    enable_fix;
} audio;

struct {
  tzf::ParameterBool*    stutter_fix;
  tzf::ParameterFloat*   fudge_factor;
  tzf::ParameterBool*    allow_fake_sleep;
  tzf::ParameterBool*    yield_processor;
  tzf::ParameterBool*    allow_windowed_mode;
  tzf::ParameterBool*    minimize_latency;
  tzf::ParameterInt*     speedresetcode_addr;
  tzf::ParameterInt*     speedresetcode2_addr;
  tzf::ParameterInt*     speedresetcode3_addr;
  tzf::ParameterInt*     limiter_branch_addr;
  tzf::ParameterBool*    disable_limiter;
  tzf::ParameterBool*    auto_adjust;
} framerate;

struct {
  tzf::ParameterInt*     fovy_addr;
  tzf::ParameterInt*     aspect_addr;
  tzf::ParameterFloat*   fovy;
  tzf::ParameterFloat*   aspect_ratio;
  tzf::ParameterBool*    aspect_correct_vids;
  tzf::ParameterBool*    aspect_correction;
  tzf::ParameterBool*    complete_mipmaps;
  tzf::ParameterInt*     rescale_shadows;
  tzf::ParameterInt*     rescale_env_shadows;
  tzf::ParameterFloat*   postproc_ratio;
  tzf::ParameterBool*    disable_scissor;
} render;


struct {
  tzf::ParameterBool*    allow_broadcasts;
} steam;

struct {
  tzf::ParameterStringW* intro_video;
  tzf::ParameterStringW* version;
} sys;


bool
TZF_LoadConfig (std::wstring name) {
  // Load INI File
  std::wstring full_name = name + L".ini";
  dll_ini = new tzf::INI::File ((wchar_t *)full_name.c_str ());

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


  framerate.stutter_fix =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Stutter Elimination")
      );
  framerate.stutter_fix->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"FixStutter" );

  framerate.fudge_factor =
    static_cast <tzf::ParameterFloat *>
      (g_ParameterFactory.create_parameter <float> (
        L"Fudge Factor")
      );
  framerate.fudge_factor->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"FudgeFactor" );

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

  framerate.allow_windowed_mode =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Allow Windowed Mode")
      );
  framerate.allow_windowed_mode->register_to_ini (
    dll_ini,
      L"TZFIX.FrameRate",
        L"AllowWindowedMode" );

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

  render.complete_mipmaps =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Force Complete Mipmaps (FULL MipMap Creation)")
      );
  render.complete_mipmaps->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"CompleteMipmaps" );

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

  render.disable_scissor =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Disable Scissor Rectangle")
      );
  render.disable_scissor->register_to_ini (
    dll_ini,
      L"TZFIX.Render",
        L"DisableScissor" );

   render.rescale_env_shadows =
     static_cast <tzf::ParameterInt *>
       (g_ParameterFactory.create_parameter <bool> (
         L"Rescale Enviornmental Shadows")
       );
   render.rescale_env_shadows->register_to_ini (
     dll_ini,
       L"TZFIX.Render",
         L"RescaleEnvShadows" );


  steam.allow_broadcasts =
    static_cast <tzf::ParameterBool *>
      (g_ParameterFactory.create_parameter <bool> (
        L"Allow Steam Broadcasting")
      );
  steam.allow_broadcasts->register_to_ini(
    dll_ini,
      L"TZFIX.Steam",
        L"AllowBroadcasts" );

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

  //
  // Load Parameters
  //
  if (audio.channels->load ())
    config.audio.channels = audio.channels->get_value ();

  if (audio.sample_rate->load ())
    config.audio.sample_hz = audio.sample_rate->get_value ();

  if (audio.compatibility->load ())
    config.audio.compatibility = audio.compatibility->get_value ();

  if (audio.enable_fix->load ())
    config.audio.enable_fix = audio.enable_fix->get_value ();


  if (framerate.stutter_fix->load ())
    config.framerate.stutter_fix = framerate.stutter_fix->get_value ();

  if (framerate.fudge_factor->load ())
    config.framerate.fudge_factor = framerate.fudge_factor->get_value ();

  if (framerate.allow_fake_sleep->load ())
    config.framerate.allow_fake_sleep = framerate.allow_fake_sleep->get_value ();

  if (framerate.yield_processor->load ())
    config.framerate.yield_processor = framerate.yield_processor->get_value ();

  if (framerate.allow_windowed_mode->load ())
    config.framerate.allow_windowed_mode = framerate.allow_windowed_mode->get_value ();

  if (framerate.minimize_latency->load ())
    config.framerate.minimize_latency = framerate.minimize_latency->get_value ();

  if (framerate.speedresetcode_addr->load ())
    config.framerate.speedresetcode_addr = framerate.speedresetcode_addr->get_value ();

  if (framerate.speedresetcode2_addr->load ())
    config.framerate.speedresetcode2_addr = framerate.speedresetcode2_addr->get_value ();

  if (framerate.speedresetcode3_addr->load ())
    config.framerate.speedresetcode3_addr = framerate.speedresetcode3_addr->get_value ();

  if (framerate.limiter_branch_addr->load ())
    config.framerate.limiter_branch_addr = framerate.limiter_branch_addr->get_value ();

  if (framerate.disable_limiter->load ())
    config.framerate.disable_limiter = framerate.disable_limiter->get_value ();

  if (framerate.auto_adjust->load ())
    config.framerate.auto_adjust = framerate.auto_adjust->get_value ();



  if (render.aspect_addr->load ())
    config.render.aspect_addr = render.aspect_addr->get_value ();

  if (render.fovy_addr->load ())
    config.render.fovy_addr = render.fovy_addr->get_value ();

  if (render.aspect_ratio->load ())
    config.render.aspect_ratio = render.aspect_ratio->get_value ();

  if (render.fovy->load ())
    config.render.fovy = render.fovy->get_value ();

  if (render.aspect_correct_vids->load ())
    config.render.blackbar_videos = render.aspect_correct_vids->get_value ();

  if (render.aspect_correction->load ())
    config.render.aspect_correction = render.aspect_correction->get_value ();

  if (render.complete_mipmaps->load ())
    config.render.complete_mipmaps = render.complete_mipmaps->get_value ();

  if (render.postproc_ratio->load ())
     config.render.postproc_ratio = render.postproc_ratio->get_value ();

  if (render.rescale_shadows->load ())
    config.render.shadow_rescale = render.rescale_shadows->get_value ();

  if (render.disable_scissor->load ())
    config.render.disable_scissor = render.disable_scissor->get_value ();

  if (render.rescale_env_shadows->load ())
    config.render.env_shadow_rescale = render.rescale_env_shadows->get_value ();


  if (steam.allow_broadcasts->load ())
    config.steam.allow_broadcasts = steam.allow_broadcasts->get_value ();

  if (sys.version->load ())
    config.system.version = sys.version->get_value ();

  //if (sys.intro_video->load ())
    //config.system.intro_video = sys.intro_video->get_value ();

  if (empty)
    return false;

  return true;
}

void
TZF_SaveConfig (std::wstring name, bool close_config) {
  audio.channels->set_value    (config.audio.channels);
  audio.channels->store        ();

  audio.sample_rate->set_value (config.audio.sample_hz);
  audio.sample_rate->store     ();

  audio.compatibility->set_value (config.audio.compatibility);
  audio.compatibility->store     ();

  audio.enable_fix->set_value (config.audio.enable_fix);
  audio.enable_fix->store     ();


  framerate.stutter_fix->set_value (config.framerate.stutter_fix);
  framerate.stutter_fix->store     ();

  framerate.fudge_factor->set_value (config.framerate.fudge_factor);
  framerate.fudge_factor->store     ();

  framerate.allow_fake_sleep->set_value (config.framerate.allow_fake_sleep);
  framerate.allow_fake_sleep->store     ();

  framerate.yield_processor->set_value (config.framerate.yield_processor);
  framerate.yield_processor->store     ();

  framerate.allow_windowed_mode->set_value (config.framerate.allow_windowed_mode);
  framerate.allow_windowed_mode->store     ();

  framerate.minimize_latency->set_value (config.framerate.minimize_latency);
  framerate.minimize_latency->store     ();

  framerate.speedresetcode_addr->set_value (config.framerate.speedresetcode_addr);
  framerate.speedresetcode_addr->store     ();

  framerate.speedresetcode2_addr->set_value (config.framerate.speedresetcode2_addr);
  framerate.speedresetcode2_addr->store     ();

  framerate.speedresetcode3_addr->set_value (config.framerate.speedresetcode3_addr);
  framerate.speedresetcode3_addr->store     ();

  framerate.limiter_branch_addr->set_value (config.framerate.limiter_branch_addr);
  framerate.limiter_branch_addr->store     ();

  framerate.disable_limiter->set_value (config.framerate.disable_limiter);
  framerate.disable_limiter->store     ();

  framerate.auto_adjust->set_value (config.framerate.auto_adjust);
  framerate.auto_adjust->store     ();


  render.aspect_addr->set_value (config.render.aspect_addr);
  render.aspect_addr->store     ();

  render.fovy_addr->set_value (config.render.fovy_addr);
  render.fovy_addr->store     ();

  render.aspect_ratio->set_value (config.render.aspect_ratio);
  render.aspect_ratio->store     ();

  render.fovy->set_value (config.render.fovy);
  render.fovy->store     ();

  render.aspect_correct_vids->set_value (config.render.blackbar_videos);
  render.aspect_correct_vids->store     ();

  render.aspect_correction->set_value (config.render.aspect_correction);
  render.aspect_correction->store     ();

  render.complete_mipmaps->set_value (config.render.complete_mipmaps);
  render.complete_mipmaps->store     ();

  render.postproc_ratio->set_value (config.render.postproc_ratio);
  render.postproc_ratio->store     ();

  render.rescale_shadows->set_value (config.render.shadow_rescale);
  render.rescale_shadows->store     ();

  render.disable_scissor->set_value (config.render.disable_scissor);
  render.disable_scissor->store     ();

  render.rescale_env_shadows->set_value (config.render.env_shadow_rescale);
  render.rescale_env_shadows->store     ();



  steam.allow_broadcasts->set_value (config.steam.allow_broadcasts);
  steam.allow_broadcasts->store     ();

  sys.version->set_value       (TZF_VER_STR);
  sys.version->store           ();

  //sys.intro_video->set_value   (config.system.intro_video);
  //sys.intro_video->store       ();

  dll_ini->write (name + L".ini");

  if (close_config) {
    if (dll_ini != nullptr) {
      delete dll_ini;
      dll_ini = nullptr;
    }
  }
}