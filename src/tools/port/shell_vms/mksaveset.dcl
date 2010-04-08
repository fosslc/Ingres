$! MKSAVESET.COM
$!	This script makes a saveset from the II_RELEASE_DIR.  The saveset is
$!	put into the [.saveset] directory under II_RELEASE_DIR.
$!
$! PARAMETERS:
$!	P1 = Saveset identifier (e.g. "640002A")
$!	P2 = "NO RENAME REQUIRED"  (optional set if RENDUPFIL has already
$!                                  been carried out)
$!
$!! History:
$!!	10-jul-91 (sandyd)
$!!		Created for spliced-together FE/BE toolset.
$!!	12-aug-91 (sandyd)
$!!		Added tools saveset for Product Testing.
$!!	15-aug-91 (sandyd)
$!!		Added support saveset for archiving .MAP files.  Also changed
$!!		backup commands to put [INGRES.DEBUG]*.STB files onto the
$!!		main saveset.
$!!	19-sep-91 (sandyd)
$!!		One more change to exclude the version.rel from the debug
$!!		directory on the non-debug saveset.  (This is a nasty business,
$!!		putting SOME of the debug directory on one saveset and SOME
$!!		on the other.  If new filetypes ever get added, God help us.)
$!!	15-jun-92 (sandyd)
$!!		Removed "/buffer" option since it is obsolete as of VMS 5.3.
$!!	22-feb-93 (sandyd)
$!!		Completely reworked to conform to new "buildrel" subdirectory
$!!		structure.
$!!	24-jun-93 (ricka)
$!!		Onyx doesn't like savesets that begin with [r65.stagearea...]
$!!	10-sep-93 (ricka)
$!!		will now be installing with VMSINSTAL, which does not like the
$!!		.bck extensions.  now using .A .B .C .D. and .E.
$!!      12-oct-93 (huffman)
$!!              1) rename all of the front_stage files to be version ';1'
$!!              2) if AXP don't use any block size.
$!!	27-oct-93 (ricka)
$!!		saveset A now starts at [INGRES]
$!!      20-jan-94 (mhuishi)
$!!              changed saveset filenames to ingres010
$!!      26-sep-94 (wolf) 
$!!              Formalize RickA's earlier changes to the saveset name. 
$!!	21-Apr-98 (kinte01)
$!!		Changed saveset filenames to Ingres instead of OpenIngres
$!!	15-Nov-2000 (kinte01)
$!!		Added building of Licensing saveset for better inclusion
$!!		into the release
$!!	21-jun-2004 (abbjo03)
$!!	    Clean up code for open source.
$!!	28-feb-2005 (abbjo03)
$!!	    Rewrite for open source Jam build.
$!!	17-mar-2005 (abbjo03)
$!!	    The subdirectory for QA tools is 'tools' not 'ingres'.
$!!     08-Jan-2008 (bolke01) 119936
$!!         Added the renaming of the saveset files as a pre-requisite.
$!!     14-May-2008 (horda03)
$!!         The "renfile" variable is being defined incorrectly.
$!!         Also remove the renfile so that it will be created.
$!!     11-Feb-2010 (horda03)
$!!         Allow the "Renaming files" to be skipped. Note the
$!!         'renfile' must exist.
$!!
$!=============================================================================
$ if (p1 .eqs. "") then goto USAGE
$ set noon
$ write sys$output "Creating savesets for ", f$trnlnm("II_RELEASE_DIR")
$ set default II_RELEASE_DIR
$ if ($severity .ne. 1) then goto BAD_CD
$ if f$getsyi("hw_model") .ge. 1024
$ then	blksize :=
$ else	blksize := "/block_size=8192"
$ endif
$ stagedir=f$env("default")
$ dev=f$parse(stagedir,,,"device","no_conceal")
$ path=f$parse(stagedir,,,"directory","no_conceal")
$ stagedir=dev+path-"]["-"]"
$ define /nolog /trans=conc stagearea 'stagedir'.]
$ if f$search("saveset.dir") .eqs. "" then create /dir [.saveset]
$!
$!--- VMSINSTAL does not handle duplicate file names
$!
$!    rendupfil identifies duplicate files and adds the directory
$!    as an extension to the file-type.
$!    RENAMED.LST is created in II_RELEASE_DIR:[A.INGRES.INSTALL]
$!
$ reldir = f$trnlnm("II_RELEASE_DIR")
$ renfile = f$parse(reldir,,,,"no_conceal") - "][" - "].;" + -
        ".A.INGRES.INSTALL]RENAMED.LST"
$!
$ if (p2 .nes. "NO RENAME REQUIRED")
$ then
$    if f$search("''renfile'") .nes. "" 
$    then 
$       delete/nolog 'renfile';*
$    endif
$!
$    write sys$output "Renaming files..."
$!
$    rendupfil
$ endif
$!
$! Check RENAMED.LST was created successfully
$!
$ if f$search("''renfile'") .eqs. "" then goto RENAME_ERR
$!
$while:
$	dir = f$parse(f$search("*.dir"),,,"name")
$	if dir .eqs. "" then goto endwhile
$	if f$edit(dir, "lowercase") .eqs. "saveset" then goto while
$	purge /log [.'dir'...]
$	rename [.'dir'...]*.*;*/excl=;1 [.'dir'...]*.*;1
$	define /nolog /trans=conc stagearea_'dir' 'stagedir'.'dir'.]
$	if dir .eqs. "d"
$	then subdir = "tools"
$	else subdir = "ingres"
$	endif
$	backup 'blksize' /log STAGEAREA_'dir:['subdir'...] -
		STAGEAREA:[saveset]Ingres_'p1'.'dir'/save_set/nocrc/group=0
$!
$!	Now make a listing, for comparisons between savesets.
$!
$	backup/list=STAGEAREA:[saveset]Ingres_'p1'.'dir'_lis -
		STAGEAREA:[saveset]Ingres_'p1'.'dir'/save_set
$	goto while
$endwhile:
$ exit
$
$RENAME_ERR:
$ write sys$output "FAIL - Renaming duplicate filenames failed."
$ exit
$
$BAD_CD:
$ write sys$output "Cannot CD to stage area."
$ exit
$
$USAGE:
$ write sys$output "Must specify a saveset identifier (e.g. ""302104A"")"
$ exit
