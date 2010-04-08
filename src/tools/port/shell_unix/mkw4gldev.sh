:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkw4gldev.sh -- build the w4gldev executable and w4gldev.rxt under
#                 MainWin XDE Environment. 
#
# History: 
#	04-Sept-98 (yeema01)
#	    Created the script.
##      13-Nov-98 (yeema01)
##          Took out the steps that created Mainwin libraries which are
##          now moved to port/release/orprerls.sh
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

PROG_NAME=mkw4gldev
APPED_SRC=$ING_SRC/w4gl/fleas/develop
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
# Create OpenRoad Development executable - w4gldev
#
echo ""
echo "$PROG_NAME: Create OpenRoad Development executable - w4gldev "
cd $APPED_SRC
mwmake -f MWmakefile  > MW.OUT 2>&1

cd $DESTDIR

if [ -f w4gldev ]
then
   chmod 755 $ING_BUILD/bin/w4gldev
   echo ""
   echo "$PROG_NAME: Created OpenRoad Development executable - w4gldev ... "
   echo " Done.."
else
   echo ""
   echo "$PROG_NAME: Create w4gldev failed, exiting..."
   exit 1
fi

exit 0
