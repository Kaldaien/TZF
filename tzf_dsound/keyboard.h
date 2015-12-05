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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Zestiria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __TZF__KEYBOARD_H__
#define __TZF__KEYBOARD_H__

#include <unordered_map>
#include <string>

namespace tzf {
  namespace Keyboard {
    void Init     (void);
    void Shutdown (void);

    extern std::unordered_map <std::wstring, short> name_to_virtkey;
    extern std::unordered_map <short, std::wstring> virtkey_to_name;
  }
}

#endif /* __TZF__KEYBOARD_H__ */