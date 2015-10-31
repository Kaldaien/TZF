//#include "../Epsilon/STL.h"
//#include "../Epsilon/types.h"
//#include "../Epsilon/timer.h"
#include "console.h"
#include "command.h"

//#include "prototype/render_context.h"

//static eTimer timer;


/* TODO: Integrate this into eTB_Console... */
void
AppendText ( const char*                   szAppend,
             std::vector <eTB_Text *>&     text__,
             std::vector <eTB_TextLine *>& lines__ )
{
  eTB_Text*     pText     = new eTB_Text (szAppend, 0xffffffff);
  eTB_TextLine* pTextLine = NULL;
  bool          bNewLine  = false;

  text__.push_back (pText);

  /* If there are already lines, try to add this span to the last one... */
  if (lines__.size ()) {
    std::vector <eTB_TextLine *>::iterator line = (lines__.end () - 1);

    /* If the line _DOES NOT_ contain a newline, then add this span... */
    if (! (*line)->IsTerminated ()) {
      (*line)->InsertSpan (pText);
    }

    /* Otherwise, start a new span, for a new line. */
    else
      bNewLine = true;
  } else {
    /* Otherwise, create a new line. */
    bNewLine = true;
  }

  /* If we determined above that this text span requires a new line, then
       this is where the magic happens... */
  if (bNewLine) {
    pTextLine = new eTB_TextLine ();
    pTextLine->InsertSpan  (pText);
    lines__.push_back      (pTextLine);
  }
}

/* TODO: Integrate this into eTB_Console... */
void
AppendColorText ( const char*                   szAppend,
                  DWORD                         color,
                  std::vector <eTB_Text *>&     text__,
                  std::vector <eTB_TextLine *>& lines__ )
{
  eTB_Text*     pText     = new eTB_Text (szAppend, color);
  eTB_TextLine* pTextLine = NULL;
  bool          bNewLine  = false;

  text__.push_back (pText);

  /* If there are already lines, try to add this span to the last one... */
  if (lines__.size ()) {
    std::vector <eTB_TextLine *>::iterator line = (lines__.end () - 1);

    /* If the line _DOES NOT_ contain a newline, then add this span... */
    if (! (*line)->IsTerminated ()) {
      (*line)->InsertSpan (pText);
    }

    /* Otherwise, start a new span, for a new line. */
    else
      bNewLine = true;
  } else {
    /* Otherwise, create a new line. */
    bNewLine = true;
  }

  /* If we determined above that this text span requires a new line, then
       this is where the magic happens... */
  if (bNewLine) {
    pTextLine = new eTB_TextLine ();
    pTextLine->InsertSpan  (pText);
    lines__.push_back      (pTextLine);
  }
}

