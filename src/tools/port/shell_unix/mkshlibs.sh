:
#
# Copyright (c) 2004, 2007 Ingres Corporation
#
# Name:	
#	mkshlibs.sh
#
# Usage:
#	     mkshlibs [compat] [libq] [frame] [interp] [api] 
#		      [oiddi] [oiicens] [oiiceap] [neworder]
#		      [kerberos] [odbc] [cli] [trace ] [oiutil]
#                     [oinodesmgr] [oijniquery]
#
#	if no argument given, it will create all eleven.
#
# Description:
#
#	Creates the shared libraries for the front ends. Creation of libq needs
#	compat, frame needs libq and compat, interp needs frame, libq and 
#	compat; and api, kerberos, icesgadi, icensapi and iceisapi need compat. 
#
#	Before running this script, you need to set up various variables such
#	as shlink_cmd and shlink_opts, krblink_cmd and krblink_opts
#	in shlibinfo.sh.
#
# DEST = utility
#
#
# History
#
#	30-apr-93 (vijay)
#		Created.
#	05-may-93 (vijay)
#		Remove extraneous ; causing syntax error on hp.
#	11-may-93 (vijay)
#		On hp8, we shouldn't link in the "lower level" libraries
#		when linking the shared object, since this causes the loader to
#		look for the shared libraries in the same directory at
#		runtime.
#	06-jul-93 (vijay)
#		Remove tli files from unwanted list.
#	05-oct-93 (vijay)
#		Use -n option of xargs for ris_us5, since nm is broken. It is
#		taking a few arguments and ignoring the rest.
#	14-oct-1993 (fredv)
#		Sun Solaris doesn't require the "lower level" libraries
#		either. Essentially same as hp8_us5, but added an entry
#		for it anyway.
#	19-oct-93 (peterk)
#		remove dy from unwanted list (no longer exists)
#	26-oct-93 (vijay)
#		remove echo for unwanted dirs not found. Do a ming -A after 
#		doing ming ultrace.o etc to force ming to behave as if the
#		state variables have not changed. Else it rebuilds the directory
#		after every mkshlibs invocation.
#	17-nov-93 (vijay)
#		Add dl to unwanted directories, it has extern main() for
#		ris_us5, which ld cannot find at link time.
#	07-apr-94 (vijay)
#		Change perms on the libraries to allow overwrites.
#	27-dec-1994 (canor01)
#		AIX 4.1 no longer supports the -p option.  
#		The -B option is similar.
#       05-jan-1995 (chech02)
#               Added rs4_us5 for AIX 4.1.
#	12-may-95 (thoro01)
# 		Added shared library info for hp/ux (hp8_us5) systems.
#	16-may-95 (brucel)
# 		Added gcsunlu62.o to unwanted list 
#	24-Jul-95 (gordy)
#		Create libiiapi shared library.
#	18-sep-95 (thoro01)
#		For AIX (ris_us5), modified api and libq so that they build
#		properly with shared libraries.
#	28-sep-95 (thoro01)
#		Added shared library support for AIX (ris_us5) platforms.
#		Had to modify LIBQ.
#       28-sep-95 (thoro01)
#               For AIX (ris_us5), modified api and libq so that they build
#               properly with shared libraries.
#	3-Nov-95 (hanch04)
#               On hp8_us5, we shouldn't link in the "lower level" libraries
#               when linking the shared object, since this causes the loader to
#               look for the shared libraries in the same directory at
#               runtime.
#	10-Nov-1995 (walro03)
#		For DG/UX (dg8_us5), libq needs libabfrt.a.
#               properly with shared libraries.
#	01-Dec-1995 (mosjo01/walro03)
#               For DG/UX (dgi_us5 on Intel based machines, dg8_us5 on Motorola
#               based machines), ld -G needs -h name option to bypass hard
#               coded library names in the shared object.
#	04-Dec-1995 (thoro01)
#		For ris_us5, modified libq precess so there is a different case
#		for this build.  Libcompat.a was missing from this ld process.
#	06-Dec-1995 (morayf)
#		Added SNI RM400/600 SINIX support (rmx_us5). Like hp8_us5.
#	27-Mar-1996 (tsale01)
#		Change ris_us5 creation of shared libq to use shared
#		libcompat. Otherwise you will get 2 copies of CM_AttrTab
#		and if you set II_CHARSET?? to anything, you may
#		not get what you want.
#	22-Apr-1996 (rajus01)
#               gcn dir is excluded from unwanted dirs list. libq.so.1
#		needs gcn dir now.
#       19-jul-96 (harpa06)
#               Create icesgadi shared library.
#	12-aug-96 (harpa06/hanch04)
#		ICESGADILIB needs links for other ingres shared libraries.
#		added -L $INGLIB -lframe.1 -lq.1 -lcompat.1
#	27-jul-1996 (canor01)
#		Change references to 'cs' to 'csmt' when using operating-
#		system threads.
#	04-sep-96 (merja01) Add additional links for axp_osf for share
#		libs libq, libiiapi, and icesgadi
#	09-jan-1997 (hanch04)
#		Use cs or csmt if either exists
#	18-feb-1997 (hanch04)
#		OS threads will now use both cs and csmt
#	28-feb-1997 (harpa06)
#		Create icensapi and iceisapi shared libraries.
#	06-feb-1997 (harpa06)
#		Fixed build of iceisapi.
#	17-mar-97 (toumi01) Add additional links for axp_osf for share
#		libs icensapi and iceisapi
#	02-sep-1997 (muhpa01)
#		Add gchplu62.o to list of unwanted objects for build of
#		libcompat.1.sl for HP-UX
#	15-sep-1997/15-apr-1997 (popri01)
#		Corrected typo in build of iceisapi.
#	15-sep-1997/17-apr-1997 (popri01/hweho01)
#		For UnixWare (usl_us5), linker option '-h name' is needed to 
#		avoid the hard coded pathname in the shared object. 
#	25-feb-1997 (walro03)
#		On AIX (ris_us5), icesgadi.a was not created correctly 
#		because the call to archive_lib should use icesgadi.1 as a 
#		parameter instead of icesgadi.  Corrected the call.  Also
#		icensapi and iceisapi.
#	23-jun-1997 (walro03)
#		Add support for ICL DRS 6000 (dr6_us5).
#	11-Jul-1997 (allmi01)
#		Added Solaris x/86 ifdefs.
#	29-jul-1997 (walro03)
#		Added shared library support for Tandem NonStop (ts2_us5).
#       29-aug-1997 (musro02)
#               For sqs_ptx - use -h option without any reference to any other
#               ingres shared library.
#       24-sep-1997 (allmi01)
#               Added support for sos_us5 (x-integ from 1.2/01)
#       24-sep-1997 (mosjo01 & linke01)
#               Use -B option to retrieve symbols in the third column in
#               /usr/bin/nm. If use -p option, symbol is in the first column,
#               and nm is in /usr/bin but not in /usr/ucb. 
#               Also, added rs4_us5 when making libq.1 
#                   , added rs4_us5 when making libiiapi.1
#                   , added rs4_us5 when making icesgadi.1
#       24-sep-1997 (mosjo01)
#               added -L$INGLIB -lname.1 for rs4_us5 to correctly
#               build shared libraries without including the "build path name"
#		in the import file ID string in the loader section of the 
#		output file (name.1.o).  (major difference from ris_us5).
#	07-Oct-1997 (kosma01)
#		for rs4_us5, when building icensapi.1.a use -r to ignore
#		undefined symbols later resolved by netscape shared libs.
#		Also, take only unique symbols for the export symbol list.
#       12-Mar-1998 (allmi01/linke01)
#               Added support for pym_us5 which looks like most SVR4 variants.
#	19-mar-1998 (toumi01)
#		For Linux (lnx_us5) specify "-soname=" GNU ld option to avoid
#		hard-coded path names in the shared object.
#	26-Mar-1998 (muhpa01)
#		Added setting for new HP config, hpb_us5.
#	15-dec-1998 (marro11)
#		Tweaked a bit further to work properly in baroque envs.
#		Introduced notion of:
#		    pnoise  - private path noise; one's ppath version
#		    bnoise  - base path noise; where the real potatoes are
#		    tnoise  - transient noise; to be filled in later
#		Corrected typo on "rm -f ice.*[.1].SLSFX" stmts-- failure to
#		do so left symlinks out there, and updates to base path libs
#	31-Mar-98 (gordy)
#		Removed dl from unwanted dirs, its needed for GCS.  Added
#		gcn to unwanted dirs.
#	14-Jan-1999 (peeje01)
#		Added oiddi
#	05-Feb-1999 (peeje01)
#		Added oiice
#	26-feb-1999 (hanch04)
#	    Add ccm to unwanted dirs.
#	05-Mar-1999 (peeje01)
#		Adjust oiice libs
#	23-Mar-1999 (peeje01)
#		Correct typos in oiice build
#	07-apr-1999 (walro03)
#		Added SLDDIHEAD and SLDDITAIL for Sequent (sqs_ptx).
#	09-Apr-1999 (peeje01)
#		Move oiice.1.so to ice/bin
#	10-Apr-1999 (popri01)
#		For Unixware 7 (usl_us5), changes required 
#		for dynamic loading of liboiddi by icesvr:
#		- modify oiddi name string
#		- add "need" libs required by oiddi
#		- correct ice shared object names
#		Also, in general, separate Unixware commands
#		in order to use shorthand library notation.
#	20-Apr-1999 (peeje01)
#		Build libplay_neworder.so.2.0
#	22-Apr-1999 (rosga02)
#		Do not build icensapi for sui_us5
#       11-May-1999 (rajus01)
#               Build Kerberos shared library.
#	17-May-1999 (hanch04)
#		Removed old ice libs, icesgadi icensapi iceisapi
#	23-May-1999 (muhpa01)
#		Change build of neworder shared lib for HP.
#       22-jun-1999 (musro02)
#               Define shared libs for nc4_us5 - use sqs_ptx as model
#       03-jul-99 (podni01)
#               Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
#	14-Sep-1999 (hanch04)
#		Corrected the extension for liboiddi.$SLSFX.2.0
#		The SLDDIHEAD should be lib and
#		the SLDDITAIL should be $SLSFX.2.0 for all platforms
#		It's used the same as libplay_NewOrder.$SLSFX.2.0
#	06-oct-1999 (toumi01)
#		Change Linux config string from lnx_us5 to int_lnx.
#	15-Dec-1999 (vande02) (bonro01)
#		Added shared libraries for DG/UX Intel (dgi_us5) to resolve
#		Unresolved dynamic link errors for ice shared library
#		oiddi, oiice, and neworder. Added $SLDDITAIL variable to
#		-hoiddi and added '-z defs' flag to check for unresolved
#		references always
#	30-Dec-1999 (vande02) 
#		Removed the '-z defs' flag from make of oiice
#	31-Dec-1999 (vande02)
#		Added echo statement so we can see oiice.1.so is being moved
#       10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
#               Correct rux_us5 changes that were applied using the rmx_us5
#               label.
#	13-Jan-2000 (somsa01)
#		For backward compatibility with Ingres II 2.0, make a copy
#		of oiice.1.$SLSFX called ice.1.$SLSFX (web servers do not
#		like links, which is why we have to make a copy).
#       14-Jan-2000 (mosjo01)
#               Creation of liboiddi.so.2.0 (sqs_ptx) requires quite a list
#               of shared and archived libraries in a prescribed order.
#	19-Jan-2000 (podni01)
#             	Added shared libraries for Siemens (rux_us5) to resolve
#             	unresolved dynamic link errors for ice shared library oiddi
#	21-Jan-2000 (somsa01)
#		Separated oiice into oiicens (Netscape) and oiiceap
#		(Apache).
#	28-jan-2000 (somsa01)
#		Modify build of oiiceap on Solaris so that the Apache web
#		server properly loads the shared library.
#	07-mar-2000 (somsa01)
#		On HP, removed linkage of the oiice shared libraries with
#		libframe and libq. This was causing duplicate code paths
#		with common GCA functions at runtime.
#	25-apr-2000 (somsa01)
#		Added capability for building other products.
#       23-May-2000 (hweho01)
#               Added support for AIX 64-bit platform (ris_u64).
#	30-May-2000 (wansh01)
#		For rmx_us5, linker option '-h name' is needed to 
#		avoid the hard coded pathname in the shared object. 
#	15-June-2000 (hanje04)
#		Added ibm_lnx to Linux configuration.
#	28-Jun-2000 (hanje04)
#               Added '-L $INGLIB -l(PROG0PRFX)frame.1
#             -l(PROG0PRFX)q.1' to build for oiddi on int_lnx,
#             to resolve adg_srv_size.
#	29-Jun-2000 (ahaal01)
#		Added -lddf for AIX (rs4_us5) to link oiddi for icesvr.
#       19-apr-1999 (hanch04)
#           Added su4_us5
#	11-Sep-2000 (hanje04)
#		Added support for Alpha Linux (axp_lnx).
#       09-Oct-2000 (hanal04) Bug 102493.
#               Added $INGLIB/libcompat.1.$SLSFX for axp_osf.
#               Part of the 4.0F porting changes.
#       10-Oct-2000 (hweho01)
#               Removed -r from the linker option list of oiiceap on  
#               ris_u64 platform.
#	02-jan-2001 (somsa01)
#		Amended last change to make mkshlibs properly work.
#	06-Jan-2001 (hanje04)
#	    As part of fix for SIR 103876, added call to mksvrshlibs
#	    if use_shared_svr_libs=true.
#       10-Jan-2001 (musro02)
#               Added support for use_kerberos
#               Ensure kerberos is needed before invoking mkkrb5shlib
#        7-mar-2001 (mosjo01)
#               Expand libcompat.1.so to be support ICE. Also ld -Ur to use
#               multiple passes to resolve references. 
#               To build oiicens and oiiceap required using static libraries
#               on (sqs_ptx).
#	05-Mar-2001 (wansh01)
#               Added support for sgi platform.
#               -no_unresolved option is used to make sure all needed libs are 
#               specified. for oiicens and oiiceap share libs build,
#               libcompat.a is used to support the DSO module  
#	18-Apr-2001 (bonro01)
#		Corrected shared libraries for ICE support.
#	23-Apr-2001 (wansh01)
#               added : in the col1 of the first line. 
#       24-apr-2001 (mosjo01)
#               Removed libc.a from creation of libcompat.1.so for sqs_ptx.
#               Including libc.a was somehow causing false errors to appear
#               in HQA be/api (IIapi_functions). Very bizzarre.
#       24-apr-2001 (mosjo01)
#               Removed libc.a from creation of libcompat.1.so for sqs_ptx.
#               Including libc.a was somehow causing false errors to appear
#               in HQA be/api (IIapi_functions). Very bizzarre.
#        3-may-2001 (mosjo01)
#               Using a mix of Static and Dynamic system libraries for sqs_ptx
#		and libc.1.so replaces libc.a. There is NO shared library of
#               libllrt.a where _divdi3 can be found.
#       17-May-2001 (musro02)
#               For nc4_us5, changed creation of libq to include from several
#		additional system libraries.
#		Changed creation of liboiddi to include libcompat, libframe, and
#		libc. These changes keep oiddi from having unresolved references
#		All of these changes to allow icesvr to start.
#	18-May-2001 (hweho01)
#		Amended the build option for Apache ICE shared libraries.
#       24-May-2001 (musro02)
#               For nc4_us5, changed creation of oiice.. to prevent unresolved 
#		refs in ICE
#       31-May-2001 (musro02)
#               For nc4_us5, changed creation of libq and oiddi
#               to prevent unresolved refs in ICE
#	05-Jun-2001 (thoda04)
#		Added ODBC.
#      11-june-2001 (legru01)
#               Added options to build Apache ICE shared libraries for sos_us5
#       22-Jun-2001 (linke01)
#               changed options of making libcompat.1 and oiddi for usl_us5
#               in order to bring ice server up.
#	27-Jun-2001 (hanje04)
#		Fixed broken lines in sos_us5 part of the script.
#	20-jul-2001 (stephenb)
#		Add support for i64_aix
#	26-Jul-2001 (somsa01)
#		Fixed up the making of the ODBC shared library on HP.
#	11-sep-2001 (devjo01)
#		Various corrections for Tru64 build.
#	21-jun-2001 (devjo01)
#		Added most cx objects to unwanted list.   Since we only want
#		cxapi.o, it would be nicer if mkshlibs had the "wanted object
#		in unwanted directory" logic that mergelibs has, but I took
#		the lazy way out.
#	03-Dec-2001 (hanje04)
#		Added support for IA64 Linux (i64_lnx)
#	11-dec-2001 (somsa01)
#		Changes for hp2_us5.
#	15-Jan-2002 (hanal04) Bug 106820
#		mkshlibs oiddi fails for sos_us5 due to missing continuation
#		characters.
#	15-Feb-2002 (bonro01)
#		For Unixware (usl.us5)
#		Added -lthread to build of libcompat because Apache
#		web server may not be linked with -lthread and this
#		causes 'mutex_init' to be undefined at Apache startup.
#	1-Mar-2002 (bonro01)
#		For UnixWare (usl.us5)
#		Removed -lddf from build of oiddi.so shared library.  This
#		was causing a run-time problem that caused user authentication
#		errors in ICE.  Lib ddf was linked with both icesvr and oiddi,
#		and this caused the problem.  The solution was to add link
#		options to export the symbols from the icesvr executable.
#       19-Mar-2002 (hweho01)
#               Porting changes for rs4_us5 (AIX 32-bit) platform: 
#               1) Added Apache httpd.exp file as the import symbol file
#                  for building oiiceap shared lib.  
#               2) Removed -ddf from the option list of building oiddi. 
#               3) Fixed up the making of ODBC shared lib.  
#               4) Removed -r option when making oiicens and neworder   
#                  shared libraries. 
#       11-jun-2002 (hanch04)
#               Changes for 32/64 bit build.
#               Use one version of mkshlibs for 32 or 64 bit builds.
#	27-aug-2002 (somsa01)
#		Removed gcn, cs, and di from the unwanted list. They are
#		used by the GUI tools.
#	09-sep-2002 (somsa01)
#		Removed mucs.o from the unwanted list.
#	30-Sep-2002 (hanch04)
#		Make sure the 64-bit ice libs go into the 64 directory.
#	09-oct-2002 (somsa01)
#		On HP-UX, recompile cshl.c defining "HPUX_DO_NOT_INCLUDE"
#		when making the compat shared library, as well as including
#		"-lrt".
#	17-Oct-2002 (bonro01)
#		Hybrid 32/64-bit changes for sgi_us5.
#       21-Oct-2002 (hweho01)
#               Changes for 32/64 hybrid build on AIX.
#      15-Jan-2002 (hanal04) Bug 106820
#         mkshlibs oiddi fails for sos_us5 due to missing continuation
#         characters.
#      15-Jul-2002 (zhahu02) 
#         Deleted the -r flag for Apache server on rs4_us5 (b108245).
#	06-Nov-2002 (hanje04)
#		Added support for AMD x86-64 Linux and genericise Linux defines
#		where apropriate.
#       20-Dec-2002  (hweho01)
#               Put back the AIX changes that were dropped out in rev.113.    
#       15-Jan-2003  (hweho01)
#               For the 32/64 hybrid build on AIX, the shared libraries
#               that require the library path to be loaded will have a 
#               suffix '64' and reside in $II_SYSTEM/ingres/lib directory.
#               Because the alternate shared library path is not available.
#	11-Mar-2003 (hanje04)
#	    Don't run mksvrshlibs if mkshlibs is run with arguments (i.e to
#	    make specific shared libraries.
#	04-Apr-2003 (hanje04)
#	    Save number of parameters before processing them so that we can
#	    check the original number later.
#       11-Mar-2003 (hweho01)
#           For AIX, included apache support file httpd.exp in     
#           $ING_SRC/front/ice/apache/support directory, it is used 
#           as the import symbol file when building oiiceap shared lib. 
#	07-Feb-2003 (bonro01)
#		Added support for IA64 HP (i64_hpu)
#       21-Jul-2003 (hanje04)
#           Link ODBC library to lib frame to resolve symbols at runtime on
#           Linux.
#       03-Jul-2003 (hweho01)
#          Modified the change dated 11-Mar-2003, so httpd.exp can be       
#          addressed in baroque environment on AIX platform. 
#       19-Aug-2003 (hweho01)
#          For rs4_us5 platform:
#          1) changed to include libembedc.a in libq.1.a shared library
#             (libc.a has been renamed to libembedc.a).
#          2) removed libc.a for libframe, libinterp , libapi, oiicens, 
#             oiiceap,  oiddi and odbcdriver shared libraries.
#          3) Removed the duplicate echo statements.  
#       16-Sep-2003 (hanal04) Bug 110922
#          Resolve unresolved symbols when building oiddi and neworder
#          on axp.osf.
#       19-Sep-2003 (hweho01)
#          Initialize the variable BITSFX.
#       06-Jan-2004 (sheco02) 
#          Remove mucs.o from the unwanted object list.
#	07-Jan-2004 (hanch04)
#	   Added $RO to the loop that builds the odbcdriver lib.
#	06-Apr-2004 (drivi01)
#	   Added two shared library oijniquery and oinodesmgr.
#       13-Apr-2004 (loera01)
#          Added ODBC CLI.
#       19-Apr-2004 (loera01)
#          Renamed ODBC CLI library with product prefix and version extension.
#       30-Apr-2004 (sheco02)
#          Add the baroque environment path and add do_hyb check condition 
#	   for odbc shared library.
#	30-Apr-2004 (hweho01)
#          Added procedure to build liboiutil (remote API for Unicenter) 
#          shared library.
#	30-Apr-2004 (hanje04)
#	   Set do_oijniquery=true when mkshlibs is run without arguments.
#       06-May-2004 (loera01)
#          Never run archive_lib on the ODBC CLI or driver.  This will
#          prevent the libraries from being dynamically loaded.
#	11-Jun-2004 (somsa01)
#	   Cleaned up code for Open Source.
#       29-Jun-2004 (loera01)
#          Added ODBC tracing shared library.
#	01-Jul-2004 (hanje04)
#	   If we're not building with ming we're building with jam so
#	   build ultrace.o with jam instead of ming.
#	12-Jul-2004 (karbh01) (b112630)
#	   Unlink the link before copying. Changed for rs4_us5:64 bit.
#	15-Jul-2004 (hanje04)
#		Define DEST=utility so file is correctly pre-processed
#      01-Aug-2004 (hanch04)
#          First line of a shell script needs to be a ":" and
#         then the copyright should be next.
#	30-aug-2004 (stephenb)
#	    libq fails on hybrid jam build, lp64 case missing for ultrace
#	4-Oct-2004 (drivi01)
#		Due to merge of jam build with windows, some directories
#		needed to be updated to reflect most recent map.
#	15-Oct-2004 (bonro01)
#	    Remove ming from jam builds on HP
#	21-Oct-2004 (hanje04)
#	    libframe now references functions contained in libinterp so
#	    make sure we link to it here. Only made change for Linux and
#	    default cases.
#	27-Oct-2004 (hanje04)
#	    Backout linking of libframe to libinterp.
#	    Include cx objects in libcompat
#	    libframe should be linked to libcompat (Solaris wasn't)
#	30-Nov-2004 (bonro01)
#	    oiice.1.sl could not be loaded by Apache because of an
#	    unresolved reference.  Modified the build to remove the
#	    reference.
#       01-Dec-2004 (hweho01)
#           Corrected the odbcdriver lib build process for AIX (rs4_us5). 
#       30-Dec-2004 (hweho01)
#           Modified the 64-bit build process for odbc client lib on AIX. 
#	02-Mar-2005 (hanje04)
# 	    SIR 114034
#	    Add support for reverse hybrid builds. If conf_ADD_ON64 is 
#	    defined then script will behave as it did prior to change.
#	    However, if conf_ADD_ON32 is set -lp32 flag will work like
#	    the -lp64 flag does for the conf_ADD_ON64 case except that
#	    a 32bit libraries will be built under $ING_BUILD/lib/lp32.
#	15-Mar-2005 (hanje04)
#	    Make ODBCRO driver creation work with Jam.
#	17-Mar-2005 (hanje04)
#	    Don't build read only odbc library on Linux. We don't ship it.
#	18-Jan-2005 (bonro01)
#	    Added support for Solaris AMD64 a64_sol
#       18-Apr-2005 (hanje04)
#           Add support for Max OS X (mac_osx).
#           Based on initial changes by utlia01 and monda07.
#	09-May-2005 (sheco02)
#	    Build read only odbc library on Unix/Linux as requested by 
#	    loera01. 
#	09-Jun-2005 (lazro01)
#	    libq.1.so could not be loaded by icesvr because of an
#	    unresolved reference.  Modified the build to resolve the
#	    unresolved reference.
#       04-Aug-2005 (zicdo01)
#           Remove check to build as ingres
#       09-May-2005 (hanal04) Bug 114470
#           In do_odbc for nc4 we had a missing line continuation character
#           before the -h option.
#      29-dec-2005 (hweho01)
#           Don't run archive_lib on oiutil for AIX platform. This will
#           prevent the libraries from being dynamically loaded.
#	6-Feb-2006 (kschendel)
#	    Solaris isn't happy with pushd.  Not sure why we didn't see
#	    this before, but anyway .. use subshell instead of pushd/popd.
#       06-Feb-2006 (hweho01)
#           Modified the 64-bit shared library names for AIX, so they     
#           will have the subfix "64" in $ING_BUILD/lib directory only, 
#           and use the same jam rule IISHLIBRARY as other hybrid build 
#           platforms.  (No alternate path for 64-bit library on aix, 
#           need subfix "64" for the referencing). 
#        6-Nov-2006 (hanal04) SIR 117044
#           Add int.rpl for Intel Rpath Linux build.
#        9-Nov-2006 (hanal04) SIR 117044
#           Modified shlink_opts for int_rpl
#       11-Dec-2006 (hweho01)
#           Included libinstalla to build oiutil shared library for
#           Unix/Linux platforms.
#	05-Jan-2007 (kiria01) b117402
#	    Correct symbol passed for compat library that removes loader
#	    dependancies on the hp8_us5 and hpb_us5 variants.
#       20-Feb-2007 (hweho01)
#           Added libcuf.a to the link list for building oiiceap (Apache)   
#           shared library, fix the unresolved reference to ss_encode.   
#       21-May-2007 (Ralph Loen)  SIR 117786
#           Link the ODBC CLI with the API and LIBQ libraries in order to
#           utilize the API routines.
#       21-May-2007 (hanal04) Bug 118367
#           Don't cleanup shared library files if IIOPTIM set. It probably
#           means we have a debug build and on Solaris removing the
#           working files prevents debug of shared libraries.
#       11-Jun-2007 (Ralph Loen) Bug 118482
#           For HP platforms, include references to libq so that the GCF
#           references are resolved.  Add a reference to the pthread library
#           when the compatlib is linked.
#	11-Jun-2007 (bonro01)
#	    Fix typo from previous change for SIR 117786
#	08-Oct-2007 (hanje04)
#	    SIR 114907
#	    Use OSX frameworks when linking instead of linking directly 
#	    with the libraries.
#	08-Feb-2008 (hanje04)
#	    SIR S119978
#	    Add support for Intel OS X
#       22-Apr-2009 (Ralph Loen) Bug 121972
#           Include the LIBQ and API libraries for the ODBC CLI on 
#           rs4_us5 (AIX and 64-bit AIX).
#	18-Jun-2009 (kschendel) SIR 122138
#	    Hybrid mechanisms changed, fix here.
#	    Clean up some unreachable aix stuff, remove AIX itanic.
#	    Revert Nov-2007 change since it was caused by changing
#	    shlink_cmd on a64_lnx from ld to gcc, and it's now changed back.
#	23-Feb-2010 (hanje04)
#	    SIR 123296
#	    Add support for LSB builds which use versioned shared libraries
#	21-May-2010 (bonro01)
#	    Remove ICE from builds
#	30-Sep-2010 (troal01)
#	    If conf_WITH_GEO is defined, we must link libq with the geospatial
#		dependencies.
#	23-Nov-2010 (kschendel)
#	    Drop a few more obsolete ports.
#

