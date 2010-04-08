:
#
# Copyright (c) 1990, 2008 Ingres Corporation
#
#
# This script creates the iisysdep file, which contains system- or
# architecture-dependent definitions which are used by all of the
# install scripts.  All install scripts which have even the least
# potential for having system dependencies -- which is to say, probably
# all of them period -- should include iisysdep, just in case.  Any
# system-dependent code in such files should be if'd or case'd on some-
# thing defined in iisysdep.
#
## History:
##      25-jun-90 (jonb)
##              Created
##      20-jul-90 (achiu)
##              replace "export IPCSRM"  by IPCRM, there was no IPCSRM defined.
##              replace "\\c"  by "`\\\\c` to get correct \c for ECHO_CHAR.
##              add PAHT = ... to make sure getting right path.
##      24-jul-90 (jonb)
##              Generate preprocessed ming/default.h definitions into
##              iisysdep.
##      05-sep-90 (vijay)
##              Removed quotes for the variable output_sink.
##              put new line in the awk script to make it work on my fussy awk.
##              Changed the logic for AWKMAJOR etc. Instead of checking if
##              ls -L works, we now check if the 5th element of the output
##              of ls -l is the month, ie [A-Z,a-z]*.
##      01-oct-90 (vijay)_
##              Put in LINKCMD.
##       01-oct-90 (hhsiung)
##               Don't check swap space on bu* because command pstat
##               doesn't exist.
##      08-nov-90 (rog)
##              Don't check swap space on hp3*.
##      08-nov-90 (rog)
##              Remove DLL_FLAGS because jonb claims there no longer necessary.
##      04-dec-90 (kirke)
##              Changed quoting for WHOAMI.  Don't check swap space on hp8*.
##              Added quotes back to output_sink.
##       19-mar-91 (dchan)
##              Added define for ds3_ulx for pstat parameter positions.
##              and added information for decnet libraries.
##       21-mar-91 (dchan)
##              The decision to include decnet libraries are now surrounded
##               by a case statement to avoid clashes with other boxes.
##       22-mar-91 (dchan)
##               Deleted the extraneous setting for "iisys".
##               Adjusted pstat command for dec boxes.
##      22-apr-1991 (jonb)
##              Change CHECK_MEMORY_RESOURCES and CHECK_SWAP_SPACE flags
##              to be true/false, rather than yes/no.
##      23-apr-91 (rudyw)
##              Changed check_swap_space and pstat variables for odt.
##      23-apr-91 (rudyw)
##              Changed check_swap_space to use false instead of no.
##      24-apr-1991 (jonb)
##              Put in LINKCMD.
##      24-apr-1991 (jonb)
##              Two changes involving the default.h variables.  Generalized
##              dchan's 21-mar-91 change, so that all non-existent libraries
##              are removed from LIBMACH, LDLIB, and LDLIBMACH at runtime.
##              Expanded code to grab all symbols defined in default.h,
##              including the ones defined with #define.
##      25-apr-1991 (jonb)
##              In the previous change I clobbered the code that gets the
##              platform name for the platform-specific check.  Restore it.
##              Also, don't generate PSTAT_* variables if we're not going
##              to check swap space.
##      6-may-1991 (jonb)
##              Changed test for null sed command to prevent the "test"
##              utility from trying to interpret the sed flags.
##      13-may-1991 (bonobo)
##              Added $noise when looking for default.h.
##      18-jun-1991 (jonb)
##              Removed AWKMAJOR and AWKMINOR.  The only script that used
##              them, mkrawlog, has been changed so they're unnecessary.
##      25-jun-91 (jonb)
##              Use the LDLIBPATH definition from default.h to determine
##              which directories should contain system libraries, and
##              trim libraries that don't exist in those dirs from the
##              various library lists.
##       14-aug-91 (hhsiung)
##               Move uname in front of hostname for HOSTNAME setup.  Bull
##               name server domain hostname command will return a full
##               e-mail hostname like angus.bull.com.
##               Add test /usr/bin/whoami before using IFS... command.
##               DPX/2 korn shell has a bug, and cannot get correct
##               username using IFS... command.
##      11-aug-91 (vijay)
##              Need to export LANG=C for AIX3.1. Else most utilities' output
##              (eg. dd) comes out different from the usual (bugs 37526 and
##              38127). Also changed the "##" to  "#" in echolog, else lines
##              removed by ming !
##       20-aug-91 (hhsiung)
##               Add section to define BPATHSED to handle different
##               device name convention among differnet machine.
##               BPATHSED will be used by sed command in mkrawlog later on.
##       16-sep-91 (hhsiung)
##               BPATHSED should be produced during run-time.  Defer it to
##               iisysdep. Also, correct a typo "esca" should be "esac".
##       20-sep-91 (hhsiung)
##               Remove the single quote from BPATHSED setup section.
##               It should not be part of BPATHSED pass to mkrawlog's
##               sed command.
##      05-nov-91 (kirke)
##              Added fgrep to remove XTERMLIB, XNETLIB, and WINSYSLIB from
##              the default.h information.  These libraries are only needed
##              by ming and the $(ING_BUILD) possibly embedded in their
##              definitions causes problems in iisysdep.
##      06-nov-91 (vijay)
##              Added a new variable ACPART_NOT_ALLOWED which is set to true
##              by default. It is used by the mkrawlog script. It stands for
##              whether the raw log is allowed on 'a' or 'c' partition.
##              On IBM and HP machines, this restriction is not present, but
##              we were unnecessarily restricting the user not to have
##              raw log partitions in 'a' or 'c'.
##      07-nov-91 (vijay)
##              pstat is broken on the RS/6000. Using 'lsps' command to
##              get the swap space. Also not checking memory resources
##              since most of the semaphore and shared memory resources are
##              either huge or dynamically allocated (and not easily
##              determined.)
##      07-nov-91 (szeng)
##              Added DF which stands for BSD style df. In HP SYSV
##              df has caused diskfree get the wrong field for the number
##              free disk block for NFS disks and the unexpected value,
##              512-byte blocks instead of kilebyte blocks, for local
##              file system.
##      22-nov-91 (rudyw)
##              Set ACPART_NOT_ALLOWED to false for odt_us5.
##              Change swap space parameters for odt_us5.
##              Change the delimiter char for BPATHSED from '/' to ',' to
##               allow removal of '\' char in string which confused ODT.
##              Add sco* to odt* case to accomodate change in port id.
##      22-oct-91 (Sanjive)
##               Add dr6_us5 and dra_us5 to list of machines using 'swap -s'
##              and taking care to escape the specials chars [a-z] to avoid
##              interpretation by the shell.
##      11-dec-1991 (swm)
##              Integration of ingres6302p change for dr6_us5 and dra_us5:
##              Eliminate libelf.a from applications search library list on
##              dr6_us5 and dra_us5; though this library is normally present
##              on these machines it is only needed for linking the rcheck
##              executable during an INGRES build.
##      29-jan-1992 (rudyw)
##              Add system dependency variable HAS_DEVSYS to be set to true
##              or false based on the availability of a development system.
##              Used in 'mkvalidpw' and 'iilink' to determine whether the
##              load operation may be performed. Default is true.
##      30-jan-1992 (szeng)
##              Added double quote for BPATHSED for ds3_ulx.
##      04-feb-1992 (rudyw)
##              Add export of recently created HAS_DEVSYS variable.
##      25-feb-1992 (jonb)
##              Wrap current logic for determining definition of HOSTNAME
##              in a case on platform ID.  On Sun, we want to use "hostname"
##              rather than "uname -n" to avoid SysV truncation at 9 chars.
##              (b42616)
##      01-apr-1992 (johnr)
##              Added condition for ds3_osf to handle ps options.
##              Also added pstat null values and turned off swap tests
##              OSF/1 does not have pstat and the alternatives are unclear.
##              used "df -k" to get disk space in bytes.  This requires the
##              escape quotes on the echo DF command.
##      06-apr-92 (rocky, deannaw)
##               Added security checks for ds3_ulx. This change is mainly
##              Integration of ingres6302p change for dr6_us5 and dra_us5:
##              Eliminate libelf.a from applications search library list on
##              dr6_us5 and dra_us5; though this library is normally present
##              on these machines it is only needed for linking the rcheck
##              executable during an INGRES build.
##      29-jan-1992 (rudyw)
##              Add system dependency variable HAS_DEVSYS to be set to true
##              or false based on the availability of a development system.
##              Used in 'mkvalidpw' and 'iilink' to determine whether the
##              load operation may be performed. Default is true.
##      30-jan-1992 (szeng)
##              Added double quote for BPATHSED for ds3_ulx.
##      04-feb-1992 (rudyw)
##              Add export of recently created HAS_DEVSYS variable.
##      25-feb-1992 (jonb)
##              Wrap current logic for determining definition of HOSTNAME
##              in a case on platform ID.  On Sun, we want to use "hostname"
##              rather than "uname -n" to avoid SysV truncation at 9 chars.
##              (b42616)
##      01-apr-1992 (johnr)
##              Added condition for ds3_osf to handle ps options.
##              Also added pstat null values and turned off swap tests
##              OSF/1 does not have pstat and the alternatives are unclear.
##              used "df -k" to get disk space in bytes.  This requires the
##              escape quotes on the echo DF command.
##      06-apr-92 (rocky, deannaw)
##               Added security checks for ds3_ulx. This change is mainly
##              for mkvalidpw in admin/install/shell/iirungcc.sh.
##      25-Mar-1992 (gautam)
##              On RS/6000, we do check memory resources. BPATHSED, and df
##              command to check for raw device being used for a filesystem
##              is different on the RS/6000
##   9-April-1992 (gautam)
##       Corrected the DFCMD, since it did'nt work correctly on the SUN
##      16-oct-1992 (lauraw)
##              Uses readvers to get config string -- before PATH is set!
##      30-oct-1992 (lauraw)
##              Changed the "trap '' 0" at the end of this script to "trap 0"
##              because on some platforms the former has no effect. In any
##              case, the latter is correct -- we are *unsetting* the trap
##              for 0, not setting it to be ignored.
##      19-nov-1992 (lauraw)
##              Don't need to check VERSION.ING because we can get
##              release id from genrelid. This loosens up some of the
##              initial build dependencies.
##      11-dec-1992 (smc)
##              Added axp_osf to ds3_osf case for OSF/1 configuration.
##      13-jan-1993 (terjeb)
##              Added section of code to properly process minor number on
##              HP machines. Minor numbers on these systems are expressed
##              in hex numbers. This change must be taken along with the
##              change made to mkrawlog.sh. Added variable SED_MAJOR_MINOR
##              for this purpose. Also added BPATHSED case for HP machines.
##      19-jan-93 (sweeney)
##              usl_us5 uses swap -s. Also check for .so libs as well as .a
##      05-mar-93 (dianeh)
##              Modified so that no iisysdep.sh is generated, since there's
##              really no reason to go through that step; changed the echo'd
##              header so that the first line isn't so long; removed the
##              echo'd "End of ...." at the bottom; changed History comment
##              block to double pound-signs (also cleaned up line lengths).
##      09-Apr-1993 (fredv)
##              Changed the permission of iisysdep to 755 instead of 770.
##      23-apr-1993 (lauraw)
##              Added AR_L_OPT variable. Mergelibs uses "l" with "ar"
##              (although it seems to work the same with or without it) and
##              su4_us5 doesn't have "ar l". On the chance that other
##              machines may also lack "ar l", let's define it in iisysdep.
##      30-apr-93 (vijay)
##              Add shared library variables. Add ING_SRC/bin to path.
##      13-may-93 (swm)
##              If a private copy of tools is being built add
##              $ING_SRC/$ING_VERS/bin to path rather than $ING_SRC/bin.
##      14-jun-93 (lauraw)
##              Configure pstat for su4_us5.
##      9-aug-93 (robf)
##              Add su4_cmw
##      14-oct-93 (vijay)
##              Removed defunct HAS_DEVSYS, which used to determine if
##              compiler is available.
##      27-oct-93 (seiwald)
##              Delete file before moving on top of it to avoid pesking
##              prompting from mv.
##      09-nov-93 (smc)
##              Added LSSIZEFLAGS to allow for variable 'ls' flags.
##      15-nov-93 (vijay)
##              Add READ_KMEM from ingres63p. If set to false, then syscheck
##              will not check if /dev/kmem is readable. on rs/6000, rl.c
##              merely prints out documented information, it does not need to
##              read kmem.
##      16-dec-93 (lauraw)
##              default.h has moved.
##      12-jan-94 (tyler)
##              Added SU_FLAGS to be used by mkrawlog.
##      13-jan-94 (vijay)
##              On AIX, use full path for invoking 'lsps'. It is in /etc
##              and /usr/sbin.
##      20-jan-94 (dianeh)
##              Fix the echo on SU_FLAGS to avoid getting 2 echo'd out to
##              the file.
##      25-jan-94 (arc)
##              Support Sun CMW (B1 secure O/S).
##      30-jan-94 (tomm)
##              Hard wire PSCMD to /bin/ps for su4_us5.  This prevents
##              us from accidently picking up /usr/ucb/ps.
##      30-jan-94 (tomm)
##              This previous change fixed bug #59239.  Needed to note that.
##      09-feb-94 (ajc)
##               Modified for hp8_bls in line with hp8_us5.
##      01-mar-94 (nrb)
##              Bug #60263.
##              On axp_osf, iisudbms fails to start owing to shared memory
##              detach problems. The ipcs command displays its key value
##              column in decimal but INGRES utilities such as ipcclean expect
##              key values to be displayed in hex.
##              Changed IPCSCMD to post-process ipcs output to display
##              key values in hex. on axp_osf.
##      24-mar-94 (robf)
##               Add secure options:
##               ADDPRIV_SERVER - Server privileges
##               ADDPRIV_MAINT  - Maintenance privileges
##               ADDPRIV_CLIENT - Client privileges
##               SETLABEL       - Set file label
##      13-apr-94 (arc)
##              Add 'sys_trans_label' to SERVER/MAINT privileges for
##              Sun CMW (slutil requires it to translate INGRES_HIGH label).
##      14-apr-94 (arc)
##              Add 'file_setpriv' to maintenance utilities as iisetlab
##              requires this privilege to successfully use chpriv on Sun CMW.
##              Also require 'file_[up/down]grade_[sl/il]' privileges
##              as iisues may be used to alter INGRES_LOW/HIGH to a label
##              that is dominated by or that dominates current session.
##      18-nov-1994 (harpa06)
##              Integrated bug fix #64417 from INGRES 6.4 by nick:
##              Set etcrcfiles for several flavors rather than the default
##      14-Feb-1995 (canor01)
##              B62644: users on Sun may have /usr/ucb/echo in the path
##              in front of /bin/echo, so check all flavors
##      08-mar-95 (nick)
##              Now calculate swap space in kbytes rather in bytes to
##              avoid overflow problems in `expr`. #33399
##      09-mar-95 (nick)
##              Set bsd_df to 'df -k' for su4_us5 to fix bug 65726 - this
##              also has the advantage of avoiding possible overflow
##              'cos we don't use 'expr' with BSD style output. Should
##              probably do this for other SVR4 flavour OS versions.
##      31-mar-1995 (canor01)
##              Set COLUMNS=132 for AIX 4.1 PSCMD. Otherwise the full
##              command line gets cut off.
##      14-apr-1995 (harpa06)
##              Backed out cross integration change on 18-Nov-1994. Bad change.
##              Fixed the change here.
##      22-Nov-94 (nive)
##              During OpenINGRES 1.1 port onto dg8_us5
##              1. Created BPATHSED for dg8_us5.
##              2, Don't check swap space on dg8* because pstat doesn't exist.
##              3. Added USE_SYMBOLIC_LINKS mechanism from 6.4.DG mknod doesn't
##                 preserve files during reboot.
##              4. Added -h to search for libraries which exist as links
##	05-may-1995 (smiba01)
##		Added support for dr6_ues and dr6_uv1
##	3-may-1995 (wadag01)
##		Changed swap parameters for odt_us5 (odt*|sco* actually)
##		to align with 6.4/05.
##		Note, pstat_byte_factor in 6.4/05 (=512) is replaced by
##		pstat_kbyte_factor in OpING 1.1/03 (='1/2'). The single
##		quotes are required to protect the / char (0.5 doesn't work).
##	15-may-1995 (popri01)
##		Pick up rs4_us5 PSCMD change for AIX 3.2 (ris_us5)
##	09-Jun-1995 (walro03)
##		From nive's change of 22-Nov-94, "test -h" is a valid test for
##		symbolic links on some platforms.  On others, the flag is "-L".
##		This change uses the -L flag for AIX (ris_us5) and -h for
##		all others.
##	15-jun-1995 (popri01)
##		Add  appropriate pstat for nc4_us5
##	17-jul-1995 (morayf)
##		Added sos.us5 to odt/sco swap parameters section.	
##	25-jul-1995 (allmi01)
##		Added support for dgi_us5 (DG-UX on Intel based platforms).
##	 9-oct-1995 (nick)
##		USE_SYMBOLIC_LINKS code incomplete.  Need to initialise to
##		false and export.  Removed vestigal pstat_byte_factor which
##		is now obsolete (replaced by pstat_kbyte_factor some time ago).
##	10-nov-1995 (murf)
##		Added sui_us5 along side su4_us5. Porting Solaris on Intel.
##	17-nov-1995 (hanch04)
##		added define of PINGCMD
##	 3-dec-1995 (allst01)
##		Corrected the 14-Apr-95 change which had adverse affects on
##		Trusted Solaris (su4_cmw).
##		B1 changes for su4_cmw.
##		Added ADDPRIV_KMEM for reading /dev/kmem
##		Added downgrade privs to server progs, should finally fix
##		the symbol.tbl problem !
##		Added ADDPRIV_SLUTIL so that slutil can change the
##		process label during installation.
##		Must have priv proc_tranquil as well on CMW to change the
##		MAC label of a file that is open.
##		Also need proc_nofloat to stop ILs causing problems.
##	27-Nov-1995 (spepa01)
##		PSTAT_KBYTE_FACTOR value must be twice quoted for dr6_us5.
##	17-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
##		Added IPCSCMD2 in the case where extra parameters are to be
##		appended (cannot append parameters after an awk is done). In
##		the case this is to be used, the hex format is not necessary.
##	23-jan-1996 (toumi01)
##		"test -h" and "test -L" are invalid for Digital OSF1 release
##		3.0, and there is no equivalent.  For axp_osf and for this
##		release only set LINKTESTFLAG='-f'.
##	9-may-1996 (angusm)		
##		Add PINGARGS for platforms where 'ping' without arguments
##		continues indefinitely: rewrite some of the PINGCMD stuff
##		also (bug 75611)	
##	14-nov-1996 (kinpa04)
##		Changed order of testing ECHO command so that the default
##		of ECHO_CHAR=\\c would apply to 3.x & 4.x of axp_osf since
##		the use of -n flag changed between System releases.
##	30-may-1997 (muhpa01)
##		Pick up shared libs with '.sl' extension for LDLIB & LDLIBMACH
##		(hp8_us5 port)
##	23-jun-1997 (popri01)
##		Clean-up Unixware (usl_us5): remove archive "l" option.
##	23-jun-1997 (walro03)
##		Reverse change of 11-dec-91 and 6-apr-92; allow -lelf in
##		LDLIBMACH for dr6_us5 (ICL DRS 6000); it is needed for iilink.
##	28-jul-1997 (walro03)
##		Update for Tandem NonStop (ts2_us5).
##      29-aug-1997 (musro02)
##              Add comments concerning the calculation of BTPERBLK
##              For sqs-ptx 
##                  use swap -f for pstat, use /etc/rc2.d for etcrcfiles
##                  use /etc/ping for PINGCMD
##      01-sep-1997 (rigka01) bug #85043
##		change IPCSCMD so that hex value in printf is always 
##              8 hex characters long
##	23-Sep-1997 (kosma01)
##		Have rs4_us5 select /usr/sbin/ping for its PINGCMD.
##      24-sep-1997/14-feb-1997 (mosjo01/linke01)
##              Set PINGCMD=/usr/ucb/ping for dgi_us5.
##	16-feb-1998 (toumi01)
##		Added support for Linux (lnx_us5).
##	12-mar-1998 (walro03)
##		Set PSCMD to "/bin/ps -ef" for ICL DRS (dr6.us5).
##  03-Mar-1998 (merja01)
##		Digital has changed the display format for ipcs between 
##		different versions of Digital UNIX 4.0.  I made a change to
##		check the format and set IPCSCMD appropriately.  This fixes
##		star issue 6060149
##	26-Mar-1998 (muhpa01)
##		Added settings for new HP config, hpb_us5, to mimic current
##		hp8_us5 settings
##	14-may-1998 (toumi01)
##		Some GNU versions of expr object to leading ^ in : so
##		conditionally leave it out for Linux.
##		Some GNU versions of ps object to arguments with "-", so
##		add lnx_us5 to platforms using "ps auxw".
##	02-jul-1998 (toumi01)
##		Conditional change for Linux to echo ECHO_CHAR as "\\c"
##		instead of as "\\\\c".
##	11-jun-1998 (somsa01)
##		On Digital UNIX 4.0D, the hex output from ipcs will not
##		contain leading zeros if it needs it. Therefore, set the
##		hex version of IPCSCMD to add leading zeros (if necessary).
##		(Bug #91383)
##	08-Jul-1998 (kosma01)
##		Supply the pstat configuration information for sgi_us5
##	18-sep-1998 (walro03)
##		For rs4_us5:
##		(1) Force WHOAMI to be /usr/bin/whoami.
##		(2) The lsps command is in /usr/sbin.
##	30-sep-1998 (toumi01)
##		For lnx_us5 set LSSIZEFLAGS="-l --no-group" for iifilsiz.
##	31-mar-1999 (natjo01)
##		Fix the case statement for sed_major_minor, was using
##		the default case on hpb.us5 platforms. (b96131)
##	01-Apr-1999 (kosma01)
##		on sig_us5 /etc/swap -s can gives units of k(kilo), m (mega)
##		or g(giga) bytes, depending on how much swap space a machine
##		has. To be more generic use /etc/swap -s -b which always gives
##		the measure in units of 512 byte blocks.  
##	20-Sep-1999 (bonro01)
##		Changes required for unix cluster support.
##	10-may-1999 (walro03)
##		Remove obsolete version string bu2_us5, bu3_us5, dr6_ues,
##		dr6_uv1, dra_us5, ds3_osf.
##      30-Jun-1999 (podni01)
##              Added condition for rmx_us5 to handle ps options.
##      03-jul-1999 (podni01)
##              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_u
##	06-oct-1999 (toumi01)
##		Change Linux config string from lnx_us5 to int_lnx.
##	31-dec-1999 (toumi01)
##		An earlier change reordered testing of the echo command to
##		fix an axp_osf system version problem.  But this breaks Linux,
##		since the \\c test succeeds on the Linux build machine but
##		fails on some Linux variants, while echo using -n to suppress
##		the output of the trailing newline works on all Linux systems.
##		For int_lnx only, do the echo -n test first.
##	24-apr-2000 (somsa01)
##		Added support for other product builds.
##	23-May-2000 (hweho01)
##              Added support for AIX 64-bit platform (ris_u64).
##	01-feb-2000 (marro11)
##		Modify DF for dg8_us5, dgi_us5, nc4_us5, pym_us5 and rs4_us5,
##		from "df" to "df -k" for Kbyte value.  Corresponding change 
##		to iidsfree has also been made.
##	15-Jun-2000 (hanje04)
##		Added support for ibm_lnx to mimic int_lnx behavior
##	04-Apr-2001 (mosjo01)
##		Added sqs_ptx to platforms without 'l' for AR_L_OPT value.
##		Besides I was getting an annoying warning in mergelibs.
##	06-Feb-2001 (hanje04)
##		As part of fix for SIR 103876 added USE_SHARED_SVR_LIBS
##		and LD_RUN_PATH to iisysdep when use_shared_svr_libs is 
##		defined in shlibinfo.
##	09-Feb-2001 (hanje04)
##		Added support for Alpha Linux (axp_lnx)
##	17-Apr-2001 (hweho01)
##              Removed LANG=C for AIX platforms, it affects the 
##              bourne-shell ability from one shell script to source  
##              another shell script on AIX rel 4.3.3.
##	21-jun-2001 (devjo01)
##		Remove DG clust support, add Tru64.
##	18-jul-2001 (stephenb)
##		From allmi01 original, add support for i64_aix
##	05-sep-2001 (somsa01)
##		On HP-UX 11i, 'ps -auxw' actually means something, but is
##		not the right ps format we need. Corrected conditions.
##	11-dec-2001 (somsa01)
##		Porting changes for hp2_us5.
##	02-May-2002 (hanje04)
##	        Added -e to ECHO_FLAG for Linux so that it will interpret
##		backslash-escaped characters (e.g. \n for new line)
##	08-May-2002 (hanje04)
##	 	Added support for Itanium Linux (i64_lnx)
##	09-Sep-2002 (hanch04)
##		Removed . from BPATHSED.  It was cutting out the first
##		character of the directory name.  Bug 44388
##	17-Oct-2002 (bonro01)
##		Corrected 'df' command parameters for sgi_us5.
##	01-Nov-2002 (hanje04)
##		Added support for AMD x64-64 Linux - replacing individual
##		linux defns which generic LNX and *_lnx where ever possible.
##	04-Nov-2002 (somsa01)
##		Changed setting of HOSTNAME to use iipmhost.
##	26-Nov-2002 (somsa01)
##		Un-did last change. Those in need of iipmhost should use
##		CONFIG_HOST (set via iishlib).
##	02-Dec-2002(somsa01)
##		For hybrid builds, print out the appropriate header
##		information.
##	21-May-2003 (bonro01)
##		Add support for HP Itanium (i64_hpu)
##	7-Aug-2003 (bonro01)
##		Updated SED_MAJOR_MINOR for i64_hpu. Bug 110524
##	6-Jan-2004 (schka24)
##		Define an AWK variable that gives a "real" awk; on Solaris,
##		awk is still the ancient wimpy one, and nawk is what
##		every other platform thinks of as awk.
##	08-Mar-2004 (hanje04)
##		SIR 107679
##		Define read_kmem=false for Linux so that syslim doesn't
##	 	fail trying to read it.
##	09-Mar-2004 (hanje04)
##	 	SIR 111952
##		Define RCSTART and RCSTOP as prefixes for Ingres rc files.
##		Define RC1 thru RC5 to point to locations for Ingres rc scripts
##		for startup and shutdown at corresponding run levels.
##		Also define run levels to add start and stop scripts to.
##	27-mar-2004 (devjo01)
##		Make CLUSTERSUPPORT respect conf_CLUSTER_BUILD.
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##      20-Aug-2004 (hanal04) Bug 112845 INGINS252
##              Set PINGARGS to "-c 1" for int_lnx.
##	22-Sep-2004 (hanje04)
##	    Format of Release ID has changed. Adjust awk script for obtaining
##	    platform info accordingly
##	22-Sep-2004 (hanje04)
## 	    SIR 111413
##	    Enable support for Intel Linux build running on AMD64 Linux.
##	    If running under AMD64 Linux, we need to tell the compiler to
##	    create 32bit binaries and not 64bit.
##	03-oct-2004 (somsa01)
##	    default.h has moved.
##	01-Nov-2004 (hanje04)
##	    On linux, some system libraries have a numeric suffix, make sure
##	    we find them.
##	05-nov-2004 (abbjo03)
##	    default.h was moved, again.
##	24-Nov-2004 (bonro01)
##	    Discovered a bug on Solaris where the ECHO_CHAR='\\\\c' was
##	    being interpreted differently between Solaris 8 and 9
##	    Instead of fixing just Solaris, we will use /usr/bin/printf
##	    to replace echo.  /usr/bin/printf is a posix command that is
##	    now available on most platforms.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	03-Feb-2005 (hanje04)
##	   Replace archaic search method for determining link libraries
##	   with a loop which tries to link a simple program with each of
##	   the specified libraries. Drop the ones which fail.
##	18-Feb-2005 (hanje04)
##	   Move re-setting of CCMACH for Intel Linux running on AMD64.
##      04-Mar-2005 (hanje04)
##	   SIR 114034 
##         Add support for reverse hybrid builds.
##	15-Mar-2005 (hanje04)
##	   Quiet error produced by compiler not being install on Solaris
##	15-Mar-2005 (bonro01)
##	    Add support for Solaris AMD64 a64_sol
##      18-Apr-2005 (hanje04)
##          Add support for Max OS X (mac_osx).
##          Based on initial changes by utlia01 and monda07.
##          Mac OSX needs the extra w in PSCMD otherwise output is limited 
##	    to 132 characters.
##	15-May-2005 (bonro01)
##	    Solaris AMD64 a64_sol does not need to read /dev/kmem
##	24-Aug-2005 (schka24)
##	    Nor does solaris-10 SPARC.
##	27-sep-2005 (devjo01) b115278/11408758;01
##	    Reduce CLUSTER_MAX_NODE to 15 since this is the maximum
##	    legal node number, not the number of array entries.
##	29-Sep-2005 (hanje04)
##	    Change mac.osx to m5g.osx, missed it before.
##	07-Oct-2005 (bonro01)
##	    Use 'id' command on usl_us5 for WHOAMI because this port
##	    also runs on SCO OpenServer 6 machine which does not
##	    have the /usr/ucb/whoami command.
##	25-Jan-2006 (hanje04)
##	    Remove -fwritable-strings flag from CCMACH on Linux so
##	    we can support SuSE 10 and other distro's that us GCC 4.0
##	20-Feb-2006 (kschendel)
##	    iisysdep takes approximately forever on slower Solaris boxes,
##	    pruning LIBMACH, because the C compiler is slow.  Compile
##	    the test program once and link N times.  Also, the loops are
##	    wrong, as LDLIBMACH contains a possible -L prefix, followed
##	    by $LDLIB.  If we're going to do the check we should do it right.
##	    Ideally, we should cache the answer from this and other tests, but
##	    this change brings the time down to something tolerable.
##      23-May-2006 (liuli01)
##          Bug 116131 change pstat for handling foreign characters
##	9-Sep-2006 (bonro01)
##	    Simplify genrelid so that is does not require -lpHB parm.
##	11-Sep-2006 (wanfr01)
##	    Bug 116844
##	    Add support for su9.us5 and r64.us5
##	10-Sep-2006(bonro01)
##	    Fix syntax error introduced by last change.
##	17-Oct-2006(bonro01)
##	    Fix error in change 483828.
##	17-Oct-2006 (wanfr01)
##	    Bug 116844
##	    Rewrite of fix - rather than add 64 bit versions, change
##	    iisys to use the 32 bit string instead.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##      20-Nov-2006 (hanal04) SIR 117044
##          Correct logical OR syntax introduced by int.rpl changes.
##      25-Nov-2006 (hanal04) SIR 117044
##          Correct int_rpl that should have been int.rpl
##	23-Mar-2007
##	    SIR 117985
##	    Add support for 64bit PowerPC Linux (ppc_lnx)
##      26-Apr-2007 (hweho01)
##          Fixed the PSTAT command for AIX.
##      28-Apr-2007 (hweho01)
##          Defined fullpath name for spatial object library for AIX and
##          Solaris in iilink.
##      04-May-2007 (hweho01)
##          Add SPAT_LIB_IS_STATIC_ARCHIVE and UDT_LIB_IS_STATIC_ARCHIVE
##          flags. They are needed for linking the spatial objects and 
##          UDT objects as static archive files with iimerge in iilink. 
##	4-Jun-2007 (bonro01)
##	    Defined fullpath name for spatial object library for HP
##	11-Jun-2007 (bonro01)
##	    Fix for Linux reverse Hybrid builds.
##      22-Jun-2007 (hweho01)
##          Defined fullpath name for spatial object library for 
##          AMD Solaris platform.
##      06-Sep-2007 (smeke01) b118879
##          Changed LD_RUN_PATH to include /lib & /usr/lib before 
##          $II_SYSTEM/ingres/lib.
##      12-Sep-2007 (hweho01)
##          Added a switch to the library test, so caller can skip
##          the checking if it is not needed.
##      8-Oct-2007 (hanal04) Bug 119262
##          Added archive iimerge.a alternative to shared library server.
##      5-Nov-2007 (hweho01) 
##          Modified the pstat command for a64_sol, not to use letters   
##          in the pattern list, avoid error in other locales such as     
##          Japanese. 
##	16-Jan-2008 (kschendel)
##	    Ignore #if / #endif lines in default.h.
##	08-Feb-2008 (hanje04)
##	    SIR S119978
##	    Add support for Intel OSX
##	22-Feb-2008 (bonro01)
##	    Correct default RC run levels.
##	    Fix RCSTOP level number which should be complementary so that
##	    Ingres is one of the first services stopped. And one of the
##	    last to be started.  Change RCSTART to be 90 so that it is
##	    possible to define services that start after Ingres.
##      24-Mar-2008 (hweho01)
##          Modified the pstat command for Solaris (a64_sol and su9_us5),
##          so the swap space size can be retrieved correctly when the
##          locale is set to Chinese.
##	25-Mar-2008 (bonro01)
##	    All platforms that use Spatial shared libraries should
##	    specify the full path on the link because SETUID programs
##	    can't use LD_LIBRARY_PATH.
##	19-Jun-2009 (kschendel) SIR 122138
##	    Make sure that build_arch is emitted to iisysdep.
##	    Reflect changes to hybrid scheme here.
##	    Remove special linux processing from default.h, we don't use
##	    -fwritable-strings any more.
##	07-Dec-2009 (frima01) SIR 122138
##	    Removed setting of SLSFX since it relied on shlibinfo which
##	    isn't included anymore.
##		
##
#-------------------------------------------------------------------------

