$!  Copyright (c) 2004 Ingres Corporation
$!
$!
$!  Name: mkjams -- Generate multiple Jamfiles using mkjam
$!
$!  Usage:
$!	mkjams
$!
$!  Description:
$!	This script can be used to generate Jamfiles thoughout
$!	the source tree. It ONLY generates Jamfiles in top levels
$!	of source tree because there are static Jamfiles submitted in
$!	the source code directories
$!
$!  Exit Status:
$!	 1		Jamfile successfully created
$!	 %x10000012	Creation of Jamfile failed
$!!
$!! History:
$!!	25-aug-2004 (abbjo03)
$!!	    Created from mkjams.sh.
$!
$ echo := write sys$output
$ origwd = f$env("default")
$ CMD="mkjams"
$ if f$trnl("ING_SRC") .eqs. ""
$ then echo "ING_SRC: must be set"
$	goto exit_fail
$ endif
$ LP64=""
$ i = 1
$while:
$	if p'i .eqs. "" then goto endwhile
$	if p'i .eqs. "-LP64"
$	then	LP64="-lp64"
$	else if p'i.eqs. "-D"
$	then	next = i + 1
$		if p'next .eqs. "" then goto usage
$		gosub process_list
$		DIRS=f$edit(LIST, "lowercase,trim")
$		i = next
$	else if p'i .eqs. "-P"
$	then	next = i + 1
$		if p'next .eqs. "" then goto usage
$		gosub process_list
$		projects=f$edit(LIST, "lowercase,trim")
$		i = next
$	else echo "Illegal option: ", p'i
$		echo "Usage:"
$		goto usage1
$	endif
$	endif
$	endif
$	i = i + 1
$	if i .le. 8 then goto while
$endwhile:
$ if "''DIRS'" .eqs. "" then -
	DIRS="admin back cl common dbutil front gl sig testtool tools TOP"
$!
$ if "''projects'" .eqs. ""
$ then	projects="ingres"
$	echo "Building Jamfile files for: ''projects'"
$ endif
$!
$! Call mkjam with the subsystem-level directories, the facility-level
$! directories, then the group-level directory for each project (except
$! vec31, which only gets group-level).
$!
$ set def ING_SRC:[000000]
$ i = 0
$projloop:
$	project=f$elem(i, " ", projects)
$	if project .eqs. " " then goto endproj
$	if project .eqs. "ingres"
$	then	echo "Doing MANIFEST files"
$		lvl=1
$               root="ING_SRC"
$		d="''root':[*]"
$               supp=0
$	findmanf:
$		f=d+"MANIFEST."	
$	findnext:
$		fn=f$sear(f, 1)
$		if fn .eqs. "" then goto nxtlvl
$		dir=f$parse(fn,,,"directory")
$		mkjam "''root':''dir'"
$		goto findnext
$	nxtlvl:
$		lvl=lvl+1
$		if lvl .gt. 5 then goto endfind
$		d=d-"]"+".*]"
$		goto findmanf
$	endfind:
$		j = 0
                if supp .ne. 0 then goto enddirs
$	dirsloop:
$		dir=f$elem(j, " ", DIRS)
$		if dir .eqs. " " then goto enddirs
$		if dir .eqs. "admin" .or. dir .eqs. "back" .or. -
			dir .eqs. "cl" .or. dir .eqs. "common" .or. -
			dir .eqs. "dbutil" .or. dir .eqs. "front" .or. -
			dir .eqs. "gl" .or. dir .eqs. "sig" .or. -
			dir .eqs. "testtool" .or. dir .eqs. "tools"
$		then	d="ING_SRC:[''dir']*.dir"
$			args=""
$			argc=0
$		subdirloop:
$			dn=f$sear(d, 2)
$			if dn .eqs. "" then goto endsubdirs
$			dp="[''dir'.''f$par(dn,,,"name")']"
$			args=args+" "+dp
$			argc=argc+1
$			if argc .lt. 7 then goto subdirloop
$			mkjam 'LP64' 'args'
$			args=""
$			argc=0
$			goto subdirloop
$		endsubdirs:
$			mkjam 'LP64' 'args'
$			mkjam 'LP64' ['dir']
$		else if dir .eqs. "TOP" .or. dir .eqs. "top"
$		then	mkjam 'LP64' ING_SRC
$		endif
$		endif
$		j = j + 1
$		goto dirsloop
$	enddirs:
$               if supp .eq. 0
$               then
$                  supp = 1
$                  lvl = 1
$                  root="COMMON_ING_SRC"
$                  d="''root':[*]"
$                  goto findmanf
$               endif
$	else echo "''CMD: Don't know how to build project ''project'. Skipping..."
$	endif
$	i = i + 1
$	goto projloop
$endproj:
$ set def 'origwd
$ exit
$usage:
$ echo ""
$usage1:
$ type sys$input
mkjams [ -d <dir>[,<dir>[,<n>...]] ] [ -p <project>[,<project>[,<n...>]] ]

       where:

         <dir>         is the name of the source directory (e.g., back)
                       to rebuild Jamfile files in; if more than one
                       directory is specified, separate directory names
                       with commas (e.g., -d back,cl,gl).

         <project>     is the name of the project -- currently supported
                       is "ingres" (default is "ingres"); if more than
                       one project is specified, separate the project
                       names with commas (e.g., -p foo,bar,ingres).

$exit_fail:
$ exit %x10000012
$!
$!  process_list - process a list of directories or projects
$!
$process_list:
$ j=0
$ LIST=""
$loop:
$	parm=f$elem(j, ",", p'next)
$	if parm .eqs. "," then return
$	LIST=LIST + parm + " "
$	j = j + 1
$	goto loop
