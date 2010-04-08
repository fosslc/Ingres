$! Copyright (c) 2004 Ingres Corporation
$!
$! INGDBADEF.COM -- Define INGRES DBA symbols.
$!
$!!	11-jun-1998 (kinte01)
$!!	    Cross integrate change 435890 from oping12
$!!          18-may-1998 (horda03)
$!!            X-Integrate Change 427896 to enable Evidence Sets.
$!!      24-apr-2001 (chash01)
$!!          Add gwalias and gwsetlogin for oracle gateway 
$!!	23-may-2001 (kinte01)
$!!	    gwalias and gwsetlogin are scripts instead of executables
$!!	    and need to be called accordingly
$!!	20-nov-2002 (abbjo03)
$!!	    Remove finddbs.
$!!	21-nov-2002 (abbjo03)
$!!	    Remove idbg, iiexcept and mkexcept since they are already included
$!!	    in ingsysdef.com.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!     20-jan-2010 (horda03) B123119
$!!         Add adbtofst.
$!! 
$!
$ @ii_system:[ingres]ingusrdef	! Include user symbols.
$ ii_installation = f$trnlnm("II_INSTALLATION")
$!
$ adbtofst      :== @ii_system:[ingres.bin]adbtofst.com
$ alterdb	:== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe alterdb
$ auditdb	:== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe auditdb
$ cacheutil	:== $ii_system:[ingres.bin]cacheutil.exe
$ ckpdb		:== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe ckpdb
$ createdb	:== $ii_system:[ingres.bin]createdb.exe
$ destroydb	:== $ii_system:[ingres.bin]destroydb.exe
$ ingchkdate	:== $ii_system:[ingres.bin]cichkcap.exe -D
$ ipm		:== $ii_system:[ingres.bin]ipm.exe
$ iimonitor	:== $ii_system:[ingres.bin]iimonitor.exe
$ infodb	:== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe infodb
$ lartool	:== $ii_system:[ingres.bin]lartool.exe
$ lockstat	:== $ii_system:[ingres.bin]lockstat.exe
$ logdump	:== $ii_system:[ingres.bin]logdump.exe
$ logstat	:== $ii_system:[ingres.bin]logstat.exe
$ optimizedb	:== $ii_system:[ingres.bin]optimizedb.exe
$ relocatedb	:== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe relocdb
$ rollforwarddb :== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe rolldb
$ fastload	:== $ii_system:[ingres.bin]dmfjsp'ii_installation.exe fastload
$ statdump	:== $ii_system:[ingres.bin]statdump.exe
$ sysmod	:== $ii_system:[ingres.bin]sysmod.exe
$ upgradedb	:== $ii_system:[ingres.bin]upgradedb.exe
$ verifydb	:== $ii_system:[ingres.bin]verifydb.exe
$ gwalias       :== @ii_system:[ingres.utility]gwalias.com
$ gwsetlogin    :== @ii_system:[ingres.utility]gwsetlogin.com
$!
$ write sys$output "INGRES Database Administration Symbols Defined."
$ write sys$output ""
