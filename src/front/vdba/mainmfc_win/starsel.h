/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : starsel.h
**
**  Project  : CA-OpenIngres
**
**  Author   : Emmanuel Blattes
**
**  Purpose  : Specific STAR statements
**
**  History:
**	16-feb-2000 (somsa01)
**	    properly commented STARSEL_HEADER.
*/

#ifndef STARSEL_HEADER
#define STARSEL_HEADER

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
}

typedef struct StarSourceInfo
{
  UCHAR DBName[MAXOBJECTNAME];
  UCHAR objectname[MAXOBJECTNAME];
  UCHAR szSchema[MAXOBJECTNAME];

  BOOL bRegisteredAsLink;
  char objType;           // 'I' for index, 'V' for view, 'P' for procedure

  char szLdbObjName[33];
  char szLdbObjOwner[33];
  char szLdbDatabase[33];
  char szLdbNode[33];
  char szLdbDbmsType[33];

} STARSOURCEINFO, FAR * LPSTARSOURCEINFO;

int GetStarObjSourceInfo (LPUCHAR NodeName, LPSTARSOURCEINFO lpSrcInfo);

#endif /* STARSEL_HEADER */
