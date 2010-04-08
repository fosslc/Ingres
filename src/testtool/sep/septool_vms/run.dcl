$!-----------------------------------------------------------------------------
$! History:
$!---------
$! Created  ?
$!
$! Modified
$!    12-06-2001  (bolke01)
$!                Added increased functionality as described in the Usage notes"
$!    18-08-2009  (horda03)
$!                Command was failing if the directory path started with a $.
$!
$! Inputs
$!-------
$!    P1        valid filename or symbol
$!    P2 - P8   parameters to P1
$!
$! Outputs
$!--------
$!     None
$!
$! Description:
$!-------------
$!
$! This function will run anything that is passed in that exists in
$! either the local directory, is accessed via a symbol, or is in
$! a remote directory that is fully specified.
$!
$! A check is made on the file type and only STMLF files are treated
$! as command files.  All others are executed by adding a '$' to the
$! passed in file name
$!
$!-----------------------------------------------------------------------------
$ def_ver = f$verify(0)
$ if f$type(test).nes."INTEGER" then $ test == 0
$ t = f$verify(test)
$ def_dir = f$environment("DEFAULT")
$!
$ runfile = p1
$!
$ if runfile - "HELP" -"?" .nes. runfile .or. "''p1'" .eqs. ""
$ then
$  write sys$output " "
$  write sys$output "Usage run.com"
$  write sys$output " "
$  write sys$output "   @run.com [?|HELP|runfile[.COM|.EXE] p2 [p3] [p4] ... [p8] "
$  write sys$output "   where runfile  is one of:"
$  write sys$output "       1   symbol with or without embedded parameters"
$  write sys$output " "
$  write sys$output "       2   local command file - normal extension = .COM "
$  write sys$output "               ( no extension = .COM or .EXE with precidence on .COM)"
$  write sys$output " "
$  write sys$output "       3   local executable - normal extension .EXE"
$  write sys$output " "
$  write sys$output "       4   executable from another location - normal extension .EXE"
$  write sys$output " "
$  write sys$output "   NOTE: The file attribute RFM is checked for correct type :"
$  write sys$output "          RFM = STMLF is treated as a command file
$  write sys$output "          all others are considered to be executables"
$  write sys$output " "
$  write sys$output " "
$  exit
$ endif
$!
$ have_dollar = 0			     	! assume no dollar in position 0 to start with
$ tmpparam = ""					! reset var for holding stripped embedded params
$!
$ gosub eval_parameters
$!
$ if nr_of_p.eq.0 then $ exit
$!
$! Check to see if file exists, or if the extension isn't defined if its a .com file
$
$ tmprun = f$parse(runfile,"''def_dir'.COM")
$ if f$search("''tmprun'") .nes. ""
$ then
$    cmd_file = "''tmprun'"
$ else
$    ! ** does not exist so check for EXE
$    tmprun = f$parse("''runfile'","''def_dir'.EXE")
$    if f$search(tmprun) .nes. ""
$    then
$       cmd_file = "''tmprun'"
$    else
$       ! ']', ':' or '.' in the name mean it can't be a symbol
$       if runfile - "]" - ":" - "." .NES. runfile
$       then
$           bad_cmd = 1
$       else
$          ! check if a symbol
$          if f$type( 'runfile ) .EQS. ""
$          then
$             bad_cmd = 1
$          else
$             bad_cmd = 0
$          endif
$       endif
$
$       if bad_cmd
$       then
$          write sys$output "Not found [''P1'] as ''tmprun'"
$          exit
$       endif
$
$       cmd = runfile			     	! symbol - do nothing
$
$       GOTO run_cmd
$    endif
$ endif
$
$ ! To reach here means we have identified the file to execute, so use
$ ! the appropriate execute command.
$
$ if f$edit(f$parse("''cmd_file'",,,"TYPE"), "upcase") .EQS. ".EXE"
$ then
$    cmd := mc 'cmd_file'
$ else
$    cmd := @'cmd_file'
$ endif
$!
$run_cmd:
$ 'cmd' 'p2' 'p3' 'p4' 'p5' 'p6' 'p7' 'p8'
$!
$ exit
$!*********************************************************************!
$eval_parameters:
$!**********************************************************************!
$!
$ nr_of_p = 0
$ i = 1
$ j = 1
$_start_eval_loop:
$ if i.eq.9   then $ goto _end_eval_loop
$ p = p'i'
$ if p.eqs."" then $ goto _cont_eval_loop
$!
$ if i.ne.j then $ p'j' = p
$ nr_of_p = j
$ j = j + 1
$!
$_cont_eval_loop:
$ i = i + 1
$ goto _start_eval_loop
$_end_eval_loop:
$!
$ i = nr_of_p + 1
$_start_eval_loop2:
$ if i.eq.9 then $ goto _end_eval_loop2
$!
$ p'i' = ""
$!
$ i = i + 1
$ goto _start_eval_loop2
$_end_eval_loop2:
$!
$ return
$!****************************************************!
$ exit
