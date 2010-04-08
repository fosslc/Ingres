$! Copyright (c) 2004 Ingres Corporation
$!
$!
$! mkidir:
$!
$!	Set permissions on build directories; create them if they don't	exist.
$!	This must be run as ingres; on sysV, this means ruid and euid.
$!	May be run more than once.
$!
$!! History:
$!!	13-sep-2004 (abbjo03)
$!!	    Created from mkidir.sh.
$!!     29-apr-2008 (horda03)
$!!         Define LIB_DIR 
$!!
$ echo := write sys$output
$ mkdir := create /dir
$ origwd = f$env("default")
$ define /nolog tmp SYS$SCRATCH
$!
$ if f$trnl("ING_SRC") .eqs. ""
$ then	echo "ING_SRC: must be set"
$	exit %x10000012
$ endif
$ CMD=f$parse(f$env("procedure"),,,"name")
$!
$ if f$trnl("ING_VERS") .nes. "" then noise=".''f$trnl("ING_VERS")'.src"
$!
$ CONFDIR="ING_SRC:[tools.port''noise'.conf]
$!
$ bldpath=f$trnl("ING_BUILD")
$ if bldpath .eqs. ""
$ then	echo "ING_BUILD: must be set"
$	exit %x10000012
$ endif
$ bldpath=bldpath-".]"+"]"
$ if f$parse(bldpath) .eqs. ""
$ then	echo "Creating ''bldpath'..."
$	dir=bldpath
$	on error then goto create_err
$	mkdir 'bldpath'
$ endif
$
$ do_def="-DLIB_BLD=library"
$!
$ set def 'bldpath'
$ echo "Checking ING_BUILD directories..."
$ pipe ccpp -DDIR_INSTALL 'do_def' 'CONFDIR'idirs.ccpp >tmp:mkidir-ing_build.tmp
$ open /read ccppfile tmp:mkidir-ing_build.tmp
$readloop:
$ read /end=endread ccppfile line
$ perm=f$elem(0, " ", line)
$ dir=f$elem(1, " ", line)
$ gosub convdir
$ if f$parse(dir) .eqs. ""
$ then	echo "Creating ''dir'..."
$	on error then goto create_err
$	mkdir 'dir'
$ endif
$ goto readloop
$endread:
$ close ccppfile
$ del tmp:mkidir-ing_build.tmp;*
$!
$! The chmod ought to be implemented on VMS too.
$!
$!UNIX-do
$!UNIX-    chmod $perm $dir ; }
$!UNIX-done
$!
$! Check for II_SYSTEM, create it if we need to
$!
$ iisyspath=f$trnl("II_SYSTEM")
$ if iisyspath .eqs. ""
$ then	echo "II_SYSTEM: must be set"
$	exit %x10000012
$ endif
$ iisyspath=iisyspath-".]"+"]"
$ if f$parse(iisyspath) .eqs. ""
$ then	echo "Creating ''iisyspath'..."
$	dir=iisyspath
$	on error then goto create_err
$	mkdir 'iisyspath'
$ endif
$!
$! Check for link back to $ING_BUILD, create it if we need to
$ iisyspath=iisyspath+"ingres.DIR"
$ bld1=f$parse(bldpath)-"].;"
$ bld2=f$parse(bldpath-"]"+".-]")-"].;"
$ blddir=bld1-bld2-"."+".DIR"
$ blddir=bld2+"]"+blddir
$!Need to check for symbolic link
$!UNIX-[ -L ${II_SYSTEM}/ingres ] ||
$ if f$sear(iisyspath) .eqs. ""
$ then echo "Creating II_SYSTEM:[ingres] -> ING_BUILD: link...'
$	set file /enter='iisyspath' 'blddir'
$	if $severity .ne. 1
$	then	echo "''CMD': Cannot create link II_SYSTEM:[ingres]. Check permissions." 
$		exit %x10000012
$	endif
$ endif
$!
$! Create $ING_SRC target directories: bin, lib, man1.
$! Create then under ING_TOOLS and link them back to $ING_SRC
$!
$! FIXME: Still to be done:
$! echo "Checking ING_SRC directories..."
$ set def 'origwd'
$ exit
$ set def ING_SRC:[000000]
$!UNIX-ccpp -Uunix -DDIR_SRC 'CONFDIR'idirs.ccpp |  while read perm dir
$!UNIX-do
$!UNIX-  [ -d $dir ] ||
$!UNIX-  { echo "Creating $dir..." ; mkdir $dir 2>/dev/null ||
$!UNIX-    { echo "$CMD: Cannot create $dir. Check permissions." ; exit 1 ; }
$!UNIX-  chmod $perm $dir ; }
$!UNIX-done
$!UNIX-
$!UNIX-[ $? -eq 1 ] && exit 1
$!UNIX-exit 0
$!
$! Convert Unix directory to VMS syntax
$!
$convdir:
$ if dir .eqs. "."
$ then	dir="[]"
$	return
$ endif
$ if f$extr(0, 2, dir) .nes. "./"
$ then	echo "''CMD': Cannot convert directory ""''dir'"""
$	exit %x10000012
$ endif
$ i=0
$ path=f$extr(2,255,dir)
$ dir="["
$while:
$ subdir=f$elem(i, "/", path)
$ if subdir .eqs. "/" then goto endwhile
$ dir=dir+"."+subdir
$ i=i+1
$ goto while
$endwhile:
$ dir=dir+"]"
$ return
$!
$create_err:
$ echo "''CMD': Cannot create ''dir'. Check permissions."
$ exit %x10000012
