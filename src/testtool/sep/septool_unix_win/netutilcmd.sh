:
#
# Copyright (c) 2004 Ingres Corporation
#
# netutilcmd.sh
#
# This is a wrapper script for netutil. It is invoked by SEP scripts
# to run netutil with command line arguments.
#
## History:
##
##	11-oct-1993 (DonJ) Created
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##
exec netutil $*
