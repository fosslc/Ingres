:
#
# Copyright (c) 2004 Ingres Corporation
#
# orprerls --  prepare OpenROAD $ING_BUILD files for copying to a release area.
#              These include all the MainWin files and libbrary, and the
#              w4gldev.rel and w4glrun.rel
#               
#
# History: 
#	12-Nov-98 (yeema01)
#	    Created the script.
#       01-June-99 (yeema01) Bug# 97039
#           Set the permission for all the files according to "buildrel"'s
#           requirement. For example, for MainWin files, changed permission 
#           from 775 to 755, and changed 664 to 644.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

PROG_NAME=orprerls
  
ensure ingres rtingres || exit 1

#
# Sanity checks
#

if [ -d ${ING_BUILD} ] 
then 
   echo $PROG_NAME: \$ING_BUILD not set.
   exit 1
fi
 
if [ -z "$MWHOME" ]
then
    echo $PROG_NAME: \$MWHOME not set.
    exit 1
fi

. readvers
vers=$config


case $vers in
su4_us5)   echo "su4_us5"
           LIB1=lib-sunos5
           LIB2=lib-sunos5_debug
           LIB3=lib-sunos5_optimized
           LIB4=lib-sparc_sunos5
           BIN1=bin-sunos5_optimized
           ;;
ris_us5)   echo  "ris_us5"
           LIB1=" "
           ;;
hp8_us5)   echo "hp8_us5"
           LIB1=" "
           ;;
esac

# Check w4gldev.rel and w4gldev.rel is avaliable for Release

cd $ING_BUILD

if [ ! -f "w4gldev.rel" ]
then
    echo " Please Create w4gldev.rel file before release " 
fi
   
if [ ! -f "w4glrun.rel" ]
then
    echo " Please Create w4glrun.rel file before release " 
fi

# Copy MainWin Shared Libraries to $INGLIB for release 
echo ""
echo "$PROG_NAME: Copy MainWin libraries "


while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
	  cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
win.ini
setup-mwuser
setup-mwuser.csh
setupmainwin.csh
system/registry.bin
HERE

while read filename
do
      [ "$line" -o "$filename" ] || continue
      echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
      cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
afm/Courier-Bold.afm
afm/Courier-BoldOblique.afm
afm/Courier-Oblique.afm
afm/Courier.afm
afm/Helvetica-Bold.afm
afm/Helvetica-BoldOblique.afm
afm/Helvetica-Oblique.afm
afm/Helvetica.afm
afm/Times-Bold.afm
afm/Times-BoldItalic.afm
afm/Times-Italic.afm
afm/Times-Roman.afm
HERE

while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
bin/mwgetconfigname
bin/mwrmprops
bin/mwshowprops
bin/mwregconvold
bin/winhelp
bin/mwregedit
bin/regedit4
bin/regsvr
bin/tibrowse
HERE

while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
lib/mwregconvold
lib/mwgetconfigname
lib/mwuseconfig.csh
lib/mwuseconfig.sh
lib/mwregedit
lib/regsvr
HERE

#  Copy OS specific files 

while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
$BIN1/mwfwrapper
$BIN1/mwprip
$BIN1/rpcss
$BIN1/mwregsvr
$BIN1/winhlp32
$BIN1/mwregsvr.rxt
$BIN1/winhlp32.rxt
$BIN1/winhlp32.rxb
HERE

while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
$LIB1/mwregedit
$LIB1/tibrowse
$LIB1/regedit4
$LIB1/tibrowse.rxt
$LIB2/mainwin.rxt
$LIB2/mainwin.rxb
$LIB2/locale/fr/mainwin.rxt
$LIB2/locale/fr/mainwin.rxb
$LIB2/locale/de/mainwin.rxt
$LIB2/locale/de/mainwin.rxb
$LIB3/mainwin.rxt
$LIB3/mainwin.rxb
$LIB3/locale/fr/mainwin.rxt
$LIB3/locale/fr/mainwin.rxb
$LIB3/locale/de/mainwin.rxt
$LIB3/locale/de/mainwin.rxb
HERE

while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
$LIB3/libmw32.so
$LIB3/libftsrch.so
$LIB3/libcomctl32.so
$LIB3/libriched32.so
$LIB3/libmfc400s.so
$LIB3/libadvapi32.so
$LIB3/librpcssp.so
$LIB3/librpcrt4.so
$LIB3/librpcltscm.so
$LIB3/librpcltccm.so
$LIB3/libntrtl.so
$LIB3/libcoolmisc.so
$LIB3/libshlwapi.so
$LIB3/libolepro32.so
$LIB3/libole32.so
$LIB3/liboleaut32.so
$LIB3/libole2ui.so
$LIB3/libuuid.so
$LIB3/libatl.so
$LIB3/libcomcat.so
$LIB3/libactxprxy.so
$LIB3/libmwicons.so
$LIB3/libcomdlg32.so
$LIB3/libshell32.so
$LIB3/atl.tlb
$LIB3/stdole2.tlb
$LIB3/olepro32.tlb
$LIB3/stdole32.tlb
$LIB3/regsvr
HERE

