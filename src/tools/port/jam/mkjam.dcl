$!  Copyright (c) 2004, 2005  Ingres Corporation
$!
$!
$!  Name: mkjam -- Generate Jamfile
$!
$!  Usage:
$!       mkjam [dir1,dir2....,dirn]
$!
$!  Description:
$!	This script can be used to generate Jamfiles in new facilities
$!	or update existing Jamfiles when new files have been added
$!
$!  Exit Status:
$!	 1		Jamfile successfully created
$!	 %x10000012	Creation of Jamfile failed
$!
$!! History: 
$!!	02-sep-2004 (abbjo03)
$!!	    Created from mkjam.sh.
$!!	03-nov-2004 (abbjo03)
$!!	    Test first for VMS when deciding which directories to include.
$!!	16-nov-2004 (abbjo03)
$!!	    Initialize ext each time through the directories loops.
$!!	10-jan-2005 (abbjo03)
$!!	    Don't create a Jamfile for front!ice. Only delete an existing
$!!	    Jamfile right before creating the new one.
$!!	11-jan-2005 (abbjo03)
$!!	    Don't create a Jamfile for front!embed.
$!!	28-jan-2005 (abbjo03)
$!!	    The changes to skip over front!ice and front!embed need to continue
$!!	    the loop, not exit the script.
$!!	04-aug-2005 (abbjo03)
$!!	    Support ING_ROOT being defined as incomplete device or logical,
$!!	    e.g., dkXN: or usrN:, instead of node$dkXN:.
$!!     19-dec-2007 (horda03/bolke01) 119936
$!!         Re-write parts to make the mkjam more maintainable.
$!!     14-Jul-2009 (horda03)
$!!         As VMS doesn't have symbolic links, can't use 1 area on a machine
$!!         for things like PATCHTOOLS. Allow ING_SRC to be defined as a
$!!         multipart concealled device.
$!!     29-Nov-2009 (horda03)
$!!         Make "d" lowercase.
$!!
$ echo := write sys$output
$ origwd = f$env("default")
$ if f$trnl("ING_VERS") .nes. "" then noise=".''f$trnl("ING_VERS")'.src"
$ if f$trnl("ING_SRC") .eqs. ""
$ then echo "ING_SRC: must be set"
$	goto exit_fail
$ endif
$ do_64bit="false"
$!
$! Parse arguments, if any
$ i = 1
$ dirs=""
$while:
$	if p'i .eqs. "" then goto endwhile
$	if p'i .eqs. "-LP64"
$	then	do_64bit="true"
$	else if f$extr(0, 1, p'i) .eqs. "-"
$	then	echo "Illegal option: ", p'i
$		echo "Usage:"
$		goto usage1
$	else	dirs=dirs + p'i + " "
$	endif
$	endif
$	i = i + 1
$	if i .le. 8 then goto while
$endwhile:
$ dirs=f$edit(dirs,"lowercase,trim")
$ platform="vms"
$!
$! Default Jamfile name
$ jamdflt="Jamfile"
$!jamdflt64="Jamfile64"
$ jamdbg="Jamdbg"
$!jamdbg64="Jamdbg64"
$!
$! Build the Jamfile files
$ if dirs .eqs. "" then dirs=origwd
$ i = 0
$forloop:
$	d=f$edit(f$elem("''i'", " ", "''dirs'"), "lowercase")
$	if "''d'" .eqs. " " then goto endloop
$!NOTE: the following is in a subshell on Unix
$
$       SET DEF ING_SRC:[000000]
$       sd=F$EDIT(F$PARSE(d,,,,"NO_CONCEAL") - "][" - ".;" - ".000000", "lowercase")
$       if f$element( 0, ":", d) .nes. "common_ing_src"
$       then
$          if "''sd'" .eqs. ""
$          then 
$             echo "ERROR: ''d'"
$             goto contloop
$          else
$             l=f$len(sd)
$          endif
$
$          d=sd
$       else
$          d = F$EDIT(F$PARSE(d) - ".;", "lowercase")
$   	   l=f$len(d)
$       endif
$
$	ext=""
$	if f$locate("_vms", d) .ne. l
$	then	
$          ext="vms"
$	else	
$          if f$locate("_unix", d) .ne. l
$	   then	
$             ext="unix"
$	   else	
$             if f$locate("_win", d) .ne. l
$	      then
$   	         ext="win"
$	      endif
$	   endif
$	endif
$	if "''ext'" .nes. "" .and. ext .nes. platform then goto contloop
$	set default 'sd
$ if do_64bit
$ then	
$       LP64="64"
$!--- Main processing
$ else
$	LP64=""
$	LP64DIR=""
$	if "''jamfile'" .eqs. "" then jamfile=jamdflt
$!	write out Jamdbg file for debug builds in all directories
$	if f$sear(jamdbg) .nes. "" then delete 'jamdbg.;*
$	create 'jamdbg.
#
# INGRES Jamfile DBG file
#

CCMACH = $(CCMACH_DBG)
CCLDMACH = $(CCLDMACH_DBG)
OPTIM = $(OPTIM_DBG)

$	open /app out 'jamdbg.
$	write out "Include ''jamfile' ;"
$	close out
$ endif
$!
$! Tokenize the current directory name and put the pieces into the
$! positional parameters ; then parse the pieces to determine group,
$! facility, subsystem, and "action" (ACT).
$! For example: back/dmf/dml is GROUP=$1, FACILITY=$2, SUBSYSTEM=$3;
$! for BAROQUE paths: GROUP=$1, FACILITY=$2, SUBSYSTEM=$5
$!
$  ingsrcpath=F$EDIT( F$PARSE( "ING_SRC:[000000]",,,,"NO_CONCEAL") - -
                      ".][000000].;" + "]", "lowercase")
$  if "''d'" .eqs. "''ingsrcpath'"
$  then
$     dir="ING_SRC"
$     p1=dir
$  else
$     if f$element(0, ":", d) .eqs. "common_ing_src"
$     then
$        dir = f$element(1, "[", d) - "]"
$     else
$        dir = d - (ingsrcpath - "]") -"]" - "."
$     endif
$
$     IF dir .eqs. (d - "]" - ".")
$     THEN
$        ! This isn't a SUB DIR od ingsrcpath so ERROR ?
$
$        echo "ERROR: ''d' not a sub-directory of ING_SRC"
$
$        goto endloop
$     endif
$
$     p1=F$ELEMENT(0, ".", dir) - "."
$     p2=F$ELEMENT(1, ".", dir) - "."
$     p3=F$ELEMENT(2, ".", dir) - "."
$     p4=F$ELEMENT(3, ".", dir) - "."
$ endif
$!
$ GROUP="''p1'"
$ if "''dir'" .eqs. "ING_SRC"	! Top level ING_SRC directory
$ then	ACT="top"
$ else if dir .eqs. "admin" .or. dir .eqs. "back" .or. dir .eqs. "cl" .or. -
	dir .eqs. "common" .or. dir .eqs. "dbutil" .or. dir .eqs. "front" .or. -
	dir .eqs. "gl" .or. dir .eqs. "sig" .or. dir .eqs. "testtool" .or. -
	dir .eqs. "tools"
$	then	ACT="group"
$	else if f$len(dir-"."-"."-".") .eq. f$len(dir)-3
$	then	FACILITY="''p2'"
$		SUBSYS="''p3'"
$		COMPONENT="''p4'"
$		ACT="demos"
$	else if f$extr(0,4, dir) .eqs. "sig." .and. -
			f$len(dir-"."-".") .eq. f$len(dir)-2
$	then	FACILITY="''p2'"
$		SUBSYS="''p3'"
$		ACT="sfiles"
$!                */*/*) #6.0
$	else if f$len(dir-"."-".") .eq. f$len(dir)-2
$	then	FACILITY="''p2'"
$		SUBSYS="''p3'"
$		ACT="source"
$!                  */*) #6.0 cl                  ## What does this mean -- cl?
$!						  ## Is cl somehow different?
$	else if f$len(dir-".") .eq. f$len(dir)-1
$	then	SUBSYS="''p2'"
$		ACT="facility"
$	endif
$	endif
$	endif
$	endif
$	endif
$ endif
$! Set uppercase subsys (USUBSYS) for prepending to LIB
$! and initial-cap subsys (LSUBSYS) for w4gldemo stuff
$ UFACILITY=f$edit("''FACILITY'", "upcase")
$!
$ if "''ACT'" .eqs. "" then ACT="source"
$!
$! Set generic values for BINDIR and FILDIR
$ BINDIR="$(INGBIN''LP64')"
$ FILDIR="$(INGFILES)"
$ MAN1DIR="$(TOOLSMAN1)"
$!
$! Point LIB variable appropriately 
$! HDR's defined in Jamrules file.
$!
$ LIB="$(LIBRARY)"
$ if GROUP .eqs. "back" .or. GROUP .eqs. "common"
$ then	LIB="''UFACILITY'LIB''LP64'"
$ else	if GROUP .eqs. "cl" .or. GROUP .eqs. "gl"
$ then	LIB="COMPATLIB''LP64'"
$ else	if GROUP .eqs. "dbutil"
$ then	LIB="DBUTILLIB''LP64'"
$ else	MAN1DIR="$(TECHMAN1)"
$	LIB="''USUBSYS'LIB''LP64'"
$ endif
$ endif
$ endif
$!
$! special cases
$!
$ if " ''COMPONENT'" .eqs. " "
$ then	gfs="''GROUP'!''FACILITY'!''SUBSYS'"
$	if f$locate("!!", gfs) .ne. f$len(gfs) then gfs=gfs-"!"
$	l=f$len(gfs)-1
$	if f$extr(l, 1, gfs) .eqs. "!" then gfs=f$extr(0, l, gfs)
$ else	gfs="''GROUP'!''FACILITY'!''SUBSYS'!''COMPONENT'"
$ endif
$ if gfs .eqs. "front!ice" .or. gfs .eqs. "front!embed" then goto contloop
$ echo "''gfs': doing ''ACT'"
$ JAMFILE="''jamfile'''LP64'"
$ if f$sear(jamfile) .nes. "" then delete 'jamfile.;*
$! Write out Jamfile file
$!
$! Start with header
$ create 'JAMFILE.
#
$ open /app out 'JAMFILE.
$	write out "# Jamfile''LP64' file for ''gfs'"
$	write out "#"
$	write out ""
$!
$! Tell jam we are part of a bigger picture
$ if GROUP .eqs. "ING_SRC" then GROUP=""
$ gfsc="''GROUP' ''FACILITY' ''SUBSYS' ''COMPONENT'"
$ write out f$edit("SubDir ING_SRC ''gfsc' ;", "compress")
$ write out ""
$!
$! Preserve MANIFEST files. If one exists, include it and move on
$ if f$sear("MANIFEST''LP64'.", 1) .nes. ""
$ then	write out "include ''f$env("default")'MANIFEST''LP64' ;"
$	close out
$	goto contloop
$ endif
$!
$! Calculate hdr and other information for directory
$ if "''GROUP'" .nes. ""
$ then	write out f$edit("IISUBSYS ''gfsc' ;", "compress")
$	write out ""
$ endif
$ if ACT .eqs. "top" .or. ACT .eqs. "group" .or. ACT .eqs. "facility"
$ then	cwd = f$parse(f$env("default"),,,"directory")-"]"
$	arg="[.*]"+JAMFILE+"."
$	forlocloop:
$		loc = f$parse(f$search(arg, 2),,,"directory")-"]"-cwd-"."
$		if loc .eqs. "" then goto endlocs
$		l=f$len(loc)
$		loc=f$edit(loc, "lowercase")
$		ext=""
$		if f$locate("_vms", loc) .ne. l
$		then	ext="vms"
$		else	if f$locate("_unix", loc) .ne. l
$		then	ext="unix"
$		else	if f$locate("_win", loc) .ne. l
$		then	ext="win"
$		endif
$		endif
$		endif
$		if "''ext'" .nes. "" .and. "''ext'" .nes. platform -
			then goto forlocloop
$		write out f$edit("IIINCLUDE ''gfsc' ''loc' ;", "compress")
$		goto forlocloop
$	endlocs:
$ else
$ endif			! end of case-$ACT-in
$ close out
$contloop:
$	i = i + 1
$	goto forloop
$endloop:		! end of for-d-in loop
$ set def 'origwd
$ exit
$usage1:
$ type sys$input
mkjam [ <dir> [<dir>...] ]


       where:

         <dir>       is the pathname of the directory(s) you want
                     MING files generated in.

       Examples:
       # Do source-level directories in front/st
         mkjam ING_SRC:[front.st.*]
       # Do source-level directories and facility-level directory in front/st
         mkjam ING_SRC:[front.st.*] ING_SRC:[front.st]
       # Do only facility-level directory in tools/port and admin/install
         mkjam ING_SRC:[tools.port] ING_SRC:[admin.install]

$exit_fail:
$ exit %x10000012