if [ -n "$ING_VERS" ] ; then

    # In a baroque environment, we either are building the base area,
    # or a private area; the former can be identified by comparing ING_BUILD
    # with II_SYSTEM (they should be the same); the latter should have a 
    # "BASE" file indicating which base area they ppathed against.
    #
    # "BASE" file should have been created on each ppath invocation
    #	Format:  basepath=s00
    #

    # First get physical dirnames using csh (sh may not translate symlinks)
    #
    bdir=`csh -fc "cd $ING_BUILD ; pwd"`
    idir=`csh -fc "cd $(PRODLOC)/(PROD2NAME) ; pwd"`

    if [ "$bdir" = "$idir" ] ; then
	bnoise="$ING_VERS"

    else
	basefile="$ING_SRC/clients/$ING_VERS/BASE"
	if [ ! -r "$basefile" ] ; then
	    echo "Unable to determine base path used; missing file."
	    echo "File:  $basefile"
	    exit 1
	fi

	# Extract base string; add "src" below
	#
	bnoise=`grep "^basepath=" $basefile | head -1 | cut -f2 -d=`
	if [ -z "$bnoise" ] ; then
	    echo "Unable to determine base path used; odd format."
	    echo "File:  $basefile"
	    exit 1
	fi
    fi

    tnoise="__noise__"
    bnoise="/$bnoise/src"
    pnoise="/$ING_VERS/src"