. readvers
vers=$config
vers_bits=$config
if [ -n "$build_arch" ] ; then
    vers_bits="$vers_bits ($build_arch)"
fi

iisys=`echo $config | tr "_" "."`
isys_name=`genrelid INGRES`

# Determine whether a private copy of the tools is being built
[ -n "$PRIVATE_TOOLS" ] && privtools="/$ING_VERS"

PATH=$II_SYSTEM/ingres/utility:$II_SYSTEM/ingres/bin:/bin:/usr/bin:/usr/ucb:/etc:$ING_SRC$privtools/bin

sysdep_name=iisysdep
sysdep=$ING_BUILD/utility/$sysdep_name

date=`date`

#-------------------------------------------------------------------------
#  Create sysdep file and set stdout to it...

exec > $sysdep
trap 'rm -f $sysdep' 0 

#-------------------------------------------------------------------------

cat << !
#!/bin/sh
#
# INGRES 6.1+ $sysdep_name
# created on $date
# by the mksysdep shell script
#
# Configuration: $vers_bits
# Release ID   : $isys_name
#
!


#-------------------------------------------------------------------------

echo ""
echo "#  Set WHOAMI to the current logon:"

case $vers in
	rs4_us5 | r64_us5)
		echo 'WHOAMI=`/usr/bin/whoami`'
		;;
	usl_us5)
		echo 'WHOAMI=`IFS="()"; set - \`id\`; echo $2`'
		;;
	*)
		if [ -f /usr/ucb/whoami ] 
		then
			echo 'WHOAMI=`/usr/ucb/whoami`'
		else
			if [ -f /usr/bin/whoami ]
			then
				echo 'WHOAMI=`/usr/bin/whoami`'
			else
				echo 'WHOAMI=`IFS="()"; set - \`id\`; echo $2`'
			fi
		fi
		;;
