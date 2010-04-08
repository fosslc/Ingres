$ set noon
$!
$!  HISTORY
$!    09-feb-1994 (huffman)
$!           Inherited version from ricka.  This will build the correct
$!           testtool saveset ( stagearea_d )
$!    06-Jun-1994 (mhuishi)
$!           Added coord.com and slave.com
$!    20-Jun-1995 (dougb)
$!	     We now build Achilles for AXP/VMS as well.
$!    05-Jul-1995 (albany)
$!	     Since we build echo.exe, let's put in the saveset, too.
$!    11-Jul-1997 (rosga02)
$!	     Add CTSNODES.COM to be copied to [tools.bin]
$!    09-jul-1998 (kinte01)
$!	     Put sed,lex,& yacc in the saveset also
$!    13-jul-1998 (kinte01)
$!	     modify rosga02's undocumented change to only copy the 
$!	     SEVMS specific files if II_SEVMS = SEVMSBUILD
$!    23-oct-1998 (kinte01)
$!	     fix typo -- copied SED twice instead of SED & YACC
$!    06-jan-1999 (kinte01)
$!	     Copy new executables for Achilles Stress test suite that used
$!	     to be built by QA - actests, gcf, bugs, callt
$!    02-feb-1999 (kinte01)
$!	     Renamed all files to version 1
$!    22-Oct-2000 (horda03)
$!           For AXM_VMS builds need to copy SEPCC_AXM.COM to
$!           SEPCC.COM.
$!    10-Nov-2000 (horda03) Bug 59343
$!           Copy RUN.COM to [TOOLS.UTILITY] and [TOOLS.BIN]
$!    25-Jul-2008 (upake01) Bug 97260
$!           Copy QAWTL.EXE to [TOOLS.BIN]
$!==============================================================================
$ cwd=f$env("default")
$ set default II_RELEASE_DIR
$ if $severity .ne. 1 then goto bad_cd
$ set default [.d]
$ if f$search("tools.dir") .eqs. "" then create /dir [.tools]
$ set default [.tools]
$ if f$search("bin.dir") .eqs. "" then create /dir [.bin]
$ if f$search("files.dir") .eqs. "" then create /dir [.files]
$ if f$search("utility.dir") .eqs. "" then create /dir [.utility]
$!
$ copy ING_BUILD:[bin]DELETER.EXE		[.bin]
$!
$ copy ING_TOOLS:[bin]ACCOPIER.EXE	[.bin]
$ copy ING_TOOLS:[bin]ACHILLES.EXE	[.bin]
$ copy ING_TOOLS:[bin]READLOG.EXE	[.bin]
$ copy ING_TOOLS:[bin]VALID_USER.EXE	[.bin]
$!
$ copy ING_TOOLS:[bin]ACSTART.COM	[.bin]
$!
$ copy ING_TOOLS:[bin]EXECUTOR.EXE	[.bin]
$ copy ING_TOOLS:[bin]LISTEXEC.EXE	[.bin]
$ copy ING_TOOLS:[bin]PEDITOR.EXE	[.bin]
$ copy ING_TOOLS:[bin]SEPTOOL.EXE	[.bin]
$ copy ING_TOOLS:[bin]SETUSER.EXE	[.bin]
$ copy ING_TOOLS:[bin]ECHO.EXE		[.bin]
$ copy ING_TOOLS:[bin]QAWTL.EXE		[.bin]
$!
$ copy ING_SRC:[testtool.sep.files_unix_win]BASICKEYS.SEP	[.files]
$ copy ING_SRC:[testtool.sep.files_unix_win]COMMANDS.SEP	[.files]
$ copy ING_SRC:[testtool.sep.files_unix_win]HEADER.TXT	[.files]
$ copy ING_SRC:[testtool.sep.files_unix_win]MAKELOCS.KEY	[.files]
$ copy ING_SRC:[testtool.sep.files_unix_win]SEPTERM.SEP	[.files]
$ copy ING_SRC:[testtool.sep.files_unix]TERMCAP.SEP	[.files]
$ copy ING_SRC:[testtool.sep.files_unix_win]VT100.SEP	[.files]
$ copy ING_SRC:[testtool.sep.files_unix_win]VT220.SEP	[.files]
$!
$ copy ING_TOOLS:[bin]RUN.COM		[.utility]
$ copy ING_TOOLS:[bin]SEPCC.COM		[.utility]
$ copy ING_TOOLS:[bin]SEPESQLC.COM	[.utility]
$ copy ING_TOOLS:[bin]SEPLIB.COM	[.utility]
$ copy ING_TOOLS:[bin]SEPLNK.COM	[.utility]
$!
$ copy ING_TOOLS:[bin]IIBUILDTOOLS.COM	[]
$ copy ING_TOOLS:[bin]VERSION.COM	[.utility]
$!
$ copy ING_TOOLS:[bin]MTS.COM		[.bin]
$ copy ING_TOOLS:[bin]MTSCOORD.EXE	[.bin]
$ copy ING_TOOLS:[bin]MTSSLAVE.EXE	[.bin]
$ copy ING_TOOLS:[bin]GETSEED.EXE	[.bin]
$!
$ copy ING_TOOLS:[bin]CTS.COM		[.bin]
$ copy ING_TOOLS:[bin]Coord.COM		[.bin]
$ copy ING_TOOLS:[bin]slave.COM		[.bin]
$ copy ING_TOOLS:[bin]ctsnodes.com	[.bin]
$ copy ING_TOOLS:[bin]CTSCOORD.EXE	[.bin]
$ copy ING_TOOLS:[bin]CTSSLAVE.EXE	[.bin]
$!
$ copy ING_TOOLS:[bin]MSFCRUN.COM	[.bin]
$ copy ING_TOOLS:[bin]MSFCSET.COM	[.bin]
$ copy ING_TOOLS:[bin]MSFCSLV.COM	[.bin]
$ copy ING_TOOLS:[bin]MSFCSUB.COM	[.bin]
$ copy ING_TOOLS:[bin]MSFCCHK.EXE	[.bin]
$ copy ING_TOOLS:[bin]MSFCCLDB.EXE	[.bin]
$ copy ING_TOOLS:[bin]MSFCCLUP.EXE	[.bin]
$ copy ING_TOOLS:[bin]MSFCCREA.EXE	[.bin]
$ copy ING_TOOLS:[bin]MSFCDRV.EXE	[.bin]
$ copy ING_TOOLS:[bin]MSFCSYNC.EXE	[.bin]
$ copy ING_TOOLS:[bin]MSFCTEST.EXE	[.bin]
$ goto end
$!
$! copy Achilles Stress Test Suite Executables
$!
$ copy  ING_TOOLS:[bin]AC001.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC002.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC003.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC004.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC005.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC006.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC007.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC008.EXE	[.bin]
$ copy  ING_TOOLS:[bin]AC011.EXE	[.bin]
$ copy  ING_TOOLS:[bin]ACTEMPLATE.EXE	[.bin]
$ copy  ING_TOOLS:[bin]COMPRESS.EXE	[.bin]
$ copy  ING_TOOLS:[bin]TP1LOAD.EXE	[.bin]
$ copy  ING_TOOLS:[bin]UNCOMPRESS.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP1.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP2.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP3.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP4.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP5.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP6.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP7.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP8.EXE	[.bin]
$ copy  ING_TOOLS:[bin]BGRP9.EXE	[.bin]
$ copy  ING_TOOLS:[bin]B_BUGINFO.EXE	[.bin]
$ copy  ING_TOOLS:[bin]B_VERSINFO.EXE	[.bin]
$ copy  ING_TOOLS:[bin]C_TSINFO.EXE	[.bin]
$ copy  ING_TOOLS:[bin]P_PATCH.EXE	[.bin]
$ copy  ING_TOOLS:[bin]CALLMIX.EXE	[.bin]
$ copy  ING_TOOLS:[bin]CTRP1.EXE	[.bin]
$ copy  ING_TOOLS:[bin]CTRP2.EXE	[.bin]
$ copy  ING_TOOLS:[bin]CTRP3.EXE	[.bin]
$ copy  ING_TOOLS:[bin]CTRP4.EXE	[.bin]
$ copy  ING_TOOLS:[bin]CTRP5.EXE	[.bin]
$ copy  ING_TOOLS:[bin]GRAB.EXE	[.bin]
$ copy  ING_TOOLS:[bin]HANDLE.EXE	[.bin]
$ copy  ING_TOOLS:[bin]KNOWLEDGE.EXE	[.bin]
$ copy  ING_TOOLS:[bin]NEWCALL.EXE	[.bin]
$ copy  ING_TOOLS:[bin]YOURCALL.EXE	[.bin]
$ copy  ING_TOOLS:[bin]ACH_ACTESTS.COM [.utility]
$ copy  ING_TOOLS:[bin]RUN_ACTESTS.COM [.utility]
$ copy  ING_TOOLS:[bin]SETUP_ACTESTS.COM [.utility]
$ copy  ING_TOOLS:[bin]ACH_BUGS.COM [.utility]
$ copy  ING_TOOLS:[bin]RUN_BUGS.COM [.utility]
$ copy  ING_TOOLS:[bin]SETUP_BUGS.COM [.utility]
$ copy  ING_TOOLS:[bin]ACH_CALLTRACK.COM [.utility]
$ copy  ING_TOOLS:[bin]RUN_CALLTRACK.COM [.utility]
$ copy  ING_TOOLS:[bin]SETUP_CALLTRACK.COM [.utility]
$ copy  ING_TOOLS:[bin]ACERROR.COM [.utility]
$ copy  ING_TOOLS:[bin]ACH_STDENV.COM [.utility]
$ copy  ING_TOOLS:[bin]ACH_STRESS.COM [.utility]
$ copy  ING_TOOLS:[bin]ACMSG.COM [.utility]
$ copy  ING_TOOLS:[bin]ACSLEEP.COM [.utility]
$ copy  ING_TOOLS:[bin]DROPTABLES.COM [.utility]
$ copy  ING_TOOLS:[bin]RUN_STRESS.COM [.utility]
$ copy  ING_TOOLS:[bin]SETUP_STRESS.COM [.utility]
$ copy  ING_TOOLS:[bin]STRESSBUILD.COM [.utility]
$ copy  ING_TOOLS:[bin]TARSTRESS.COM [.utility]
$!
$end:
$ set default 'cwd'
$ exit
$bad_cd:
$ write sys$output "Cannot set default to II_RELEASE_DIR"
$ exit %X100184CC
