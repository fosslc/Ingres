:
#
#  Copyright (c) 2010 Ingres Corporation
#
#  Name: ingenv.bashrc
#
#  Usage:
#       source ingenv.bashrc
#
#  Description:
#	Environment script for Ingres LSB builds, installed to
#	/usr/share/ingres
#
##
## History:
##	20-Jan-2010 (hanje04)
##	    SIR 123296
##	    Created
##	11-Mar-2010 (hanje04)
##	    SIR 123296
##	    Remove TERM_INGRES, now set in symbol table and needs to be
##	    user set-able.
##	    Protect the build environement from the contents of this file
[ "$ING_BUILD" ] ||
{
    II_SYSTEM=/usr/libexec
    II_ADMIN=/var/lib/ingres/files
    II_LOG=/var/log/ingres
    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    export II_SYSTEM II_CONFIG II_ADMIN PATH
}