/* NOTE: This is _NOT_ thread safe. */
int
eTB_Console::color_printf (DWORD color, const char* szFormat, ...)
{
  va_list argv;

  //Assert (szFormat != NULL, _T ("Invalid format string"));

  if (szFormat == NULL)
    return 0;

  /* Length of the output string thus far... */
  int len = 0;

  static char szOut     [16384]; // Limit formatted text    to 16 Kb...
  static char szTabExp  [32768]; // Limit tab expansion     to 32 Kb...
  static char szSubLine [32768]; // Limit newline expansion to 32 Kb...

  *szOut     = '\0';
  *szTabExp  = '\0';
  *szSubLine = '\0';

  va_start (argv, szFormat);
    len += vsnprintf (szOut, 16384, szFormat, argv);
  va_end (argv);

  const char _TABCHAR = ' '; /* Replace each '\t' with _TABSTOP-many of */
  const int  _TABSTOP = 5;   /*   these characters... */
  int              nt = 0;   /* <-- nt = Number of tabs expanded. */

  /* Expand Tabs. */
  for (int i = 0; i < len; i++) {
    if (szOut [i] != '\t') {
      szTabExp [i + (nt * (_TABSTOP - 1))] = szOut [i];
    } else {
      for (int j = 0; j < _TABSTOP; j++) {
        szTabExp [i + (nt * (_TABSTOP - 1)) + j] = _TABCHAR;
      }
      ++nt;
    }
  }

  const int tabbed_len = len + (nt * (_TABSTOP - 1));

  /* Terminate the new string at the correct location... */
  szTabExp [tabbed_len] = '\0';

  int num_trailing_newlines = 0;
  int num_leading_newlines  = 0;

  for (int i = 0; i < tabbed_len; i++) {
    if (szTabExp [tabbed_len - i - 1] == '\n')
      num_trailing_newlines++;
    else
      break;
  }

  if (num_trailing_newlines > 0) {
    szTabExp [tabbed_len - num_trailing_newlines] = '\0';
  }

  for (int i = 0; i < tabbed_len; i++) {
    if (szTabExp [i] == '\n')
      num_leading_newlines++;
    else
      break;
  }

  /* If the number of leading newlines is equal to the strlen, then let the
       trailing newline detection code take care of this... */
  if (num_leading_newlines > 0) {
    for (int i = 0; i < num_leading_newlines; i++) {
      AppendColorText ("\n", color, text_, lines_);
    }
  }

  bool  tok    = true;
  char* szLine = strtok (szTabExp + num_leading_newlines, "\n");

  /* No newlines (aside from leading and/or trailing)... */
  if (szLine == NULL) {
    szLine = szTabExp + num_leading_newlines;
    tok    = false; /* No embedded new-lines... */
  }

  /* For Each Line... */
  while (szLine != NULL) {
    const size_t szLine_len = strlen (szLine);

    strcpy (szSubLine, szLine);

    /* szSubLine is part of a string of text with one or more embedded
         newline characters... */
    if (tok) {
      szLine = strtok (NULL, "\n");

      /* Add a newline character, where the next sub-string begins. */
      if (szLine != NULL) {
        szSubLine [szLine_len]     = '\n';
        szSubLine [szLine_len + 1] = '\0';
      } else {
        szLine = NULL;
      }
    } else {
      szLine = NULL;
    }

    AppendColorText (szSubLine, color, text_, lines_);

    /* Triggered when we reach the end of the string, add any trailing
         newlines back into the text at this point... */
    if (szLine == NULL) {
      if (num_trailing_newlines > 0) {
        for (int i = 0; i < num_trailing_newlines; i++) {
          AppendColorText ("\n", color, text_, lines_);
        }
      }
    }
  }

  return len;
}

/* NOTE: This is _NOT_ thread safe. */
int
eTB_Console::printf (const char* szFormat, ...)
{
  va_list argv;

  //Assert (szFormat != NULL, _T ("Invalid format string"));

  if (szFormat == NULL)
    return 0;

  /* Length of the output string thus far... */
  int len = 0;

  static char szOut     [16384]; // Limit formatted text    to 16 Kb...
  static char szTabExp  [32768]; // Limit tab expansion     to 32 Kb...
  static char szSubLine [32768]; // Limit newline expansion to 32 Kb...

  *szOut     = '\0';
  *szTabExp  = '\0';
  *szSubLine = '\0';

  va_start (argv, szFormat);
    len += vsnprintf (szOut, 16384, szFormat, argv);
  va_end (argv);

  const char _TABCHAR = ' '; /* Replace each '\t' with _TABSTOP-many of */
  const int  _TABSTOP = 5;   /*   these characters... */
  int              nt = 0;   /* <-- nt = Number of tabs expanded. */

  /* Expand Tabs. */
  for (int i = 0; i < len; i++) {
    if (szOut [i] != '\t') {
      szTabExp [i + (nt * (_TABSTOP - 1))] = szOut [i];
    } else {
      for (int j = 0; j < _TABSTOP; j++) {
        szTabExp [i + (nt * (_TABSTOP - 1)) + j] = _TABCHAR;
      }
      ++nt;
    }
  }

  const int tabbed_len = len + (nt * (_TABSTOP - 1));

  /* Terminate the new string at the correct location... */
  szTabExp [tabbed_len] = '\0';

  int num_trailing_newlines = 0;
  int num_leading_newlines  = 0;

  for (int i = 0; i < tabbed_len; i++) {
    if (szTabExp [tabbed_len - i - 1] == '\n')
      num_trailing_newlines++;
    else
      break;
  }

  if (num_trailing_newlines > 0) {
    szTabExp [tabbed_len - num_trailing_newlines] = '\0';
  }

  for (int i = 0; i < tabbed_len; i++) {
    if (szTabExp [i] == '\n')
      num_leading_newlines++;
    else
      break;
  }

  /* If the number of leading newlines is equal to the strlen, then let the
       trailing newline detection code take care of this... */
  if (num_leading_newlines > 0) {
    for (int i = 0; i < num_leading_newlines; i++) {
      AppendText ("\n", text_, lines_);
    }
  }

  bool  tok    = true;
  char* szLine = strtok (szTabExp + num_leading_newlines, "\n");

  /* No newlines (aside from leading and/or trailing)... */
  if (szLine == NULL) {
    szLine = szTabExp + num_leading_newlines;
    tok    = false; /* No embedded new-lines... */
  }

  /* For Each Line... */
  while (szLine != NULL) {
    const size_t szLine_len = strlen (szLine);

    strcpy (szSubLine, szLine);

    /* szSubLine is part of a string of text with one or more embedded
         newline characters... */
    if (tok) {
      szLine = strtok (NULL, "\n");

      /* Add a newline character, where the next sub-string begins. */
      if (szLine != NULL) {
        szSubLine [szLine_len]     = '\n';
        szSubLine [szLine_len + 1] = '\0';
      } else {
        szLine = NULL;
      }
    } else {
      szLine = NULL;
    }

    AppendText (szSubLine, text_, lines_);

    /* Triggered when we reach the end of the string, add any trailing
         newlines back into the text at this point... */
    if (szLine == NULL) {
      if (num_trailing_newlines > 0) {
        for (int i = 0; i < num_trailing_newlines; i++) {
          AppendText ("\n", text_, lines_);
        }
      }
    }
  }

  return len;
}

