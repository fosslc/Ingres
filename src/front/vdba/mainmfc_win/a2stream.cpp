/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : a2stream.cpp: implementation for the Archive & IStream manipulation
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Transfer the CArchive to/from IStream
**
** History:
**
** 29-Sep-2000 (uk$so01)
**    Created
** 19-Mar-2002 (uk$so01)
**    SIR #106648, Enhance the library.
**/


#include "stdafx.h"
#include "a2stream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define IO_BUFFERSIZE  1024


BOOL CArchiveToIStream  (CArchive& ar, IStream** ppStream)
{
	BOOL bOK = FALSE;
	ULONG ulRead, ulWritten;
	BYTE aBuffer[IO_BUFFERSIZE]; 
	ASSERT(ar.IsLoading());
	*ppStream = NULL;
	if (ar.IsLoading()) 
	{
		long lSize;
		ar >> lSize;
		if (lSize > 0)
		{
			COleStreamFile file;
			bOK = file.CreateMemoryStream();
			if (!bOK)
				return FALSE;
			IStream* pStream = file.GetStream();

			while (lSize > 0)
			{
				if (lSize < IO_BUFFERSIZE)
					ulRead = lSize;
				else
					ulRead = IO_BUFFERSIZE;

				ar.Read((void*)aBuffer, ulRead);
				pStream->Write((const void*)aBuffer, ulRead, &ulWritten);
				ASSERT(ulWritten == ulRead);

				lSize -= ulRead;
			}
			*ppStream = pStream;
			file.Detach();
		}
		bOK = TRUE;
	}

	return bOK;
}


BOOL CArchiveFromIStream(CArchive& ar, IStream* pStream)
{
	BOOL bOK = FALSE;
	ASSERT (ar.IsStoring());
	ULONG ulRead, ulRemain, ulWritten = 0;
	BYTE aBuffer[IO_BUFFERSIZE]; 
	if (ar.IsStoring()) 
	{
		if (!pStream)
		{
			ar << (int)0;
			return FALSE;
		}

		COleStreamFile file (pStream);
		file.SeekToBegin();
		STATSTG statstg;
		HRESULT hr = pStream->Stat( &statstg,STATFLAG_NONAME );
		if (FAILED(hr))
		{
			ar << (int)0;
			file.Detach();
			return FALSE;
		}

		ulRemain  = statstg.cbSize.LowPart;
		ar << ulRemain;

		while (ulRemain > 0)
		{
			hr = pStream->Read (aBuffer, min(ulRemain, IO_BUFFERSIZE), &ulRead);
			ulRemain -= ulRead;

			if (hr != S_OK)
				break; // End of stream or Error;
			else
			{
				ar.Write ((const void*)aBuffer, ulRead);
				ulWritten += ulRead;
			}
		}
		bOK = TRUE;
		file.Detach();
	}
	return bOK;
}



BOOL CObjectToIStream (CObject* pObj, IStream** ppStream)
{

	BOOL bOK = FALSE;
	*ppStream = NULL;
	COleStreamFile file;
	bOK = file.CreateMemoryStream();
	if (!bOK)
		return FALSE;

	CArchive ar(&file, CArchive::store);
	file.SeekToBegin();
	pObj->Serialize (ar);
	ar.Flush();
	*ppStream = file.Detach();

	return bOK;
}

BOOL CObjectFromIStream(CObject* pObj, IStream*  pStream)
{
	BOOL bOK = TRUE;
	COleStreamFile file (pStream);
	CArchive ar(&file, CArchive::load);
	file.SeekToBegin();
	pObj->Serialize (ar);
	ar.Flush();
	file.Detach();
	return bOK;
}


BOOL CaDBObjectToIStream (CaDBObject* pObj, IStream** ppStream)
{
	BOOL bOk = FALSE;
	CMemFile file;
	CArchive ar(&file, CArchive::store);
	pObj->Serialize (ar);
	ar.Flush();

	bOk = CArchiveToIStream (ar, ppStream);
	return bOk;
}

BOOL CaDBObjectFromIStream(CaDBObject* pObj, IStream*  pStream)
{
	BOOL bOk = FALSE;
	CMemFile file;
	CArchive ar(&file, CArchive::load);
	bOk = CArchiveFromIStream(ar, pStream);
	if (bOk)
	{
		file.SeekToBegin();
		pObj->Serialize(ar);
	}
	return bOk;
}


BOOL CaDBObjectListToIStream (CTypedPtrList< CObList, CaDBObject* >* pList, IStream** ppStream)
{
	BOOL bOk = FALSE;
	CMemFile file;
	CArchive ar(&file, CArchive::store);
	pList->Serialize (ar);
	ar.Flush();

	bOk = CArchiveToIStream (ar, ppStream);
	return bOk;
}

BOOL CaDBObjectListFromIStream  (CTypedPtrList< CObList, CaDBObject* >* pList, IStream*  pStream)
{
	BOOL bOk = FALSE;
	CMemFile file;
	CArchive ar(&file, CArchive::load);
	bOk = CArchiveFromIStream(ar, pStream);
	if (bOk)
	{
		file.SeekToBegin();
		pList->Serialize(ar);
	}
	return bOk;
}


