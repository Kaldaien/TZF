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

#ifndef __TZF__RENDER_H__
#define __TZF__RENDER_H__

#include "command.h"

struct IDirect3DDevice9;
struct IDirect3DSurface9;

#include <Windows.h>

namespace tzf
{
  namespace RenderFix
  {
    void Init     ();
    void Shutdown ();

    class CommandProcessor : public eTB_iVariableListener {
    public:
      CommandProcessor (void);

      virtual bool OnVarChange (eTB_Variable* var, void* val = NULL);

      static CommandProcessor* getInstance (void)
      {
        if (pCommProc == NULL)
          pCommProc = new CommandProcessor ();

        return pCommProc;
      }

    protected:
      eTB_Variable* fovy_;
      eTB_Variable* aspect_ratio_;

    private:
      static CommandProcessor* pCommProc;
    };

    extern uint32_t           width;
    extern uint32_t           height;

    extern IDirect3DSurface9* pPostProcessSurface;
    extern IDirect3DDevice9*  pDevice;
    extern HWND               hWndDevice;

    extern uint32_t           dwRenderThreadID;
    extern bool               bink; // True if a bink video is playing

    extern HMODULE            d3dx9_43_dll;
    extern HMODULE            user32_dll;
  }
}

struct game_state_t {
  BYTE*  base_addr    =  (BYTE *)0x2130309;

  BYTE*  Title        =  (BYTE *) base_addr;       // Title
  BYTE*  OpeningMovie =  (BYTE *)(base_addr + 1);  // Opening Movie

  BYTE*  Game         =  (BYTE *)(base_addr + 2);  // Game
  BYTE*  GamePause    =  (BYTE *)(base_addr + 3);  // Game Pause

  SHORT* Loading      = (SHORT *)(base_addr + 4);  // Why are there 2 states for this?

  BYTE*  Explanation  =  (BYTE *)(base_addr + 6);  // Explanation (+ Bink)?
  BYTE*  Menu         =  (BYTE *)(base_addr + 7);  // Menu

  BYTE*  Unknown0     =  (BYTE *)(base_addr + 8);  // Unknown
  BYTE*  Unknown1     =  (BYTE *)(base_addr + 9);  // Unknown - Appears to be battle related
  BYTE*  Unknown2     =  (BYTE *)(base_addr + 10); // Unknown

  BYTE*  Battle       =  (BYTE *)(base_addr + 11); // Battle
  BYTE*  BattlePause  =  (BYTE *)(base_addr + 12); // Battle Pause

  bool hasFixedAspect (void) {
    if (*OpeningMovie ||
        *GamePause    ||
        *Loading      ||
        *Explanation  ||
        *Menu         ||
        *BattlePause)
      return true;
    return false;
  }
  bool needsFixedMouseCoords(void) {
    return (*GamePause    ||
            *Menu         ||
            *BattlePause  ||
            *Title);
  }
} static game_state;

#endif /* __TZF__RENDER_H__ */