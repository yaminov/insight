/* Insight Definitions for GDB, the GNU debugger.
   Written by Keith Seitz <kseitz@sources.redhat.com>

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

   This file is part of Insight.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#include "defs.h"
#include "interps.h"
#include "target.h"
#include "ui-file.h"
#include "ui-out.h"
#include "cli-out.h"
#include <string.h>
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "exceptions.h"
#include "event-loop.h"
#include "top.h"

#include "tcl.h"
#include "tk.h"
#include "gdbtk.h"

#ifdef __MINGW32__
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif


static void hack_disable_interpreter_exec (char *, int);
void _initialize_gdbtk_interp (void);

struct gdbtk_interp_data
{
  struct ui_file *_stdout;
  struct ui_file *_stderr;
  struct ui_file *_stdlog;
  struct ui_file *_stdtarg;
  struct ui_file *_stdtargin;
  struct ui_out *uiout;
};

/* See note in gdbtk_interpreter_init */
static void
hack_disable_interpreter_exec (char *args, int from_tty)
{
  error ("interpreter-exec not available when running Insight");
}

static void *
gdbtk_interpreter_init (struct interp *interp, int top_level)
{
  /* Disable interpreter-exec. It causes us big trouble right now. */
  struct cmd_list_element *cmd = NULL;
  struct cmd_list_element *alias = NULL;
  struct cmd_list_element *prefix = NULL;
  struct gdbtk_interp_data *data;

  data = XCNEW (struct gdbtk_interp_data);
  data->_stdout = gdbtk_fileopen ();
  data->_stderr = gdbtk_fileopen ();
  data->_stdlog = gdbtk_fileopen ();
  data->_stdtarg = gdbtk_fileopen ();
  data->_stdtargin = gdbtk_fileopenin ();
  data->uiout = cli_out_new (data->_stdout),

  gdbtk_init ();

  if (lookup_cmd_composition ("interpreter-exec", &alias, &prefix, &cmd))
    {
      set_cmd_cfunc (cmd, hack_disable_interpreter_exec);
    }

  return data;
}

static int
gdbtk_interpreter_resume (void *data)
{
  static int started = 0;
  struct gdbtk_interp_data *d = (struct gdbtk_interp_data *) data;
  gdbtk_add_hooks ();

  gdb_stdout = d->_stdout;
  gdb_stderr = d->_stderr;
  gdb_stdlog = d->_stdlog;
  gdb_stdtarg = d->_stdtarg;
  gdb_stdtargin = d->_stdtargin;

  /* 2003-02-11 keiths: We cannot actually source our main Tcl file in
     our interpreter's init function because any errors that may
     get generated will go to the wrong gdb_stderr. Instead of hacking
     our interpreter init function to force gdb_stderr to our ui_file,
     we defer sourcing the startup file until now, when gdb is ready
     to let our interpreter run. */
  if (!started)
    {
      started = 1;
      gdbtk_source_start_file ();
    }

  return 1;
}

static int
gdbtk_interpreter_suspend (void *data)
{
  return 1;
}

static struct gdb_exception
gdbtk_interpreter_exec (void *data, const char *command_str)
{
  return exception_none;
}

/* Callback telling gdb we are supporting command editing. */
static int
gdbtk_supports_command_editing (struct interp *self)
{
  return 0;
}

/* This function is called before entering gdb's internal command loop.
   This is the last chance to do anything before entering the event loop. */

static void
gdbtk_pre_command_loop (struct interp *self)
{
  /* We no longer want to use stdin as the command input stream: disable
     events from stdin. */
  main_ui->input_fd = -1;

  if (Tcl_Eval (gdbtk_interp, "gdbtk_tcl_preloop") != TCL_OK)
    {
      const char *msg;

      /* Force errorInfo to be set up propertly.  */
      Tcl_AddErrorInfo (gdbtk_interp, "");

      msg = Tcl_GetVar (gdbtk_interp, "errorInfo", TCL_GLOBAL_ONLY);
#ifdef _WIN32
      MessageBox (NULL, msg, NULL, MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
      fputs_unfiltered (msg, gdb_stderr);
#endif
    }

#ifdef _WIN32
  close_bfds ();
#endif
}

static struct ui_out *
gdbtk_interpreter_ui_out (struct interp *interp)
{
  struct gdbtk_interp_data *data;

  data = (struct gdbtk_interp_data *) interp_data (interp);
  return data->uiout;
}

/* Factory for GUI interpreter. */
static struct interp *
gdbtk_interp_factory (const char *name)
{
  static const struct interp_procs gdbtk_interp_procs = {
    gdbtk_interpreter_init,             /* init_proc */
    gdbtk_interpreter_resume,           /* resume_proc */
    gdbtk_interpreter_suspend,	        /* suspend_proc */
    gdbtk_interpreter_exec,             /* exec_proc */
    gdbtk_interpreter_ui_out,		/* ui_out_proc */
    NULL,                               /* set_logging_proc */
    gdbtk_pre_command_loop,             /* pre_command_loop_proc */
    gdbtk_supports_command_editing      /* supports_command_editing_proc */
  };

  return interp_new (name, &gdbtk_interp_procs, NULL);
}

void
_initialize_gdbtk_interp (void)
{
  /* Does not run in target-async mode. */
  target_async_permitted = 0;
  interp_factory_register (INTERP_INSIGHT, gdbtk_interp_factory);
}
