:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkorruntime.sh -- build the liborruntime.so and liborruntime.rxt under
#                 MainWin XDE Environment. 
#
# History: 
#	04-Sept-98 (yeema01)
#	    Created the script.
#	15-Sept-2000 (yeema01)
#	    Modified to support all Unix platforms.
#	29-Sept-2000 (matbe01) Sir# 102620
#	    Removed suffix from MWmakefile and substituted the variable
#	    name since it has been initialized with the platform value.
#	14-feb-2001 (crido01)
#	    Created from mkw4glrun
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

PROG_NAME=mkorruntime
APPED_SRC=$ING_SRC/w4gl/w4glcl/spec_unix/orrun
DESTDIR=$ING_BUILD/lib
BINDIR=$ING_BUILD/bin
OS=""
MWmakefile=""
  
ensure ingres rtingres || exit 1

# Get Platform configuration
. readvers

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

# Set the Platform that you are porting
case $config in
rs4_us5)
	OS="aix4"
	MWmakefile="MWmakefile.aix4"
	;;
hp8_us5)
	OS="ux10"
	MWmakefile="MWmakefile.ux10"
	;;
su4_us5)
	OS="su4"
	MWmakefile="MWmakefile.su4"
	;;
hpb_us5)
	OS="ux11"
	MWmakefile="MWmakefile.ux11"
	;;
esac


#
# Create OpenRoad Fused Runtime - liborruntime
#
echo "" 
echo "Create OpenRoad Fused Runtime - liborruntime "

cd $ING_SRC/w4gl/w4glcl/spec_unix/orrun
mwmake -f $MWmakefile > MW.OUT 2>&1

cd $DESTDIR
if [ -f liborruntime.so ]
then
   chmod 755 $ING_BUILD/lib/liborruntime.so
else
   echo "$PROG_NAME: Create liborruntime failed, exiting..."
   exit 1
fi

echo ""
echo "$PROG_NAME: Created OpenRoad Fused Runtime - liborruntime ... "

# Set the Platform that you are porting
case $config in
rs4_us5)
	OS="aix4"
	MWmakefile="MakeExe.aix4"
	;;
hp8_us5)
	OS="ux10"
	MWmakefile="MakeExe.ux10"
	;;
su4_us5)
	OS="su4"
	MWmakefile="MakeExe.su4"
	;;
hpb_us5)
	OS="ux11"
	MWmakefile="MakeExe.ux11"
	;;
esac

#
# Create OpenRoad MakeExe Utility
#
echo "" 
echo "Create OpenRoad MakeExe Utility - makeexe "

cd $ING_SRC/w4gl/w4glcl/spec_unix/orrun
mwmake -f $MWmakefile > MW.OUT 2>&1

cd $BINDIR
if [ -f makeexe ]
then
   chmod 755 $ING_BUILD/bin/makeexe
else
   echo "$PROG_NAME: Create makeexe failed, exiting..."
   exit 1
fi

echo ""
echo "$PROG_NAME: Created makeexe Utility... "
echo " Done.."
exit 0