void
eTB_Console_StdOut::Display (void)
{
  std::vector <eTB_Text *>::const_iterator line      = text_.begin ();
  std::vector <eTB_Text *>::const_iterator last_line = text_.end   ();

  if ((line + last_printed) < last_line) {
    line = (line + last_printed);

    while (line != last_line) {
      ::printf ("%s", (*line)->GetMessage ());

      ++last_printed;
      ++line;
    }

    /* Differs from last_printed, in that this keeps count of
        the number of times new data was displayed.

         * Messages are buffered before displaying, and so it is
             possible for many messages to be displayed in a single
              call to Display (...).

          <=> This helps evaluate the efficiency of the Display method.
    */
    ++display_count;
  }
}

int
eTB_Console_StdOut::scanf (const char* szFormat, ...)
{
  /* It's typical to print a prompt to the console before calling
       scanf (...), so make sure we update the console's output before
         scanning for new input. */
  Display ();

  //Assert (szFormat != NULL, _T ("Invalid format string"));

  if (szFormat == NULL)
    return 0;

  int args = 0;

  /* This is the smart way of doing this... */
#ifdef HAS_VSSCANF
  va_list argv;

  va_start (argv, szFormat);
    args += vscanf (szFormat, argv);
  va_end (argv);

  return args;
#else
  /* MSVC is a mess, to keep things portable, we use a macro to
       solve this problem on older stdlib implementations...

         -> The problem being, a lack of vscanf (...)
  */
  return -1;
#endif
}

//#include "../Epsilon/epsilon.h"

//#include "../Epsilon/window.h"

//#include "../Epsilon/Epsilon3D/Epsilon3D.h"
//#include "../Epsilon/Epsilon3D/rendercontext.h"
//#include "../Epsilon/Epsilon3D/viewport.h"


//#include "states.h"

//#include "../Epsilon/Epsilon3D/OpenGL.h"

//#include "window.h"

//#include "cli_parse/structs.h"
//#include "cli_parse/flags.h"

//#include "cli_parse/sector.h"
//#include "cli_parse/room.h"

//#include "cli_parse/level.h"

//#include "cli_parse/render_batch.h"

//#include "cli_parse/line.h"
//#include "cli_parse/sector.h"
//#include "cli_parse/room.h"
//#include "cli_parse/level.h"

//#include "cli_parse/util.h"

//#include "cli_parse/render.h"
//#include "cli_parse/util.h"

//#include "font.h"

//#include "input.h"
//#include "../Epsilon/EpsilonInput/keyboard.h"

//extern eFont* font;

#define g_Window states->window

#if 0
eTB_Console_Render::eTB_Console_Render (void) : eTB_Console   (),
                                                anchor_line   (0),
                                                display_count (0),
                                                active        (false),
                                                visible       (false)
{
  states->input->InstallListener (this);

  repeat_delay_init = 250;
  repeat_delay      = 50;

  key_change  = 0;
  last_repeat = 0;

  last_command_num = 0;
  cursor_pos       = 0;

  memset (command_line, 0, sizeof (char) * 1024);
};

void
eTB_Console_Render::Display (void)
{
  Display (this);
}

