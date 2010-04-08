:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name: iiunloadfix -- replace system_maintained with sys_maintained
#
#  Usage:
#	iiunloadfix
#
#  Description:
#	Due to making system_maintained a reserved key word, the
#	copy.in script produced from unloaddb will fail.  This
#	script will updated the create table ii_atttype statement
#	to create the correct column called sys_maintained.
#
## History:
##	27-nov-2000 (hanch04)
##	    Created.  Bug 103338
##
#  PROGRAM = (PROG1PRFX)unloadfix
#
#  DEST = utility
#----------------------------------------------------------------------------

mv copy.in copy.in.bak
sed "s/system_maintained/sys_maintained/" copy.in.bak > copy.in
