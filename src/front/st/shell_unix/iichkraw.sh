:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: iichkraw.sh - determine whether named file is character special
#
#  Usage:
#	iichkraw file
#
#  Description
#	Called by the forms-based condiguration utility to determine
#	whether a transaction log file is a special-character file
#	(i.e. a raw log).
#
## History:
##	20-sep-93 (tyler)
##		Created.	
##	01-may-2000 (somsa01)
##		Updated MING program hint to reflect the product prefix.
##
#  PROGRAM = (PROG4PRFX)chkraw
#
#  DEST = utility
#----------------------------------------------------------------------------

[ $# != 1 ] && exit 1

[ -c $1 ] && exit 0

exit 1
