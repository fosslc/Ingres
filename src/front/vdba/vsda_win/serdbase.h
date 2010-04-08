/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : serdbase.h, Header File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Database in the compound file.
**           
** History:
**
** 18-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#if !defined (_VSDSERIALIZE_DATABASE_HEADER)
#define _VSDSERIALIZE_DATABASE_HEADER
#include "snaparam.h"
#include "compfile.h"

BOOL VSD_StoreDatabase (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile, BOOL bInstallation);


#endif // _VSDSERIALIZE_DATABASE_HEADER
