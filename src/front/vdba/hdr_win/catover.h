/*
**  Copyright (C) 2005-2010 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : catover.h
**
**     
** History:
**	02-feb-2004 (somsa01)
**	    ver.h is obsolete; use winver.h instead.
**	08-Jan-2007 (drivi01)
**	    Update copyright notice.
**	03-Jan-2008 (drivi01)
**	    Update copyright notice.
**	12-Feb-2009 (bonro01)
**	    Update copyright notice.
**	12-Feb-2010 (frima01) SIR 123269
**	    Update copyright notice to 2010 for Ingres 10.
*/

#include <winver.h>

#define _PRODUCTVERSION_        1,0,0,0
#define _PRODUCTVERSIONSTRING_  "1.0.0\0"
#define _PRODUCTGENLEVELSTRING_ "GA 1\0"

#ifdef DEBUG
	#define _FILEFLAGS_             VS_FF_DEBUG | VS_FF_PRERELEASE
	#define _FILEFLAGSMASK_         VS_FF_DEBUG | VS_FF_PRERELEASE
#else
	#define _FILEFLAGS_             0
	#define _FILEFLAGSMASK_         0
#endif

#define _COMPANYNAME_           "Ingres Corporation.\0"
#define _LEGALCOPYRIGHT_        "Copyright \251 2005-2010 Ingres Corporation. All Rights Reserved.\0"
#define _PRODUCTNAME_           "DBADLG1\0"