while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
$LIB3/ftsrch.rxt
$LIB3/ftsrch.rxb
$LIB3/comctl32.rxt
$LIB3/comctl32.rxb
$LIB3/riched32.rxt
$LIB3/mfc400s.rxt
$LIB3/mfc400s.rxb
$LIB3/shlwapi.rxt
$LIB3/shlwapi.rxb
$LIB3/olepro32.rxt
$LIB3/olepro32.rxb
$LIB3/ole32.rxt
$LIB3/ole32.rxb
$LIB3/ole2ui.rxt
$LIB3/ole2ui.rxb
$LIB3/atl.rxt
$LIB3/atl.rxb
$LIB3/comcat.rxt
$LIB3/comcat.rxb
$LIB3/mwicons.rxt
$LIB3/mwicons.rxb
$LIB3/comdlg32.rxt
$LIB3/comdlg32.rxb
$LIB3/shell32.rxt
$LIB3/shell32.rxb
$LIB3/regsvr.rxt
HERE


while read filename
do
    [ "$line" -o "$filename" ] || continue
    echo "Copy $MWHOME/$filename ..." | tee -a MWCOPY.OUT
    cp $MWHOME/$filename $ING_BUILD/mainwin/$filename
done <<HERE
$LIB4/mwperl
$LIB4/nls/BIG5.nls
$LIB4/nls/KSC.nls
$LIB4/nls/PRC.nls
$LIB4/nls/PRCP.nls
$LIB4/nls/XJIS.nls
$LIB4/nls/c_037.nls
$LIB4/nls/c_10000.nls
$LIB4/nls/c_10001.nls
$LIB4/nls/c_10002.nls
$LIB4/nls/c_10003.nls
$LIB4/nls/c_10004.nls
$LIB4/nls/c_10005.nls
$LIB4/nls/c_10006.nls
$LIB4/nls/c_10007.nls
$LIB4/nls/c_10008.nls
$LIB4/nls/c_10010.nls
$LIB4/nls/c_10017.nls
$LIB4/nls/c_10029.nls
$LIB4/nls/c_10079.nls
$LIB4/nls/c_10081.nls
$LIB4/nls/c_10082.nls
$LIB4/nls/c_1026.nls
$LIB4/nls/c_1250.nls
$LIB4/nls/c_1251.nls
$LIB4/nls/c_1252.nls
$LIB4/nls/c_1253.nls
$LIB4/nls/c_1254.nls
$LIB4/nls/c_1255.nls
$LIB4/nls/c_1256.nls
$LIB4/nls/c_1257.nls
$LIB4/nls/c_1258.nls
$LIB4/nls/c_1361.nls
$LIB4/nls/c_20866.nls
$LIB4/nls/c_28592.nls
$LIB4/nls/c_29001.nls
$LIB4/nls/c_437.nls
$LIB4/nls/c_500.nls
$LIB4/nls/c_708.nls
$LIB4/nls/c_720.nls
$LIB4/nls/c_737.nls
$LIB4/nls/c_775.nls
$LIB4/nls/c_850.nls
$LIB4/nls/c_852.nls
$LIB4/nls/c_855.nls
$LIB4/nls/c_857.nls
$LIB4/nls/c_860.nls
$LIB4/nls/c_861.nls
$LIB4/nls/c_862.nls
$LIB4/nls/c_863.nls
$LIB4/nls/c_864.nls
$LIB4/nls/c_865.nls
$LIB4/nls/c_866.nls
$LIB4/nls/c_869.nls
$LIB4/nls/c_870.nls
$LIB4/nls/c_874.nls
$LIB4/nls/c_875.nls
$LIB4/nls/c_932.nls
$LIB4/nls/c_936.nls
$LIB4/nls/c_949.nls
$LIB4/nls/c_950.nls
$LIB4/nls/ctype.nls
$LIB4/nls/l_except.nls
$LIB4/nls/l_intl.nls
$LIB4/nls/locale.nls
$LIB4/nls/sortkey.nls
$LIB4/nls/sorttbls.nls
$LIB4/nls/unicode.nls
HERE

# After copy all the MainWin files, make sure all the MainWin files
# had the appropriate permission for buildrel.

cd $ING_BUILD/mainwin
find . -perm 775 -exec chmod 755 {} \;
find . -perm 664 -exec chmod 644 {} \;

exit 0
