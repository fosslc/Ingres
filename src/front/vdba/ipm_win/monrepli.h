/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : monrepli.h, Header file 
**    Project  : INGRES II/ Monitoring.
**    Author   : schph01
**    Purpose  : 
**
** History:
**
** xx-Feb-1997 (schph01)
**    Created
*/

#ifndef _MONREPLI_H
#define _MONREPLI_H
#include "repitem.h"
#define MAX_FLAGS   26
class CdIpmDoc;

/***************************** Monitoring Replication *************** */

// Read runrepl.opt files and fill class CaReplicationItemList
int GetReplicServerParams   (CdIpmDoc* pIpmDoc, LPREPLICSERVERDATAMIN lpReplicServerDta , CaReplicationItemList *alist );
   // Write structure REPLSERVPARAM in runrepl.opt
int SetReplicServerParams   (CdIpmDoc* pIpmDoc, LPREPLICSERVERDATAMIN lpReplicServerDta , CaReplicationItemList *pList );
   // Read File Replicat.log
void GetReplicStartLog       (LPREPLICSERVERDATAMIN lpReplicServerDta, CString *csAllLines );
/**** Start and Stop the replicator server ****/
int StartReplicServer       (int hdl, LPREPLICSERVERDATAMIN lpReplicServerDta );
int StopReplicServer        (int hdl, LPREPLICSERVERDATAMIN lpReplicServerDta );
int MonitorReplicatorStopSvr (int hdl, int svrNo ,LPUCHAR lpDbName  ); // Fonction in SqlTls.SC

void VerifyAndUpdateVnodeName (CString *rcsRunNode);

#endif   // _MONREPLI_H

