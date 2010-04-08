:
# Copyright (c) 2004 Ingres Corporation
# $Header $
#
# Name:
#	clientchk
#
# Usage:
#	clientchk 
#
# Description:
#	This script checks that $II_SYSTEM/ingres directory is setup to run
#	a client installation. It checks the following things:
#		- the ingres directory is ok and all the subdirectories are ok
#		- The server_files directory exists
#		- The files directory is not mounted
#		- The bin, lib and utility directories are mounted
#
#	If the directory is ok, it diffs VERSION.ING and .version to see
#	if this in an update and the files directory needs to be updated.
#
#
# Exit status:
#	0	OK
#	1	II_SYSTEM is not defined
#	2	Directory is not setup t run a client installation
#
##  History: 
##      28-jun-90 (jonb)
##              Include iisysdep header file to define system-dependent
##              commands and constants.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
# ---------------------------------------------------------------------

. iisysdep

CMD=clientchk

[ $II_SYSTEM ] ||
{
    cat << !

$CMD: FATAL ERROR 1 --------------------------------

Environment variable II_SYSTEM is not defined.  Before running $CMD 
you must define II_SYSTEM as follows:

    In sh:	II_SYSTEM=XXX; export II_SYSTEM

    In csh:	setenv II_SYSTEM XXX

where XXX is the full path name of the directory to contain the installation.
!
    exit 1
}

cd $II_SYSTEM/ingres

# First check that the directory is setup for a client installation

error=false
if [ ! -d server_files ] || [ ! -d bin ] || [ ! -d files ] || [ ! -d lib ] || [ ! -d utility ] 
then
	error=true
elif [ ! -r files/users ] 
then
	error=true
elif [ ! -r .version ]
then
	error=true
else
	mount | grep $II_SYSTEM/ingres/bin > /dev/null 2>&1 
	if [ $? != 0 ]
	then
		error=true
	fi
fi

if $error
then
	cat << !

Clientchk: FATAL ERROR 2 ---------------------------------------------

This directory is not setup to run a client installation. Before running
this script make sure that INGRES has been installed in the server node
and that mkclientdir has been executed sucessfully.

Please refer to the Installation and Operations Guide for more information
on how to setup a client installation.

!
	exit 2
fi

cmp .version utility/VERSION.ING > /dev/null
if [ $? != 0 ]
then			# this is an update
	[ -f files/symbol.tbl ] && cp files/symbol.tbl files/symbol.save
	cp -r server_files/* files/.
	cd files
	[ -f errlog.log ] && rm errlog.log
	[ -f II_ACP.LOG ] && rm II_ACP.LOG
	[ -f II_RCP.LOG ] && rm II_RCP.LOG
	[ -f symbol.tbl ] && rm symbol.tbl
	[ -f symbol.save ] && mv symbol.save symbol.tbl
	cd $II_SYSTEM/ingres
	cp utility/VERSION.ING .version
	chmod 644 .version
fi

exit 0

