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

#ifndef __TZF__INI_H__
#define __TZF__INI_H__

#include <string>
#include <map>

namespace tzf {
namespace INI {

  class File
  {
  public:
             File (wchar_t* filename);
    virtual ~File (void);

    void parse  (void);
    void import (std::wstring import_data);
    void write  (std::wstring fname);

    class Section
    {
    public:
      Section (void) {
      }

      Section (std::wstring section_name) {
        name = section_name;
      }

      // Technically, this isn't 1:1 ... but as far as WE'RE concerned, all the
      //   keys we're interested in _are_.
      std::wstring& get_value     (std::wstring key);
      bool          contains_key  (std::wstring key);
      void          add_key_value (std::wstring key, std::wstring value);

      //protected:
      //private:
      std::wstring                               name;
      std::multimap <std::wstring, std::wstring> pairs;
    };

    const std::map <std::wstring, Section>& get_sections (void);

    Section& get_section      (std::wstring section);
    bool     contains_section (std::wstring section);

  protected:
  private:
    FILE*     fINI;
    wchar_t*  wszName;
    wchar_t*  wszData;
    std::map <std::wstring, Section>
              sections;
  };

}
}

#endif
