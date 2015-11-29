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
#ifndef __TZF__SOUND_H__
#define __TZF__SOUND_H__

#include <mmeapi.h>

#include "command.h"

namespace tzf
{
  namespace SoundFix
  {
    void Init     (void);
    void Shutdown (void);

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
      eTB_Variable* sample_rate_;
      eTB_Variable* channels_;
      eTB_Variable* enable_;
      eTB_Variable* compatibility_;

    private:
      static CommandProcessor* pCommProc;
    };

    // True once the game has initialized sound
    extern bool wasapi_init;

    extern WAVEFORMATEX snd_core_fmt;
    extern WAVEFORMATEX snd_bink_fmt;
    extern WAVEFORMATEX snd_device_fmt;

    // Hold references to these DLLs
    extern HMODULE      dsound_dll;
    extern HMODULE      ole32_dll;
  }
}

#endif /* __TZF__SOUND_H__ */