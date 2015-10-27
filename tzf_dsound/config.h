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
#ifndef __TZF__CONFIG_H__
#define __TZF__CONFIG_H__

#include <Windows.h>
#include <string>

extern std::wstring TZF_VER_STR;

struct tzf_config_t
{
  struct {
    uint32_t sample_hz     = 44100;
    uint32_t channels      = 6;
    bool     compatibility = false;
  } audio;

  struct {
    std::wstring
            version     = TZF_VER_STR;
    std::wstring
            intro_video = L"";
  } system;
};

extern tzf_config_t config;

bool TZF_LoadConfig (std::wstring name         = L"tzfix");
void TZF_SaveConfig (std::wstring name         = L"tzfix",
                     bool         close_config = false);

#endif __TZF__CONFIG_H__