else
    # Non-baroque; initialize these to null
    #
    tnoise=""
    bnoise=""
    pnoise=""
fi

#
# Translates dirname $1 to physical location if it exists.
#
# Supply:
#	$1	: directory (full path), with "tnoise" in string.
#
# Return:
#	""	: if non-baroque, and $1 doesn't exist
#	$1	: if non-baroque, and $1 does exist
#	base	: if baroque, and ppath dir for $1 doesn't exist, but base does
#	private	: if baroque, and ppath dir for $1 does exist
#	""	: if baroque, and neither base nor ppath dir for $1 exists
#
translate_dir()
{

    tdir=""
    if [ -n "$ING_VERS" ] ; then
	# baroque; use base, but override with private if there
	#
	xdir=`echo "$1" | sed "s@$tnoise@$bnoise@"`
	[ -d "$xdir" ] && tdir="$xdir"
	xdir=`echo "$1" | sed "s@$tnoise@$pnoise@"`
	[ -d "$xdir" ] && tdir="$xdir"

    else
	[ -d "$1" ] && tdir="$1" 
    fi

    echo "$tdir"
    return 0
}


. readvers

#  Get info about shared libraries.
. shlibinfo

if $use_shared_libs
then
	:
else
	echo 	"$0: Build not set up for using shared libraries"
	exit
fi

: ${ING_BUILD?} ${ING_SRC?}

#
# Set hybrid variables based on build_arch variable
#
RB=xx	## regular / primary build
HB=xx	## hybrid (add-on) build
if [ "$build_arch" = '32+64' ] ; then
    RB=32
    HB=64
elif [ "$build_arch" = '64+32' ] ; then
    RB=64
    HB=32
