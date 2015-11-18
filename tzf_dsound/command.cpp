#include "command.h"

template <>
str_hash_compare <std::string, std::less <std::string> >::size_type
str_hash_compare <std::string, std::less <std::string> >::hash_string (const std::string& _Keyval) const
{
  const bool case_insensitive = true;

        size_type   __h    = 0;
  const size_type   __len  = _Keyval.size ();
  const value_type* __data = _Keyval.data ();

  for (size_type __i = 0; __i < __len; ++__i) {
    /* Hash Collision Discovered: "r_window_res_x" vs. "r_window_pos_x" */
    //__h = 5 * __h + eTB_CaseAdjust (__data [__i], case_insensitive);

    /* New Hash: sdbm   -  Collision Free (08/04/12) */
    __h = eTB_CaseAdjust (__data [__i], case_insensitive) +
                         (__h << 06)  +  (__h << 16)      -
                          __h;
  }

  return __h;
}


template <>
str_hash_compare <std::string, std::less <std::string> >::size_type
str_hash_compare <std::string, std::less <std::string> >::operator() (const std::string& _Keyval) const
{
  return hash_string (_Keyval);
}

template <>
bool
str_hash_compare <std::string, std::less <std::string> >::operator() (const std::string& _lhs, const std::string& _rhs) const
{
  return hash_string (_lhs) < hash_string (_rhs);
}

class eTB_SourceCmd : public eTB_Command
{
public:
  eTB_SourceCmd (eTB_CommandProcessor* cmd_proc) {
    processor_ = cmd_proc;
  }

  eTB_CommandResult execute (const char* szArgs) {
    /* TODO: Replace with a special tokenizer / parser... */
    FILE* src = fopen (szArgs, "r");

    if (! src) {
      return
        eTB_CommandResult ( "source", szArgs,
                              "Could not open file!",
                                false,
                                  NULL,
                                    this );
    }

    char line [1024];

    static int num_lines = 0;

    while (fgets (line, 1024, src) != NULL) {
      num_lines++;

      /* Remove the newline character... */
      line [strlen (line) - 1] = '\0';

      processor_->ProcessCommandLine (line);

      //printf (" Source Line %d - '%s'\n", num_lines++, line);
    }

    fclose (src);

    return eTB_CommandResult ( "source", szArgs,
                                 "Success",
                                   num_lines,
                                     NULL,
                                       this );
  }

  int getNumArgs (void) {
    return 1;
  }

  int getNumOptionalArgs (void) {
    return 0;
  }

  const char* getHelp (void) {
    return "Load and execute a file containing multiple commands "
           "(such as a config file).";
  }

private:
  eTB_CommandProcessor* processor_;

};

eTB_CommandProcessor::eTB_CommandProcessor (void)
{
  eTB_Command* src = new eTB_SourceCmd (this);

  AddCommand ("source", src);
}

const eTB_Command*
eTB_CommandProcessor::AddCommand (const char* szCommand, eTB_Command* pCommand)
{
  if (szCommand == NULL || strlen (szCommand) < 1)
    return NULL;

  if (pCommand == NULL)
    return NULL;

  /* Command already exists, what should we do?! */
  if (FindCommand (szCommand) != NULL)
    return NULL;

  commands_.insert (eTB_CommandRecord (szCommand, pCommand));

  return pCommand;
}

bool
eTB_CommandProcessor::RemoveCommand (const char* szCommand)
{
  if (FindCommand (szCommand) != NULL) {
    std::unordered_map <std::string, eTB_Command*, str_hash_compare <std::string> >::iterator
      command = commands_.find (szCommand);

    commands_.erase (command);
    return true;
  }

  return false;
}

eTB_Command*
eTB_CommandProcessor::FindCommand (const char* szCommand) const
{
  std::unordered_map <std::string, eTB_Command*, str_hash_compare <std::string> >::const_iterator
    command = commands_.find (szCommand);

  if (command != commands_.end ())
    return (command)->second;

  return NULL;
}



