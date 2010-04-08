:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkw4glrun.sh -- build the w4glrun executable and w4glrun.rxt under
#                 MainWin XDE Environment. 
#
# History: 
#	04-Sept-98 (yeema01)
#	    Created the script.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

PROG_NAME=mkw4glrun
APPED_SRC=$ING_SRC/w4gl/fleas/runtime
DESTDIR=$ING_BUILD/bin
  
ensure ingres rtingres || exit 1

#
# Sanity checks
#
if [ -z "$ING_SRC" ]
then
    echo $PROG_NAME: \$ING_SRC not set.
    exit 1
fi

if [ ! -d $ING_SRC ]
then
    echo "$PROG_NAME: Can not find the \$ING_SRC ($ING_SRC) directory."
    exit 1
fi

if [ -z "$MWHOME" ]
then
    echo $PROG_NAME: \$MWHOME not set.
    exit 1
fi


#
# Create OpenRoad Development executable - w4glrun
#
echo "" 
echo "Create OpenRoad Development executable - w4glrun "

cd $APPED_SRC
mwmake -f MWmakefile > MW.OUT 2>&1  

cd $DESTDIR
if [ -f w4glrun ]
then
   chmod 755 $ING_BUILD/bin/w4glrun
else
   echo "$PROG_NAME: Create w4glrun failed, exiting..."
   exit 1
fi

echo ""
echo "$PROG_NAME: Created OpenRoad Development executable - w4glrun ... "
echo " Done.."
exit 0
