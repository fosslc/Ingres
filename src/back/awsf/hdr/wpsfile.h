/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998 (fanra01)
**	    Corrected incomplete last line.
**      28-Apr-2000 (fanra01)
**          Bug 100312
**          Add SSL support.  Modified WPSRequest prototype to include
**          scheme string like http; https etc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WSM_FILE_INCLUDED
#define WSM_FILE_INCLUDED

#include <wcs.h>

typedef struct __WPS_FILE {
	char*				name;
	char*				key;
	i4			timeout_end;
	u_i4				used;
	u_i4				count;
	char				*url;
	WCS_FILE		*file;
	struct __WPS_FILE *previous;
	struct __WPS_FILE *next;
} WPS_FILE, *WPS_PFILE;

GSTATUS
WPSRequest (	
	char*		 key, 
	char*		 scheme,
	char*		 host,
	char*		 port,
	char*		 location_name,
	u_i4		 unit,
	char*		 name, 
	char*		 suffix,
	WPS_PFILE	 *file,
	bool		 *status);

GSTATUS
WPSRelease (	
	WPS_PFILE	*file);

GSTATUS
WPSRefreshFile (	
	WPS_PFILE file,
	bool	  checkAdd);

GSTATUS
WPSRemoveFile (	
	WPS_PFILE *file,
	bool		checkAdd);

GSTATUS
WPSCleanFilesTimeout (	
	char*		key);

GSTATUS
WPSBrowseFiles(
	PTR		*cursor,
	WPS_PFILE *file);

GSTATUS 
WPSFileInitialize ();

GSTATUS 
WPSFileTerminate ();
 
#endif