const eTB_Variable*
eTB_CommandProcessor::AddVariable (const char* szVariable, eTB_Variable* pVariable)
{
  if (szVariable == NULL || strlen (szVariable) < 1)
    return NULL;

  if (pVariable == NULL)
    return NULL;

  /* Variable already exists, what should we do?! */
  if (FindVariable (szVariable) != NULL)
    return NULL;

  variables_.insert (eTB_VariableRecord (szVariable, pVariable));

  return pVariable;
}

bool
eTB_CommandProcessor::RemoveVariable (const char* szVariable)
{
  if (FindVariable (szVariable) != NULL) {
    std::unordered_map <std::string, eTB_Variable*, str_hash_compare <std::string> >::iterator
      variable = variables_.find (szVariable);

    variables_.erase (variable);
    return true;
  }

  return false;
}

const eTB_Variable*
eTB_CommandProcessor::FindVariable (const char* szVariable) const
{
  std::unordered_map <std::string, eTB_Variable*, str_hash_compare <std::string> >::const_iterator
    variable = variables_.find (szVariable);

  if (variable != variables_.end ())
    return (variable)->second;

  return NULL;
}



eTB_CommandResult
eTB_CommandProcessor::ProcessCommandLine (const char* szCommandLine)
{
  if (szCommandLine != NULL && strlen (szCommandLine))
  {
    char*  command_word     = strdup (szCommandLine);
    size_t command_word_len = strlen (command_word);

    char*  command_args     = command_word;
    size_t command_args_len = 0;

    /* Terminate the command word on the first space... */
    for (size_t i = 0; i < command_word_len; i++) {
      if (command_word [i] == ' ') {
        command_word [i] = '\0';

        if (i < (command_word_len - 1)) {
          command_args     = &command_word [i + 1];
          command_args_len = strlen (command_args);

          /* Eliminate trailing spaces */
          for (unsigned int j = 0; j < command_args_len; j++) {
            if (command_word [i + j + 1] != ' ') {
              command_args = &command_word [i + j + 1];
              break;
            }
          }

          command_args_len = strlen (command_args);
        }

        break;
      }
    }

    std::string cmd_word (command_word);
    std::string cmd_args (command_args_len > 0 ? command_args : "");
    /* ^^^ cmd_args is what is passed back to the object that issued
             this command... If no arguments were passed, it MUST be
               an empty string. */

    eTB_Command* cmd = command.FindCommand (command_word);

    if (cmd != NULL) {
      return cmd->execute (command_args);
    }

    /* No command found, perhaps the word was a variable? */

    const eTB_Variable* var = command.FindVariable (command_word);

    if (var != NULL) {
      if (var->getType () == eTB_Variable::Boolean)
      {
        if (command_args_len > 0) {
          eTB_VarStub <bool>* bool_var = (eTB_VarStub <bool>*) var;
          bool                bool_val = false;

          /* False */
          if (! (stricmp (command_args, "false") && stricmp (command_args, "0") &&
                 stricmp (command_args, "off"))) {
            bool_val = false;
            bool_var->setValue (bool_val);
          }

          /* True */
          else if (! (stricmp (command_args, "true") && stricmp (command_args, "1") &&
                      stricmp (command_args, "on"))) {
            bool_val = true;
            bool_var->setValue (bool_val);
          }

          /* Toggle */
          else if ( !(stricmp (command_args, "toggle") && stricmp (command_args, "~") &&
                      stricmp (command_args, "!"))) {
            bool_val = ! bool_var->getValue ();
            bool_var->setValue (bool_val);

            /* ^^^ TODO: Consider adding a toggle (...) function to
                           the bool specialization of eTB_VarStub... */
          } else {
            // Unknown Trailing Characters
          }
        }
      }

      else if (var->getType () == eTB_Variable::Int)
      {
        if (command_args_len > 0) {
          int original_val = ((eTB_VarStub <int>*) var)->getValue ();
          int int_val = 0;

          /* Increment */
          if (! (stricmp (command_args, "++") && stricmp (command_args, "inc") &&
                 stricmp (command_args, "next"))) {
            int_val = original_val + 1;
          } else if (! (stricmp (command_args, "--") && stricmp (command_args, "dec") &&
                        stricmp (command_args, "prev"))) {
            int_val = original_val - 1;
          } else
            int_val = atoi (command_args);

          ((eTB_VarStub <int>*) var)->setValue (int_val);
        }
      }

      else if (var->getType () == eTB_Variable::Short)
      {
        if (command_args_len > 0) {
          short original_val = ((eTB_VarStub <short>*) var)->getValue ();
          short short_val    = 0;

          /* Increment */
          if (! (stricmp (command_args, "++") && stricmp (command_args, "inc") &&
                 stricmp (command_args, "next"))) {
            short_val = original_val + 1;
          } else if (! (stricmp (command_args, "--") && stricmp (command_args, "dec") &&
                        stricmp (command_args, "prev"))) {
            short_val = original_val - 1;
          } else
            short_val = (short)atoi (command_args);

          ((eTB_VarStub <short>*) var)->setValue (short_val);
        }
      }

      else if (var->getType () == eTB_Variable::Float)
      {
        if (command_args_len > 0) {
//          float original_val = ((eTB_VarStub <float>*) var)->getValue ();
          float float_val = (float)atof (command_args);

          ((eTB_VarStub <float>*) var)->setValue (float_val);
        }
      }

      free (command_word);

      return eTB_CommandResult (cmd_word, cmd_args, var->getValueString (), true, var, NULL);
    } else {
      free (command_word);

      /* Default args --> failure... */
      return eTB_CommandResult (cmd_word, cmd_args); 
    }
  } else {
    /* Invalid Command Line (not long enough). */
    return eTB_CommandResult (szCommandLine); /* Default args --> failure... */
  }
}

