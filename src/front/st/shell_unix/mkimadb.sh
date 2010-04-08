:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
$II_SYSTEM/ingres/bin/createdb -u\$ingres imadb
sql -u\$ingres imadb < $II_SYSTEM/ingres/files/mkimau.sql


