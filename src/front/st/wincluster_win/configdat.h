/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  History:
**	25-may-2004 (wonst02)
**	    Created.
*/

#pragma once
# include "resource.h"		// main symbols
# include <compat.h>
# include <er.h>
# include <lo.h>
# include <pm.h>
# include <pmutil.h>
# include <si.h>
# include <st.h>
# include <nm.h>
# include <util.h>

class WcConfigDat
{
public:
	WcConfigDat(void);
	~WcConfigDat(void);

private:
	bool verbose;
	PM_CONTEXT *pm_id;
	PM_SCAN_REC state;
	char	    change_log_buf[ MAX_LOC + 1 ];	
	FILE	    *change_log_fp;
	LOCATION    change_log_file;

public:
	// Public methods
	bool Init(void);
	// Scan for resource names matching the regular expression.
	bool ScanResources(LPSTR lpszPattern, LPCSTR from, LPCSTR to);
	bool WcConfigDat::SetResource(char *name, char *value);
	bool Term(void);
};
