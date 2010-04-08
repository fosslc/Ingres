:
#*******************************************************************************
# 
# Copyright (c) 1989, 2006 Ingres Corporation
#
# Name: mkvalidpw
#
# Usage:
#       mkvalidpw
#
# Description:
# 	This shell script will be shipped with INGRES/NET to those systems
# 	using shadow password files and must be run by root.
#	This shell script will compile ingvalidpw.c and move
#	the executable to $II_SYSTEM/ingres/bin. The backend will use this
#	program to verify the user/password pairs.
#
# Exit Status:
#       0       OK
#       255     didn't set II_SYSTEM or didn't run by root
#
## History:
##	28-June-1989 (fredv)
##		written.
##	24-Jul-89 (GordonW)
##	 	"who am i" change
##	12-Mar-90 (GordonW)
##		Made it more general for all types of shadow passwords.
##	14-Mar-90 (GordonW)
##		More problems with linking within this script. What we 
##		generate within the release area ($II_SYSTEM/ingres/iipwd) is:
##			iiconfig.h
##			ingvalidpw.c
##		Which can be linked by the user as a stand-alone program.
##	09-apr-90 (seng)
##		Added libraries to be linked in with ingvalidpw for ODT.
##	24-sep-90 (vijay)
##		Added file to 6.3p from 6.2 source.
##	14-Aug-91 (szeng)
##		Taked defining LIBS out of [$ING_SRC] section so that
##		the user also can correctly compile the program.
##		Also Added libraries to be linked in for DECstation.
##	11-sep-91 (jonb)
##		Added DEST ming hint to make this end up in tools executable
##		directory rather than deliverable area.  Changed name of
##		ingvalidpw source file from ingvalidpw.c to ingvalidpw.x.
##	12-sep-91 (jonb)
##		Wrong.  This _is_ a deliverable tool, so remove the ming
##		hint that makes it not be one.
##	02-oct-91 (rudyw)
##		Add -lcrypt_d to list of libraries for odt_us5. Delete -lc_s.
##	28-jan-92 (rudyw)
##		Add $BIN to front of ingsetenv to avoid PATH problems for root.
##		Add call to system dependency file iisysdep and remove the 
##		redundant determination of WHOAMI and vers in this file.
##		Add if-then-else around the loading of executable ingvalidpw
##		based on availability of a development system. Assume that
##		platforms where no dev sys a possibility will ship ingvalidpw.
##		The change to mksysdep.sh (which generates iisysdep) must be
##		picked up at the same time as this changed file for HAS_DEVSYS.
##		Change ownership of symbol.tbl to ingres after exercising
##		ingsetenv in case this command (invoked as root) created it.
##	10-feb-1992 (jonb)
##		Chmod ingvalidpw.c after it's copied to the distribution area.
## 		This will result in seting setting it to the appropriate
##		permissions so it will be overwriteable in an upgrade (42347).
##	17-feb-92 (rudyw)
##		Shipped ingvalidpw must be delivered in same directory with
##		ingvalidpw source to avoid overwrite problems in bin with a
##		previously installed ingvalidpw that has root permissions. 
##		Change the action of copying ingvalidpw to bin to a copy/remove
##		so that the executable is gone from files/iipwd and avoid 
##		potential problems for later upgrades depositing the executable.
##		Modify a user message as per reviewers request.
##	25-mar-92 (swm)
##		Specify -lnsl -lsocket in library list for dr6_us5 and dra_us5.
##		This is now needed due to V5 operating system changes.
##      01-apr-92 (johnr)
##              Piggybacked ds3_ulx for ds3_osf.
##	02-nov-92 (jonb)
##		Don't set II_SHADOW_PWD if the ingvalidpw executable isn't
##		there or doesn't compile.  (47635)
##	31-mar-93 (vijay)
##		Clean up. Move the build tasks to buildvalidpw.sh.
##	14-oct-93 (vijay)
##		Change permissions to 4755, so that ingres can read the
##		executable into the release area. ingvalidpw is now a
##		deliverable since some libs or compiler may not be available.
##	27-Jan-1995 (canor01)
##		Make ingvalidpw install into the iipwd directory, because
##		ingbuild can't install on top of a root setuid executable
##	08-may-95 (johnst)
##		Add usl_us5 to dr6_us5 chain and update entry to pick up correct
##		libs from $LDLIBMACH in iisysdep.
##	20-Mar-1995 (smiba01)
##		Added dr6_ues to dr6_* in LIBS case.
##      1-may-95 (wadag01)
##		Aligned with 6.4/05:
##		"Checked for U.S. standard libcrypt_d.a or rest-of-world
##		 supplement library libcrypt_i.a odt_us5.
##		 Needed libx.a to build because nap is needed by libprot.a.
##		 odt_us5 must always ship the ingvalidpw executable.
##		 Turn off the HAS_DEVSYS flag."
##     12-nov-96 (merja01)
##		Allow creation of AXP.OSF version.  Requires -lsecurity in order
##		for ingvalidpw to link.
##     24-Apr-97 (merja01)
##      Digital Unix 4.0 requires two include libs for invalidpw.  I have
##      added an additional variable called INCLUDE which will be set to
##      the value of SRC by default, but can be modified to include more    
##      than one include library on a per platform basis.
##      29-aug-97 (musro02)
##              Add sqs_ptx to config strings that set LIBS=$LDLIBMACH.
##      03-Dec-97 (muhpa01)
##              Add hp8_us5 to list of platforms which require libsec be
##		linked with ingvalidpw.
##	04-feb-1998 (somsa01) X-integration of 432504, 432929 from oping12 (somsa01)
##	    On HP-UX 10.00-10.20, shadow passwords are not used. HP says that
##	    this is "fixed" in HP-UX 10.30 . Therefore, if the platform is
##	    10.00-10.20, define "hp10_us5" in pwconfig.h; this will cause
##	    ingvalidpw.c to use the non-shadow stuff.  (Bug #71099)
##	18-jun-1998 (muhpa01)
##		Use $CC on compile line instead of cc in order to use defined
##		compiler for platform.  Also, added DFLAG=-D_HPUX_SOURCE for
##		hp8_us5.
##  	24-jun-1998 (loera01) Bug 91182
##	    The 04-feb-1998 change didn't work for HP users with trusted
##	    systems--they have shadowed passwords.  We make an additional
##	    check for /tcb/files/auth, and exit if this path is not
##	    present.  II_SHADOW_PWD doesn't need to be defined in this case. 
##	16-jul-1998 (bobmart)
##	    Modified loera01's above change; we may be building ingvalidpw.dis
##	    for the distribution (and not have /tcb/files/auth).  
##	16-Jul-1998 (bobmart)
##	    Added "DFLAG=-D_ALL_SOURCE" for ris_us5.
##	19-Aug-1998 (muhpa01)
##	    Added hpb_us5 (HP-UX 11.00 32-bit) to HP build setup
##	12-oct-98 (toumi01)
##	    For lnx_us5 use LIBS=$LDLIBMACH and check whether shadow
##	    passwords are in use.
##	10-may-1999 (walro03)
##	    Remove obsolete version string dr6_ues, dra_us5, ds3_osf, ix3_us5,
##	    odt_us5, vax_ulx.
##	06-oct-1999 (toumi01)
##	    Change Linux config string from lnx_us5 to int_lnx.
##      01-Aug-2000 (loera01) Bug 102237
##          For hpb_us5 platform, added check for /tcb/files/auth directory, 
##          and exit if the path is not found.
##	16-Jul-1999 (hweho01)
##	    Added MODEOPT in the option list for $CC command. 
##          For ris_u64 use MODEOPT=-q64 to specify the 64 bit compiler mode. 
##	28-apr-2000 (somsa01)
##	    Enabled for multiple product builds.
##	19-apr-1999 (hanch04)
##	    Add DFLAG from su9_us5
##	14-jun-2000 (hanje04)
##	    For ibm_lnx use LIBS=$LDLIBMACH and check whether shaddow 
##	    passwords are in use.
##	15-Sep-2000 (hanje04)
##	    Added support for Alpha Linux (axp_lnx)
##	23-jul-2001 (stephenb)
##	    Add support for i64_aix
##	04-Dec-2001 (hanje04)
## 	    Added support for IA 64 Linux (i64_lnx)
##      08-Oct-2002 (hanje04)
##          As part of AMD x86-64 Linux port, replace individual Linux
##          defines with generic *_LNX define where apropriate.
##	21-May-2003 (bonro01)
##	    Add support for HP Itanium (i64_hpu)
##	05-may-06 (hayke02)
##	    Change copyright from Computer Associates to Ingres Corporation.
##      10-oct-06 (Ralph Loen) Bug 116831
##	09-Jul-2004 (hanje04)
##	    Add -b flag to allow ingvalidpwd to be built build process
##	    as ingres no root. 
##	    NOTE: ingvalidpwd only functions correctly when mkvalidpw is
##	    run as root.
##      10-oct-06 (loera01) Bug 116831
##          On HP, add "-DSHADOW_EXISTS" to DFLAGS precompiler switch if
##          mkvalidpw finds /etc/shadow.
##      11-oct-06 (Ralph Loen) Bug 116831
##          On HP, add "-DHP_TRUSTED" to DFLAGS precompiler switch if
##          mkvalidpw finds /tcb/auth.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##      10-Nov-2006 (hanal04) Bug 117085
##          Make sure we pick up the correct return code from $CC by
##          issuing the command and then checking $?
##      13-Nov-2006 (hanal04) Bug 117085
##          Correct typo in previous submission.
##      06-Sep-2006 (smeke01) Bug 118879
##          Unset LD_RUN_PATH to restrict runtime linking. 
##          In the *_lnx case set libcrypt as the only library 
##          explicitly linked to.
## 	22-May-2008 (rajus01) Sir 120420, SD issue 116370
##	    Common code/definitions for Ingvalidpam and Ingvalidpw are moved
##	    ingpwutil.c and inpwutil.h respectively. 
## 	25-Mar-2009 (frima01) Sir 120420, SD issue 116370
##	    Added -qlanglvl=extc99 to MODEOPT for AIX.
##	22-Jun-2009 (kschendel) SIR 122138
##	    Update symbols for new hybrid scheme.  Clean up .o's in case
##	    the compiler leaves them around (Sun Studio 12).
##	13-Jul-2009 (hanje04)
##	    Bug 118879
##	    Unset LD_RUN_PATH using 'unset' rather that setting it to "".
##	    which still leaves it defined and this causes problems for
##	    rpmbuild which flags an empty LD_RUN_PATH variable and a 
##	    security risk.
##
#  PROGRAM = (PROG0PRFX)mkvalidpw
#******************************************************************************

