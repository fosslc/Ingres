:
# Copyright (c) 2004 Ingres Corporation
#
# Name:
#       ingsql- wrapper to Unicode Terminal Monitor.
#
## History:
##      04-feb-2004 (drivi01)
##          Created. 
##
#  PROGRAM = ingsql
# -----------------------------------------------------------------

exec ${II_JRE_HOME}/bin/java -jar -Djava.library.path=${II_SYSTEM}/ingres/lib ${II_SYSTEM}/ingres/lib/UTMProject.jar "$@"
~

