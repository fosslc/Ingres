:
#*******************************************************************************
# 
# Copyright (c) 2004 Ingres Corporation
#
# Name: bvalidpw.sh
#
# Usage:
#       bvalidpw  /* called from clf/specials/MINGH */
#
# Description:
# 	This shell script is used to put into the build area the source files
#	required to compile ingvalidpw, the passwd verification program
#	on machines using shadow passwds. This script should be run as ingres.
#	This used to be a part of mkvalidpw.sh.
#
# Exit Status:
#       0       OK
#       1     didn't set ING_BUILD, ...
#
## History:
## History from mkvalidpw follows:
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
## End of History from mkvalidpw
##	31-mar-93 (vijay)
##		Modified from mkvalidpw. mkvalidpw will now only do the compile
##		install task, while this script will do the build part.
##	19-nov-1996 (canor01)
##		Changed name of 'iiconfig.h' to 'pwconfig.h' for portability
##		to Jasmine.
##	30-jul-2004 (stephenb)
##		handy is now handy_unix in subversion
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##     22-May-2008 (rajus01) SIR 120420, SD issue 116370
##		Added PAM support in Ingres.
##	       
##
#******************************************************************************

: ${ING_BUILD?} ${ING_SRC?}

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

. readvers

DEST=$ING_BUILD/files/iipwd

# check if destination path is there
[ ! -d $DEST ] && mkdir $DEST
chmod 755 $DEST

# Copy the files over to the build area

echo "#define $config" > $DEST/pwconfig.h
cp $ING_SRC/cl/clf$noise/handy_unix_win/ingvalidpw.x $DEST/ingvalidpw.c
cp $ING_SRC/cl/clf$noise/handy_unix_win/ingvalidpam.c $DEST/ingvalidpam.c
cp $ING_SRC/cl/clf$noise/handy_unix_win/ingpwutil.c $DEST/ingpwutil.c
cp $ING_SRC/cl/clf$noise/handy_unix_win/ingpwutil.h $DEST/ingpwutil.h
chmod 644 $DEST/ingvalidpw.c $DEST/pwconfig.h

exit 0
