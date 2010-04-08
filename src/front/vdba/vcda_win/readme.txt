/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : readme.txt : documentation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of tab control to display the list of differences
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
*/

========================================================================
		ActiveX Control DLL : VCDA
========================================================================

!!!!  Documentation of ipm.ocx       !!!!!!!!!!!!!!

********* II_SYSTEM *******
ii.*.c2.audit_log_1:                    'C:\\IngresII\\ingres\\files\\audit.1'
ii.*.c2.audit_log_2:                    'C:\\IngresII\\ingres\\files\\audit.2'
ii.*.c2.log.audit_log_1:                'C:\\IngresII\\ingres\\files\\audit.1'
ii.*.c2.log.audit_log_2:                'C:\\IngresII\\ingres\\files\\audit.2'
ii.uk-so01.gcf.mechanism_location:      'C:\\IngresII\\ingres\\lib'
ii.uk-so01.rcp.log.log_file_1:          'C:\\IngresII'
II_DATABASE
II_DUMP
II_WORK
II_DATABASE

********* HOST ************ (if a host name is part of value is it affected by "ignore host"?)
all ii.<hostname>.XXXXXXX:
ii.*.config.server_host:                UK-SO01
ii.uk-so01.gcn.local_vnode:             UK-SO01

********* USER ************
ii.uk-so01.privileges.user.<username>: SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED


********* II (Installation ID, environment variable only where xx is replaced by II) ************
II_BIND_SVC_xx
II_CHARSETxx
II_GCNxx_PORT
II_GCNxx_LCL_VNODE
II_GCx_TRACE
