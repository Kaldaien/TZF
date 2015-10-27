/**
* This file is part of Batman "Fix".
*
* Batman "Fix" is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* The Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Batman "Fix" is distributed in the hope that it will be useful,
* But WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Batman "Fix". If not, see <http://www.gnu.org/licenses/>.
**/

#include "config.h"
#include "parameter.h"
#include "ini.h"
#include "log.h"

std::wstring TZF_VER_STR = L"0.1.3";

static tzf::INI::File*  dll_ini = nullptr;

tzf_config_t config;

tzf::ParameterFactory g_ParameterFactory;

struct {
  tzf::ParameterInt*     sample_rate;
  tzf::ParameterInt*     channels;
  tzf::ParameterBool*    compatibility;
} audio;

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
      (g_ParameterFactory.create_parameter <int> (
        L"Compatibility Mode")
      );
  audio.compatibility->register_to_ini (
    dll_ini,
      L"TZFIX.Audio",
        L"CompatibilityMode" );

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