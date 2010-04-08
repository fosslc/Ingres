:
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name:
#	iiask4it- prompt and read a y/n response
#
# Usage:
#	if iiask4it "prompt-string" [default]
#	then
#		echo entered yes
#	else
#		echo entered no
#	fi
#
# Description:
#	prints prompt-string and reads "y" or "n" from stdin;
#	requires user to enter [yY] or [nN] before exiting.
#	If a second parameter is passed it will choose that parameter
# 	as default and it will not ait for an answer.
#
# Exit status:
#	exit 0 if [yY]
#	exit 1 if [nN]
#
## History: 
##      012495 (newca01) Copied from 6.4 "ask" and changed name 
##	26-apr-2000 (somsa01)
##	    Updated MING hint for program name to account for different
##	    products.
##
#  PROGRAM = (PROG1PRFX)ask4it
#
#  DEST = utility
# -----------------------------------------------------------------

while :
do
	iiechonn $1 "(y/n)? "
	if [ ! -n "$2" ]
	then
		read answer junk
	else
		answer=$2
		echo $answer
	fi
	case $answer in
	    y|Y)
		exit 0;;
	    n|N)
		exit 1;;
	    *)
		echo "Please enter 'y' or 'n'." ;;
	esac
done

