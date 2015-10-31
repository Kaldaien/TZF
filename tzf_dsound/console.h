#ifndef __EPSILON_TESTBED__CONSOLE_H__
#define __EPSILON_TESTBED__CONSOLE_H__

#include <Windows.h>

class eFont;

//#include "../Epsilon/string.h"
//#include "../Epsilon/types.h"

#include <vector>

#include <string>

/* Convenient Shorthand Macros;  Requires: `#include "states.h"' */
#define eTB_Printf         ::states->console->printf
#define eTB_ColorPrintf    ::states->console->color_printf
#define eTB_Flush          ::states->console->Display
#ifdef HAS_VSSCANF
#define eTB_Scanf          ::states->console->scanf
#else
/* MSVC Hack (older versions lack vs[s]canf) */
#define eTB_Scanf(fmt,...)                           \
  ::states->console->scanf (fmt, __VA_ARGS__) == 0 ? \
    0 : ::scanf (fmt, __VA_ARGS__);
#endif
  

class eTB_Text;
class eTB_TextLine;

class eTB_TextLine
{
public:
   eTB_TextLine (void) : terminated_ (false) { };
  ~eTB_TextLine (void) {
    spans_.clear ();
  }

  size_t    getNumSpans (void) const { return spans_.size (); }
  eTB_Text* getSpan     (size_t idx) const;


  void InsertSpan (eTB_Text* pText);

  bool IsTerminated (void) const { return terminated_; }

protected:
private:
  bool                     terminated_;
  std::vector <eTB_Text *> spans_;
};

class eTB_Console
{
public:
  enum ConsoleType
  {
    CONSOLE_STDOUT = 0x01,
    CONSOLE_RENDER = 0x02,
    CONSOLE_NULL   = 0x04,

    NUM_CONSOLE_TYPES = 3
  };


           eTB_Console (void) {
  }

  virtual ~eTB_Console (void) {
//    text_.erase ();
  }

  /* Returns the length in characters of the formatted message. */
  int printf (const char* szFormat, ...);

  int color_printf (DWORD color, const char* szFormat, ...);

  /** Returns the number of input items successfully scanned.
   *
   *   NOTE: This is dependent upon the console's input capabilities,
   *         and is therefore pure virtual.
  **/
  virtual int scanf (const char* szFormat, ...) = 0;

  /* Must be implemented by the subclass. */
  virtual void Display (void) = 0;
  virtual void Display (eTB_Console* parent) { Display (); };


  virtual ConsoleType getType (void) const { return CONSOLE_NULL; }

  virtual std::vector <eTB_Text *>& GetText (void) { return text_; }
  virtual std::vector <eTB_TextLine *>& GetTextLines (void) { return lines_; }

protected:
  std::vector <eTB_TextLine *> lines_;
  std::vector <eTB_Text *>     text_;

private:
};



class eTB_Console_StdOut : public eTB_Console
{
public:
  eTB_Console_StdOut (void) : eTB_Console   (),
                              last_printed  (0),
                              display_count (0)     { };

  int scanf (const char* szFormat, ...);

  void Display (void);


  virtual ConsoleType getType (void) const { return CONSOLE_STDOUT; }

protected:
private:
//  std::vector <eTB_Text*>::const_iterator
  int   last_printed;
  int   display_count; /* Number of times new lines have been displayed... */
};


//#include "input.h"
class eTB_Console_Render : public eTB_Console/*, public eInput_iListener*/
{
public:
  eTB_Console_Render (void);

  void Activate   (void) { active = true;  }
  void Deactivate (void) { active = false; }
  bool IsActive   (void) { return active; }

  void Show (void) { visible = true;  }
  void Hide (void) { visible = false; }
  bool IsVisible (void) { return visible; }

  void Display (void);
  void Display (eTB_Console* parent);

  int  scanf   (const char* szFormat, ...);

  virtual ConsoleType getType (void) const { return CONSOLE_RENDER; }

  bool OnKeyPress   (DWORD key);
  bool OnKeyRelease (DWORD key);

protected:
  bool active;
  bool visible;

private:
  int anchor_line;   /* The line to display at the top of the console... */
  int display_count;

  char command_line [1024]; /// Limit to 1023 characters...

  int  repeat_delay_init;   /// The initial delay before key repeating begins.
  int  repeat_delay;        /// How long to wait before repeating a held key.
  int  last_repeat;         /// When the last repeat was performed...
  int  key_change;          /// Time (in msecs) when the last key down event
                            ///   was received.
  DWORD last_key;

  std::vector <std::string>
       command_history;
  unsigned int last_command_num;

  unsigned int cursor_pos;  /// The offset from the end of the command
                            ///   string, where editing will take place...
};


class eTB_Text {
public:
  eTB_Text ( const char* szMessage,
             DWORD       color,
             eFont*      pFont = NULL );

  ~eTB_Text (void)
  {
    if (message_ != NULL) {
      free (message_);
      message_ = NULL;
    }
  }

  eTB_Text (const eTB_Text& text)
  {
    // Opt not to use this, be verbose if it ever happens...
    ::printf ("XXX: eTB_Text Copy Constructor called!!!\n");

    /* Make sure the application continues running, even if
        this should happen... */
    message_ = strdup (text.message_);
    color_   = text.color_;
    font_    = text.font_;
  }

  const char* GetMessage (void) const { return message_; }
  eFont*      GetFont    (void) const { return font_;    }
  DWORD       GetColor   (void) const { return color_;   }

protected:
private:
  eFont* font_;
  DWORD  color_;
  char*  message_;
//  eStringA message_;
};

#endif /* __EPSILON_TESTBED__CONSOLE_H__ */
