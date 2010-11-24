:
#
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name:
#	iisuabf
#
#  Usage:
#	iisuabf  (called from ingbuild, generally)
#
#  Description:
#	Configure the ING_ABFDIR location and set link files for ABF
#
#  Exit status:
#	0	OK
#       99	ABF is not installed
#
##
## History:
##	05-feb-93 (jonb)
##		Created
##	17-mar-93 (dianeh)
##              Added check for batch-mode, so it won't prompt;
##              cosmetic clean-up.
##	30-apr-93 (vijay)
##		If the release uses shared libraries, use those instead of
##		libingres.a.
##	03-may-93 (dianeh)
##		Remove erroneous extra pound-sign in front of DEST.
##	02-nov-93 (tyler)
##		Create config.dat if it doesn't exist.
##	22-nov-93 (tyler)
##		Replaced system calls with shell procedure invocations.
##	29-nov-93 (tyler)
##		Use pause() shell function
##	07-dec-93 (vijay)
##		Add -no_shared_libs option. Also, change echo << ! to cat.
##	05-jan-94 (tyler)
##		Set ING_ABFDIR to II_SYSTEM (don't prompt) by default.
##		Removed dead code.  Improved error handling.  cd to
##		$II_SYSTEM/ingres/files before dealing with abflnk.opt
##		and abfdyn.opt
##	11-jan-94 (vijay)
##		Change abf*.opt to use -l for specifying shared libs.
##		Else on HP and maybe Solaris, user executables will not
##		be able to move between installations, since they will
##		always look for the libraries in the original II_SYSTEM.
##	19-jan-94 (tomm)
##		ING_ABFDIR should not be II_SYSTEM, but II_SYSTEM/ingres/abf.
##		Furthermore, if this directory does not exist, we should create
##		it and set the correct permissions.
##	25-jan-94 (tomm)
##		needs to check for shared libraries as well as static ones.
##		add this to library check.
##	16-feb-94 (vijay)
##		Allow for both options to work at the same time. -batch and
##		-no_shared_libs.
##	22-nov-95 (hanch04)
##		added log of install.
##	25-jan-95 (morayf)
##		Changed shared library path from starting with a hard-wired
##		/lib to starting with -L before each entry in LDLIBPATH in
##		the same order as they were specified in mkdefault.sh.
##		This should reduce the number of ABF-specific RNS sections
##		needed.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##	08-Feb-2000 (hanch04)
##		If including F77, then include sunmath.  It will only get
##		included if it exists.
##    27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##	26-Sep-2000 (hanje04)
##		Added new flag (-vbatch) to generate verbose output whilst
##		running in batch mode. This is needed by browser based
##		installer on Linux.
##	14-Sep-2001 (gupsh01)
##		Added new flag (-mkresponse) to create a response file but 
## 		do not actually do the setup.
##	23-Oct-2001 (gupsh01)	
##		Implemented flag (-exresponse). 
##	02-nov-2001 (gupsh01)
##		Added support for user defined response file.
##    30-Jan-2004 (hanje04)
##        Changes for SIR 111617 changed the default behavior of ingprenv
##        such that it now returns an error if the symbol table doesn't
##        exists. Unfortunately most of the iisu setup scripts depend on
##        it failing silently if there isn't a symbol table.
##        Added -f flag to revert to old behavior.
##    27-Aug-2003 (hanje04)
##            Added -rpm flag to support installing from RPM files.
##	03-feb-2004 (somsa01)
##		Backed out symbol.tbl change for now.
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##	02-Mar-2004 (hanje04)
##	    Duplicate trap inside do_su function so that it is properly
##	    honoured on Linux. Bash shell doesn't keep traps across functions.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	02-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	23-Mar-2010 (hanje04)
##	    SIR 123296
##	    Set II_ABF_LNK_OPT for LSB builds
##      29-Sep-2010 (thich01)
##          Call iisetres to indicate this script ran successfully so the post
##          install script detects its success.
##
#  DEST = utility
#----------------------------------------------------------------------------
. iisysdep
. iishlib
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    ingbindir=/usr/bin
    inglibdir=/usr/lib/ingres
    abfdir=/var/lib/ingres/abf
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    ingbindir=$II_SYSTEM/ingres/bin
    inglibdir=$II_SYSTEM/ingres/lib
    abfdir=$II_SYSTEM/ingres/abf
    II_CONFIG=$II_SYSTEM/ingres/files
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc

if [ "$1" = "-rmpkg" ] ; then
   cat << !
  Application-By-Forms has been removed

!
exit 0
fi

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=FALSE

# check for batch flags and sharedlibs
BATCH=false
INSTLOG="2>&1 | tee -a $II_LOG/install.log"

while [ $# != 0 ]
do
    case "$1" in
        -batch)
            BATCH=true ; export BATCH ; 
	    INSTLOG="2>&1 | cat >> $II_LOG/install.log"
	    export INSTLOG;
	    shift ;;
        -rpm)
	    RPM=true
            BATCH=true ; export BATCH ; 
	    INSTLOG="2>&1 | cat >> $II_LOG/install.log"
	    export INSTLOG;
	    shift ;;
	-vbatch)
	    BATCH=true ; export BATCH ;
            export INSTLOG;
            shift ;;
	-mkresponse)
	    WRITE_RESPONSE=true; export WRITE_RESPONSE;
   	    BATCH=false
   	    if [ "$2" ] ; then
        	RESOUTFILE=$2
		shift;
   	    fi
	    export RESOUTFILE;
	    export INSTLOG;
	    shift ;;
	-exresponse)
	    READ_RESPONSE=true; export READ_RESPONSE;
   	    BATCH=true
   	    if [ "$2" ] ; then
        	RESINFILE=$2
		shift;
   	    fi
	    export RESINFILE;
	    export INSTLOG;
            shift ;;
        -no_shared_libs)
            USE_SHARED_LIBS=false ; shift ;;
	*)
	    echo "ERROR: Unknown option to iisuabf: $1" ;
	    echo "usage: iisuabf [ -batch ] [-vbatch] [ -rpm ] [ -no_shared_libs ] \
 [-mkresponse] [-exresponse] [response file]"
	    exit 1 ;;
    esac
done

trap "exit 1" 0 1 2 3 15

