:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name: iiechonn
#
#  Usage:
#	iiechonn string 
#
#  Description:
#	echo a string without issuing a new line.
#
## History:
##	18-jan-93 (tyler)
##		Adapted from echon.
##	22-nov-93 (tyler)
##		Deleted from piccolo code line by someone who didn't
##		understand that "echo -n" isn't portable. 
##	29-nov-93 (tyler)
##		Reinstated.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##
#  PROGRAM = (PROG1PRFX)echonn
#
#  DEST = utility
#----------------------------------------------------------------------------

eval '$ECHO_CMD $ECHO_FLAG "$*$ECHO_CHAR"'

