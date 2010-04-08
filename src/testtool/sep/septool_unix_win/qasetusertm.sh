:
#
# Copyright (c) 2004 Ingres Corporation
#
# qasetusertm.sh
#
# This is a wrapper script for qasetuser. It is invoked by SEP scripts
# to run qasetuser to bring up the terminal monitor.
#
## History:
##
##	02-oct-1991 (lauraw) Created
##
##	14-Aug-92 (GordonW)
##		Overlay shell with requested program.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##

exec qasetuser $*
