/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : serseq.h, header File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : Schalk Philippe (schph01)
** Purpose  : Load / Store Sequence in the compound file.
**           
** History:
**
** 07-May-2003 (schph01)
**    SIR #107523 Manage Sequence Objects
*/

#if !defined (_VSDSERIALIZE_SEQUENCE_HEADER)
#define _VSDSERIALIZE_SEQUENCE_HEADER
#include "snaparam.h"
#include "compfile.h"


BOOL VSD_StoreSequence (IStorage* pIParent, CaVsdStoreSchema& storeSchema, CaCompoundFile& cmpFile);

#endif // _VSDSERIALIZE_SEQUENCE_HEADER