: ${(PRODLOC)?}

. $(PRODLOC)/(PROD2NAME)/utility/(PROG1PRFX)sysdep

unset LD_RUN_PATH

notrootok=
[ "$1" = "-b" ] && notrootok=TRUE

# Must be root to run this script
if [ "$WHOAMI" != "root" ] && [ ! "$notrootok" ]
then
	echo "This script must be run by root."
	echo "Please login as root and retry."
	exit 255
fi

BIN=$(PRODLOC)/(PROD2NAME)/bin
SRC=$(PRODLOC)/(PROD2NAME)/files/iipwd
INCLUDE=$SRC

MODEOPT=""

if [ ! -r $SRC/ingvalidpw.c ]
then
	echo "Cannot find $SRC/ingvalidpw.c, exiting .."
	exit 255
fi

# If the operating system is HP-UX 10.00-10.20, verify whether or not
# the system is trusted.  Exit without modification if it's not.
if [ "$VERS" = "hp8_us5" ]
then
	if [ `uname -r | cut -d"." -f 2` -eq 10 -a `uname -r | cut -d"." -f 3` -lt 30 ]
	then
            if [ ! -d /tcb/files/auth -a -z "$ING_SRC" -a ! -f /etc/shadow ]
	    then
		echo "\"(PROG3PRFX)_SHADOW_PWD\" is not needed. Exiting..."
		ingunset (PROG3PRFX)_SHADOW_PWD
		exit
	    fi
	fi
