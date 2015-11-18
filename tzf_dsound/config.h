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
#ifndef __TZF__CONFIG_H__
#define __TZF__CONFIG_H__

#include <Windows.h>
#include <string>

extern std::wstring TZF_VER_STR;

struct tzf_config_t
{
  struct {
    uint32_t sample_hz        = 48000;
    uint32_t channels         = 8;
    bool     compatibility    = false;
    bool     enable_fix       = true;
  } audio;

  struct {
    bool  yield_processor      = true;
    bool  allow_fake_sleep     = false;
    bool  stutter_fix          = false;
    float fudge_factor         = 1.666f;
    bool  allow_windowed_mode  = false;
    bool  minimize_latency     = false;
    DWORD speedresetcode_addr  = 0x0046C529;
    DWORD speedresetcode2_addr = 0x0056E441;
    DWORD speedresetcode3_addr = 0x0056D93F;
    DWORD limiter_branch_addr  = 0x00990873;
    bool  disable_limiter      = true;
  } framerate;

  struct {
    bool capture            = false;
  } file_io;

  struct {
    bool allow_broadcasts   = false;
  } steam;

  struct {
    float   fovy              = 0.785398f;
    float   aspect_ratio      = 1.777778f;
    DWORD   aspect_addr       = 0x00D52398;
    DWORD   fovy_addr         = 0x00D5239C;
    bool    blackbar_videos   = true;
    bool    aspect_correction = false;
    bool    complete_mipmaps  = false;
    int32_t shadow_rescale    = -2;
    float   postproc_ratio    =  1.0f;
    bool    disable_scissor   = false;
  } render;

  struct {
    std::wstring
            version         = TZF_VER_STR;
    std::wstring
            intro_video     = L"";
  } system;
};

extern tzf_config_t config;

bool TZF_LoadConfig (std::wstring name         = L"tzfix");
void TZF_SaveConfig (std::wstring name         = L"tzfix",
                     bool         close_config = false);

#endif __TZF__CONFIG_H__