do_iisuabf()
{
echo "Setting up Application-By-Forms..."
trap "exit 1" 0 1 2 3 15

check_response_file #check if the response files exist.

if [ "$WRITE_RESPONSE" = "true" ] ; then
    mkresponse_msg
else  #SKIP the REST

  [ -f ${ingbindir}/abf ] ||
  {
     echo "ERROR: Attempted to set up ABF when ABF is not installed!"
   cat << !

Attempted to set up ABF, but abf executable is missing.

!
     exit 1 
  }

touch_files # make sure symbol.tbl and config.dat exist

ING_ABFDIR=`ingprenv ING_ABFDIR`
if [ -z "$ING_ABFDIR" ] 
then
	ingsetenv ING_ABFDIR $abfdir
	ING_ABFDIR=$abfdir
fi

if [ ! -d $ING_ABFDIR ]
then
	echo Creating ABF Directory...
	mkdir $ING_ABFDIR
	chmod 777 $ING_ABFDIR
fi
cat << !

The following directory will be used to store Application-By-Forms, Vision
and VisionPro applications:

	$ING_ABFDIR

This default location can be changed later by redefining the Ingres
variable ING_ABFDIR, using the command: 

	ingsetenv ING_ABFDIR <location>

This variable can also be defined in the Unix environment.

!

$BATCH || pause

config_location "ING_ABFDIR" "ABF application object files" || 
{
   cat << !

 -----------------------------------------------------------------------
|                             ***ERROR***                               |
|									|
| Unable to configure the default storage location for Application-By-	|
| Forms, Vision, and VisionPro application files.  Please attempt to	|
| resolve this problem and re-execute this setup program.		|
|                                                                        |
 ------------------------------------------------------------------------

!
   exit 1
}

cd ${cfgloc}

[ -r abf.opt ] &&
{ 
   cat << !

 -----------------------------------------------------------------------
|                            *** NOTICE ***				|
|									|
| This release uses two separate files, abflnk.opt and abfdyn.opt, to	|
| list linker libraries and options for ABF Image and Go operations	|
| respectively.  These two files replace the abf.opt file.		|
|									|
| Any customozations made to abf.opt will need to be manurally added	|
| to abflnk.opt and abfdyn.opt if desired.				|
|									|
 -----------------------------------------------------------------------

!
}

[ -r abflnk.opt ] &&
{ 
   cat << !

 -----------------------------------------------------------------------
|                            *** NOTICE ***				|
|									|
| A version of abflnk.opt already exists, but needs to be upgraded.  A	|
| copy of the existing file will be saved in abflnk.opt.old and a new	|
| version will be created.  Any customizations to the original version	|
| will need to be manually added to the new file.			|
|									|
 -----------------------------------------------------------------------

!
   $BATCH || pause
   mv abflnk.opt abflnk.opt.old
}


ldlibpath=`echo " $LDLIBPATH"|sed 's%[ 	]\([^ 	]\)% -L\1%g'`
	

if $USE_SHARED_LIBS ; then
   echo $ldlibpath -L$inglibdir > abflnk.opt
   echo -linterp.1 >> abflnk.opt
   echo -lframe.1 >> abflnk.opt
   echo -lq.1 >> abflnk.opt
   echo -lcompat.1 >> abflnk.opt
else
   if [ -f ${inglibdir}/libingres1.a ] ; then
      echo ${inglibdir}/libingres1.a > abflnk.opt
      echo ${inglibdir}/libingres2.a >> abflnk.opt
   elif [ -f ${inglibdir}/libingres.a ] ; then
      echo ${inglibdir}/lib/libingres.a > abflnk.opt
   fi
fi	# USE _SHARED_LIBS

if [ x"$conf_LSB_BUILD" = x"TRUE" ] ;
then
    II_ABF_LNK_OPT=$II_ADMIN/abflnk.opt
    if [ -f $II_ABF_LNK_OPT ]
    then
	ingsetenv II_ABF_LNK_OPT $II_ABF_LNK_OPT
    else
	cat << !
ERROR: Cannot locate ABF link options definition file:

    $II_ABF_LNK_OPT

setup cannot complete without this file.

!
	exit 1
    fi
fi

[ -z "$LIBMACH" ] || echo $LIBMACH >> abflnk.opt

if [ -f ${ingbindir}/eqf -o -f ${ingbindir}/esqlf ] ; then	
   liblist="F77 sunmath I77 U77 cl Optf77 Opti77 Optu77 xlf ots"
   for libdir in $LDLIBPATH
   do
      libflags=
      for lib in $liblist
      do
         if [ -f $libdir/lib${lib}.a -o -f $libdir/lib${lib}.${SLSFX} ] ; then
            [ -z "$libflags" ] && libflags="-L$libdir"
            libflags="$libflags -l$lib"
            liblist=`echo $liblist | sed "s/ $lib / /"`
         fi
      done
      [ -z "$libflags" ] || echo $libflags >> abflnk.opt
   done
fi

[ -r abfdyn.opt ] &&
{ 
   cat  << !

 -----------------------------------------------------------------------
|                            *** NOTICE ***				|
|                                                                       |
| A version of abfdyn.opt already exists, but needs to be upgraded.  A	|
| copy of the existing file will be saved in abfdyn.opt.old and a new	|
| version will be created.  Any customizations to the original version	|
| will need to be manually added to the new file.			|
|                                                                       |
 -----------------------------------------------------------------------

!
   $BATCH || pause
   mv abfdyn.opt abfdyn.opt.old
}

cp abflnk.opt abfdyn.opt

chmod 644 abflnk.opt abfdyn.opt 

cd $inglibdir
rm -f abfmain.o abfimain.o
ln abfmain.obj abfmain.o
ln abfimain.obj abfimain.o

fi	# SKIPPED THE REST

cat << !

Application-By-Forms setup complete.

!
   if [ -f $II_SYSTEM/ingres/install/release.dat ] ; then
       VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
       {
           cat << !
   
$VERSION

!
          exit 1
       }
   elif [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
      VERSION=`head -1 /usr/share/ingres/version.rel` ||
   {
       cat << !

Missing file /usr/share/ingres/version.rel

!
      exit 1
   }
   else
       VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
       {
           cat << !
   
Missing file $II_SYSTEM/ingres/version.rel
   
!
          exit 1
       }
   fi

   RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`
   SETUP=`iigetres ii.$CONFIG_HOST.config.abf.$RELEASE_ID`
   if [ "$SETUP" = "complete" ]
   then
        # If testing, then pretend we're not set up, otherwise scram.
        $DEBUG_DRY_RUN ||
        {
            cat << !
   
The $VERSION version of Application-By-Forms has
already been set up on local host "$HOSTNAME".
   
!
            $BATCH || pause
            trap : 0
            exit 0
        }
        SETUP=""
   fi

iisetres ii.$CONFIG_HOST.config.abf.$RELEASE_ID complete
$BATCH || pause
trap : 0
exit 0
}
eval do_iisuabf $INSTLOG
trap : 0
exit 0