esac

echo export WHOAMI



#-------------------------------------------------------------------------
case $vers in
  ds3_ulx)
      echo BPATHSED="'s/.*\/.//'"
      ;;
  hp3_us5 | hp8_us5 |  hp8_bls | hpb_us5 | hp2_us5 | i64_hpu)
    echo BPATHSED="'s/.*\///'"
    ;;
  ris_us5 | rs4_us5 | r64_us5)
    cat << !
BPATHSED='s/r//'
DFCMD="exec df -M"
!
      ;;
  dg8_us5 | dgi_us5)
    echo BPATHSED='"s/.*\/.//"'
    ;;
  *)
    echo BPATHSED='s,.*/,,'
	echo 'DFCMD="exec df"'
    ;;
esac

echo export BPATHSED
echo export DFCMD

#-------------------------------------------------------------------------

echo ""
echo "#  Set up variables for the echon script"

ECHO_CMD=""
for ECHO in /usr/bin/printf /bin/echo /usr/ucb/echo echo
do
	if [ "$ECHO" != "echo" -a ! -f $ECHO ]
	then
		continue
	fi
	if [ "$ECHO" = "/usr/bin/printf" ]
	then
		ECHO_CMD=$ECHO
		echo ECHO_CMD=$ECHO
		echo ECHO_FLAG=
		echo ECHO_CHAR=
		break
	fi
	if [ "$vers" = "int_lnx" ] || [ "$vers" = "ibm_lnx" ] || \
	   [ "$vers" = "axp_lnx" ] || [ "$vers" = "i64_lnx" ] || \
	   [ "$vers" = "a64_lnx" ] || [ "$vers" = "mg5_osx" ] || \
           [ "$vers" = "int_rpl" ] || [ "$vers" = "ppc_lnx" ] || \
	   [ "$vers" = "int_osx" ]
	then
		if [ "`$ECHO -n hi`" = "hi" ]
		then
			ECHO_CMD=$ECHO
			echo ECHO_CMD=$ECHO
			echo ECHO_FLAG=-n
			echo ECHO_CHAR=
			break
		fi
	fi
	if [ `$ECHO "hi\c"` = "hi" ]
	then
		ECHO_CMD=$ECHO
		echo ECHO_CMD=$ECHO
		echo ECHO_FLAG=
		if [ "$vers" = "int_lnx" ] || [ "$vers" = "ibm_lnx" ] || \
		   [ "$vers" = "axp_lnx" ] || [ "$vers" = "i64_lnx" ] || \
	       	   [ "$vers" = "a64_lnx" ] || [ "$vers" = "int_rpl" ] || \
		   [ "$vers" = "ppc_lnx" ]
		then
			echo ECHO_CHAR="\\\\c"
		else
			echo ECHO_CHAR='\\\\c'
		fi
		break
	elif [ "`$ECHO -n hi`" = "hi" ]
	then
		ECHO_CMD=$ECHO
                echo ECHO_CMD=$ECHO
		echo ECHO_FLAG=-n
		echo ECHO_CHAR=
		break
	fi
			
