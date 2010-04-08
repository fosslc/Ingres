:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       iirsh -- Set up a minimal Ingres environment for an Ingres command.
#
#  Usage:
#       iirsh <II_SYSTEM value> <command> [ <command_args ... ]
#
#  Description:
#       This program is typically used to process remote ingstart/ingstop
#	commands in a clusered environment.
#
#  Exit Status:
#       0       the requested operation was attempted
#       1       Insufficient/bad parameters passed.
#
## History:
##	07-jan-2002	devjo01
##	    Initial version.
##
#  PROGRAM = iirsh
#
#  DEST = utility
#----------------------------------------------------------------------------

# Set up the minimal environment
II_SYSTEM="$1"
[ $# -lt 2 -o ! -d $II_SYSTEM/ingres ] && exit 1
export II_SYSTEM
PATH="${PATH}:${II_SYSTEM}/ingres/bin:${II_SYSTEM}/ingres/utility"
export PATH
LD_LIBRARY_PATH="/lib:$II_SYSTEM/ingres/lib"
export LD_LIBRARY_PATH

# Execute the passed command.
shift
$@
