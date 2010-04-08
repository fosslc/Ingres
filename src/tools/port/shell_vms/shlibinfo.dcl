$! Copyright (c) 2005 Ingres Corporation
$!
$! DEST = utility
$!
$!	This script is 'sourced' to obtain generic and port specific info
$! 	needed for building shared libraries.
$!
$!	This is not a deliverable, but may be used by mksysdep to put info into
$!	iisysdep, which is a deliverable. This script sets several variables.
$!	Remember to run mksysdep and iisuabf after editing this.
$!	This script is called from mkshlibs, mkming, mksysdep, seplnk..
$!
$!	Use:	. shlibinfo
$!
$! For now this is a dummy on VMS, in order not to disturb the Jam build too
$! much.
$!
$!! History:
$!!	28-jan-2005 (abbjo03)
$!!	    Created from shlibinfo.sh.
$!!
$ CMD=f$parse(f$env("procedure"),,,"name")
$!
$ if f$trnl("ING_VERS") .nes. "" then noise = "."+f$trnl("ING_VERS")+".src"
$!
$ @ING_TOOLS:[bin]readvers
$!
$ use_shared_libs="false"
$ use_kerberos="false"
$ use_shared_svr_libs="false"
$ DEFHDR="ING_SRC:[tools.port''noise'.hdr_unix_vms]default.h"
$!
$! End of processing for shared library builds.
$ exit
