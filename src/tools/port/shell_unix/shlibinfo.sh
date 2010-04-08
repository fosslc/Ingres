:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
# DEST = utility
#
#	This script is 'sourced' to obtain generic and port specific info
# 	needed for building shared libraries.
#
#	This is not a deliverable, but may be used by mksysdep to put info into
#	iisysdep, which is a deliverable. This script sets several variables.
#	Remember to run mksysdep and iisuabf after editing this.
#	This script is called from mkshlibs, mkming, mksysdep, seplnk..
#
#	Use:	. shlibinfo
#	
#	History:
#
#	30-apr-93 (vijay)
#		Created.
#	03-may-93 (vijay)
#		Cannot check default.h since this may be run in env's where
#		ming directory may not be present. Use ming -lv. Thanks peterk.
#	27-may-93 (vijay)
#		BAROQUE does not have VERS.
#	27-may-93 (vijay)
#		COPYLIB goes into libq.a. So, it needs to go into shared lib
#		too.
#	04-jun-93 (vijay)
#		Do ming -nlv and not ming -lv. Else, a MING.mo file will be
#		created during mkming, whose "time" is same as MING.
#		This will confuse ming on faster machines.
#	21-jun-93 (tomm)
#		HP8 and Solaris should both used Shared libraries. 
#	13-jul-93 (vijay)
#		We can now use readvers in all envs. Now, a client can decide
#		not to build shared libraries by setting NO_SHL in VERS file.
#	16-Sep-1993 (fredv)
#		Moved qsys from the LIBQ_LIBS list to FRAME_LIBS list.
#	17-oct-93 (vijay)
#		Moved qsys back to libq. The code has been corrected to
#		maintain the layering.
#	09-dec-93 (dianeh)
#		Removed call to ming -- ming is obsolescent; removed the need
#		for tmp files; simplified some code; added some error-checking;
#		allow for baroque weirdness.
#	09-dec-93 (dianeh)
#		Changed pointer to default.h to tools/port/hdr.
#	15-dec-93 (robf)
#               Removed final exit 0. This broke mkming 
#		which does a ". shlibinfo", and the invoking shell
#	        quits early without completing the MING file when it
#		sees the exit statement. 
#	16-dec-93 (lauraw)
#		When baroqueness was added here, a variable named "vers"
#		was introduced. However, this script is dot-included
#		by other scripts that already have "vers" set to something
#		else (e.g., mergelibs). Since "vers" was only used here
#		to hold "/$ING_VERS" for path references, and paths seem to
#		be referenceable with extra /'s (e.g., tools/port//hdr
#		is a valid reference to tools/port/hdr), I just took "vers"
#		back out.
#	19-jan-94 (lauraw)
#		It turns out that mkming fails when it tries to use 
#		this include file if there is no tools/port/hdr directory,
#		because it tries to grep something out of
#		tools/port/hdr/default.h when setting up shared libraries.
#		In the DBMS dev environment, there may or may not be source
#		for tools/port. To fix this, I've moved the test for
#		NO_SHL to the beginning of this script so it can skip
#		around the stuff that gets done for shared library builds.
#       01-jan-95 (chech02)
#               Added rs4_us5 for AIX 4.1.
#	12-may-95  (thoro01)
#		For HP/UX systems, added info for shared library support.
#		Added shlink_opts=" -lm -lc" code line.
#	24-Jul-95 (gordy)
#		Added Open-Ingres/API as shared library.
#	08-Nov-1995 (walro03)
#		Added shared library support for DG/UX (dg8_us5).
#       12-dec-95  (schte01)
#               For axp_osf, added info for shared library support.
#	24-Jul-95 (gordy)
#		Added Open-Ingres/API as shared library.
#	08-Nov-1995 (walro03)
#		Added shared library support for DG/UX (dg8_us5).
#	06-Dec-1995 (morayf)
#		Added shared library support for SNI RM400/600 (rmx_us5).
#	03-Jan-1996 (moojo03)
#		Added shared library support for DG/UX (dgi_us5).
#	17-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
#		For axp_osf, added info for shared library support.
#       19-jul-96 (harpa06)
#               Added OpenIngres/ICE as a shared library.
#	28-feb-97 (harpa06)
#		Modified build of ICESGADI and added ICENSAPI and ICEISAPI.
#	04-mar-97 (harpa06)
#		Correct build of all ICE libraries.
#	26-mar-97 (harpa06)
#		Include API library when building shared libraries for ICE.
#	12-may-97 (bonbo01/mosjo01)
#		dg8_us5/dgi_us5 duplicated in case stmt. -lthread needed
#		for shlink_opts.
#   27-May-97 (merja01)
#       Add following pthread and AIO options to shlink_opts for axp_osf
#       -laio -lpthread -lmach -lexc
#	21-feb-1997 (walro03)
#		Reapply linke01's OpIng 1.2/01 change of 28-oct-96:
#		Added link editor library liconv in shlink_opts for ris_us5 and
#		rs4_us5 to include symbol .liconv_open etc.
#	23-jun-1997 (walro03)
#		Add support for ICL DRS 6000 (dr6_us5).
#	11-Jul-1997 (allmi01)
#		Added Solaris x/86 ifdefs
#	29-jul-1997 (walro03)
#		Added shared library support for Tandem NonStop UX (ts2_us5).
#       29-aug-97 (musro02)
#               Add shared library support for sqs_ptx
#	10-sep-97/08-apr-97 (hweho01) 
#		Added shared library support for UnixWare (usl_us5). 
#       24-sep-1997 (allmi01)
#               Added support for sos_us5 (x-integ from 1.2/01)
#       24-Sep-1997 (mosjo01 & linke01)
#               Added link editor library liconv in shlink_opts for rs4_us5
#               to include symbol .liconv_open etc. 
#	24-Sep-1997 (kosma01)
#		For PTHREADS on rs4_us5 add library lpthread and lc_r to
#		libraries on shlink_opts.
#	04-Sep-97 (bonro01)
#		Add thread library to shlink_opts.
#		Enable shared_library support.
#	25-feb-98 (toumi01)
#		Add support for Linux (lnx_us5).
#        5-Mar-1998 (linke01)
#               Added shared library support for pym_us5
#	26-Mar-1998 (muhpa01)
#		Added shared library support for hpb_us5
#	20-Apr-1998 (merja01)
#		Modified shlink_opts for axp_osf.  Removed -laio and
#		placed the -lrt flag before -lm.
#	05-Jun-1998 (merja01)
#		Modified shlink_opts for axp_osf. Placed the -lrt
#		flag before -lm.
#	10-Jun-1998 (allmi01)
#		Added entried for Silicon Graphics port (sgi_us5)
#	24-jun-1998 (walro03)
#		Added CA licensing library.
#	14-Jan-1999 (peeje01)
#		Added oiddi library
#	04-Mar-1999 (matbe01)
#		Added -ldl to shlink_opts for rs4_us5.
#	15-Mar-1999 (popri01)
#		For Unixware, the use of the "ld" command is never
#		recommended - use "cc" instead. Remove "/usr/lib" from
#		LIB PATH as it is searched by default.
#	17-Mar-1999 (kosma01)
#		sgi_us5 supports 64 bit libs, new (elf) 32 bit libs,
#		and old (coff) 32 bit libs. It also supports mips3 and
#		mips4. Ingres uses the new 32 bit libs for mips3 cpus.
#		libs /usr/lib32i/mips3.
#	20-Apr-1999 (peeje01)
#		Added neworder
#    22-Apr-1999 (rosga02)
#         Do not use icensapi for sui_us5 in OIICE_LIBS
#	4-May-1999 (rajus01)
#		Added support for Kerberos.
#       14-May-1999 (muhpa01)
#               Removed inclusion of libingres.a in build of the
#               neworder shared lib on HP.
#	17-May-1999 (hanch04)
#		Remove old ice libs.
#       22-jun-1999 (musro02)
#               Define shared libs for nc4_us5 - use sqs_ptx as model
#       03-jul-99 (podni01)
#               Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
#	15-Jul-1999 (ahaal01)
#		Replaced -bM:SRE with -G -bnoentry in shlink_cmd for rs4_us5.
#		Removed -e _nostart from shlink_opts for rs4_us5.
#	06-oct-1999 (toumi01)
#		Change Linux config string from lnx_us5 to int_lnx.
#	21-Jan-2000 (somsa01)
#		Changed OIICE_LIBS to OIICENS_LIBS and added OIICEAP_LIBS for
#		Apache.
#	11-may-2000 (somsa01)
#		Added support for other product builds.
#	22-May-2000 (ahaal01)
#		Removed static link to lic98.0 file for AIX (rs4_us5) to enable
#		dynamic linking to the CA license code.
#       23-may-2000 (hweho01)
#               Added support for AIX 64-bit platform (ris_u64). 
#	31-Jul-2000 (hanje04)
#		Added krblink_cmd and krblink_opts for Kerberos support on
#		Intel Linux (int_lnx)
#       19-apr-1999 (hanch04)
#           Added su4_us5
#	15-aug-2000 (hanje04)
#		Added support for ibm_lnx
#	11-Sep-2000 (hanje04)
#		Added support for Alpha Linux (axp_lnx).
#       17-Jan-2001 (musro02)
#               Added support for use_kerberos in conjunction with krblink...
#	08-Feb-2001 (ahaal01/bonro01)
#		Added krblink_cmd and krblink_opts for Kerberos support on
#		AIX (rs4_us5).
#		Added MWSLSFX for MainWin libraries.
#	05-Feb-2001 (hanje04)
#		As part of the implimentation of the fix for SIR 103876, 
#		shlibinfo now sets use_shared_svr_libs=true if option=SVR_SHL
#		is present in the VERS file.
#	21-Feb-2001 (wansh01)  
#		Added support for sgi_us5
#	18-Apr-2001 (bonro01)
#		Updates necessary for rmx_us5 support.
#               Removed inclusion of libingres.a in build of the
#               neworder shared lib on rmx_us5. This has been replaced
#		with -lingres on the link in mkshlibs.sh
#	23-Apr-2001 (hweho01)
#		Added krblink_cmd and krblink_opts for Kerberos support on
#		Compaq Tru64 (axp_osf) platform.
#       2-May-2001 (hweho01)
#               1) Setup OBJECT_MODE to 64 for ris_u64 platform, it is   
#                  required for building the 64-bit mode shared library. 
#               2) Added support for Kerberos shared lib build on ris_u64.
#	05-Jun-2001 (thoda04)
#		Added iiodbcdriver library
#       22-Jun-2001 (linke01)
#               chenged shlink_cmd from cc to ld for usl_us5
#	20-jul-2001 (stephenb)
#	 	Add support for i64_aix
#	22-Aug-2001 (hanje04)
#	    Added -lpthread to shlink_opt for Linux.
#       25-Aug-2001 (hweho01)
#               Set switch use_kerberos to true for rs4_us5 platform.
#	03-Dec-2001 (hanje04)
#	    Added support for IA64 Linux (i64_lnx).
#	11-dec-2001 (somsa01)
#		Porting changes for hp2_us5.
#	26-Mar-2002 (hanje04)
#	    Added -ldl and -lcrypt to shlink_opts for Linux  and -lelf and 
#	    -lposix4 for Solaris to resolve build issues.
#	16-apr-2002 (somsa01)
#		Added changes for 64-bit builds.
#	16-May-2002 (hanch04)
#	    32/64 bit hybrid build should have 64 bit lib in their
#	    own directory.
#	17-Oct-2002 (bonro01)
#	    Hybrid 32/64-bit changes for sgi_us5.
#	21-Oct-2002 (hweho01)
#           Removed kerberos shared lib build on AIX (Bug #106743) 
#	06-Nov-2002 (hanje04)
#	    Added support for AMD x86-64 Linux and genericise Linux defines 
#	    where apropriate.
#	07-Feb-2003 (bonro01)
#	    Added support for IA64 HP (i64_hpu)
#       21-jan-2004 (loera01)
#           Added libodbccfg (ODBC configuration) library.
#	04-mar-2004 (somsa01)
#	    Added specific i64_lnx stuff; basically, added inclusion of
#	    libgcc_s.so library.
#	09-Feb-2004 (hanje04)
#	    Define OINODESMGR_LIBS for Visual (Unicode) Terminal Monitor
#	09-Feb-2004 (hanje04)
#	    Define OIJNIQUERY_LIBS for Visual (Unicode) Terminal Monitor
#       13-apr-2004 (loera01)
#           Added libodbc (ODBC CLI) library.
#       19-apr-2004 (loera01)
#           Renamed libodbc.a to libodbccli.a
#       20-Apr-2004 (hweho01)
#           Added liboiutil ( remote API for Unicenter) library.
#	11-Jun-2004 (somsa01)
#	    Cleaned up code for Open Source.
#       29-jun-2004 (loera01)
#           Added ODBC tracing shared library.
#	15-Jul-2004 (hanje04)
#		Define DEST=utility so file is correctly pre-processed
#       01-Aug-2004 (hanch04)
#           First line of a shell script needs to be a ":" and
#           then the copyright should be next.
#	30-aug-2004 (stephenb)
#	    Remove -i from solaris shlink_cmd
#	4-Oct-2004 (drivi01)
#	    Updated contents of FRAME_LIBS, LIBQ_LIBS.
#	    Updated path to default.h.
#	19-Oct-2004 (bonro01)
#	    Remove old 64-bit variables.  Add missing library to 
#	    FRAME_LIBS
#	27-Oct-2004 (hanje04)
#	    Remove copyutil from libframe.
#       27-Oct-2004 (drivi01)
#           shell directories on unix were replaced with shell_unix,
#           this script was updated to reflect that change.
#	05-nov-2004 (abbjo03)
#	    Change path to default.h to tools/port/hdr_unix_vms.
#	02-Mar-2005 (hanje04)
#	    SIR 114034
#	    Add support for reverse hybrid.
#	    Add support for ANY hybrid builds on Linux.
#	17-Mar-2005 (hanje04)
# 	    Reverse hybrid on IA64 Linux uses cross compiler, set
#	    shlink_cmd etc. apropriately.
#	    Use config64 as config for reverse hybrids.
#	16-Mar-2005 (bonro01)
#	    Add support for Solaris AMD64 a64_sol
#       18-Apr-2005 (hanje04)
#           Add support for Max OS X (mac_osx).
#           Based on initial changes by utlia01 and monda07.
#	28-Jun-2005 (hweho01)
#	    Build kerberos shared lib for Tru64 (axp_osf).
#        6-Nov-2006 (hanal04) SIR 117044
#           Add int.rpl for Intel Rpath Linux build.
#	23-Mar-2007 (hanje04)
#	    SIR 117985
#	    Add support for 64bit PowerPC Linux (ppc_lnx)
#	31-May-2007 (hanje04)
#	    SIR 118425
#	    Expose XERCVERS for Xerces version.
#        8-Oct-2007 (hanal04) Bug 119262
#           Added archive iimerge.a alternative to shared library server.
#	08-Oct-2007 (hanje04)
#	    SIR 114907
#	    Use OSX frameworks when linking instead of linking directly 
#	    with the libraries.
#       29-Oct-2007 (smeke01) b119370
#           Added '/' before $0 so that if the bash shell is used, 
#           the '-' prefix character doesn't cause a problem.
#	08-Feb-2008 (hanje04)
#	    SIR S119978
#	    Add support for Intel OS X
#	26-Feb-2008 (hanje04)
#	    SIR 119978
#	    Fix up shlink_opts for older OS X releases
#	21-Apr-2008 (hanje04)
#	    Remove libgcc_s fromm 32bit link line for x86_64 Linux.
#	    It's not needed and causes problems because of a linker bug
#	21-July-2008 (frima01) Bug 119773
#	    Added -lrt to shlink_opts for *_lnx, hp8_us5 and su4_us5
#       23-Apr-2009 (frima01) Bug 119773
#           Now added -lrt to shlinkHB_opts for *_lnx too.
#	19-Jun-2009 (kschendel) SIR 122138
#	    New hybrid scheme: for hybrid-capable platforms, generate
#	    xx32 and xx64 strings as needed, let caller choose.
#	17-Sep-2009 (hanje04)
#	    Make Intel OSX port 64bit. Add XERCLIB to variables passed from
#	    default.h
#       23-Feb-2010 (hanje04)
#           SIR 123296
#	    Fix up build flags for kerberos
#           Add support for LSB builds which use versioned shared libraries
#	    and unique build ids
#

