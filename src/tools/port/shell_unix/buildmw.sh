:
#
# Copyright (c) 2004 Ingres Corporation
#
# Build MainWin INGRES tools from scratch
#
# This script is executed from within newbuild.sh .
#
## History
##	16-feb-2000 (somsa01)
##	    Created.
##	08-mar-2000 (somsa01)
##	    Added vnodemgr (ingnet).
##	09-may-2000 (somsa01)
##	    Added settings for other products.
##	11-Jun-2004 (somsa01)
##	    Cleaned up code for Open Source.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

CMD=`basename $0`

[ "$SHELL_DEBUG" ] && set -x
# Check for required environment variables
: ${ING_SRC?"must be set"} ${ING_BUILD?"must be set"} ${MWHOME?"must be set"}

# Check for user ingres
[ `IFS="()"; set - \`id\`; echo $2` = ingres ] ||
{ echo "$CMD: You must be ingres to run this command. Exiting..." ; exit 1 ; }

[ "$ING_VERS" ] && { vers_src="/$ING_VERS/src" ; vers="/$ING_VERS" ; }

. $MWHOME/setupmainwin

#
# newbuild would have done the work in copying the changed files in
# front/vdba/?? to front/vdba/??/mainwin. However, since mwprepro was
# not in the path, we need to run mwprepro on those subdirectories before
# building.
#
for dirname in camask cadate canum caspin catime catobal catolist \
	catostbx containr dbadlg1 lbtreelp nanact oidt treedll winmain \
	vcbf vnodemgr mainmfc
{
    echo "**** Building" $dirname "..."; date
    cd $ING_SRC/front/vdba$vers_src/$dirname/mainwin
    mwprepro
    cd ..
    touch MINGH
    mk
    echo ""
}

#
# Build IVM
#
echo "**** Building ivm ..."; date
cd $ING_SRC/front/vdba$vers_src/ivm/mainwin
mwprepro
cd ..
touch MINGH
mk
echo ""

#
# Now, build winstart.
#
echo "**** Building winstart ..."; date
cd $ING_SRC/front/st$vers_src/winstart/mainwin
mwprepro
cd ..
touch MINGH
mk

echo ""; echo "$CMD done: `date`"
exit 0
