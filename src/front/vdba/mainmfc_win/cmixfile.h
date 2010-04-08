/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cmixfile.h, Header file.
** 
**  
**    Project  : CA-OpenIngres/VDBA - Monitoring.
** 
** History since 14-Apr-2004:
** 
** 14-Apr-2004: (uk$so01)
**    BUG #112150 /ISSUE 13350768 
**    VDBA, Load/Save fails to function correctly with .NET environment.
*/

#ifndef CMIXFILE_HEADER
#define CMIXFILE_HEADER
class CMixFile : public CStdioFile
{
public:
   UINT Read(void* lpBuf, UINT nCount);
   void Write(const void* lpBuf, UINT nCount);
	ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
};

extern CMixFile * GetCMixFile();

#endif