done
unset ECHO
if [ "$ECHO_CMD" = "" ]
then
	echo ECHO_CMD=echo
	echo ECHO_FLAG=
	echo ECHO_CHAR=
fi

echo export ECHO_CMD ECHO_FLAG ECHO_CHAR


#-------------------------------------------------------------------------

echo ""
echo "#  Define the form of the ps command:"

if [ "$vers" = "axp_osf" ] || [ "$vers" = "int_lnx" ] || \
   [ "$vers" = "ibm_lnx" ] || [ "$vers" = "axp_lnx" ] || \
   [ "$vers" = "i64_lnx" ] || [ "$vers" = "a64_lnx" ] || \
   [ "$vers" = "int_rpl" ] || [ "$vers" = "ppc_lnx" ]
then
    echo PSCMD=\"ps auxw\"
elif [ "$vers" = "rs4_us5" ] || [ "$vers" = "r64_us5" ]
then
    echo PSCMD=\"env COLUMNS=132 ps -ef\"
elif [ "$vers" = "su4_us5" -o "$vers" = "sui_us5" -o "$vers" = "su9_us5" -o "$vers" = "dr6_us5" -o "$vers" = "rmx_us5"  -o "$vers" = "rux_us5" -o "$vers" = "hpb_us5" -o "$vers" = "hp2_us5" -o "$vers" = "i64_hpu" -o "$vers" = "a64_sol" ]
then
    echo PSCMD=\"/bin/ps -ef\"