CMD=`basename /$0`

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src" 

# Make sure we have what we need
if [ -f $ING_SRC/bin/readvers ] ; then
  READVERS=$ING_SRC/bin/readvers
elif [ -f $ING_SRC/tools/port$noise/shell_unix/readvers.sh ] ; then
  READVERS=$ING_SRC/tools/port$noise/shell_unix/readvers.sh
else
  echo "No readvers file found." ; exit 1
fi

. $READVERS

use_shared_libs=false
use_kerberos=false
use_shared_svr_libs=false
use_archive_svr_lib=false
slvers=1

# If the VERS file contains option=NO_SHL we skip all the shared 
# library setup.
[ "$conf_NO_SHL" = "true" ] ||
{

DEFHDR=$ING_SRC/tools/port/$ING_VERS/hdr_unix_vms/default.h
[ -f $DEFHDR ] || { echo "$CMD: $DEFHDR not found." ; exit 1 ; }

# Which shared libs contain which static libraries. We need the names in "ming"
# format too. If you are updating one format, be sure to update both. This info
# is used by mkshlibs and mkming.


COMPAT_LIBS="(PROG0PRFX)compat"
MCOMPAT_LIBS="COMPATLIB "

LIBQ_LIBS="ui qsys (PROG0PRFX)q qgca gcf ug fmt afe adf cuf sqlca qxa "
MLIBQ_LIBS="UILIB LIBQSYSLIB LIBQLIB COPYLIB LIBQGCALIB GCFLIB UGLIB FMTLIB \
	AFELIB ADFLIB CUFLIB SQLCALIB LIBQXALIB "

FRAME_LIBS="abfrt generate oo uf runsys runtime fd mt vt ft feds \
	copyform tblutil"
MFRAME_LIBS="ABFRTLIB GENERATELIB OOLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB \
	MTLIB VTLIB FTLIB FEDSLIB COPYFORMLIB TBLUTILLIB "

INTERP_LIBS="ilrf ioi iaom (PROG0PRFX)interp"
MINTERP_LIBS="ILRFLIB IOILIB IAOMLIB INTERPLIB "

API_LIBS="(PROG1PRFX)api"
MAPI_LIBS="APILIB"

OIICENS_LIBS="nsapi iceclient gcf ddf"
#
# sui_us5 doesn't have a Netscape server yet, do not build icensapi for now
#
[ "$config" = "sui_us5" ] && OIICENS_LIBS="iceclient gcf ddf"

MOIICENS_LIBS="NETSCAPELIB ICECLILIB GCFLIB DDFLIB"

OIICEAP_LIBS="apapi iceclient gcf ddf"
MOIICEAP_LIBS="APACHELIB ICECLILIB GCFLIB DDFLIB"

OIDDI_LIBS="ddi (PROG1PRFX)api"
MOIDDI_LIBS="OIDDILIB"

OIUTIL_LIBS="(PROG0PRFX)tngapi"

case "$config" in
	hp8_us5 | hpb_us5 | hp2_us5 | i64_hpu)
		NEWORDER_LIBS="play_NewOrder"
		MNEWORDER_LIBS="PLAY_NEWORDER"
		;;
	rmx_us5)
		NEWORDER_LIBS="play_NewOrder"
		MNEWORDER_LIBS="PLAY_NEWORDER"
		;;
	*)
		NEWORDER_LIBS="play_NewOrder ingres"
		MNEWORDER_LIBS="PLAY_NEWORDER INGRES"
		;;
