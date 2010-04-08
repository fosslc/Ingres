$! Copyright (c) 2004, 2005  Ingres Corporation
$!
$!
$!	This script builds `gv.h'
$!
$!! History:
$!!	09-sep-2004 (abbjo03)
$!!	    Created from mkgv.sh.
$!!	07-apr-2005 (abbjo03)
$!!	    Turn procedure verification off when redirecting output.
$!!	01-Jun-2005 (fanra01)
$!!	    SIR 114614
$!!	    Add definition of GV_BLDLEVEL to gv.h definitions.
$!!
$ echo := write sys$output
$ CMD=f$parse(f$env("procedure"),,,"name")
$ define /nolog tmp SYS$SCRATCH
$!
$ header="gv.h"
$ date=f$time()
$!
$! don't leave it around if we have trouble
$! FIXME: need "on warning"
$!
$! create a new target:
$!
$ create 'header'
/*
$ open /append out 'header'
$ write out "** ''header' created on ''date'"
$ write out "** by the ''f$env("procedure")' shell script"
$ write out "*/"
$!
$ @ING_TOOLS:[bin]readvers
$!
$ write out "# include <gvcl.h>"
$!
$ vers=IIVERS_config
$ l1=f$extr(0, 1, vers)
$ if f$type(l1) .eqs. "INTEGER" then vers="x"+vers
$ write out "# define	''vers'"
$ write out ""
$!
$ v=f$verify(0)
$ pipe ccpp -s GV_ENV >tmp:readvar-mkgv.tmp
$ v=f$verify(v)
$ gosub readvar
$ gvenv=varvalue
$ write out "# define	GV_ENV ", gvenv
$!
$! Check for product.
$!
$ if "''p1'" .eqs. "" .or. "''p1'" .eqs. "INGRES"
$ then	prod="INGRES"
$ else	echo "''CMD': Invalid product type ''p1'"
$	exit %x10000012
$ endif
$ if "''p2'" .nes. ""
$ then	echo "''CMD': Too many arguments"
$	exit %x10000012
$ endif
$!
$ if "''IIVERS_config64'" .nes. "" then LP64="-lp64"
$ v=f$verify(0)
$ pipe genrelid 'LP64' 'prod' >tmp:readvar-mkgv.tmp
$ if $severity .ne. 1
$ then	echo "''CMD': Unable to generate release id. Check CONFIG, VERS, etc."
$	v=f$verify(v)
$	exit %x10000012
$ else	v=f$verify(v)
$	gosub readvar
$	gvver=varvalue
$	write out "# define	GV_VER ""''gvver'"""
$ endif
$!
$ write out ""
$ write out "/*"
$ write out "**"
$ write out "** Value definitions for components of the version string."
$ write out "*/"
$ close out
$ v=f$verify(0)
$ pipe ccpp -s ING_VER | sed -e "s/.* //g" -e "s,/,.,g" | gawk --field-separator=. --assign=build='IIVERS_build' "{printf ""#define GV_MAJOR %d\n#define GV_MINOR %d\n#define GV_GENLEVEL %d\n#define GV_BLDLEVEL %d\n"", $1, $2, $3, build}" >tmp:mkgv.tmp
$ v=f$verify(v)
$ edit /sum tmp:mkgv.tmp /out=tmp:
$ append tmp:mkgv.tmp 'header'
$ del tmp:mkgv.tmp;*
$ open /append out 'header'
$!
$ if "''IIVERS_conf_DBL'" .nes. ""
$ then	write out "# define GV_BYTE_TYPE 2"
$ else	write out "# define GV_BYTE_TYPE 1"
$ endif
$!
$ ihw=f$extr(0, 3, IIVERS_config)
$ ios=f$extr(4, 3, IIVERS_config)
$!
$! Note: /usr/bin/echo should be used if the OS is not Linux.
$!
$! Example: if $ihw is "su4", then the generated statement should be:
$!          "# define GV_HW 0x737534"
$ str=ihw
$ gosub convhex
$ write out "# define GV_HW ", str
$!
$! Example: if $ios is "us5", then the generated statement should be:
$!          "# define GV_OS 0x757335"
$ str=ios
$ gosub convhex
$ write out "# define GV_OS ", str
$!
$ if "''IIVERS_inc'" .nes. ""
$ then	write out "# define GV_BLDINC  ''IIVERS_inc'"
$ else	write out "# define GV_BLDINC  00"
$ endif
$!
$ write out ""
$ write out ""
$ write out "GLOBALREF ING_VERSION  ii_ver;"
$ write out ""
$ write out ""
$ write out "/* End of ''header' */"
$done:
$ close out
$ deas tmp
$ exit
$!
$convhex:
$ l1=f$extr(0, 1, str)
$ l2=f$extr(1, 1, str)
$ l3=f$extr(2, 1, str)
$ str=f$edit(f$fao("0x!XB!XB!XB", f$cvui(0, 8, l1), f$cvui(0, 8, l2), -
	f$cvui(0, 8, l3)), "lowercase")
$ return
$!
$readvar:
$ open /read varfile tmp:readvar-mkgv.tmp
$nextline:
$ read /end=closevar varfile varvalue
$closevar:
$ close varfile
$ del tmp:readvar-mkgv.tmp;*
$ return