fi


#
#	Check arguments.
#

do_compat=false do_libq=false do_frame=false do_interp=false do_api=false do_oiddi=false do_oiicens=false do_oiiceap=false do_neworder=false do_kerberos=false do_odbc=false do_cli=false do_odbc_trace=false do_oinodesmgr=false do_oijniquery=false do_oiutil=false
[ $# = 0 ] && do_compat=true do_libq=true do_frame=true do_interp=true do_api=true do_oiddi=true do_oiicens=true do_oiiceap=true do_neworder=true do_kerberos=true do_odbc=true do_cli=true do_odbc_trace=true do_oinodesmgr=true do_oijniquery=true do_oiutil=true
[ $# = 1 ] && [ $1 = "-lp${HB}" -o $1 = "-lp${RB}" ] && do_compat=true do_libq=true do_frame=true do_interp=true do_api=true do_oiddi=true do_oiicens=true do_oiiceap=true do_neworder=true do_kerberos=true do_odbc=true do_cli=true do_odbc_trace=true do_oinodesmgr=true do_oijniquery=true do_oiutil=true

do_reg=true do_hyb=false

param=$#

while [ $# -gt 0 ]
do
    case $1 in
	-lp${HB})	do_hyb=true; do_reg=false; shift;;
	-lp${RB})	do_reg=true; shift ;;
	compat)		do_compat=true; shift ;;
	libq)		do_libq=true; shift ;;
	frame)		do_frame=true; shift ;;
	interp)		do_interp=true; shift ;;
	api)		do_api=true; shift ;;
	oiddi)		do_oiddi=true; shift ;;
	oiicens)	do_oiicens=true; shift ;;
	oiiceap)	do_oiiceap=true; shift ;;
	neworder)	do_neworder=true; shift ;;
	kerberos)	do_kerberos=true; shift ;;
	odbc)   	do_odbc=true; shift ;;
	cli)    	do_cli=true; shift ;;
	trace)    	do_odbc_trace=true; shift ;;
	oinodesmgr)   	do_oinodesmgr=true; shift ;;
	oijniquery)     do_oijniquery=true; shift ;;
	oiutil)   	do_oiutil=true; shift ;;
        *)	echo "Usage: mkshlibs [-lp32|-lp64] [compat] [libq] [frame] [interp] [api]"
		echo "       [oiddi] [kerberos] [odbc] [cli] [trace] [oiutil]"
		[ -n "$conf_BUILD_ICE" ] && echo "       [oiicens] [oiiceap] [neworder]"
		exit 1
		;;
    esac
done

if [ -z "$conf_BUILD_ICE" ] ; then
    do_oiicens=false
    do_oiiceap=false
    do_neworder=false
fi

if ($do_hyb && $do_reg)
then
        echo    "$0: 32 and 64 bit shared libraries can not be build at the same time."
        exit
fi

BITSFX=
if $do_hyb
then
	LPHB=lp${HB}
	INGLIB=$ING_BUILD/lib/${LPHB}
	BITS=$HB
else
	LPHB=
	INGLIB=$ING_BUILD/lib
	BITS=$RB
fi
if [ -n "$build_arch" -a "$BITS" = 'xx' ] ; then
    BITS="$build_arch"
fi
## the upshot of the above is that BITS is set to whatever arch size
## is being built, or xx for non-hybrid-capable platforms.

# shlibinfo returns 32/64-bit variants for hybrid capable, select the
# one wanted this time around.
if [ -n "$build_arch" ] ; then
    eval shlink_cmd=\$"shlink${BITS}_cmd"
    eval shlink_opts=\$"shlink${BITS}_opts"
fi
if [ -z "$shlink_cmd" ] ; then
    echo "Shared library link command shlink_cmd not resolved?"
    [ -n "$build_arch" ] && echo "LPHB is $LBHB, $BITS bits"
    exit 1
fi

# If it is making 64-bit build on AIX platform
# specify the 64 bit mode for ar command.
if [ "$config64" = 'r64_us5' -a "$BITS" = 64 ] ; then
    OBJECT_MODE="64" ; export OBJECT_MODE
    BITSFX=64
fi


workdir=$INGLIB/shared
mkdir $workdir 2>/dev/null
cd $workdir

trap 'cd $workdir; rm -rf $workdir/*; exit 6' 1 2 3 15

[ `(PROG1PRFX)dsfree` -le 20000 ] && echo "** WARNING !! You may run out of diskspace **"

#
# AIX 4.1 no longer supports the -p option.  The -B option is similar.
#
[ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] &&
mkexplist()
{
    ls -1 *.o | xargs -n50 /usr/bin/nm -B |
      awk ' $2=="D"||$2=="B" { print $3 } ' | sort | uniq > expsym_list
}
#
#
[ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] &&
archive_lib()
{
    libname=$1
    save_dir=`pwd`
    cd $INGLIB
    rm -f $libname.o
    mv $libname.$SLSFX $libname.o
    ar ru $libname.$SLSFX $libname.o
    rm -f $libname.o
    cd $save_dir
}


#
# 	check for existence of all the required libraries.
#

echo "Checking static libraries"
missing_libs=
check_libs=
if $do_compat ; then check_libs=$COMPAT_LIBS ; fi
if $do_libq ; then check_libs="$check_libs $LIBQ_LIBS" ; fi
if $do_frame ; then check_libs="$check_libs $FRAME_LIBS" ; fi
if $do_interp ; then check_libs="$check_libs $INTERP_LIBS" ; fi
if $do_api ; then check_libs="$check_libs $API_LIBS" ; fi
if $do_oiddi ; then check_libs="$check_libs $OIDDI_LIBS" ; fi
if $do_oiicens ; then check_libs="$check_libs $OIICENS_LIBS" ; fi
if $do_oiiceap ; then check_libs="$check_libs $OIICEAP_LIBS" ; fi
if $do_neworder ; then check_libs="$check_libs $NEWORDER_LIBS" ; fi
if $do_odbc ; then check_libs="$check_libs $ODBCDRIVER_LIBS" ; fi
if $do_cli ; then check_libs="$check_libs $ODBCCLI_LIBS" ; fi
if $do_odbc_trace ; then check_libs="$check_libs $ODBTRACE_LIBS" ; fi
if $do_oinodesmgr ; then check_libs="$check_libs $OINODESMGR_LIBS" ; fi
if $do_oijniquery ; then check_libs="$check_libs $OIJNIQUERY_LIBS" ; fi
if $do_oiutil ; then check_libs="$check_libs $OIUTIL_LIBS" ; fi

for lname in $check_libs
do
    if [ ! -r $INGLIB/lib$lname.a ]
    then
	echo Missing library lib$lname.a ...
	missing_libs=yes
    fi
done

[ $missing_libs ] && { echo "Aborting !!" ; exit 2 ;}

#
#	 Make list of unwanted objects
#
# FIXME: some of these may have been removed from the source. Or need to be.
# Got most of these from mergelibs.
#

echo Making list of unwanted objects
unwanted_os="gcccl.o gcapsub.o gcctcp.o gccdecnet.o gccptbl.o bsdnet.o \
	bsnetw.o cspipe.o iamiltab.o clnt_tcp.o clnt_udp.o pmap_rmt.o svc.o \
	svc_run.o svc_tcp.o rngutil.o meuse.o meconsist.o medump.o \
	mebdump.o meberror.o mexdump.o fegeterr.o gcsunlu62.o gchplu62.o "

# unwanted dirs cs, dy, *50, di, ck, jf, sr for cl and gcc for libq.
unwanted_dirs="cl/clf$tnoise/ck_unix_win cl/clf$tnoise/jf_unix_win cl/clf$tnoise/sr_unix_win \
	common/cuf$tnoise/ccm"

bados=$workdir/bado
echo "" > $bados
for i in $unwanted_dirs 
do

    tdir=`translate_dir "$ING_SRC/$i"`

    if [ -n "$tdir" ] ; then

	#  get directory source file list
	#
	( cd $tdir ; ls -1 *.c *.s *.roc 2>/dev/null ) | \
		sed -e 's/\.c$/\.o/g' -e 's/\.s$/\.o/g' -e 's/\.roc$/\.o/g' \
		>> $bados 2>/dev/null 
    fi
done
echo $unwanted_os >> $bados


#
#	Making libcompat.1
#

if $do_compat
then
    echo ""
    echo " Building lib(PROG0PRFX)compat$BITSFX.$slvers.$SLSFX "
    echo ""
    mkdir $workdir/compat 2> /dev/null ; cd $workdir/compat 
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    for lname in $COMPAT_LIBS
    do
	ar x $INGLIB/lib$lname.a
    done

    rm -f `cat $bados`

    # FIXME: objects removed for compat: should these stay ?
    # pcsspawn.c accesses in_dbestart which is declared in in50/inglobs.o
    # pcfspawn needs pcsspawn
    rm -f pcsspawn.o pcfspawn.o

    # On HP-UX, recompile cshl.c defining "HPUX_DO_NOT_INCLUDE"
    # then making the compat shared library, as well as including
    # "-lrt".
    if [ "$config" = "hpb_us5" -o "$config" = "hp8_us5" -o "$config" = 'hp2_us5' ] ; then
	tdir=`translate_dir "$ING_SRC/cl/clf$tnoise/cs_unix"`
	cd $tdir
	if $do_hyb
	then
	    jam -a -sIIOPTIM=-DHPUX_DO_NOT_INCLUDE "<cl!clf!cs_unix>${LPHB}/cshl.o"
	    cp ${LPHB}/cshl.o $workdir/compat
	    rm -f ${LPHB}/cshl.o
	else
	    jam -a -sIIOPTIM=-DHPUX_DO_NOT_INCLUDE "<cl!clf!cs_unix>cshl.o"
	    cp cshl.o $workdir/compat
	    rm -f cshl.o
	fi
	cd $workdir/compat
    fi

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG0PRFX)compat.$slvers.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \*.o $shlink_opts 
    case $config in
    usl_us5 )
           $shlink_cmd -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX *.o \
            -lc -lnsl -lsocket \
            /usr/ccs/lib/libcrt.a \
            $shlink_opts -hlib(PROG0PRFX)compat.1.$SLSFX
           ;;
    *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX *.o \
		$shlink_opts -soname=lib(PROG0PRFX)compat.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -fs lib(PROG0PRFX)compat.${slvers}.$SLSFX \
		 lib(PROG0PRFX)compat.1.$SLSFX
	fi
	
	;;
    sgi_us5)
        $shlink_cmd  -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX *.o\
        -no_unresolved \
        -lc -lpthread \
        $shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX *.o \
	-lrt -lpthread $shlink_opts
	;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX *.o $shlink_opts
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX ]
        then
	   echo $0 Failed,  aborting !!
	   exit 4
        fi
	archive_lib lib(PROG0PRFX)compat.1 
	chmod 755 $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX
	ls -l $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX
        if  [ "$do_hyb" = "true" ] ; then
           [ -r $ING_BUILD/lib/lib(PROG0PRFX)compat$BITSFX.1.$SLSFX ] && \
	   unlink $ING_BUILD/lib/lib(PROG0PRFX)compat$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX  $ING_BUILD/lib/lib(PROG0PRFX)compat$BITSFX.1.$SLSFX  
        fi 
        ;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX *.o \
        $shlink_opts -framework DirectoryService \
	-framework Security -prebind -install_name lib(PROG0PRFX)compat.1.$SLSFX
        ;;

    *)
           $shlink_cmd -o $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)compat.$slvers.$SLSFX ]
      then
	  echo $0 Failed,  aborting !!
	  exit 4
      else
       	# on some systems, libs being non writable may add to performance 
	chmod 755 $INGLIB/lib(PROG0PRFX)compat.$slvers.$SLSFX
	ls -l $INGLIB/lib(PROG0PRFX)compat.$slvers.$SLSFX
      fi
    fi
    echo Cleaning up ....
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then 
	rm -rf compat
    fi