fi
if [ "$VERS32" = "hpb_us5" ]
then
    if [ ! -d /tcb/files/auth -a -z "$ING_SRC" -a ! -f /etc/shadow ]
    then
        echo "\"II_SHADOW_PWD\" is not needed. Exiting..."
        ingunset II_SHADOW_PWD
        exit
    fi
fi

# decide on what libraries to use
LIBS=
case "$VERS" in
        axp_osf)        pn=`basename $0`
                        shlib="/usr/shlib/libsecurity.so"
                        if [ -f $shlib ]
                        then
                                LIBS="-lsecurity"
                        else
                                echo "$pn: Security library ($shlib) is missing"
                        fi
                        INCLUDE="$INCLUDE -I/usr/sys/include/sys" 
                        HAS_DEVSYS=true
                        ;;
	hp2_us5|hp8_us5|hpb_us5|i64_hpu)	LIBS="-lsec"
                        if [ -d /tcb/files/auth ]
                        then
                            DFLAG="-DHP_TRUSTED -D_HPUX_SOURCE"
                        elif [ -f /etc/shadow ]
                        then
                            DFLAG="-DSHADOW_EXISTS -D_HPUX_SOURCE"
                        else
                            DFLAG=-D_HPUX_SOURCE
                        fi
			;;
	sco_us5|sos_us5)
                        # check if U.S. libcrypt is supplied (with decrypt)
                        # otherwise, the supplement libcrypt must be there
                        if [ -f /lib/libcrypt_d.a ]
                        then
                                LIBS="-lprot -lcrypt_d -lx"
                        else
                                LIBS="-lprot -lcrypt_i -lx"
                        fi
                        HAS_DEVSYS=false;;

	ds3_ulx) 
			# check if KERBEROS/BIND is installed
			if [ -f /usr/lib/libckrb.a ]
			then
				DFLAG=-DKERBEROS_EXIST
				LIBS="-lckrb -lkrb -lknet -ldes -lauth"
			else
				LIBS="-lknet -lauth"
			fi;;
	dr6_us5|usl_us5|sqs_ptx) LIBS=$LDLIBMACH ;;
	*_lnx|int_rpl)	LIBS="-lcrypt"
			if [ -f /etc/shadow ]
			then
				DFLAG=-DSHADOW_EXISTS
			fi;;
	su9_us5)        DFLAG=-xarch=v9 ;;
	rs4_us5)        MODEOPT="-qlanglvl=extc99" ;;
	r64_us5)        MODEOPT="-q64 -qlanglvl=extc99"
                        ;;