elif [ "$vers" = "mg5_osx" ] || [ "$vers" = "int_osx" ] ; then
# Need ww to stop truncation
    echo PSCMD=\"ps auxww\"
elif (ps -xauw) > /dev/null 2>&1
then
    echo PSCMD=\"ps -auxw\"
else
    echo PSCMD=\"ps -ef\"
fi
echo export PSCMD


#-------------------------------------------------------------------------

echo ""
echo "#  Define the form of the ipcs and ipcrm commands:"

if (ipcs) > /dev/null 2>&1
then
## On axp_osf some versions display key values a decimal
## and others display as hex.  Following logic checks for
## axp_osf and if true, check if displays with hex or dec
## and set IPCSMD accordingly.  Those that don't display with
## hex will be formatted via awk
if [ "$vers" = "axp_osf" ]
then
 echo "if [ \`ipcs | awk '{ printf\"%d\", match(\$3, \"0x\") }'\` -gt 0 ]"
 echo " then "
 echo "IPCSCMD=\"eval ipcs | awk '{
	if(length(\\\$3)>0) {
		for(i_key=substr(\\\$3, 3, 8); length(i_key)<8; i_key=sprintf(\\\"0%s\\\", i_key)){};
		printf\\\"%s %s 0x%s %s %s %s\\\\\\\n\\\", \\\$1, \\\$2, i_key, \\\$4, \\\$5, \\\$6 }
	else
		printf\\\"%s %s %s %s %s %s\\\\\\\n\\\", \\\$1, \\\$2, \\\$3, \\\$4, \\\$5, \\\$6 }' \""
 echo "else"
 echo "## ipcs needs to be formated to hex"
 echo "IPCSCMD=\"eval ipcs | awk '
/^[qms]/ {
	printf \\\"%s %s 0x%.8x %s %s %s\\\\\\\n\\\", \\\$1, \\\$2, \\\$3, \\\$4, \\\$5, \\\$6
}
! /^[qms]/ {
	print
}'\""
 echo " fi"
else
    echo IPCSCMD=ipcs
fi
    echo IPCRM=ipcrm
    echo IPCSCMD2=ipcs
else
    echo IPCSCMD=\"att ipcs\"
    echo IPCSCMD2=\"att ipcs\"
    echo IPCRM=\"att ipcrm\"
fi
echo export IPCSCMD IPCRM IPCSCMD2


#-------------------------------------------------------------------------

echo ""
echo "#  Define the local of the ping cmd:"
 
case $vers in
    su4_us5|\
    su9_us5|\
    rs4_us5|\
    r64_us5|\
    pym_us5|\
    rmx_us5|\
    ts2_us5|\
    rux_us5|\
    ris_u64|\
    a64_sol|\
    i64_hpu)
        echo PINGCMD=/usr/sbin/ping
        ;;
    dg8_us5)
        echo PINGCMD=/usr/bin/ping
        ;;
    dgi_us5)
        echo PINGCMD=/usr/ucb/ping
        ;;
    hp8_us5|sqs_ptx|hpb_us5|hp2_us5)
        echo PINGCMD=/etc/ping
        ;;
    axp_osf)
        echo PINGCMD=\"/usr/sbin/ping -c 1\"
        ;;
    sco_us5|sos_us5)
        echo PINGCMD=\"/usr/bin/ping -c 1\"
        ;;
	sgi_us5)
        echo PINGCMD=\"/usr/etc/ping -c 1\"
        ;;
    *_lnx|int_rpl)
        echo PINGCMD=/bin/ping
        ;;
      *_osx)
        echo PINGCMD=/sbin/ping
        ;;
    *)
        echo PINGCMD=/usr/etc/ping
        ;;
