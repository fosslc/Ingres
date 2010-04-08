$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iirunrmcmd.com -- called by ingstart to launch the INGRES
$!	remote command process. 
$!
$!  Usage:
$!      @iirundbms uic ii_installation
$!
$!  Description:
$!	launches rmcmd.exe 
$!
$!------------------------------------------------------------------------
$!
$ on error then continue
$ run ii_system:[ingres.bin]rmcmd.exe
