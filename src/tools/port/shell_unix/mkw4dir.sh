:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkw4dir:
#
#	Make the Windows 4GL installation tree.
#	Follow the structure of mkidir
#
#	This must be run as ingres; on sysV, this means ruid and euid.
#	May be run more than once.
#
# History:
#	05-nov-90 (andys)
#		Created by hacking 6202p mkidir.sh
#	14-mar-91 (jab)
#		Moved to W4GL04 set of source.
#	9-apr-91 (jab)
#		Added "w4glapps" and "files/english" to the list of
#		directories created.
#	24-apr-91 (jab)
#		Added "utility" for the build scripts and [eventually]
#		an ilist/dlist.
#	12-feb-92 (jab)
#		Added to 63p library, support for W4GL II should be there.
#	02-apr-92 (szeng)
#		Added "X11R4" and "X11R4/include" to the list of 
#		directories created.
#	05-feb-93 (gillnh2o)
#		Added support for both the creation of X11R4 and X11R5
#		directories. Added a source of the "readvers" file.
#		This means you must have option=[X11R4 || X11R5]
#		defined in your tools!port!conf VERS file.
#	05-feb-93 (jab)
#		Bug fix - wasn't creating w4gldemo dirs...
#	07-apr-93 (dwilson)
#		Create X11R5/lib/X11 to be able to add in the nls stuff.
#	21-apr-93 (dwilson)
#		Create X11R5/files to be able to add X*DB files
#       10-sep-98 (yeemao1) 
#               Add a new section to create Build directories
#               for OpenROAD with MainWin xde environment.
#       05-Nov-98 (yeema01)
#               Took out the step that creates X11 files and added more 
#               MainWin directory.
#       01-June-99 (yeema01) Bug# 97039
#               Added "tmp" to the list of directories created. Also 
#               fixed the problem in creating "w4glsamp" directory. 
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
#

[ -d ${ING_BUILD?"must be set"} ] || { echo "Must create $ING_BUILD"; exit 1; }

ensure ingres rtingres || exit 1

. readvers

vers=$config

case $vers in
su4_us5)   echo "su4_us5"
           LIB1=lib-sunos5
           LIB2=bin-sunos5_optimized
           LIB3=lib-sunos5_debug
           LIB4=lib-sunos5_optimized
           LIB5=lib-sparc_sunos5
           ;;
ris_us5)   echo  "ris_us5"
           LIB1=" "
           ;;
hp8_us5)   echo "hp8_us5"
           LIB1=" "
           ;;
esac


cd $ING_BUILD

while read mode filename
do
	[ "$line" -o "$filename" ] || continue
	echo $filename "..."
	[ -d $filename ] || mkdir $filename
	chmod $mode $filename
done <<HERE
755 ./files/dictfiles
755 ./files/deffiles
755 ./files/w4gldemo
755 ./files/w4gldemo/Bitmaps
755 ./files/w4gldemo/External
755 ./files/w4gldemo/Strings
755 ./files/english
755 ./tmp
755 ./w4glapps
755 ./w4glsamp
755 ./w4glsamp/reporter
755 ./w4glsamp/tmdemo
755 ./utility
755 ./mainwin
755 ./mainwin/system
755 ./mainwin/afm
755 ./mainwin/bin
755 ./mainwin/lib
755 ./mainwin/$LIB1
755 ./mainwin/$LIB2
755 ./mainwin/$LIB3
755 ./mainwin/$LIB3/locale
755 ./mainwin/$LIB3/locale/fr
755 ./mainwin/$LIB3/locale/de
755 ./mainwin/$LIB4
755 ./mainwin/$LIB4/locale
755 ./mainwin/$LIB4/locale/fr
755 ./mainwin/$LIB4/locale/de
755 ./mainwin/$LIB5
755 ./mainwin/$LIB5/nls
HERE
exit 0