esac

# If the development system is available on the system build the executable
echo "Building the password validation program '(PROG2PRFX)validpw'."
$CC $MODEOPT -I$INCLUDE $DFLAG $SRC/ingvalidpw.c $SRC/ingpwutil.c $LIBS -o $SRC/(PROG2PRFX)validpw > /dev/null 2>&1
if [ "$?" = "0" ]
then
	rm -f $BIN/(PROG2PRFX)validpw
	cp $SRC/(PROG2PRFX)validpw $BIN/(PROG2PRFX)validpw
	## Apparently some cc's leave .o's behind, clean them away.
	rm -f ingvalidpw.o ingpwutil.o
else
	echo "Could not compile (PROG2PRFX)validpw: Using executable from the distribution instead."
	rm -f $BIN/(PROG2PRFX)validpw
	cp $SRC/(PROG2PRFX)validpw.dis $BIN/(PROG2PRFX)validpw
fi

[ -f $BIN/(PROG2PRFX)validpw ] || {
    echo "Executable is not available: $BIN/(PROG2PRFX)validpw"
    exit 255
}

[ ! "$notrootok" ] && chown root $BIN/(PROG2PRFX)validpw
chmod 4755 $BIN/(PROG2PRFX)validpw

# Remove ingvalidpw to avoid upgrade problems depositing shipped executable
rm -f $SRC/(PROG2PRFX)validpw

# Add entry to symbol table and insure that symbol table owned by ingres
$BIN/(PROG2PRFX)setenv (PROG3PRFX)_SHADOW_PWD $(PRODLOC)/(PROD2NAME)/bin/(PROG2PRFX)validpw
chown ingres $(PRODLOC)/(PROD2NAME)/files/symbol.tbl

echo "Executable successfully installed."
exit 0
