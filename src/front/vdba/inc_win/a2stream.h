/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : a2stream.h: interface for the Archive & IStream manipulation
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Transfer the CArchive to/from IStream
**
** History:
**
** 29-Sep-2000 (uk$so01)
**    Created
**/


#if !defined (ARCHIVE_2_STREAM_HEADER)
#define ARCHIVE_2_STREAM_HEADER
#include <ole2.h>
#include "dmlbase.h"
#include "qryinfo.h"

BOOL CArchiveToIStream  (CArchive& ar, IStream** ppStream);
BOOL CArchiveFromIStream(CArchive& ar, IStream*  pStream);

BOOL CObjectToIStream     (CObject* pObj, IStream** ppStream);
BOOL CObjectFromIStream   (CObject* pObj, IStream*  pStream);
BOOL CaDBObjectToIStream  (CaDBObject* pObj, IStream** ppStream);
BOOL CaDBObjectFromIStream(CaDBObject* pObj, IStream*  pStream);

BOOL CaDBObjectListToIStream    (CTypedPtrList< CObList, CaDBObject* >* pList, IStream** ppStream);
BOOL CaDBObjectListFromIStream  (CTypedPtrList< CObList, CaDBObject* >* pList, IStream*  pStream);


#endif // ARCHIVE_2_STREAM_HEADER