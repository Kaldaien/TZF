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

    extern uint32_t           dwRenderThreadID;

    // We will enable/disable framerate fudging code when
    //   1/3 of the frames in a second (8) are Bink.
    //
    //   In the very unlikely event that some other system
    //     shares the same vertex shader as Bink, this shoud
    //       keep us from repeatedly turning the limiter on/off.
    extern uint32_t           bink_frames;
    extern bool               bink_state;
    const  uint32_t           bink_threshold = 8UL;
  }
}

#endif /* __TZF__RENDER_H__ */