:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name: iistartup - start the INGRES server
#
#  Usage:
#	iistartup [ ii_system ]
#
#  Description:
#	This script allows ingstart to be invoked as iistartup for
#	backward compatibility.
#
## History:
##	21-jun-93 (tyler)
##		Created.	
##	22-feb-94 (tyler)
##		Fixed BUG 58908: Corrected option handling and error
##		messages explaining supported options.
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##
#  DEST = utility
#----------------------------------------------------------------------------

usage_error()
{
   cat << !

Ingres/iistartup

This script provides backward compatibility with non-interactive calls
to versions of iistartup delivered with previous releases.

Since this script is implemented as a wrapper around the Ingres
startup utility (ingstart) which does not have an interactive mode,
this script only supports the following options: 

% iistartup -s [ ii_system_value ]

To start individual Ingres processes, use ingstart.

To reconfigure any component of Ingres, use cbf. 

!
   exit 1
}

[ $# -gt 2 ] && usage_error

[ $# -gt 0 ] && [ $1 != "-s" ] && usage_error

[ $# -eq 2 ] &&
{
   II_SYSTEM=$2
   export II_SYSTEM

   PATH=$PATH:$II_SYSTEM/ingres/utility:$II_SYSTEM/ingres/bin
   export PATH
}

if $II_SYSTEM/ingres/utility/ingstart ; then
   exit 0
else
   exit 1
fi 