esac

GCSKRB_LIBS="gcskrb"
MGCSKRB_LIBS="GCSKRB"

ODBCDRIVER_LIBS="odbcdriver"
MODBCDRIVER_LIBS="ODBCDRIVERLIB"

ODBCCFG_LIBS="odbccfg"
MODBCCFG_LIBS="ODBCCFGLIB"

OINODESMGR_LIBS="gcnext"
MINODESMGR_LIBS="GCNEXTLIB"

OIJNIQUERY_LIBS="jni"
MINODESMGR_LIBS="JNILIB"

ODBCCLI_LIBS="odbccli"
MODBCCLI_LIBS="ODBCCLILIB"

ODBCTRACE_LIBS="odbctrace"
MODBCTRACE_LIBS="ODBCTRACELIB"

export  COMPAT_LIBS  LIBQ_LIBS  FRAME_LIBS  \
	INTERP_LIBS  API_LIBS  NEWORDER_LIBS  GCSKRB_LIBS \
	ODBCDRIVER_LIBS ODBCCFG_LIBS OINODESMGR_LIBS OIJNIQUERY_LIBS \
	ODBCCLI_LIBS OIUTIL_LIBS ODBCTRACE_LIBS

export MCOMPAT_LIBS MLIBQ_LIBS MFRAME_LIBS \
       MINTERP_LIBS MAPI_LIBS MNEWORDER_LIBS MGCSKRB_LIBS \
       MODBCDRIVER_LIBS MODBCCFG_LIBS MOINODESMGR_LIBS MOIJNIQUERY_LIBS \
       MODBCCLI_LIBS MODBCTRACE_LIBS

