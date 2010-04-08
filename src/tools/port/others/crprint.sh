##Copyright (c) 2004 Ingres Corporation
:
#
#  crprint - prints first and last 25 pages of object code or executable
#          - prints first 10 pages of source code
#
#  Arguments:
#
#    $1 - object or executable file
#    $2 - source code file
#
#  Example:
#
#    crprint $ING_BUILD/bin/iimerge $ING_SRC/back/scf/scs/scsqncr.c
#
#  Output:
#
#    dmp1.lst - listing of $1 in dump format
#    dmp2.lst - listing of $2 in source format
#
#    After running crprint, dmp1.lst and dmp2.lst are suitable for printing
#    for copyright registration.
#
#  Arguments for the dmp command:
#
#    $1 - filename to be printed
#
#    $2 - type of print:
#         1 - dump
#         2 - source code
#
#    $3 - pages to print
#
#    $4 - lines per page
#
#    $5 - lines to skip when printing source code
#
skip_lines=0
dmp $1 1 25 60 >dmp1.lst
dmp $2 2 10 60 $skip_lines >dmp2.lst