static e3dVertexCache* console_vcache = NULL;
static e3dIndexCache*  console_icache = NULL;

static std::vector <GlyphVertexEx>  console_verts;
static std::vector <unsigned short> console_indices;

#include "prototype/shader.h"
#include "prototype/program.h"

struct ConsoleOverlay {
  ConsoleOverlay (void) {
    shader.program = NULL;
    vbo            = NULL;
    vao            = NULL;
  }

  void init (void) {
    if (shader.program != NULL)
      return;

    eTB_RenderContext* rcx = cli_parse::states->rcx;

    shader.program =
      program_factory.createProgram ("Console Overlay");

    eTB_AttachShaderFromFile ( shader.program, 
                                 "shaders/console_overlay_fs.glsl",
                                   "fs::console_overlay",
                                     eTB_Shader::Fragment );
    eTB_AttachShaderFromFile ( shader.program,
                                 "shaders/console_overlay_vs.glsl",
                                   "vs::bitmap_font_static",
                                     eTB_Shader::Vertex );

    shader.program->BindAttrib ("vtx_pos", 0);

    LinkProgram (shader.program);

    shader.uf_ortho_matrix = shader.program->GetUniform ("ortho_matrix");
    shader.uf_color        = shader.program->GetUniform ("color");

    // This will never change, so set it once and never again...
    shader.uf_color->Set4 <GLfloat> (0.0f, 0.0f, 0.0f, 0.5f);

    glGenVertexArrays (1, &vao);
    glGenBuffers      (1, &vbo);

    rcx->BindVertexArray (vao);

    glBindBuffer ( GL_ARRAY_BUFFER, overlay_data.vbo );

    glVertexAttribPointer ( 0,
                            2,
                              GL_SHORT,
                                GL_FALSE,
                                  sizeof (ConsoleOverlay::OverlayVertex),
                                    (short *)NULL );

    glEnableVertexAttribArray (0);
  }

  struct {
    eTB_Program* program;

    eTB_Uniform* uf_ortho_matrix;
    eTB_Uniform* uf_color;
  } shader;

  struct OverlayVertex {
    short pos [2];
  } verts [4];

  GLuint vao;
  GLuint vbo;
} overlay_data;