# get shared lib suffix. We cannot deliver same suffix on all
# platforms for various reasons: ugly compiler warnings (AIX), cannot specify
# SHLIB_PATH on hp/ux ...

eval `egrep "VERS[ 	]*=|SLSFX[ 	]*=|MWSLSFX[	 ]*=|XERCVERS[	  ]*=|XERCLIB[	  ]*" \
 $DEFHDR | sed -e 's/ //g' -e 's// ; /'`
[ "$SLSFX" ] || SLSFX=so ; export SLSFX
[ "$MWSLSFX" ] || MWSLSFX=so ; export MWSLSFX

#Set up port-specific info
#
#	use_shared_libs is set to true if your release supports shared libs.
#	shlink_cmd is the command used for linking the shared objects.
#	If the linker has "bind now" option, use it. If you need to create
#	a list of export symbols, see ris_us5 section for example.
#	shlink_opts are the options tagged to the end of the above command
#	Add a comment about the runtime variable needed by the dynamic loader
#	to figure out where the libraries are.


case "$config" in
    ris_us5) 
	use_shared_libs=true
	shlink_cmd="ld -K -bM:SRE -bE:expsym_list"
	shlink_opts="-lc -lm -lld -liconv -e _nostart"
	# LIBPATH="/lib:$ING_BUILD/lib" 	# for information purposes
	;;
    r64_us5|\
    rs4_us5)
	use_shared_libs=true
	shlink32_cmd="ld -K -bE:expsym_list -G -bnoentry"
        shlink64_cmd="ld -K -b64 -bE:expsym_list -G -bnoentry"
	shlink32_opts="-lld -liconv -lpthreads -ldl -lm -lc_r -lc"
	shlink64_opts="$shlink32_opts"
	# LIBPATH="/lib:$ING_BUILD/lib" 	# for information purposes
	;;
    ris_u64)
	use_shared_libs=true
        OBJECT_MODE="64" ; export OBJECT_MODE
        shlink_cmd="ld -K -b64 -bE:expsym_list -G -bnoentry"
        shlink_opts="-lld -liconv -lpthreads -lm_r -lc_r -lc"
	# LIBPATH="/lib:$ING_BUILD/lib" 	# for information purposes
	;;
    su4_us5 | sui_us5 | su9_us5 | a64_sol)
	use_shared_libs=true
	use_kerberos=true
	shlink32_cmd="ld -G "
	shlink32_opts="-lrt -lm -lc -lelf -lposix4"
	shlink64_cmd="$shlink32_cmd"
	shlink64_opts="$shlink32_opts"
	krblink32_cmd="ld -G"
	krblink32_opts="-lm -lc -lelf -lnsl -lsocket"
	krblink64_cmd="$krblink32_cmd"
	krblink64_opts="$krblink32_opts"
	;;
    sos_us5)
        use_shared_libs=true
        shlink_cmd="ld -G "
        shlink_opts=" -lm "
        #LD_LIBRARY_PATH="$ING_BUILD/lib:/usr/lib:/lib"
        ;;
    dg8_us5)
	use_shared_libs=true
	shlink_cmd="ld -G "
	shlink_opts=" -lthread -lm -lc "
	# LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
	;;
    dr6_us5)
	use_shared_libs=true
	shlink_cmd="ld -G "
	shlink_opts=" -lm -lc "
	# LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
	;;
    dgi_us5)
	use_shared_libs=true
	shlink_cmd="ld -G "
	shlink_opts=" -lm "
	# LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
	;;
    rux_us5)
        use_shared_libs=true
        shlink_cmd="ld -G "
        shlink_opts=" "
        # LD_LIBRARY_PATH="/lib:/usr/lib:/usr/ccs/lib:$ING_BUILD/lib"
        ;;
    rmx_us5)
        use_shared_libs=true
        shlink_cmd="ld -G "
        shlink_opts=" "
	krblink_cmd="ld -G "
	krblink_opts=" "
        # LD_LIBRARY_PATH="/lib:/usr/lib:/usr/ccs/lib:$ING_BUILD/lib"
        ;;
    ts2_us5)
        use_shared_libs=true
        shlink_cmd="ld -shared "
        shlink_opts=" -lm -lc -lnsl -lsocket "
        # LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
        ;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	use_shared_libs=true
	use_kerberos=true
	shlink32_cmd="ld +s -b -B immediate "
	shlink32_opts="-lrt -lm -lc "
	shlink64_cmd="$shlink32_cmd"
	shlink64_opts="$shlink32_opts"
	krblink32_cmd="ld -b"
	krblink32_opts=""
	krblink64_cmd="$krblink32_cmd"
	krblink64_opts="$krblink32_opts"
	# SHLIB_PATH=/lib:$ING_BUILD/lib

	# NOTE: The executables have to be compiled with +s option to ld,
	# and the shared libs should be specified with the -l option on the
	# ld/cc command line. Else SHLIB_PATH will not work. see man ld.
	;;
    axp_osf)
	use_shared_libs=true
	use_kerberos=true
	shlink_cmd="ld -shared "
	shlink_opts=" -lrt -lm -lc -lbsd -ldnet_stub -lpthread -lmach -lexc -lmld"
	krblink_cmd="ld -shared "
	krblink_opts=" -lrt -lm -lc -lbsd -ldnet_stub "
	# LD_LIBRARY_PATH="/lib:$ING_BUILD/lib"
	;;
    usl_us5)
        use_shared_libs=true
        shlink_cmd="ld -G "
        shlink_opts=" -lm "
        # LD_LIBRARY_PATH="/usr/ccs/lib:$ING_BUILD/lib"
        ;; 
    nc4_us5 | sqs_ptx)
        use_shared_libs=true
        shlink_cmd="ld -G "
        shlink_opts=" -lm "
        LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
        ;;
    sgi_us5)
	use_shared_libs=true
	shlink32_cmd="ld -shared -mips3"
	shlink64_cmd="ld -shared -64"
	shlink32_opts=" -lm "
	shlink64_opts=" -lm "
	krblink32_cmd="ld -shared -mips3"
	krblink64_cmd="ld -shared -64"
	krblink32_opts=""
	krblink64_opts=""
	;;
    i64_lnx)
	use_shared_libs=true
	use_kerberos=true
	shlink64_cmd="ld -shared "
	shlink32_cmd="i686-unknown-linux-gnu-ld -shared "
	shlink64_opts="-lrt -lpthread -lm -lc -ldl -lcrypt -lgcc_s "
	shlink32_opts="-lrt -lpthread -lm -lc -ldl -lcrypt "
	krblink64_cmd="ld -shared"
	krblink32_cmd="i686-unknown-linux-gnu-ld -shared"
	krblink64_opts=""
	krblink32_opts=""
	LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
	;;
      a64_lnx|\
      int_lnx|\
      int_rpl|\
      ibm_lnx|\
      axp_lnx)
	use_shared_libs=true
	use_kerberos=true
	[ "$conf_LSB_BUILD" ] &&
	    slvers=`ccpp -s ING_VER | sed -e 's/.* //g' -e 's,/,.,g'`
	if [ -z "$build_arch" ] ; then
	    # non-hybrid-capable,  must be int_rpl, force 32-bit
	    shlink_cmd="ld -shared -melf_i386 -L/lib -L/usr/lib "
	    shlink_opts="-lrt -lm -lc -lpthread -ldl -lcrypt"
	    [ "$conf_LSB_BUILD" ] && shlink_cmd="$shlink_cmd --build-id"
	    
	    LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
	    krblink_cmd="$shlink_cmd"
	    krblink_opts="$shlink_opts"
	else
	    shlink32_cmd="ld -shared -melf_i386 -L/lib -L/usr/lib "
	    shlink64_cmd="ld -shared -melf_x86_64 -L/lib64 -L/usr/lib64 "
	    [ "$conf_LSB_BUILD" ] &&
	    {
		 shlink32_cmd="$shlink32_cmd --build-id"
		 shlink64_cmd="$shlink64_cmd --build-id"
	    }
	    # ***  DISSL_ENABLED option for crypto and ssl instead of always
	    # including it  or, if only becompat needs it, put 'em in
	    # mksvrshlibs and mkshlibs for compat ...
	    if [ -n "$conf_DISSL_ENABLED" ]
	    then

		shlink32_opts=" -lm -lc   -lpthread -ldl -lcrypt -lssl -lcrypto -L/usr/local/ssl/lib "
		LD_LIBRARY_PATH="/usr/local/ssl/lib:/usr/local/ssl/lib/engines:/lib:/usr/lib:$ING_BUILD/lib"
	    else
		shlink32_opts=" -lm -lc   -lpthread -ldl -lcrypt "
		LD_LIBRARY_PATH="/lib:/usr/lib:$ING_BUILD/lib"
	    fi
	    shlink64_opts="$shlink32_opts"
	    krblink32_cmd="$shlink32_cmd"
	    krblink32_opts="$shlink32_opts"
	    krblink64_cmd="$shlink64_cmd"
	    krblink64_opts="$shlink64_opts"
	fi
	;;
      ppc_lnx)
	use_shared_libs=true
	use_kerberos=true
	shlink_cmd="ld -melf64ppc -shared "
	shlink_opts="-lrt -lm -lc -lpthread -ldl -lcrypt "
	krblink_cmd="ld -melf64ppc -shared"
	krblink_opts=""
	LD_LIBRARY_PATH="/lib64:/usr/lib64:$ING_BUILD/lib"
	;;
       *_osx)
	use_kerberos=true
	use_shared_libs=true
	shlink_cmd="libtool -dynamic"
	archout=""
	# archout="-arch i386 -arch ppc"
	# Disabled until we can build universal binaries
	shlink_opts="$archout -vv -framework System -undefined dynamic_lookup -dead_strip"
	[ -f "/usr/lib/libcc_dynamic.a" ] && \
	    shlink_opts="-lcc_dynamic $shlink_opts"
	krblink_cmd="${shlink_cmd}"
	krblink_opts="$archout -framework System -undefined dynamic_lookup"
	DYLD_LIBRARY_PATH="$ING_BUILD/lib"
	;;
    *) 
	use_shared_libs=false
	;;
esac

# If VERS file contains option=SVR_SHL, enable building of iimerge from
# shared libraries.

	if [ -n "$conf_SVR_SHL" ]
	then
	    use_shared_svr_libs=true
	else
	    use_shared_svr_libs=false
	fi

# If VERS file contains option=SVR_AR, enable building of iimerge.a
        if [ -n "$conf_SVR_AR" ]
        then
            use_archive_svr_lib=true
        else
            use_archive_svr_lib=false
        fi

# End of processing for shared library builds.
}

export use_shared_libs use_shared_svr_libs use_archive_svr_lib shlink_cmd shlink_opts krblink_cmd krblink_opts
export shlink32_cmd shlink64_cmd shlink32_opts shlink64_opts
export krblink32_cmd krblink64_cmd krblink32_opts krblink64_opts