esac

case $vers in
        *_lnx|int_rpl)
	        echo PINGARGS=\"\-c 1\"
		;;
	hp8_us5|ris_us5|nc4_us5|hpb_us5|hp2_us5|i64_hpu)
		echo PINGARGS=\"64 1\"
		;;
	*)
		echo PINGARGS=" "
		;;
esac

echo export PINGCMD PINGARGS

#-------------------------------------------------------------------------

echo ""
echo "#  Define the form of the ln cmd:"

touch /tmp/tmp1.$$
if ln -s /tmp/tmp1.$$ /tmp/tmp2.$$ > /dev/null 2>&1
then
    echo LINKCMD=\"ln -s\"
else
    echo LINKCMD=\"ln\"
fi
rm -f /tmp/tmp[12].$$
echo export LINKCMD


#-------------------------------------------------------------------------

echo ""
echo "#  Set a variable to the system's host name:"

case $vers in
    su4_u42|su4_cmw)
	echo "HOSTNAME=\`hostname\`"
	echo SU_FLAGS=-cf
	;;
    *)
	if (uname) > /dev/null 2>&1
	then
	    echo "HOSTNAME=\`uname -n\`"
	elif (hostname) > /dev/null 2>&1
	then
	    echo "HOSTNAME=\`hostname\`"
	else
	    echo "HOSTNAME=\`uuname -l\`"
	fi
	echo SU_FLAGS=-c
	;;
esac

echo export HOSTNAME

echo export SU_FLAGS

#-------------------------------------------------------------------------

echo ""
echo "#  Define commands to extract various fields from ls -l"

touch /tmp/mksysdepls.$$
if [ `ls -l /tmp/mksysdepls.$$ | awk '{ print $5 }' | egrep "^[a-zA-Z]*$" | wc -l` -eq 1 ]
then
	ownerfield=3
	bytefield=4
else 
	ownerfield=3
	bytefield=5
fi
rm -f /tmp/mksysdepls.$$

echo "AWKBYTES=\"awk '{ print \\\$$bytefield }'\""
echo "AWKOWNER=\"awk '{ print \\\$$ownerfield }'\""
echo "AWKTOTAL=\"awk '{s += \\\$$bytefield } END {print (s * 3) / 1024}'\""
echo export AWKBYTES AWKOWNER AWKTOTAL



#-------------------------------------------------------------------------

echo ""
echo "#  Number of bytes in a single disk block:"

TMP1=/tmp/bpb1$$
TMP2=/tmp/bpb2$$
## create a file of 64 bytes
echo "123456781234567812345678123456781234567812345678123456781234567" > $TMP1
## create a file 8 times as large; 512 bytes
cat $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 > $TMP2
rm -f $TMP1
mv $TMP2 $TMP1
## create a file 8 times as large; 4096 bytes
cat $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 $TMP1 > $TMP2
## determine how may blocks the 4096 byte file occupies
## NB the braces contain a space, followed by a single tab character
sz=`du -s $TMP2 | sed "s/[ 	].*//"` 
echo BTPERBLK=`echo "4096/$sz" | bc`
echo export BTPERBLK
rm -f $TMP1 $TMP2


#-------------------------------------------------------------------------

if [ "$vers" = "ris_u64" ] || [ "$vers" = "i64_aix" ]
then
        echo ""
        echo "# Defined the 64 bit object mode for AIX "
	echo "OBJECT_MODE=64"
	echo export OBJECT_MODE
fi

#-------------------------------------------------------------------------

echo ""
echo "#  Parameters from default.h (generated by mkdefault):"

sed  -e 's/ *=/=/' -e 's/= */=/' \
	    -e '/^# *if/d' \
	    -e '/^# *endif/d' \
	    -e '/^#/s/^.*[^un]define[ 	]*\([^ 	]*\)[ 	]*\(.*\)$/\1=\2/' \
	    -e 's/=\(.* [^ ].*\)/=\"\1\"/' \
            $ING_SRC/tools/port/$ING_VERS/hdr_unix_vms/default.h | \
	    fgrep -v \
	    'XTERMLIB
	    XNETLIB
	    WINSYSLIB'
 
echo "build_arch=$build_arch"

case $vers in
    ris_us5)
      echo LINKTESTFLAG='-L'
      ;;
    axp_osf)
	if uname -a | grep "V3\.0" > /dev/null 2>&1
	then
		echo LINKTESTFLAG='-f'
	else
		echo LINKTESTFLAG='-h'
	fi
      ;;
    *)
      echo LINKTESTFLAG='-h'
      ;;
esac

cat << EOF

if \${LibTest_in_iisysdep:-true} ; then
#  Remove references to libraries that aren't present on this machine:
# Create dummy C program to link to
LIBTST_C=/tmp/libtst\$\$.c
LIBTST_EXE=/tmp/libtst\$\$
LIBTST_O=/tmp/libtst\$\$.o
cat << LIBTST >  \${LIBTST_C}
int
main( int argc, char **argv )
{
    exit(0);
}
LIBTST
EOF

cat << EOF
if ( \$CC \$CCMACH -c -o \$LIBTST_O \$LIBTST_C ) > /dev/null 2>&1
then
  sedcmd=
