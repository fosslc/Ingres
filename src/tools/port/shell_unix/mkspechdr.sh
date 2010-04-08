:
##
##  Copyright (c) 2000, Ingres Corporation
##
##  Name:  mkspechdr.sh -- Creates header for Ingres/Cobalt, RPM .spec file
##
##  Description:
##  Generates header segment of ii_cobalt.spec with correct BuildRoot
##  for building IngresII-2.5.0006.cob.i386.rpm
##
##  History:
##	07-Nov-2000 (hanje04)
##	    Created
##

echo \
"Summary: IngresII Inteligent Database
Name: IngresII
Version: 2.5.0006.cob
Release: 1
Copyright: Comercial
Group: Applications/Databases
Source: none
Patch: none
BuildRoot: $ING_SRC/Cobalt/rpm
" > $ING_SRC/Cobalt/rpm/ii_cobalt.spec