void
eTB_Console_Render::Display (eTB_Console* parent)
{
  /* Do nothing, unless the console is visible... */
  if (! IsVisible ())
    return;

  timer.tick ();

  if (key_change != 0) {
    bool repeat = false;

    /* If last_repeat is 0, then we are waiting for the initial
         repeat delay... */
    if (last_repeat == 0) {
      if ((timer.milliseconds () - key_change) > repeat_delay_init)
        repeat = true;
    } else { /* Shorter delay for successive repeats... */
      if ((timer.milliseconds () - key_change) > repeat_delay)
        repeat = true;
    }

    /* If repeat is true, we will issue a Key Press event to make
         the console behave as if the user had pressed the key again... */
    if (repeat) {
      OnKeyPress (last_key);
      last_repeat = timer.milliseconds ();
    }
  }

  ConsoleOverlay::OverlayVertex* pVert = overlay_data.verts;
  e3dRenderContext*              rc    = cli_parse::states->render_context;
  eTB_RenderContext*             rcx   = cli_parse::states->rcx;

  /* Span 1/2 of the window (from top to bottom) */
  const float fSpanRatio  = 0.5f;
  const float fViewWidth  = (float)g_Window->viewport.Width  ();
  const float fViewHeight = (float)g_Window->viewport.Height ();

  pVert->pos [0] = 0;
  pVert->pos [1] = (short)(fViewHeight * (1.0f - fSpanRatio));

  pVert++;

  pVert->pos [0] = 0;
  pVert->pos [1] = (short)fViewHeight;

  pVert++;

  pVert->pos [0] = (short)fViewWidth;
  pVert->pos [1] = (short)(fViewHeight * (1.0f - fSpanRatio));

  pVert++;

  pVert->pos [0] = (short)fViewWidth;
  pVert->pos [1] = (short)fViewHeight;

  pVert = overlay_data.verts;

  e3dRenderStates::Boolean cull       = rc->GetCullFaces ();
  e3dRenderStates::Boolean depth_test = rc->GetDepthTest ();
  e3dRenderStates::Boolean blend      = rc->GetBlend     ();

  e3dRenderStates::BlendOp src_op,
                           dst_op;

  rc->GetBlendFunc (src_op, dst_op);

  rc->SetCullFaces (e3dRenderStates::False);
  rc->SetDepthTest (e3dRenderStates::False);

  /// Originally: GL_ZERO, GL_SRC_ALPHA   - Changed to "fix" texture coord debug overlay
  rc->SetBlendFunc (e3dRenderStates::SourceAlpha,
                    e3dRenderStates::InverseSourceAlpha);
  rc->SetBlend     (e3dRenderStates::True);

  if (overlay_data.shader.program == NULL) {
    overlay_data.init ();

    glNamedBufferDataEXT ( overlay_data.vbo,
                             sizeof (ConsoleOverlay::OverlayVertex) * 4,
                               &overlay_data.verts->pos [0],
                                 GL_DYNAMIC_DRAW );
  } else {
    glNamedBufferSubDataEXT ( overlay_data.vbo, 
                                NULL,
                                  sizeof (ConsoleOverlay::OverlayVertex) * 4,
                                    &overlay_data.verts->pos [0] );
  }

  rcx->BindVertexArray (overlay_data.vao);
  rcx->BindProgram     (overlay_data.shader.program);
  {
    overlay_data.shader.uf_ortho_matrix->SetMatrix (
      ::states->window->GetOrthoMatrix () );
    glDrawArrays       ( GL_TRIANGLE_STRIP, 0, 4 );
  }
  rcx->BindProgram     (NULL);
  rcx->BindVertexArray (NULL);

  rc->SetCullFaces (cull);
  rc->SetDepthTest (depth_test);

  rc->SetBlendFunc (src_op,
                    dst_op);
  rc->SetBlend     (blend);

  float x = 0.0f;
  float y = fViewHeight * fSpanRatio;

  ///
  /// XXX: Change Me!
  ///
  /// Used only to set a default font
  ///
  std::vector <eTB_Text *> parent_text          = parent->GetText   ();
  std::vector <eTB_Text *>::const_iterator line = parent_text.begin ();

  if (! console_vcache)
    console_vcache = g_Window->rc->CreateVertexCache ();
  if (! console_icache)
    console_icache = g_Window->rc->CreateIndexCache ();

  eFont* line_font = (*line)->GetFont ();

  y += (line_font->GetRowHeight () + 9.0f);


  int last_vtx = 0;

  //
  // Print the Command Prompt first.
  //
  if (IsActive ())
  {
    const DWORD dwGreen  = 0xff00ff00;
    const DWORD dwRed    = 0xff0000ff;
    const DWORD dwYellow = dwGreen | dwRed;

    if (states->r_batch_print) {
      line_font->PrintToArray ( "[cli_parse]# ",
                                  x, y,
                                    dwRed,
                                      last_vtx,
                                        console_verts, console_indices );
    } else {
      line_font->Print        ( "[cli_parse]# ",
                                  x, y,
                                   console_vcache, console_icache );
    }

    x += (line_font->StringWidth ("[cli_parse]# "));

    if (states->r_batch_print)
      line_font->PrintToArray (command_line, x, y, dwYellow, last_vtx, console_verts, console_indices);
    else
      line_font->Print (command_line,    x, y, console_vcache, console_icache);

//      x_ += (font->StringWidth (command_line));

    static int  last_blink  = 0;
    static bool draw_cursor = false;
    timer.tick ();

    if ((timer.milliseconds () - last_blink) > 250) {
      draw_cursor = ! draw_cursor;
      last_blink  = timer.milliseconds ();
    }

    if (draw_cursor) {
      char*  szEditedCommand = strdup (command_line);
      size_t len             = strlen (szEditedCommand);

      if (cursor_pos > len)
        cursor_pos = (unsigned int)len;

      *(szEditedCommand + len - cursor_pos) = '\0';

      x += (line_font->StringWidth (szEditedCommand));

      free ((char *)szEditedCommand);

      if (states->r_batch_print)
        line_font->PrintToArray ("_", x, y - 1.0f, dwGreen, last_vtx, console_verts, console_indices);
      else
        line_font->Print ("_", x, y - 1.0f, console_vcache, console_icache);
    }

    // Begin drawing text _after_ the optional command prompt...
    y += (line_font->GetRowHeight () + 4.0f);
  }

  x = 0.0f;

  std::vector <eTB_TextLine *>&
               lines     = parent->GetTextLines ();
  const size_t num_lines = lines.size           ();

  /* NOTE: We render the lines in reverse order, so that the height of each
             line does not need to be known ahead of time. The signal to
               stop rendering comes when the y-coordinate exceedes its max.
  */

// Signed size_t on Windows...
#ifdef _MSC_VER
 typedef SSIZE_T ssize_t;
#endif

  for (ssize_t i = (num_lines - 1); i > 0; i--)
  {
    // Stop trying to print lines if we've hit the top of the console...
    if (y > fViewHeight)
      break;

    // Don't print lines _after_ the anchor line...
    if (i >= (ssize_t)(num_lines - anchor_line))
      continue;

    const size_t num_spans = lines [i]->getNumSpans ();

    for (size_t j = 0; j < num_spans; j++)
    {
      eTB_Text* pLine = lines [i]->getSpan (j);

      eFont*      span_font = pLine->GetFont    ();
      const char* szMessage = pLine->GetMessage ();
      size_t      str_len   = strlen (szMessage);

      DWORD dwColor = (pLine)->GetColor ();

      if (states->r_batch_print)
        span_font->PrintToArray ((char *)szMessage, x, y, dwColor, last_vtx, console_verts, console_indices);
      else
        span_font->Print ((char *)szMessage, x, y, console_vcache, console_icache);

      x += span_font->StringWidth ((char *)szMessage);
    }

    y += (line_font->GetRowHeight () + 4.0f);
    x = 0.0f;
  }

  if (states->r_batch_print)
    line_font->FinishArrayPrint (last_vtx, console_verts, console_indices);

  if (y < fViewHeight)
    anchor_line--;


#if 0
    /* Differs from last_printed, in that this keeps count of
        the number of times new data was displayed.

         * Messages are buffered before displaying, and so it is
             possible for many messages to be displayed in a single
              call to Display (...).

          <=> This helps evaluate the efficiency of the Display method.
    */
    ++display_count;
#endif
}