fi  # do_compat


#
#	Making libq.1
#

if $do_libq
then
    echo ""
    echo " Building lib(PROG0PRFX)q$BITSFX.${slvers}.$SLSFX "
    echo ""
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX ] ||
      { echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    mkdir $workdir/libq 2> /dev/null ; cd $workdir/libq
    [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    for lname in $LIBQ_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    rm -f `cat $bados`

    # FIXME: adbdebug.c acceses ult_set_val, ult_clear_val across boundaries.
    #
    tdir=`translate_dir "$ING_SRC/back/ulf$tnoise/ult"`
    cd $tdir

    if $do_hyb
    then
	jam "<back!ulf!ult>${LPHB}/ultrace.o"
	cp ${LPHB}/ultrace.o $workdir/libq
	rm -f ${LPHB}/ultrace.o
    else
	jam "<back!ulf!ult>ultrace.o"
	cp ultrace.o $workdir/libq
	rm -f ultrace.o
    fi

    cd $workdir/libq
    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX \*.o $shlink_opts 
    case $config in
    axp_osf)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/libabfrt.a \
	$shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
	$INGLIB/libabfrt.a \
	$shlink_opts 
	;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
        -l(PROG0PRFX)compat.1 -labfrt \
        /usr/ccs/lib/libcrt.a \
        $shlink_opts -hlib(PROG0PRFX)q.1.so
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o  \
        $INGLIB/libabfrt.a \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib  -l(PROG0PRFX)compat$BITSFX.1
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)q.1.$SLSFX ] ; then
          echo $0 Failed,  aborting !!
          exit 4
        fi
	archive_lib lib(PROG0PRFX)q.1
        chmod 755 $INGLIB/lib(PROG0PRFX)q.1.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)q.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
           [ -r $ING_BUILD/lib/lib(PROG0PRFX)q$BITSFX.1.$SLSFX ] && \
	   unlink $ING_BUILD/lib/lib(PROG0PRFX)q$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG0PRFX)q.1.$SLSFX  $ING_BUILD/lib/lib(PROG0PRFX)q$BITSFX.1.$SLSFX 
        fi
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
	$shlink_opts 
	;;
    *_lnx|int_rpl)
	if [ "$conf_WITH_GEO" ] ; then
		geo_libs=" -lgeos -lgeos_c -lproj "
		if [ "$do_hyb" = "true" ] ; then
			GEOS_LD="-L$GEOSHB_LOC -L$PROJHB_LOC $geo_libs"
		else
			GEOS_LD="-L$GEOS_LOC -L$PROJ_LOC $geo_libs"
		fi
	else
		GEOS_LD=""
	fi
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.${slvers} \
	$shlink_opts -soname=lib(PROG0PRFX)q.1.$SLSFX $GEOS_LD
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -fs lib(PROG0PRFX)q.${slvers}.$SLSFX \
		 lib(PROG0PRFX)q.1.$SLSFX
	fi
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
        -lc -no_unresolved \
        -L$INGLIB -labfrt -l(PROG0PRFX)compat.1 \
	$shlink_opts     
	;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 \
        $shlink_opts -prebind -install_name lib(PROG0PRFX)q.1.$SLSFX
        ;;
    *)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)q.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$shlink_opts 
	;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX ]
        then
          echo $0 Failed,  aborting !!
          exit 4
        else
          chmod 755 $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX
          ls -l $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf libq
    fi
fi  # do_libq



#
#       Making libframe.1
#

if $do_frame
then
    echo ""
    echo " Building lib(PROG0PRFX)frame$BITSFX.${slvers}.$SLSFX "
    echo ""
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX -a -r $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX ] ||
      { echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX, and lib(PROG0PRFX)q.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    mkdir $workdir/frame 2> /dev/null ; cd $workdir/frame
    [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    for lname in $FRAME_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    rm -f `cat $bados`

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX \*.o $shlink_opts 
    case $config in
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o  \
	$shlink_opts 
	;;
    a64_sol|\
    su9_us5|\
    su4_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 \
	$shlink_opts 
	;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o  \
        -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 \
        $shlink_opts -hlib(PROG0PRFX)frame.1.so
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)compat$BITSFX.1 -l(PROG0PRFX)q$BITSFX.1
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX ]
        then
           echo $0 Failed,  aborting !!
           exit 4
        fi
        archive_lib lib(PROG0PRFX)frame.1
        chmod 755 $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX
        if  [ "$do_hyb" = "true" ] ; then
           [ -r $ING_BUILD/lib/lib(PROG0PRFX)frame$BITSFX.1.$SLSFX ] && \
	   unlink $ING_BUILD/lib/lib(PROG0PRFX)frame$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX $ING_BUILD/lib/lib(PROG0PRFX)frame$BITSFX.1.$SLSFX 
        fi
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX *.o  \
	-L$INGLIB -l(PROG0PRFX)compat.${slvers} -l(PROG0PRFX)q.${slvers} \
	$shlink_opts -soname=lib(PROG0PRFX)frame.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG0PRFX)frame.${slvers}.$SLSFX \
		 lib(PROG0PRFX)frame.1.$SLSFX
	fi
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o  \
        -lc \
        -no_unresolved \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
	$shlink_opts 
	;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 \
        $shlink_opts -install_name lib(PROG0PRFX)frame.1.$SLSFX
	;;
    *)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX *.o  \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
	$shlink_opts 
	;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX ]
      then
        echo $0 Failed,  aborting !!
        exit 4
      else
        chmod 755 $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ....
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf frame
    fi
fi  # do_frame

#
#       Making libinterp.1
#

if $do_interp
then
    echo ""
    echo " Building lib(PROG0PRFX)interp$BITSFX.${slvers}.$SLSFX "
    echo ""
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX -a -r $INGLIB/lib(PROG0PRFX)q.${slvers}.$SLSFX -a  \
      -r $INGLIB/lib(PROG0PRFX)frame.${slvers}.$SLSFX ] ||
     { echo Need to build lib(PROG0PRFX)frame.${slvers}.$SLSFX etc first, exiting ...;  exit 5 ;}

    mkdir $workdir/interp 2> /dev/null ; cd $workdir/interp
    [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    for lname in $INTERP_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    rm -f `cat $bados`

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG0PRFX)interp.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.${slvers}.$SLSFX \*.o $shlink_opts
    case $config in
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o \
	$shlink_opts 
	;;
    a64_sol|\
    su9_us5|\
    su4_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o \
	$shlink_opts 
	;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o \
        -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 -l(PROG0PRFX)q.1 \
        $shlink_opts -hlib(PROG0PRFX)interp.1.so
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)compat$BITSFX.1 -l(PROG0PRFX)frame$BITSFX.1 \
                  -l(PROG0PRFX)q$BITSFX.1
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX ]
        then
          echo $0 Failed,  aborting !!
          exit 4
        fi
        archive_lib lib(PROG0PRFX)interp.1
        chmod 755 $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
           [ -r $ING_BUILD/lib/lib(PROG0PRFX)interp$BITSFX.1.$SLSFX ] && \
	   unlink $ING_BUILD/lib/lib(PROG0PRFX)interp$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX $ING_BUILD/lib/lib(PROG0PRFX)interp$BITSFX.1.$SLSFX 
        fi
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.${slvers}.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 \
	-l(PROG0PRFX)q.1 \
	$shlink_opts -soname=lib(PROG0PRFX)interp.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG0PRFX)interp.${slvers}.$SLSFX \
		 lib(PROG0PRFX)interp.1.$SLSFX
	fi
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o \
        -lc \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX  \
        $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        -no_unresolved \
	$shlink_opts 
	;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 -l(PROG0PRFX)q.1 \
        $shlink_opts -install_name lib(PROG0PRFX)interp.1.$SLSFX
	;;
    *)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)interp.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX $INGLIB/lib(PROG0PRFX)frame.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
	$shlink_opts 
	;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)interp.${slvers}.$SLSFX ]
      then
         echo $0 Failed,  aborting !!
         exit 4
      else
        chmod 755 $INGLIB/lib(PROG0PRFX)interp.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)interp.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf interp
    fi
fi  # do_interp



#
#	Making libiiapi.1
#

