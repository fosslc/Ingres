$!******************************************************************
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!    Project : Ingres Visual DBA
$!
$!    Source : rmcmd.com
$!    remote commands daemon startup command file
$!
$!
$!! History:
$!!
$!!      25-Jan-96 (boama01)
$!!              Created for VMS.
$!!	20-apr-98 (mcgem01)
$!!		Product name change to Ingres.
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!      25-jun-2003 (horda03)
$!!	    Set UIC to Ingres sysadmin UIC.
$!!	10-dec-2003 (abbjo03)
$!!	    Do not restore verify status because it cause VDBA to hang.  
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!******************************************************************
$!
$ set noon
$ ver = f$verify(0)
$!
$! STARTUP COMFILE FOR RMCMD DAEMON (DRIVER FOR VISUAL DBA COMMAND EXECUTION)
$!
$! This file should be customized after Visual DBA has been installed on
$! the OpenVMS machine which will support the Ingres installation, and
$! before the Remote Command Daemon (RMCMD) is started.  It must reside in
$! the [.BIN] subdirectory of the Ingres installation.
$!
$! The location of the Ingres installation (and thus the setting of
$! II_SYSTEM and II_INSTALLATION) is assumed to be the same as that in effect
$! for the INGRES user when starting the Daemon (by running RMCMD).  However,
$! those settings only serve to locate this command file; appropriate logical
$! and symbol setup is still necessary to initialize the Daemon's Ingres
$! environment.
$!
$! Insert any commands here which would be necessary for the Daemon to set
$! up access to the desired installation; for example,
$!          $ DEF II_SYSTEM/EXEC/JOB/TRAN=CONC $1$DUA0:[OPING11.]
$!          $ @II_SYSTEM:[INGRES]INGSYSDEF  (or INGDBADEF)
$!
$ sho proc
$!
$ @ii_system:[ingres]ingsysdef
$ iigetres ii.'F$GETSYI("NODENAME").*.*.vms_uic vms_uic
$ vms_uic = F$TRNLNM( "vms_uic" ) - "[" - "]"
$
$ name = F$ELEMENT( 1, ",", vms_uic )
$
$ IF name .EQS. ","
$ THEN
$    name = vms_uic
$ ENDIF
$
$ uic_int = F$IDENTIFIER( name, "NAME_TO_NUMBER" )
$
$ IF uic_int .EQ. "0"
$ THEN
$    uic = F$FAO( "[!AS]", vms_uic)
$ ELSE
$    uic = F$FAO("!%U", uic_int)
$ ENDIF
$
$ SET UIC 'uic'
$
$! Restoring the verify status, e.g., ver = f$verify(ver), causes hanging
$! so just exit.
$ exit