int
eTB_Console_Render::scanf (const char* szFormat, ...)
{
  return 0;
}


bool
eTB_Console_Render::OnKeyRelease (Epsilon::Keyboard::Key key)
{
  key_change  = 0;
  last_repeat = 0;

  return true;
}
#endif

///
/// TODO: Edit Mode (Insert / Delete)
///
void
EditCommandLine (unsigned int& edit_pos, char* string, char edit_char)
{
  bool backspace = false;

  /* '\b' means backspace, which deletes the character _before_ the current
       edit_pos. */
  if (edit_char == '\b')
    backspace = true;

  if (edit_pos != 0)
  {
    size_t pos      = edit_pos;
    size_t iLen     = strlen (string);
    char*  szBefore = strdup (string);
    char*  szEx     = strdup (string);
    char*  szAfter  = (szBefore + iLen - edit_pos - 1);

    /* Backspace will delete the character _before_ the edit pos... */
    if (backspace)
      pos++;

    if (pos > iLen) {
      free (szBefore);
      free (szEx);
      return;
    }

    *(szBefore + iLen - pos) = '\0';
    szAfter = (szEx + iLen - pos);

    if (! backspace)
      snprintf (string, 1023, "%s%c%s", szBefore, edit_char, szAfter);
    else
      snprintf (string, 1023, "%s%s", szBefore, (szAfter + 1));

    free (szBefore);
    free (szEx);
  } else {
    if (!backspace) {
      char* tmp = strdup (string);
      snprintf (string, 1023, "%s%c", tmp, edit_char);
      free (tmp);
    }
    else
      *(string + strlen (string) - 1) = '\0';
  }
}

#include "sound.h" // OpenALSource ...

char*
first_alpha_num (char* szCmd)
{
  size_t len = strlen (szCmd);

  for (size_t i = 0; i < len; i++) {
    if (! (iscntrl ((unsigned char)szCmd [i]) ||
           isspace ((unsigned char)szCmd [i]))   )
      return &szCmd [i];
  }

  return NULL;
}