if $do_api
then
    echo "APILIBS $API_LIBS"
    echo ""
    echo " Building lib(PROG1PRFX)api$BITSFX.${slvers}.$SLSFX "
    echo ""

    # check whether libcompat.${slvers}.$SLSFX exists, since api needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX ] ||
	{ echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/libiiapi 2> /dev/null ; cd $workdir/libiiapi
	 [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $API_LIBS
    do
    	ar x $INGLIB/lib$lname.a
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX \*.o $shlink_opts 
    case $config in
    axp_osf)
	$shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
	$shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	$shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o -L$INGLIB \
        -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1
	$shlink_opts 
	;;
    a64_sol|\
    su9_us5|\
    su4_us5)
	$shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o \
	$shlink_opts 
	;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)q$BITSFX.1  -l(PROG0PRFX)compat$BITSFX.1
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)api.1.$SLSFX ]
        then
          echo $0 Failed,  aborting !!
          exit 4
        fi
	archive_lib lib(PROG1PRFX)api.1
        chmod 755 $INGLIB/lib(PROG1PRFX)api.1.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)api.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
           [ -r $ING_BUILD/lib/lib(PROG1PRFX)api$BITSFX.1.$SLSFX ] && \
	   unlink $ING_BUILD/lib/lib(PROG1PRFX)api$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG1PRFX)api.1.$SLSFX $ING_BUILD/lib/lib(PROG1PRFX)api$BITSFX.1.$SLSFX 
        fi
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o \
        -l(PROG0PRFX)compat.1 \
        $shlink_opts -hlib(PROG1PRFX)api.1.so
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.${slvers} \
	$shlink_opts -soname=lib(PROG1PRFX)api.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG1PRFX)api.${slvers}.$SLSFX \
		 lib(PROG1PRFX)api.1.$SLSFX
	fi
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o \
        -lc \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        -no_unresolved \
	$shlink_opts 
	;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q \
        $shlink_opts -install_name lib(PROG1PRFX)api.1.$SLSFX
	;;
    *)
	$shlink_cmd -o $INGLIB/lib(PROG1PRFX)api.1.$SLSFX *.o \
	$shlink_opts 
	;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX ]
      then
        echo $0 Failed,  aborting !!
        exit 4
      else
        chmod 755 $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf libiiapi
    fi
fi  # do_api
 
#       Making oiice for netscape
#
 
if $do_oiicens
then
    echo ""
    echo " Building oiice$BITSFX.${slvers}.$SLSFX for Netscape "
    echo ""
 
    # check whether libcompat.${slvers}.$SLSFX exists, since oiice needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX ] ||
        { echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}
 
    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/oiicens 2> /dev/null ; cd $workdir/oiicens
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}
 
    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $OIICENS_LIBS
    do
	case $lname in
	    ddf)
		ar x $INGLIB/lib$lname.a ddfcom.o
	    ;;
	    *)
		ar x $INGLIB/lib$lname.a
	    ;;
	esac
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/oiicens.$SLSFX
    echo $shlink_cmd -o $INGLIB/oiicens.${slvers}.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
	$shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/libabfrt.a \
	$shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
        $shlink_opts -L $INGLIB -l(PROG0PRFX)compat.1
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
        $shlink_opts -lelf -L $INGLIB -l(PROG0PRFX)compat.1
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)q$BITSFX.1 -l(PROG0PRFX)compat$BITSFX.1
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
        -l(PROG0PRFX)compat.1 \
        $shlink_opts -hoiicens.1.so 
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/oiicens.${slvers}.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.${slvers} \
	$shlink_opts -soname=oiicens.$SLSFX
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
        -no_unresolved \
        $INGLIB/lib(PROG0PRFX)compat.a \
        $shlink_opts  
	;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/oiicens.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 \
        $shlink_opts -install_name oiicens.$SLSFX
	;;
    *)
        $shlink_cmd -o $INGLIB/oiicens.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts
        ;;
    esac
    if [ $? != 0 -o ! -r $INGLIB/oiicens.${slvers}.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        chmod 755 $INGLIB/oiicens.${slvers}.$SLSFX
        ls -l $INGLIB/oiicens.${slvers}.$SLSFX
        echo Cleaning up ...
        cp $INGLIB/oiicens.${slvers}.$SLSFX $ING_BUILD/ice/bin/$LPHB/ice.${slvers}.$SLSFX
        mv $INGLIB/oiicens.${slvers}.$SLSFX $ING_BUILD/ice/bin/netscape/$LPHB/oiice.${slvers}.$SLSFX
        echo "Moving $INGLIB/oiicens.${slvers}.$SLSFX to $ING_BUILD/ice/bin/netscape/$LPHB/oiice.${slvers}.$SLSFX"
        cd $workdir; 
        if [ "X$IIOPTIM" = "X" ] ; then
            rm -rf oiicens
        fi
    fi
fi  # do_oiicens
#}
 

#       Making oiice for apache
#
 
if $do_oiiceap
then
    echo ""
    echo " Building oiice.${slvers}.$SLSFX for Apache "
    echo ""
 
    # check whether libcompat.${slvers}.$SLSFX exists, since oiice needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX ] ||
       { echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/oiiceap 2> /dev/null ; cd $workdir/oiiceap
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}
 
    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $OIICEAP_LIBS
    do
	case $lname in
	    ddf)
		ar x $INGLIB/lib$lname.a ddfcom.o
	    ;;
	    *)
		ar x $INGLIB/lib$lname.a
	    ;;
	esac
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/oiiceap.$SLSFX
    echo $shlink_cmd -o $INGLIB/oiiceap.${slvers}.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
	$shlink_cmd -o $INGLIB/oiiceap.1.$SLSFX *.o \
	$INGLIB/libcompat.1.$SLSFX \
	$INGLIB/libcuf.a \
	$INGLIB/libabfrt.a \
	$shlink_opts 
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $INGLIB/oiiceap.1.$SLSFX *.o \
        $INGLIB/libcuf.a  \
        $shlink_opts -L $INGLIB -l(PROG0PRFX)compat.1
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $INGLIB/oiiceap.1.$SLSFX *.o \
        $shlink_opts -lelf -L $INGLIB -l(PROG0PRFX)compat.1 -lcuf \
        -lsocket -lnsl -lresolv -lposix4
        ;;
    r64_us5|\
    rs4_us5)
        # Use the httpd.exp file from Apache source as the import file to   
        # build the shared libraries.  
        [ -r $ING_BUILD/lib/lib(PROG0PRFX)compat$BITSFX.1.$SLSFX ] ||
          { echo Need to build lib(PROG0PRFX)compat$BITSFX.1.$SLSFX first, exiting ...;  exit 5 ;}
        exp_dir=`translate_dir "$ING_SRC/front/ice$tnoise/apache/support"`
        $shlink_cmd \
        -bI:$exp_dir/httpd.exp \
        -o $INGLIB/oiiceap.1.$SLSFX *.o \
        -L/lib $shlink_opts \
	$INGLIB/libcuf.a \
        -L$ING_BUILD/lib -l(PROG0PRFX)q$BITSFX.1 -l(PROG0PRFX)compat$BITSFX.1
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/oiiceap.1.$SLSFX *.o \
        -l(PROG0PRFX)compat.1 \
	$INGLIB/libcuf.a \
        $shlink_opts -hoiiceap.1.so    
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/oiiceap.${slvers}.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.${slvers} -lcuf \
	$shlink_opts -soname=oiiceap.$SLSFX
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/oiiceap.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.a \
	$INGLIB/libcuf.a \
	$shlink_opts -no_unresolved 
	;;
     *_osx)
	$shlink_cmd -dynamic -o $INGLIB/oiiceap.1.$SLSFX *.o \
	-L$INGLIB -l(PROG0PRFX)compat.1 \
	$INGLIB/libcuf.a \
        $shlink_opts -install_name oiiceap.1.$SLSFX
	;;
    *)
        $shlink_cmd -o $INGLIB/oiiceap.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/libcuf.a \
        $shlink_opts
        ;;
    esac
    if [ $? != 0 -o ! -r $INGLIB/oiiceap.${slvers}.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        chmod 755 $INGLIB/oiiceap.${slvers}.$SLSFX
        ls -l $INGLIB/oiiceap.${slvers}.$SLSFX
        echo Cleaning up ...
        mv $INGLIB/oiiceap.${slvers}.$SLSFX $ING_BUILD/ice/bin/apache/$LPHB/oiice.${slvers}.$SLSFX
        echo "Moving $INGLIB/oiiceap.${slvers}.$SLSFX to $ING_BUILD/ice/bin/apache/$LPHB/oiice.${slvers}.$SLSFX"
        cd $workdir; 
        if [ "X$IIOPTIM" = "X" ] ; then
            rm -rf oiiceap
        fi
    fi
fi  # do_oiiceap
#}


#{
#       Making oiddi
#
 
if $do_oiddi
then
    # Suffix for oiddi set to the default that dl expects
    # which is ".so.2.0" (we add the dot in the script below)
    if [ "$conf_LSB_BUILD" ] ; then
	SLDDIHEAD=lib;
	SLDDITAIL=$SLSFX.$slvers
    else
	SLDDIHEAD=lib;
	SLDDITAIL=$SLSFX.2.0
    fi
    echo ""
    echo " Building ${SLDDIHEAD}oiddi$BITSFX.$SLDDITAIL "
    echo ""
 
    # check whether libcompat.1.$SLSFX exists, since oiddi needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG0PRFX)compat.$slvers.$SLSFX ] ||
       { echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}
 
    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/oiddi 2> /dev/null ; cd $workdir/oiddi
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}
 
    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $OIDDI_LIBS
    do
	echo "from $INGLIB/lib$lname.a "
        ar x $INGLIB/lib$lname.a
    done
 
    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL
    echo $shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL \*.o $shlink_opts
    case $config in
    axp_osf)
	$shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$shlink_opts -L${INGLIB} -ladf -lgcf -lcuf -lddf
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
        $shlink_opts -L $INGLIB -l(PROG0PRFX)compat.1
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
        $shlink_opts -lelf -L $INGLIB -l(PROG0PRFX)frame.1 -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)q$BITSFX.1 -l(PROG0PRFX)compat$BITSFX.1
        if [ $? != 0 -o ! -r $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL ]
        then
          echo $0 Failed,  aborting !!
          exit 4
        fi
        chmod 755 $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL
        ls -l $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL
        if [ "$do_hyb" = "true" ] ; then
           [ -r $ING_BUILD/lib/${SLDDIHEAD}oiddi$BITSFX.$SLDDITAIL ] && \
	   unlink $ING_BUILD/lib/${SLDDIHEAD}oiddi$BITSFX.$SLDDITAIL
           cp $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL $ING_BUILD/lib/${SLDDIHEAD}oiddi$BITSFX.$SLDDITAIL 
        fi
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
        -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 \
        $shlink_opts -lddf -hoiddi.$SLSFX -lc
        ;;
      *_lnx)
	# Changed the file name to be consistent with the rest of
	# this section, don't know what the -soname option does though,
	# so left as is.
	$shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
	$shlink_opts -soname=oiddi.$SLSFX \
	-L $INGLIB -l(PROG0PRFX)compat.${slvers} -l(PROG0PRFX)frame.${slvers} \
	-l(PROG0PRFX)q.${slvers}
	;;
    int_rpl)
	# Changed the file name to be consistent with the rest of
	# this section, don't know what the -soname option does though,
	# so left as is.
	$shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts -soname=${SLDDIHEAD}oiddi.$SLDDITAIL \
	-L $INGLIB -l(PROG0PRFX)frame.1 -l(PROG0PRFX)q.1
	;;
      *_osx)
	$shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
	$shlink_opts  -install_name oiddi.$SLSFX \
	-L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 \
	-l(PROG0PRFX)q.1 
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
        -lc -lm     \
        -no_unresolved \
        -L$INGLIB -lddf -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1\
        $shlink_opts  \
	;;
    *)
        $shlink_cmd -o $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts
        ;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL ]
      then
        echo $0 Failed,  aborting !!
        exit 4
      else
        chmod 755 $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL
        ls -l $INGLIB/${SLDDIHEAD}oiddi.$SLDDITAIL
      fi
    fi
    echo Cleaning up ... $workdir
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf oiddi
    fi
