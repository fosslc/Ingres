/*****************************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/Ingres Visual Manager
**
**    Source : getinst.h
**    
**    -low-level functions for getting the list of instances of started ingres components,
**     initially derived from the ingstop.c source
**     adapted by : Noirot-Nerin Francois
**    - misc low level functions / classes
** 
** 27-Jan-99 (noifr01)
**	    created
** 28-May-2002 (noifr01)
**   (sir 107879) moved the prototype of the IVM_GetFileSize() function
*******************************************************************************************/


BOOL UpdInstanceLists(void);
BOOL LLGetComponentInstances (CaTreeComponentItemData* pComponent,
							  CTypedPtrList<CObList, CaTreeComponentItemData*>& listInstance);
BOOL IVM_RemoveFile(char * filename);

class FileContentBuffer
{
public:
	FileContentBuffer(char * filename, long lmaxbufsize, BOOL bEndOnlyIfTooBig = FALSE);
	~FileContentBuffer();
	char * GetBuffer();
protected:
	char   m_filename[_MAX_PATH];
	char * m_pbuffer;
	BOOL   m_bEndBytesOnly;
	BOOL   m_bWasTruncated2MaxBufSize;
};

extern BOOL bGetInstMsgSpecialInstanceDisplayed;

BOOL GetAndFormatIngMsg(long lID, char * bufresult, int bufsize);

