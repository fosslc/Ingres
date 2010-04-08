:
#
# Copyright (c) 2004 Ingres Corporation
#
#  buildOR  -  auto-build of OpenROAD with MainWin XDE environment.     
#
##  History:
##	03-sep-98 (yeema01)
##         Created. 
##      02-Nov-98 (yeema010
##         Added build of REPORTER
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##

prog="buildOR"
XDEBUILD=""

NTOLE_SRC=$ING_SRC/w4gl/woof/ntole
W4WN_SRC=$ING_SRC/w4gl/w4sys/w4wn 
W4RPLX_SRC=$ING_SRC/w4rp/reporter/lxscript
W4RPRE_SRC=$ING_SRC/w4rp/reporter/repprint

USAGE="$prog"

# Check if need to build w4gl 
. readvers


# Check if need to build with xde
if [ "$conf_xde" = "true" ];then
   echo ""
   echo "$prog: Build OpenROAD under MainWin XDE environment. "
   XDEBUILD="true"
else
   echo ""
   echo "$prog: OpenROAD build is not required. Exiting..." ; exit 1 ;
fi


# Check for required environment variables
: ${MWSRC?"must be set"} ${MWHOME?"must be set"}
 
# Start building OpenRoad 
echo ""
echo "$prog: Beginning NEW OpenROAD build..."

# Create Build area directories
echo ""
echo "$prog: Creating directories in ING_BUILD area..."
mkw4dir

# Build w4gl/hdrs? 

echo ""
echo "$prog: Build the frontend headers..."
cd $ING_SRC/w4gl && mk hdrs | tee OUT

# Build  frontend libraries ?
echo ""
echo "$prog: Build the frontend libraries..."
cd $ING_SRC/w4gl && mk lib  | tee OUT

# Build other parts of OpenRoad - e.g dictfiles and w4gldemo....
cd $ING_SRC/w4gl && mk | tee OUT


# Build message files .mnx files
echo ""
echo "$prog: Build the messages files .mnx ..."
mkw4cat  

# Build Reporter 
echo ""
echo "$prog: Build the OpenRoad Reporter..."
cd $ING_SRC/w4rp/w4rputil/w4rpkey
mk | tee OUT

cd $ING_SRC/w4rp/w4rputil/w4rpstr
mk | tee OUT 

cd $ING_SRC/w4rp/reporter/repprint
mk | tee OUT

# Use MainWin to create a few leftover
echo ""
echo "$prog: Create libntole using MainWin XDE"
cd $NTOLE_SRC
mwmake -f MWmakefile > MW.OUT 2>&1

# Use MainWin to create libor4glnt.so
echo ""
echo "$prog: Create libor4glnt.so  using MainWin XDE"
cd $W4WN_SRC
mwmake -f MWmakefile > MW.OUT 2>&1 
 
 
# Use MainWin to build rest of REPORTER

echo ""
echo " Complete building REPORTER using MainWin  "

# Use MainWin to compile w4rp/reporter/lxscript
echo ""
echo "$prog: Create libreporter.so "
cd $W4RPLX_SRC
mwmake -f MWmakefile > MW.OUT  2>&1

# Use MainWin to compile the rest of  w4rp/reporter/repprint
echo ""
echo "$prog: Create repprint "
cd  $W4RPRE_SRC
mwmake -f MWmakefile > MW.OUT  2>&1

echo ""
echo "If w4gl build are clear of errors, you are now ready to build w4glrun and
 w4gldev using mkw4glrun and mkw4gldev".

echo ""
echo " DONE..."

exit 0


