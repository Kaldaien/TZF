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
#ifndef __TZF__FRAMERATE_H__
#define __TZF__FRAMERATE_H__

#include <stdint.h>

namespace tzf
{
  namespace FrameRateFix
  {
    void Init     (void);
    void Shutdown (void);

    // True if the game is running in fullscreen
    extern bool fullscreen;

    // True if the game is being framerate limited by the DRIVER
    extern bool driver_limit_setup;

    // This is actually setup in the BMF DLL that loads this one
    extern uint32_t target_fps;


    //
    // At key points during the game, we need to disable the code that
    //  cuts timing in half. These places will be wrapped by calls to
    //    these.
    //
    void Disallow60FPS (void);
    void Allow60FPS    (void);
  }
}

#endif /* __TZF__FRAMERATE_H__ */