#if 0
bool
eTB_Console_Render::OnKeyPress (Epsilon::Keyboard::Key key)
{
  /* Load Sounds... */
  static OpenALSource* activate_sound   = NULL;
  static OpenALSource* deactivate_sound = NULL;
  static OpenALSource* cmd_sound        = NULL;
  static OpenALSource* err_sound        = NULL;

  if (! activate_sound) {
    OpenALBuffer* activate   = MakeSoundAL (38);
    OpenALBuffer* deactivate = MakeSoundAL (39);
    OpenALBuffer* cmd_enter  = MakeSoundAL (9);
    OpenALBuffer* err_msg    = MakeSoundAL (37);

    activate_sound = cli_parse::states->sound_context->createSource ();
    activate_sound->attach (activate);

    deactivate_sound = cli_parse::states->sound_context->createSource ();
    deactivate_sound->attach (deactivate);

    cmd_sound = cli_parse::states->sound_context->createSource ();
    cmd_sound->attach (cmd_enter);

    err_sound = cli_parse::states->sound_context->createSource ();
    err_sound->attach (err_msg);
  }
  /* End Sound Loading */

  /* Get a hold of the keyboard object, so we can make use of modifier
       keys, for things like modifying the behavior of the console toggle key
         when shift is held down and properly decoding uppercase / lowercase
           input sequences... */
  Epsilon::Keyboard*               keyboard =
         states->window->win->getKeyboard ();
  const
  Epsilon::Keyboard::ModifierKeys* mod      =
         keyboard->getModifierKeys        ();

  bool alt   = mod->getKey (Epsilon::Keyboard::ModifierKeys::Alt);
  bool ctrl  = mod->getKey (Epsilon::Keyboard::ModifierKeys::Control);
  bool shift = mod->getKey (Epsilon::Keyboard::ModifierKeys::Shift);

  if (key.getType () == Epsilon::Keyboard::Misc) {
    // Toggle visibility of console on tab...
    if (key.getCode () == Epsilon::Keyboard::MiscKeys::Tab) {
      if (IsVisible ()) {
        // Hide the console if (and only if) shift is not down...
        if (! shift && IsActive ()) {
          Hide       ();
          Deactivate ();

          /* Play Sound when Hiding Console. */
          deactivate_sound->play ();
        } else {
          if (IsActive ())
            Deactivate ();
          else
            Activate ();
        }
      }
      else {
        Show     ();
        Activate ();

        /* Play sound when the console pops up... */
        activate_sound->play ();
      }

      return false;
    }

    // Don't process keys if the console is not active...
    if (! (IsActive () && IsVisible ()))
      return false;

    switch (key.getCode ())
    {
      /* Key: UP ARROW / PAGE UP */

      /* Cycle through commands, or scroll up, depending on the
            status of the Shift and Control keys... */
      case Epsilon::Keyboard::MiscKeys::Up:
      case Epsilon::Keyboard::MiscKeys::PageUp:
      {
        /* CTRL + Up or Page Up --> Scroll UP. */
        if (ctrl || key.getCode () == Epsilon::Keyboard::MiscKeys::PageUp)
        {
          const int anchor_shift =
            ((shift) ? 5 : 1);

          anchor_line += anchor_shift;
        }

        /* If only shift is down, cycle through the command history... */
        else if (shift)
        {
          if (command_history.size ())
          {
            // Wrap around...
            if (last_command_num != 0)
              --last_command_num;
            else
              last_command_num = (unsigned int)command_history.size () - 1;

            strcpy (command_line, command_history [last_command_num].c_str ());
          }
        }
      } break;

      /* Key: DOWN ARROW / PAGE DOWN */

      /* Cycle through commands, or scroll down, depending on the
            status of the Shift and Control keys... */
      case Epsilon::Keyboard::MiscKeys::Down:
      case Epsilon::Keyboard::MiscKeys::PageDown:
      {
        /* CTRL + Down or Page Down --> Scroll DOWN. */
        if (ctrl || key.getCode () == Epsilon::Keyboard::MiscKeys::PageDown) {
          const int anchor_shift =
            ((shift) ? 5 : 1);

          anchor_line -= anchor_shift;

          if (anchor_line < 0)
            anchor_line = 0;
        }

        /* If only shift is down, cycle through the command history... */
        else if (shift)
        {
          if (command_history.size ())
          {
            ++last_command_num;

            if (last_command_num >= command_history.size ())
              last_command_num = 0;

            if (command_history [last_command_num].c_str () != NULL) {
              strcpy (command_line, command_history [last_command_num].c_str ());
            }
          }
        }
      } break;

      /* Key: LEFT  -  Move cursor to the left. */
      case Epsilon::Keyboard::MiscKeys::Left:
        cursor_pos++; /* Cursor pos: 0 is right-end, move left by addition.. */
        break;

      /* Key: RIGHT  -  Move cursor to the right. */
      case Epsilon::Keyboard::MiscKeys::Right:
        if (cursor_pos != 0)
          cursor_pos--; /* Opposite of the comment in the case above... */
        break;

      /* Key: HOME  -  Move cursor pos to the beginning. */
      case Epsilon::Keyboard::MiscKeys::Home:
        cursor_pos = (unsigned int)strlen (command_line);
        break;

      /* Key: END  -  Move cursor pos to the end. */
      case Epsilon::Keyboard::MiscKeys::End:
        cursor_pos = 0;
        break;
    }
  }

  // Don't process keys if the console is not active...
  if (! (IsActive () && IsVisible ()))
    return false;

  /* Ignore modifier keys when handling key repeats... */
  if (key.getType () != Epsilon::Keyboard::Modifier) {
    timer.tick ();

    last_key   = key;
    key_change = timer.milliseconds ();
  }

  if (key.getType () != Epsilon::Keyboard::Text) {
    switch (key.getCode ())
    {
      case Epsilon::Keyboard::MiscKeys::Enter:
      {
        /* Alt + Enter is a permanent alias for fullscreen mode switch... */
        if (alt)
          break; // So ignore it!

        /* Points to the first non-whitespace character
             found in (command_line). */
        char* cmd_line = first_alpha_num (command_line);

        /* If the command string was empty, then ignore it! */
        if (cmd_line == NULL)
          break;
///
///      Process Command Line
///
        eTB_CommandResult result =
          states->command->ProcessCommandLine (cmd_line);

        if (result.getStatus () != false) {
          /* Clear the command input line... */
          cursor_pos    = 0;
          *command_line = _T ('\0');

          /* Print the value of any variable we changed... */
          if (result.getVariable () != NULL) {
            const eTB_Variable* var = result.getVariable ();

            eTB_Printf ( "%s : %s\n",
                           result.getWord ().c_str (),
                             (char *)var->getValueString () );
          }

          /* Play Command Success Sound. */
          cmd_sound->play ();

          /* Store the re-joined command. */
          std::string str;

          /* Command has been split into two strings,
               join them back as needed... */
          if (result.getArgs ().size () > 0)
            str = result.getWord () + std::string (" ") + result.getArgs ();
          else
            str = result.getWord ();

          /* Push this command onto the command history. */
          command_history.push_back (str);
          last_command_num = (unsigned int)command_history.size ();
        }else {
          if (result.getResult ().empty ())
            eTB_Printf (" >> Unknown Command or Variable: %s\n", cmd_line);
          else
            eTB_Printf (" >> Command Error: %s\n", result.getResult ().c_str ());

          /* Play Command Failure Sound. */
          err_sound->play ();
        }
      } break;

      case Epsilon::Keyboard::MiscKeys::Space:
        EditCommandLine (cursor_pos, command_line, ' ');
        break;

      case Epsilon::Keyboard::MiscKeys::Backspace:
      {
        EditCommandLine (cursor_pos, command_line, '\b');
      } break;
    }
    return false;
  }

  switch (key.getType ())
  {
    case Epsilon::Keyboard::Text:
    {
      EditCommandLine ( cursor_pos,
                          command_line,
                            Epsilon::Keyboard::getKeyChar (key, mod) );
    } break;

    default:
      eTB_Printf ("OnKeyPress (...)\n");
      break;
  }

  return false;
}
#endif



void
eTB_TextLine::InsertSpan (eTB_Text* pText)
{
  //assert (pText != NULL);
  //assert (! terminated_)
  spans_.push_back (pText);

  if (strstr (pText->GetMessage (), "\n"))
    terminated_ = true;
}

eTB_Text*
eTB_TextLine::getSpan (size_t idx) const
{
  if (idx >= spans_.size ())
    return NULL;

  return spans_ [idx];
}


eTB_Text::eTB_Text ( const char* szMessage,
                     DWORD       color,
                     eFont*      pFont )
{
//    message_ = eStringA (szMessage);
  message_ = strdup (szMessage);
  color_   = color;
#if 0
  font_    = pFont;

  if (pFont == NULL)
  {
    /* Load the cnsole font if it is not already loaded... */
    if (states->r_console_font == NULL) {
      eFontInfo info;
      info.szName         = _tcsdup (_T ("Andale Mono"));
      info.numAlphaShades = 32;
      info.isBold         = false;
      info.isItalic       = false;
      info.isUnderline    = false;
      info.glyphSize      = 16;

      states->r_console_font = new eFont ();
      states->r_console_font->loadFont (&info);
    }

    font_ = states->r_console_font;
  }
#endif
}
