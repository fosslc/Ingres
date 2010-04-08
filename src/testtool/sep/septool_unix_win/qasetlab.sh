:
#
# Copyright (c) 2004 Ingres Corporation
#
# qasetlab.sh
#
# This is a wrapper script for qasetplabel. It is invoked by SEP scripts
# to run qasetplabel on an DBUTIL executable.
#
## History:
##
##      27-aug-1993 (pholman)
##           First created
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##
label=$1
shift
exec qasetplab "$label" $*