fi  # do_oiddi
#}

#{
#       Making oinodesmgr
#
 
if $do_oinodesmgr
then
    echo ""
    echo " Building liboinodesmgr.${SLSFX}"
    echo ""
 
    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/oinodesmgr 2> /dev/null ; cd $workdir/oinodesmgr
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}
 
    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $OINODESMGR_LIBS
    do
	ar x $INGLIB/lib$lname.a
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $ING_BUILD/lib/liboinodesmgr.$SLSFX
    echo $shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
	$shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
	-L$INGLIB -lcompat.1 $shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
        -L$INGLIB -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 $shlink_opts
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
	-L$ING_BUILD/lib -lcompat.1 -lq.1 -lframe.1 \
        $shlink_opts -lelf
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
	-L$ING_BUILD/lib -lcompat.1 -lq.1 -lframe.1 \
        -L/lib $shlink_opts
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
	-L$ING_BUILD/lib -lcompat.${slvers} -lq.${slvers} -lframe.${slvers} \
	$shlink_opts -soname=liboinodesmgr.$SLSFX
	;;
    sgi_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
        $INGLIB/libingres.a \
        $shlink_opts
        ;;
    *)
        $shlink_cmd -o $ING_BUILD/lib/liboinodesmgr.$SLSFX *.o \
        $shlink_opts
        ;;
    esac
    if [ $? != 0 -o ! -r $ING_BUILD/lib/liboinodesmgr.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        chmod 755 $ING_BUILD/lib/liboinodesmgr.$SLSFX
        echo Cleaning up ...
        cd $workdir; 
        if [ "X$IIOPTIM" = "X" ] ; then
            rm -rf oinodesmgr
        fi
    fi
fi  # do_oinodesmgr
#}

#{
#       Making oijniquery
#
 
if $do_oijniquery
then
    echo ""
    echo " Building liboijniquery.${SLSFX}"
    echo ""
 
    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/oijniquery 2> /dev/null ; cd $workdir/oijniquery
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}
 
    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $OIJNIQUERY_LIBS
    do
	ar x $INGLIB/lib$lname.a
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $ING_BUILD/lib/liboijniquery.$SLSFX
    echo $shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
	$shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
	-L$INGLIB -lcompat.1 $shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
        -L$INGLIB -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 $shlink_opts
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
	-L$ING_BUILD/lib -lcompat.1 -lq.1 -lframe.1 \
        $shlink_opts -lelf
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
	-L$ING_BUILD/lib -lcompat.1 -lq.1 -lframe.1 \
        -L/lib $shlink_opts
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
	-L$ING_BUILD/lib -lcompat.1 -lq.1 -lframe.1 -lfstm -lqr \
	$shlink_opts -soname=liboijniquery.$SLSFX
	;;
    sgi_us5)
        $shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
        $INGLIB/libingres.a \
        $shlink_opts
        ;;
    *)
        $shlink_cmd -o $ING_BUILD/lib/liboijniquery.$SLSFX *.o \
        $shlink_opts
        ;;
    esac
    if [ $? != 0 -o ! -r $ING_BUILD/lib/liboijniquery.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        chmod 755 $ING_BUILD/lib/liboijniquery.$SLSFX
        echo Cleaning up ...
        cd $workdir; 
        if [ "X$IIOPTIM" = "X" ] ; then
            rm -rf oijniquery
        fi
    fi
fi  # do_oijniquery
#
#
#       Making neworder
#
 
if $do_neworder
then
    if [ "$conf_LSB_BUILD" ] ; then
	SLDDIHEAD=lib;
	SLDDITAIL=$SLSFX.$slvers
    else
	SLDDIHEAD=lib;
	SLDDITAIL=$SLSFX.2.0
    fi
    echo ""
    echo " Building ${SLDDIHEAD}neworder.${SLDDITAIL} "
    echo ""
 
    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/neworder 2> /dev/null ; cd $workdir/neworder
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}
 
    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $NEWORDER_LIBS
    do
	ar x $INGLIB/lib$lname.a
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0
    echo $shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 \*.o $shlink_opts
    case $config in
    axp_osf)
	$shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 *.o \
	-L$INGLIB -lcompat.1 $shlink_opts
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 *.o \
        -L$INGLIB -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 $shlink_opts
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 *.o \
        $shlink_opts -lelf
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 *.o \
        -L/lib $shlink_opts
        ;;
    *_lnx|int_rpl)
	$shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLDDITAIL *.o \
	$shlink_opts -soname=libplay_NewOrder.$SLDDITAIL
	;;
    sgi_us5)
        $shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 *.o \
        $INGLIB/libingres.a \
        $shlink_opts
        ;;
    *)
        $shlink_cmd -o $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 *.o \
        $shlink_opts
        ;;
    esac
    if [ $? != 0 -o ! -r $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0 ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        chmod 755 $ING_BUILD/files/dynamic/${LPHB}/libplay_NewOrder.$SLSFX.2.0
        echo Cleaning up ...
        cd $workdir; 
        if [ "X$IIOPTIM" = "X" ] ; then
            rm -rf neworder
        fi
    fi
fi  # do_neworder
#}

#
#       Making libiiodbcdriver.1
#
#              libiiodbcdriver must be sufficently resolved of link symbols
#              so that is can be dynamicly loaded and not depend on
#              link symbol resolution by the ODBC application or
#              ODBC driver manager.

if $do_odbc
then
    # start of read only/read write for loop
    readwrite="getrw getro"
    for GETRO in $readwrite
    do
        if [ $GETRO = "getrw" ]
        then
	    RO=
        fi
        if [ $GETRO = "getro" ]
        then
	    RO="ro"
        fi

    echo "ODBCDRIVER_LIBS $ODBCDRIVER_LIBS"
    echo ""
    echo " Building lib(PROG1PRFX)odbcdriver$BITSFX.${slvers}.$SLSFX "
    echo ""

    # check whether libcompat.${slvers}.$SLSFX exists, since odbcdriver needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/libcompat.${slvers}.$SLSFX ] ||
      { echo Need to build libcompat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}
    # check whether libiiapi.${slvers}.$SLSFX exists, since odbcdriver needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX ] ||
      { echo Need to build lib(PROG1PRFX)api.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/libiiodbcdriver 2> /dev/null ; cd $workdir/libiiodbcdriver
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $ODBCDRIVER_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    if  [ "$do_hyb" = "true" ] ; then
 	(cd $ING_SRC; jam '<common!odbc!driver>'${LPHB}/$GETRO.o )
        cp $ING_SRC/common/odbc$pnoise/driver/${LPHB}/$GETRO.o .
    else
 	(cd $ING_SRC; jam '<common!odbc!driver>'$GETRO.o )
        cp $ING_SRC/common/odbc$pnoise/driver/$GETRO.o .
    fi

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $shlink_opts
        ;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        $shlink_opts -L$INGLIB \
                     -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 -l(PROG1PRFX)api.1
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        $shlink_opts -lelf -lsocket -L$INGLIB \
                     -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 -l(PROG1PRFX)api.1
#                    -l(PROG0PRFX)compat.1 -l(PROG1PRFX)api.1 -ladf -lgcf -lcuf
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib \
              -l(PROG0PRFX)q$BITSFX.1 -l(PROG0PRFX)compat$BITSFX.1 \
              -l(PROG1PRFX)api$BITSFX.1
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX ]
        then
           echo $0 Failed,  aborting !!
           exit 4
        fi
        chmod 755 $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
          [ -r $ING_BUILD/lib/lib(PROG1PRFX)odbcdriver$BITSFX$RO.1.$SLSFX ] && \
	   unlink $ING_BUILD/lib/lib(PROG1PRFX)odbcdriver$BITSFX$RO.1.$SLSFX
           cp $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX $ING_BUILD/lib/lib(PROG1PRFX)odbcdriver$BITSFX$RO.1.$SLSFX 
        fi
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX *.o \
        -L $INGLIB -l(PROG0PRFX)q.${slvers} -l(PROG0PRFX)compat.${slvers} \
        -l(PROG0PRFX)frame.${slvers}  -l(PROG1PRFX)api.${slvers} \
        $shlink_opts -soname=lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX \
		 lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX
	fi
        ;;
    sgi_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $shlink_opts -no_unresolved
        ;;
      *_osx)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        -L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 \
        -l(PROG1PRFX)api.1 $shlink_opts \
	-install_name lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX
        ;;
    *)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $shlink_opts
        ;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX ]
      then
        echo $0 Failed,  aborting !!
        exit 4
      else
        chmod 755 $INGLIB/lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)odbcdriver$RO.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf libiiodbcdriver
    fi
    # end of read only/read write while loop
    done
fi  # do_odbc

#
#       Making libiiodbc
#
#              libiiodbc depends only on the compat lib.