/** Variable Type Support **/


template <>
eTB_VarStub <bool>::eTB_VarStub ( bool*                  var,
                                  eTB_iVariableListener* pListener ) :
                                          var_ (var)
{
  listener_ = pListener;
  type_     = Boolean;
}

template <>
std::string
eTB_VarStub <bool>::getValueString (void) const
{
  if (getValue ())
    return std::string ("true");
  else
    return std::string ("false");
}

template <>
eTB_VarStub <const char*>::eTB_VarStub ( const char**           var,
                                         eTB_iVariableListener* pListener ) :
                                                 var_ (var)
{
  listener_ = pListener;
  type_     = String;
}

template <>
eTB_VarStub <int>::eTB_VarStub ( int*                  var,
                                 eTB_iVariableListener* pListener ) :
                                         var_ (var)
{
  listener_ = pListener;
  type_     = Int;
}

template <>
std::string
eTB_VarStub <int>::getValueString (void) const
{
  char szIntString [32];
  snprintf (szIntString, 32, "%d", getValue ());

  return std::string (szIntString);
}


template <>
eTB_VarStub <short>::eTB_VarStub ( short*                 var,
                                   eTB_iVariableListener* pListener ) :
                                         var_ (var)
{
  listener_ = pListener;
  type_     = Short;
}

template <>
std::string
eTB_VarStub <short>::getValueString (void) const
{
  char szShortString [32];
  snprintf (szShortString, 32, "%d", getValue ());

  return std::string (szShortString);
}


template <>
eTB_VarStub <float>::eTB_VarStub ( float*                 var,
                                   eTB_iVariableListener* pListener ) :
                                         var_ (var)
{
  listener_ = pListener;
  type_     = Float;
}

template <>
std::string
eTB_VarStub <float>::getValueString (void) const
{
  char szFloatString [32];
  snprintf (szFloatString, 32, "%f", getValue ());

  return std::string (szFloatString);
}

eTB_CommandProcessor command;