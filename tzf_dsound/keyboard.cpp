#include "keyboard.h"
#include <windows.h>

std::unordered_map <std::wstring, short> tzf::Keyboard::name_to_virtkey;
std::unordered_map <short, std::wstring> tzf::Keyboard::virtkey_to_name;

void
tzf::Keyboard::Init (void)
{
  name_to_virtkey [L"A"] = 'a'; name_to_virtkey [L"1"] = '1'; name_to_virtkey [L"!"] = '1';
  name_to_virtkey [L"a"] = 'a'; name_to_virtkey [L"2"] = '2'; name_to_virtkey [L"@"] = '2';
  name_to_virtkey [L"B"] = 'b'; name_to_virtkey [L"3"] = '3'; name_to_virtkey [L"#"] = '3';
  name_to_virtkey [L"b"] = 'b'; name_to_virtkey [L"4"] = '4'; name_to_virtkey [L"$"] = '4';
  name_to_virtkey [L"C"] = 'c'; name_to_virtkey [L"5"] = '5'; name_to_virtkey [L"%"] = '5';
  name_to_virtkey [L"c"] = 'c'; name_to_virtkey [L"6"] = '6'; name_to_virtkey [L"^"] = '6';
  name_to_virtkey [L"D"] = 'd'; name_to_virtkey [L"7"] = '7'; name_to_virtkey [L"&"] = '7';
  name_to_virtkey [L"d"] = 'd'; name_to_virtkey [L"8"] = '8'; name_to_virtkey [L"*"] = '8';
  name_to_virtkey [L"E"] = 'e'; name_to_virtkey [L"9"] = '9'; name_to_virtkey [L"("] = '9';
  name_to_virtkey [L"e"] = 'e'; name_to_virtkey [L"0"] = '0'; name_to_virtkey [L")"] = '0';
  name_to_virtkey [L"F"] = 'f'; name_to_virtkey [L"-"] = VK_OEM_MINUS; name_to_virtkey [L"_"] = VK_OEM_MINUS;
  name_to_virtkey [L"f"] = 'f'; name_to_virtkey [L"="] = VK_OEM_PLUS;  name_to_virtkey [L"+"] = VK_OEM_PLUS;
  name_to_virtkey [L"G"] = 'g'; name_to_virtkey [L"~"] = 0;            name_to_virtkey [L"`"] = 0; ////// TODO
  name_to_virtkey [L"g"] = 'g'; name_to_virtkey [L"Tab"]      = VK_TAB;     name_to_virtkey [L"tab"]     = VK_TAB;
  name_to_virtkey [L"H"] = 'h'; name_to_virtkey [L"CapsLock"] = VK_CAPITAL; name_to_virtkey [L"Caps"]    = VK_CAPITAL;
  name_to_virtkey [L"h"] = 'h'; name_to_virtkey [L"Shift"]    = VK_SHIFT;   name_to_virtkey [L"shift"]   = VK_SHIFT;
  name_to_virtkey [L"I"] = 'i'; name_to_virtkey [L"Ctrl"]     = VK_CONTROL; name_to_virtkey [L"Control"] = VK_CONTROL;
  name_to_virtkey [L"i"] = 'i'; name_to_virtkey [L"Alt"]      = VK_MENU;    name_to_virtkey [L"alt"]     = VK_MENU;
  name_to_virtkey [L"J"] = 'j'; name_to_virtkey [L"Space"]    = VK_SPACE;   name_to_virtkey [L"space"]   = VK_SPACE;
  name_to_virtkey [L"j"] = 'j'; name_to_virtkey [L"Windows"]  = VK_LWIN;
  name_to_virtkey [L"K"] = 'k'; name_to_virtkey [L"Left"]     = VK_LEFT;    name_to_virtkey [L"left"]    = VK_LEFT;
  name_to_virtkey [L"k"] = 'k'; name_to_virtkey [L"Right"]    = VK_RIGHT;   name_to_virtkey [L"right"]   = VK_RIGHT;
  name_to_virtkey [L"L"] = 'l'; name_to_virtkey [L"Up"]       = VK_UP;      name_to_virtkey [L"up"]      = VK_UP;
  name_to_virtkey [L"l"] = 'l'; name_to_virtkey [L"Down"]     = VK_DOWN;    name_to_virtkey [L"down"]    = VK_DOWN;
  name_to_virtkey [L"M"] = 'm'; name_to_virtkey [L"Enter"]    = VK_RETURN;  name_to_virtkey [L"Return"]  = VK_RETURN;
  name_to_virtkey [L"m"] = 'm'; name_to_virtkey [L"Delete"]   = VK_DELETE;  name_to_virtkey [L"Del"]     = VK_DELETE;
  name_to_virtkey [L"N"] = 'n'; name_to_virtkey [L"PageUp"]   = VK_PRIOR;   name_to_virtkey [L"PgUp"]    = VK_PRIOR;
  name_to_virtkey [L"n"] = 'n'; name_to_virtkey [L"PageDown"] = VK_NEXT;    name_to_virtkey [L"PgDn"]    = VK_NEXT;
  name_to_virtkey [L"O"] = 'o'; name_to_virtkey [L"Home"]     = VK_HOME;    name_to_virtkey [L"home"]    = VK_HOME;
  name_to_virtkey [L"o"] = 'o'; name_to_virtkey [L"End"]      = VK_END;     name_to_virtkey [L"end"]     = VK_END;
  name_to_virtkey [L"P"] = 'p'; name_to_virtkey [L","]        = VK_OEM_COMMA; name_to_virtkey [L"<"]     = VK_OEM_COMMA;
  name_to_virtkey [L"p"] = 'p'; name_to_virtkey [L"."]        = VK_OEM_PERIOD, name_to_virtkey [L">"]    = VK_OEM_PERIOD;
  name_to_virtkey [L"Q"] = 'q'; name_to_virtkey [L"/"]        = '/';           name_to_virtkey [L"?"]    = '/';
  name_to_virtkey [L"q"] = 'q'; name_to_virtkey [L"'"]        = '\'';          name_to_virtkey [L"\""]   = '"';
  name_to_virtkey [L"R"] = 'r'; name_to_virtkey [L";"]        = ';';           name_to_virtkey [L":"]    = ';';
  name_to_virtkey [L"r"] = 'r'; name_to_virtkey [L"\\"]       = '\\';          name_to_virtkey [L"|"]    = '\\';
  name_to_virtkey [L"S"] = 's'; name_to_virtkey [L"["]        = '[';           name_to_virtkey [L"{"]    = '[';
  name_to_virtkey [L"s"] = 's'; name_to_virtkey [L"]"]        = ']';           name_to_virtkey [L"}"]    = ']';
  name_to_virtkey [L"T"] = 't'; name_to_virtkey [L"F1"]       = VK_F1;         name_to_virtkey [L"f1"]   = VK_F1;
  name_to_virtkey [L"t"] = 't'; name_to_virtkey [L"F2"]       = VK_F2;         name_to_virtkey [L"f2"]   = VK_F2;
  name_to_virtkey [L"U"] = 'u'; name_to_virtkey [L"F3"]       = VK_F3;         name_to_virtkey [L"f3"]   = VK_F3;
  name_to_virtkey [L"u"] = 'u'; name_to_virtkey [L"F4"]       = VK_F4;         name_to_virtkey [L"f4"]   = VK_F4;
  name_to_virtkey [L"V"] = 'v'; name_to_virtkey [L"F5"]       = VK_F5;         name_to_virtkey [L"f5"]   = VK_F5;
  name_to_virtkey [L"v"] = 'v'; name_to_virtkey [L"F6"]       = VK_F6;         name_to_virtkey [L"f6"]   = VK_F6;
  name_to_virtkey [L"W"] = 'w'; name_to_virtkey [L"F7"]       = VK_F7;         name_to_virtkey [L"f7"]   = VK_F7;
  name_to_virtkey [L"w"] = 'w'; name_to_virtkey [L"F8"]       = VK_F8;         name_to_virtkey [L"f8"]   = VK_F8;
  name_to_virtkey [L"X"] = 'x'; name_to_virtkey [L"F9"]       = VK_F9;         name_to_virtkey [L"f9"]   = VK_F9;
  name_to_virtkey [L"X"] = 'x'; name_to_virtkey [L"F10"]      = VK_F10;        name_to_virtkey [L"f10"]  = VK_F10;
  name_to_virtkey [L"Y"] = 'y'; name_to_virtkey [L"F11"]      = VK_F11;        name_to_virtkey [L"f11"]  = VK_F11;
  name_to_virtkey [L"y"] = 'y'; name_to_virtkey [L"F12"]      = VK_F12;        name_to_virtkey [L"f12"]  = VK_F12;
  name_to_virtkey [L"Z"] = 'z';
  name_to_virtkey [L"z"] = 'z';
}

enum {
  FPS_PRESET_60_FIXED,
  FPS_PRESET_30_FIXED,
  FPS_PRESET_20_FIXED,
  FPS_PRESET_15_FIXED,
  FPS_PRESET_12_FIXED,
  FPS_PRESET_10_FIXED,
  FPS_PRESET_60_ADAPTIVE,
  FPS_PRESET_30_ADAPTIVE,
  FPS_PRESET_20_ADAPTIVE,
  FPS_PRESET_15_ADAPTIVE,
  FPS_PRESET_12_ADAPTIVE,
  FPS_PRESET_10_ADAPTIVE,
  FPS_TOGGLE_ADAPTIVE,
  FPS_SKIP_15_TICKS,

  OSD_TOGGLE_CONSOLE
};