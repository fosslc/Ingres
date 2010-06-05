:
#
# Copyright (c) 2010 Ingres Corporation
#
#
# Name: ingcreatedb
#
# Description:
#	This shell script is a wrapper for createdb for LSB builds.
#	It calls createdb under /usr/libexec/ingres/bin to avoid
# 	conflict with /usr/bin/createdb which is owned by PostgreSQL
#      
#
## History:
##	20-Apr-2010 (hanje04)
##	    SIR 123296
##	    Created.
#
(LSBENV)

exec ${II_SYSTEM?}/ingres/bin/createdb $*
