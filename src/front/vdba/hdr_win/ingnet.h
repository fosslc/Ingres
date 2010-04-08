/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : ingnet.h
//
//     
//
********************************************************************/
#ifndef _INGNET_H
#define _INGNET_H

#define GCN_MODIF_NORMAL 0
#define GCN_MODIF_ALTER  1
#define GCN_MODIF_DELETE 2

//#ifdef WIN32
int DBA_gcn_u_n(LPVNODESEL vnodesel);
int DBA_gcn_u_a(LPVNODESEL vnodesel);
int DBA_gcn_u_n_Modify(LPSTR lpszVNode, LPSTR lpszRemote, LPSTR lpszNetware, LPSTR lpszPort, int iaction);
int DBA_gcn_u_a_Modify(LPSTR lpszVNode, LPSTR lpszUser, LPSTR lpszPassword, int iaction);
int DBA_GCN_Init(void);
//#endif

#endif      // _INGNET_H