## LDLIB and LIBMACH first, as they may well have libs in common,
## and neither is expected to have any -L prefix.
  for word in \`echo \$LDLIB \$LIBMACH | tr ' '  '\\012' | sort | uniq\`
  do
EOF
case $vers in
    *_lnx|int_rpl|*_osx)
      echo "    lib=\`expr \$word : '-l\(.*\)\$'\`"
      ;;
    *)
      echo "    lib=\`expr \$word : '^-l\(.*\)\$'\`"
      ;;
esac

cat << EOF

    if [ -n "\$lib" ]
    then
	 sedtail=" -e 's/\$word//'"
	 if ( \$CC \$CCMACH -o \$LIBTST_EXE \$LIBTST_O -l\${lib} ) > /dev/null 2>&1
	 then
	    rm -f \$LIBTST_EXE
	 else
	    sedcmd="\$sedcmd\$sedtail"
	 fi
    fi
  done
  if [ -n "\$sedcmd" ]
  then
    LDLIB=\`echo \$LDLIB | eval "sed \$sedcmd"\`
    LIBMACH=\`echo \$LIBMACH | eval "sed \$sedcmd"\`
  fi
  sedcmd=
## Now for LDLIBMACH, but only if it has any -L prefix, because
## otherwise it's always the same as LDLIBS and LIBMACH concatenated.
## Remove the -L's first.
  lpath=
  for word in \$LDLIBMACH
  do
EOF
case $vers in
    *_lnx|int_rpl|*_osx)
      echo "    lpathone=\`expr \$word : '-L\(.*\)\$'\`"
      ;;
    *)
      echo "    lpathone=\`expr \$word : '^-L\(.*\)\$'\`"
      ;;
esac

cat << EOF

    if [ -n "\$lpathone" ]
    then
	 lpath="\$lpath"-L"\$lpathone "
    fi
  done
  if [ -n "\$lpath" ]
  then
    for word in \$LDLIBMACH
    do
EOF
case $vers in
    *_lnx|int_rpl|*_osx)
      echo "      lib=\`expr \$word : '-l\(.*\)\$'\`"
      ;;
    *)
      echo "      lib=\`expr \$word : '^-l\(.*\)\$'\`"
      ;;
esac

cat << EOF
      if [ -n "\$lib" ]
      then
	 sedtail=" -e 's/\$word//'"
	 if ( \$CC \$CCMACH -o \$LIBTST_EXE \$LIBTST_O \$lpath -l\${lib} ) > /dev/null 2>&1
	 then
	    rm -f \$LIBTST_EXE
	 else
	    sedcmd="\$sedcmd\$sedtail"
	 fi
      fi
    done
    if [ -n "\$sedcmd" ]
    then
      LDLIBMACH=\`echo \$LDLIBMACH | eval "sed \$sedcmd"\`
    fi
  fi
fi
rm -f \$LIBTST_EXE \$LIBTST_C \$LIBTST_O
fi
EOF


#-------------------------------------------------------------------------
#  Shared library info.
#

# As of 2009, all unix ports use shared libraries.  Don't bother
# with the test in shlibinfo.

echo USE_SHARED_LIBS=true


#
# Build iimerge from shared libraries ?
#

if [ -n "$conf_SVR_SHL" ] ; then
    echo USE_SHARED_SVR_LIBS=true
    ## Shared-libs server needs to use $ORIGIN else setuid problems.
    ## echo 'LD_RUN_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/bin'
else
    echo USE_SHARED_SVR_LIBS=false
fi

#
# Build iimerge from archive library ?
#

if [ -n "$conf_SVR_AR" ] ; then
    echo USE_ARCHIVE_SVR_LIB=true
else
    echo USE_ARCHIVE_SVR_LIB=false
fi

echo export USE_SHARED_LIBS USE_SHARED_SVR_LIBS SLSFX LD_RUN_PATH

#-------------------------------------------------------------------------

echo ""
echo "#---"
echo "#--- Everything past this point is explicitly based on the config string"
echo "#---"

#  Set up default values first, so the cases below only have to change
#  them if they're different...

etcrcfiles=/etc/rc
rctmplt=
rc1=
rc2=
rc3=
rc4=
rc5=
rcdeflnk=
rcstart=S90
rcstop=K10
check_memory_resources=true
output_sink='"> /dev/null"'
use_symbolic_links=false
working_awk='awk'

#  Parsing pstat: the pstat command is used by the syscheck script to
#  figure out whether there's enough swap space on the system to run 
#  Ingres.  The output from pstat seems to vary wildly from system to
#  system, but apparently it always reports the amount available,
#  and either the amount used or the total amount allocated, or both.

check_swap_space=true	# Should we do the check at all?
pstat="/etc/pstat -s"	# The command to use
pstat_used_arg=7 	# Arg number after set command: amount used...
pstat_remain_arg=9 	# ...amount remaining, or available...
pstat_total_arg=	# ...total swap space.  Null if this isn't returned.
pstat_kbyte_factor=1	# Multiplier to convert the numbers to kbytes
			# (Not to be confused with the number of bytes per
			#  disk block -- we're talking memory here.)
acpart_not_allowed=true # Can raw log be put on a or c partition ?
bsd_df=df		# set df to BSD df style
check_c2=false          # check if C2 security
read_kmem=true          # Does rcheck read /dev/kmem ? default true.
			# if set to false, syscheck will not complain if 
			# /dev/kmem is not readable.


case $iisys in
        ds3.osf|axp.osf)
                check_swap_space=false
		etcrcfiles=/sbin/rc3.d
                pstat=
                pstat_used_arg=
                pstat_remain_arg=
                pstat_total_arg=
                pstat_kbyte_factor=
                bsd_df="df -k"
                ;;
        ds3*|vax.ulx)
                pstat_used_arg=3
                pstat_remain_arg=6
                pstat_total_arg=1
		pstat="$pstat | sed 's/[^0-9 ]//g'"	# The command to use
		check_c2=true
                ;;
	sgi.us5)
		pstat="/etc/swap -s -b "
		pstat_used_arg=9
		pstat_remain_arg=12
		pstat_total_arg=
		pstat_kbyte_factor="'1 / 2'"
                bsd_df="df -Pk"
		;;
        sqs.ptx)
                etcrcfiles=/etc/rc2.d
                check_memory_resources=false
                pstat="/etc/swap -f | grep free"
## /etc/swap -f gives info "25952 used, 382368 free, 4664 wasted" in 512b blocks
## plus another line in which we have no interest
                pstat_used_arg=1
                pstat_remain_arg=3
                pstat_kbyte_factor="'1 / 2'"
                ;;
	sqs*)
		etcrcfiles=/etc/rc.sys5
		check_memory_resources=false
		pstat_used_arg=1
		pstat_remain_arg=3
		;;
        pym.us5|rmx.us5|ts2.us5|rux.us5)
# Unfortunately, sh doesn't deal well with args over $9, so we have to trim
# off the first part of swap's output first.
# /usr/sbin/swap -s: gives total figure, in 512b blocks.
# total: 79248 allocated + 37944 reserved = 117192 blocks used, 159824 blocks available
                etcrcfiles=/etc/rc2.d/
                pstat="/usr/sbin/swap -s | cut -d= -f2-"
                pstat_used_arg=1
                pstat_remain_arg=4
		pstat_kbyte_factor="'1 / 2'"
                [ `expr "$iisys" : "pym.us5"` -ne 0 ] && bsd_df="df -k"
                ;;
	pyr*)
		output_sink=
		pstat_total_arg=1
		pstat_remain_arg=5
		pstat_used_arg=
		;;
        hp8.bls)
                check_memory_resources=false
                check_swap_space=false
                acpart_not_allowed=false
                bsd_df=bdf
                read_kmem=false
                ;;
	hp3*|hp8*|hpb*|hp2*|i64.hpu)
		check_swap_space=false
		acpart_not_allowed=false
		bsd_df=bdf
		;;
	dr5*)
		pstat="df /dev/swap | grep swap"
		pstat_total_arg=3
		pstat_remain_arg=3
		;;
	odt*|sco*|sos.us5)
		acpart_not_allowed=false
		pstat="/etc/swap -l | sed '1d' |
			awk '{ blks += \\\$4 ; free += \\\$5 } END
			{ printf \\\"%d %d\\\n\\\",blks,free }'"
		pstat_used_arg=
		pstat_remain_arg=2
		pstat_total_arg=1
		pstat_kbyte_factor="'1 / 2'"
		;;
	nc4*)
		pstat="/etc/swap -l | sed '1d' |
			awk '{ blks += \\\$4 ; free += \\\$5 } END
			{ printf \\\"%d %d\\\n\\\",blks,free }'"
		pstat_used_arg=
		pstat_remain_arg=2
		pstat_total_arg=1
		pstat_kbyte_factor="'1 / 2'"
                bsd_df="df -k"
		;;
	su4.us5*|sui.us5*|su9.us5*|a64.sol)
		etcrcfiles=/etc/rc2.d
		pstat="LC_ALL=C;export LC_ALL;/usr/sbin/swap -s|tr -cd \\\"[0-9]/+/-/,/ /=/:/\\\""
		pstat_used_arg=6
		pstat_remain_arg=8
		pstat_total_arg=
		pstat_kbyte_factor=1
                bsd_df="df -k"
		working_awk='nawk'
		relver=`/bin/uname -r`
		if [ `/usr/bin/expr $relver : '5\.\(.*\)'` -ge 10 ] ; then
		    read_kmem=false
		fi
		;;
	ris.us5*|rs4.us5*|ris.u64*|r64.us5*)
		acpart_not_allowed=false
		pstat="/usr/sbin/lsps -a | awk ' BEGIN { TOT = 0; USED = 0}; FNR > 1 { TOT = TOT + \\\$4; USED = USED + \\\$4*\\\$5/100 }; END { REM = TOT - USED; print int(TOT) \\\" \\\" int(USED) \\\" \\\" int(REM)  } '"
		pstat_total_arg=1
		pstat_used_arg=2
		pstat_remain_arg=3
		pstat_kbyte_factor=1024		# = 1 Mbyte
		read_kmem=false
                [ `expr "$iisys" : "rs4.us5"` -ne 0 ] && bsd_df="df -k"
                [ `expr "$iisys" : "r64.us5"` -ne 0 ] && bsd_df="df -k"
		;;
	dr6*|dra*|usl.us5)
		etcrcfiles=/etc/rc2.d
		pstat="swap -s|tr -d \\\"[a-z]\\\""
		pstat_used_arg=6
		pstat_remain_arg=8
		pstat_total_arg=
		pstat_kbyte_factor="'1 / 2'"
		;;
        dg8*|dgi*)
                check_swap_space=false
                use_symbolic_links=true
                bsd_df="df -k"
                ;;
	*lnx|int.rpl)
		check_swap_space=false
		read_kmem=false
		etcrcfiles=/etc/init.d
		rctmplt=lnx.rc
		rc1=/etc/rc.d/rc1.d
		rc2=/etc/rc.d/rc2.d
		rc3=/etc/rc.d/rc3.d
		rc4=/etc/rc.d/rc4.d
		rc5=/etc/rc.d/rc5.d
		rcdeflnk='"2 3 5"'
		rcstart=S90
		rcstop=K10
		;;
	*osx)
		check_swap_space=false
		read_kmem=false
		;;
		
esac

# Establish command to use for finding major and minor numbers of a device
case $iisys in
hp3*|hp8*|hpb*|hp2*|i64.hpu)
        sed_major_minor="'s/^.* \([0-9][0-9]*\) *\(0x[0-9a-f]*\) .*$/\1 \2/'"
        ;;
*)
        sed_major_minor="'s/^.* \([0-9]*\), *\([0-9]*\) .*$/\1 \2/'"
        ;;
esac

echo AWK=$working_awk
echo ETCRCFILES=$etcrcfiles
echo RCTMPLT=$rctmplt
echo RC1=$rc1
echo RC2=$rc2
echo RC3=$rc3
echo RC4=$rc4
echo RC5=$rc5
echo RCDEFLNK="$rcdeflnk"
echo RCSTART=$rcstart
echo RCSTOP=$rcstop
echo CHECK_MEMORY_RESOURCES=$check_memory_resources
echo OUTPUT_SINK=$output_sink
echo ACPART_NOT_ALLOWED=$acpart_not_allowed
echo DF=\"$bsd_df\"
echo export ETCRCFILES CHECK_MEMORY_RESOURCES OUTPUT_SINK ACPART_NOT_ALLOWED DF
echo " "
echo CHECK_SWAP_SPACE=$check_swap_space
echo USE_SYMBOLIC_LINKS=$use_symbolic_links
echo SED_MAJOR_MINOR=$sed_major_minor
echo export CHECK_SWAP_SPACE SED_MAJOR_MINOR
if $check_swap_space
then
	echo " "
	echo "#  Variables used for parsing the output from the pstat program:"
	echo PSTAT=\"$pstat\"
	echo PSTAT_USED_ARG=$pstat_used_arg
	echo PSTAT_REMAIN_ARG=$pstat_remain_arg
	echo PSTAT_TOTAL_ARG=$pstat_total_arg
	echo PSTAT_KBYTE_FACTOR=$pstat_kbyte_factor
	echo export PSTAT PSTAT_USED_ARG PSTAT_REMAIN_ARG 
	echo export PSTAT_TOTAL_ARG PSTAT_KBYTE_FACTOR
fi
if $check_c2
then
    cat << !
[ \`awk '/SECLEVEL/ {print \$1}' /etc/svc.conf | grep BSD\` ] \\
      && CHECK_C2=false || CHECK_C2=true
!
else
    echo CHECK_C2=true
fi

#-------------------------------------------------------------------------
# These are used to assign security labels and privileges on machines
# supporting this functionality
#
case $vers in
    su4_cmw)
      echo SET_B1_PRIV=true
      echo SETLABEL="setlabel"
      echo 'ADDPRIV_SERVER="/usr/etc/chpriv af=file_downgrade_il,file_downgrade_sl,file_mac_read,file_mac_write,net_mac_override,net_setil,sys_trans_label,proc_tranquil,proc_nofloat"'
      echo 'ADDPRIV_MAINT="/usr/etc/chpriv af=file_downgrade_il,file_downgrade_sl,file_mac_read,file_mac_write,file_setpriv,file_upgrade_il,file_upgrade_sl,net_mac_override,net_setil,sys_trans_label,proc_tranquil,proc_nofloat"'
      echo 'ADDPRIV_CLIENT="/usr/etc/chpriv af=sys_trans_label,proc_nofloat"'
      echo 'ADDPRIV_SLUTIL="/usr/etc/chpriv a+proc_setil,proc_setsl,file_mac_read,file_mac_write"'
      echo 'ADDPRIV_KMEM="/usr/etc/chpriv af+file_dac_read"'
      echo export SET_B1_PRIV SETLABEL
      echo export ADDPRIV_SERVER ADDPRIV_MAINT ADDPRIV_CLIENT ADDPRIV_SLUTIL ADDPRIV_KMEM
      ;;
    hp8_bls)
      echo SETLABEL="chlevel"
      echo SET_B1_PRIV=false
      echo export SET_B1_PRIV SETLABEL
      echo export ADDPRIV_SERVER ADDPRIV_MAINT ADDPRIV_CLIENT ADDPRIV_KMEM ADDPRIV_SLUTIL
      ;;
    *)
      echo SET_B1_PRIV=false
      echo SETLABEL=true
      echo export SET_B1_PRIV SETLABEL
      ;;
esac

echo READ_KMEM=$read_kmem
echo export READ_KMEM

echo " "

#-------------------------------------------------------------------------
# The AR_L_OPT variable is "l" on machines that allow "ar l" 
# and null on machines that don't:
case $vers in
   sqs_ptx|\
   su4_us5|\
   su9_us5|\
   sui_us5|\
   a64_sol|\
   usl_us5)
      echo AR_L_OPT=
      ;;
   *) 
      echo AR_L_OPT=l
      ;;
esac

#-------------------------------------------------------------------------
# The LSSIZEFLAGS variable is set to force the 'ls' command to generate
# file size as $4 for parsing by 'iifilsiz.sh'.
case $vers in
   axp_osf )
      echo LSSIZEFLAGS=-o
      ;;
   *_lnx|int_rpl)
      echo LSSIZEFLAGS='"-l --no-group"'
      ;;
   *) 
      echo LSSIZEFLAGS=-lo
      ;;
esac

#-------------------------------------------------------------------------
# The LSBLOCKFLAGS variable is set to force the 'ls' command to generate
# file size in blocks as $1 for parsing by 'iifilsiz.sh'.
case $vers in
   axp_osf )
      echo LSBLOCKFLAGS=-s
      ;;
   *) 
      echo LSBLOCKFLAGS=-ls
      ;;
esac

#-------------------------------------------------------------------------
# The CLUSTERSUPPORT variable is set on machines that have OS cluster 
# support and also support Ingres running as a cluster.  This flag is checked
# in iisudbms and iimkcluster to enable prompts for cluster installation.
#
# CLUSTER_MAX_NODE s/b set to the max of 15 and the maximum number of nodes
# supported by the target platform.
#
if [ "${conf_CLUSTER_BUILD}" = "true" ]
then
case $vers in
   axp_osf )
      echo "if test -f /usr/sbin/clu_get_info && /usr/sbin/clu_get_info -q"
      echo "then"
      echo "    CLUSTERSUPPORT=true"
      echo "else"
      echo "    CLUSTERSUPPORT=false"
      echo "fi"
      echo "CLUSTER_MAX_NODE=15"
      ;;
#   dgi_us5 )
#      echo "if test -f /usr/sadm/sysadm/bin/running_with_clusters &&"
#      echo "           /usr/sadm/sysadm/bin/running_with_clusters"
#      echo "then"
#      echo "    CLUSTERSUPPORT=true"
#      echo "else"
#      echo "    CLUSTERSUPPORT=false"
#      echo "fi"
#      echo "CLUSTER_MAX_NODE=15"
#      ;;
   *) 
      echo CLUSTERSUPPORT=true
      echo "CLUSTER_MAX_NODE=15"
      ;;
esac
else
      echo CLUSTERSUPPORT=false
fi

# For Tru64 platform, the Spatial Objects and UDT Objects have to be in   
# static archive files. Because the shared libraries can't be loaded after   
# being linked with iimerge/dmfjsp/fastload (SETUID) if the user is not  
# ingres. 
case $vers in
   axp_osf )
      echo SPAT_LIB_IS_STATIC_ARCHIVE=true
      echo UDT_LIB_IS_STATIC_ARCHIVE=true
      ;;
   *)
      echo SPAT_LIB_IS_STATIC_ARCHIVE=false
      echo UDT_LIB_IS_STATIC_ARCHIVE=false
      ;;
esac

#-------------------------------------------------------------------------
chmod 755 $sysdep
trap : 0