if $do_cli
then
    echo "ODBCCLI_LIBS $ODBCCLI_LIBS"
    echo ""
    echo " Building lib(PROG1PRFX)odbc.${slvers}.$SLSFX "
    echo ""

    # check whether libcompat.${slvers}.$SLSFX exists, since odbc cli needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/libcompat.${slvers}.$SLSFX ] ||
     { echo Need to build libcompat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    # check whether libiiapi.${slvers}.$SLSFX exists, since odbc cli needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG1PRFX)api.${slvers}.$SLSFX ] ||
      { echo Need to build lib(PROG1PRFX)api.${slvers}.$SLSFX first, exiting ...;  exit 5 ;} 
    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/libodbccli 2> /dev/null ; cd $workdir/libodbccli
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $ODBCCLI_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    for lname in $ODBCCFG_LIBS
    do
       ar x $INGLIB/lib$lname.a
    done
    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG1PRFX)odbc.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.${slvers}.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $shlink_opts
        ;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        $shlink_opts -L$INGLIB \
        -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 -l(PROG1PRFX)api.1
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        $shlink_opts -lelf -lsocket -L$INGLIB \
        -l(PROG0PRFX)q.1 -l(PROG0PRFX)compat.1 -l(PROG1PRFX)api.1 
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)q$BITSFX.1 \
        -L$ING_BUILD/lib -l(PROG1PRFX)api$BITSFX.1 \
        -L$ING_BUILD/lib -l(PROG0PRFX)compat$BITSFX.1 
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX ]
        then
           echo $0 Failed,  aborting !!
           exit 4
        fi
        chmod 755 $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
          [ -r $ING_BUILD/lib/lib(PROG1PRFX)odbc$BITSFX.1.$SLSFX ] && \
           unlink $ING_BUILD/lib/lib(PROG1PRFX)odbc$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX $ING_BUILD/lib/lib(PROG1PRFX)odbc$BITSFX.1.$SLSFX 
        fi
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $shlink_opts
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.${slvers}.$SLSFX *.o \
        -L $INGLIB -l(PROG0PRFX)compat.${slvers} -l(PROG0PRFX)q.${slvers} \
        -l(PROG0PRFX)frame.${slvers} -l(PROG1PRFX)api.${slvers} \
        $shlink_opts -soname=lib(PROG1PRFX)odbc.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG1PRFX)odbc.${slvers}.$SLSFX \
		 lib(PROG1PRFX)odbc.1.$SLSFX
	fi
        ;;
    sgi_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \
        $shlink_opts -no_unresolved
        ;;
      *_osx)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        -L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 \
        -l(PROG1PRFX)api.1 $shlink_opts \
	-install_name lib(PROG1PRFX)odbcdriver$RO.1.$SLSFX
        ;;
    *)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbc.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $INGLIB/lib(PROG1PRFX)api.1.$SLSFX \

        $shlink_opts
        ;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)odbc.${slvers}.$SLSFX ]
      then
         echo $0 Failed,  aborting !!
         exit 4
      else
        chmod 755 $INGLIB/lib(PROG1PRFX)odbc.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)odbc.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf libodbccli
    fi
fi  # do_cli

#
#       Making libiiodbctrace
#
#              libiiodbctrace depends only on the compat lib.

if $do_odbc_trace
then
    echo "ODBCTRACE_LIBS $ODBCTRACE_LIBS"
    echo ""
    echo " Building lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX "
    echo ""

    # check whether libcompat.${slvers}.$SLSFX exists, since odbc trace needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/libcompat.${slvers}.$SLSFX ] ||
     { echo Need to build libcompat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/libodbctrace 2> /dev/null ; cd $workdir/libodbctrace
         [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $ODBCTRACE_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    for lname in $ODBCCFG_LIBS
    do
       ar x $INGLIB/lib$lname.a
    done
    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX \*.o $shlink_opts
    case $config in
    axp_osf)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX 
        $shlink_opts
        ;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o \
        $shlink_opts -L$INGLIB \
        -l(PROG0PRFX)compat.1
        ;;
    a64_sol|\
    su9_us5|\
    su4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o \
        $shlink_opts -lelf -lsocket -L$INGLIB \
        -l(PROG0PRFX)compat.1
        ;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)compat$BITSFX.1 
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX ]
        then
           echo $0 Failed,  aborting !!
           exit 4
        fi
        chmod 755 $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
          [ -r $ING_BUILD/lib/lib(PROG1PRFX)odbctrace$BITSFX.1.$SLSFX ] && \
           unlink $ING_BUILD/lib/lib(PROG1PRFX)odbctrace$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX $ING_BUILD/lib/lib(PROG1PRFX)odbctrace$BITSFX.1.$SLSFX 
        fi
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX *.o \
        -L$INGLIB -l(PROG0PRFX)compat.${slvers} \
        $shlink_opts -soname=lib(PROG1PRFX)odbctrace.1.$SLSFX
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX \
		 lib(PROG1PRFX)odbctrace.1.$SLSFX
	fi
        ;;
    sgi_us5)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts -no_unresolved
        ;;
    *)
        $shlink_cmd -o $INGLIB/lib(PROG1PRFX)odbctrace.1.$SLSFX *.o \
        $INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
        $shlink_opts
        ;;
    esac
    if [ $? != 0 -o ! -r $INGLIB/lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX ]
    then
         echo $0 Failed,  aborting !!
         exit 4
    else
        chmod 755 $INGLIB/lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG1PRFX)odbctrace.${slvers}.$SLSFX
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf libodbctrace
    fi
fi  # do_odbc_trace

#
#       Making liboiutil.1
#
if $do_oiutil
then
    echo ""
    echo " Building lib(PROG0PRFX)oiutil$BITSFX.${slvers}.$SLSFX "
    echo ""

    # check whether libcompat.${slvers}.$SLSFX exists, since api needs it.
    # If it exists, continue processing. Otherwise exit.
    [ -r $INGLIB/lib(PROG0PRFX)compat.${slvers}.$SLSFX ] ||
      { echo Need to build lib(PROG0PRFX)compat.${slvers}.$SLSFX first, exiting ...;  exit 5 ;}

    # Create a temporary working directory. cd to the dir. Exit, if it fails.
    mkdir $workdir/liboiutil 2> /dev/null ; cd $workdir/liboiutil
	 [ $? != 0 ] &&  { echo Aborting ...; exit 3 ;}

    # Clean up the working dir and extract the objects from archive.
    rm -f *
    echo Extracting objects ...
    for lname in $OIUTIL_LIBS
    do
    	ar x $INGLIB/lib$lname.a
    done

    [ "$config" = "r64_us5" -o "$config" = "rs4_us5" ] && mkexplist
    rm -f $INGLIB/lib(PROG0PRFX)oiutil.${slvers}.$SLSFX
    echo $shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.${slvers}.$SLSFX \*.o $shlink_opts 
    case $config in
    axp_osf)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)frame.1.$SLSFX \
	$shlink_opts -L$INGLIB -lgcf -lutil -linstall
	;;
    i64_hpu|\
    hp2_us5|\
    hpb_us5|\
    hp8_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
	  -L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1       \
	  -l(PROG0PRFX)frame.1 $INGLIB/libgcf.a $INGLIB/libutil.a \
	  $INGLIB/libinstall.a $shlink_opts
	;;
    a64_sol|\
    su9_us5|\
    su4_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
	$shlink_opts -L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)frame.1 \
	                       -l(PROG0PRFX)q.1 -lgcf -lutil -linstall
	;;
    r64_us5|\
    rs4_us5)
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o  \
        -L/lib $shlink_opts \
        -L$ING_BUILD/lib -l(PROG0PRFX)q$BITSFX.1  -l(PROG0PRFX)compat$BITSFX.1 \
           -l(PROG0PRFX)frame$BITSFX.1  -L$INGLIB -lgcf -lutil -linstall
        if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX ]
        then
          echo $0 Failed,  aborting !!
          exit 4
        fi
        chmod 755 $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX
        if [ "$do_hyb" = "true" ] ; then
          [ -r $ING_BUILD/lib/lib(PROG0PRFX)oiutil$BITSFX.1.$SLSFX ] && \
           unlink $ING_BUILD/lib/lib(PROG0PRFX)oiutil$BITSFX.1.$SLSFX
           cp $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX $ING_BUILD/lib/lib(PROG0PRFX)oiutil$BITSFX.1.$SLSFX 
        fi
        ;;
    usl_us5 )
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
        -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 -l(PROG0PRFX)frame.1 \
        $shlink_opts -hlib(PROG0PRFX)oiutil.1.so  -lgcf -lutil -linstall
        ;;
      *_lnx|int_rpl)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.${slvers}.$SLSFX *.o \
	$shlink_opts -soname=lib(PROG0PRFX)oiutil.1.$SLSFX  \
	-L $INGLIB -l(PROG0PRFX)compat.${slvers} -l(PROG0PRFX)frame.${slvers} \
	           -l(PROG0PRFX)q.${slvers} -lgcf -lutil -linstall
	if [ $? = 0 -a "$slvers" != "1" ] ; then
	    cd $INGLIB
	    ln -sf lib(PROG0PRFX)oituil.${slvers}.$SLSFX \
		 lib(PROG0PRFX)oituil.1.$SLSFX
	fi
	;;
    sgi_us5)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
        -lc -no_unresolved   \
        -L$INGLIB -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 \
                  -l(PROG0PRFX)frame.1 -lgcf -lutil -linstall    \
	$shlink_opts 
	;;
      *_osx)
        $shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
        -L$INGLIB  -l(PROG0PRFX)compat.1 -l(PROG0PRFX)q.1 \
        -l(PROG0PRFX)frame.1 -lutil -linstall $ING_BUILD/lib/libgcf.a  \
	$shlink_opts -install_name lib(PROG0PRFX)oiutil.1.$SLSFX
        ;;
    *)
	$shlink_cmd -o $INGLIB/lib(PROG0PRFX)oiutil.1.$SLSFX *.o \
	$INGLIB/lib(PROG0PRFX)compat.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)q.1.$SLSFX \
	$INGLIB/lib(PROG0PRFX)frame.1.$SLSFX \
	$shlink_opts  -lgcf -lutil -linstall
	;;
    esac
    if [ "$config" != "rs4_us5" -a "$config" != 'r64_us5' ] ; then
      if [ $? != 0 -o ! -r $INGLIB/lib(PROG0PRFX)oiutil.${slvers}.$SLSFX ]
      then
        echo $0 Failed,  aborting !!
        exit 4
      else
        chmod 755 $INGLIB/lib(PROG0PRFX)oiutil.${slvers}.$SLSFX
        ls -l $INGLIB/lib(PROG0PRFX)oiutil.${slvers}.$SLSFX
      fi
    fi
    echo Cleaning up ...
    cd $workdir; 
    if [ "X$IIOPTIM" = "X" ] ; then
        rm -rf liboiutil
    fi
fi  # do_oiutil


#
# Build Server shared libraries
#
if $use_shared_svr_libs && [ $param = 0 ]
then
    if [ "$do_hyb" = "true" ]
    then
	mksvrshlibs -lp${HB}
    else
	mksvrshlibs
    fi
fi

#
# Build Kerberos shared library
#
if $do_kerberos && $use_kerberos
then
    $do_hyb && mkkrb5shlib -lp${HB}
    $do_reg && mkkrb5shlib
fi
# any other cleanup ?
exit 0

