/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wcsdb.h>
#include <wsf.h>
#include <erwsf.h>
#include <ddgexec.h>
#include <ddfsem.h>

/*
**
**  Name: wcsdb.c - Database connection manager to extract document
**
**  Description:
**		Logic: Some specific connection are used to extract files from the 
**		repository. This number of connection is limited by wps_doc_max_session.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warning on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Jan-2003 (hweho01)
**          For hybrid build 32/64 on AIX, the 64-bit shared lib   
**          oiddi needs to have suffix '64' to reside in the   
**          the same location with the 32-bit one, due to the alternate 
**          shared lib path is not available in the current OS.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**/

static u_i2		  wps_doc_mem_tag;
static i4	  wps_doc_max_session = 1;
static u_i4	  wps_doc_nb_session = 0;
static WPS_PDOC	  wps_doc_list = NULL;
static SEMAPHORE  wps_doc_access;
static SEMAPHORE  wps_doc_empty;
static DDG_DESCR  descr;

/*
** Name: WCSDBInitialize() - 
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDBInitialize() 
{
	GSTATUS err = GSTAT_OK;
	char	*size = NULL;
	char	szConfig[256];
	char	*dictionary_name = NULL;
	char	*node = NULL;
	char	*svrClass = NULL;
	char	*dllname = NULL;
	char	*hostname = NULL;
	char	*v = NULL;

	hostname = PMhost();
	CVlower(hostname);

	STprintf (szConfig, CONF_REPOSITORY, hostname);
	if (PMget( szConfig, &dictionary_name ) != OK && dictionary_name == NULL)
		err = DDFStatusAlloc(E_WS0042_UNDEF_REPOSITORY);

#if defined(any_aix) && defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
	STprintf (szConfig, CONF_DLL64_NAME, hostname);
#else
	STprintf (szConfig, CONF_DLL_NAME, hostname);
#endif   /* aix */
	if (err == GSTAT_OK && PMget( szConfig, &dllname ) != OK && dllname == NULL)
		err = DDFStatusAlloc(E_WS0043_UNDEF_DLL_NAME);

	STprintf (szConfig, CONF_NODE, hostname);
	if (err == GSTAT_OK)
		PMget( szConfig, &node );

	STprintf (szConfig, CONF_CLASS, hostname);
	if (err == GSTAT_OK)
		PMget( szConfig, &svrClass );

	err = DDGInitialize(2, dllname, node, dictionary_name, svrClass, NULL, NULL, &descr);
	if (err == GSTAT_OK)
	{
		err = DDFSemCreate(&wps_doc_access, "WCSDB access");
		if (err == GSTAT_OK)
			err = DDFSemCreate(&wps_doc_empty, "WCSDB empty");
		if (err == GSTAT_OK)
			err = DDFSemOpen(&wps_doc_empty, TRUE);
		if (err == GSTAT_OK)
		{
			wps_doc_mem_tag = MEgettag();
			STprintf (szConfig, CONF_DOCSESSION_SIZE, PMhost());
			if (PMget( szConfig, &size ) == OK && size != NULL)
				CVan(size, &wps_doc_max_session);
		}
	}
return(err);
}

/*
** Name: WCSDBTerminate() - 
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDBTerminate() 
{
	GSTATUS err = GSTAT_OK;
	WPS_PDOC tmp = NULL;

	G_ASSERT(wps_doc_max_session == 0, E_WS0054_WCS_DB_NOT_INIT);

	err = DDFSemOpen(&wps_doc_access, TRUE);
	if (err == GSTAT_OK) 
	{
		while (wps_doc_list != NULL)
		{
			tmp = wps_doc_list;
			wps_doc_list = wps_doc_list->next;
			err = DDGDisconnect(&tmp->ddg_session);
			MEfree((PTR)tmp);
		}
		wps_doc_max_session = 0;
		DDFSemClose(&wps_doc_access);
	}
	DDFSemDestroy(&wps_doc_empty);
	DDFSemDestroy(&wps_doc_access);
return(err);
}

/*
** Name: WCSDBDisconnect() - 
**
** Description:
**	virtual disconnection. The session will not be disconnected but rather spooling.
**	If someone is waiting for a session a message will be send to him by release the semaphore
**
** Inputs:
**	WPS_PDOC session
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDBDisconnect(
	WPS_PDOC session) 
{
	GSTATUS err = GSTAT_OK;

	G_ASSERT(wps_doc_max_session == 0, E_WS0054_WCS_DB_NOT_INIT);

	err = DDFSemOpen(&wps_doc_access, TRUE);
	if (err == GSTAT_OK) 
	{
		if (wps_doc_list == NULL)
			DDFSemClose(&wps_doc_empty);
		session->next = wps_doc_list;
		wps_doc_list = session;
		DDFSemClose(&wps_doc_access);
	}
return(err);
}

/*
** Name: WCSDBConnect() - 
**
** Description:
**	Check in the spool if a session is available. If not and the maximum of session
**	is not reached, the function will open another connection to the database.
**	Else the function will wait/
**
** Inputs:
**	WPS_PDOC *session
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WCSDBConnect(
	WPS_PDOC *session) 
{
	GSTATUS err = GSTAT_OK;
	WPS_PDOC tmp = NULL;

	G_ASSERT(wps_doc_max_session == 0, E_WS0054_WCS_DB_NOT_INIT);

	err = DDFSemOpen(&wps_doc_access, TRUE);
	if (err == GSTAT_OK) 
	{
		tmp = wps_doc_list;
		if (tmp != NULL)
		{
			wps_doc_list = wps_doc_list->next;
			if (wps_doc_list == NULL)
				err = DDFSemOpen(&wps_doc_empty, TRUE);

			DDFSemClose(&wps_doc_access);
		}
		else
		{
			if (wps_doc_max_session <= wps_doc_nb_session)
			{
				DDFSemClose(&wps_doc_access);
				err = DDFSemOpen(&wps_doc_empty, TRUE);
				if (err == GSTAT_OK)
				{
					DDFSemClose(&wps_doc_empty);
					err = WCSDBDisconnect(*session);
				}
			}
			else
			{
				DDFSemClose(&wps_doc_access);
				err = G_ME_REQ_MEM(wps_doc_mem_tag, tmp, WPS_DOC, 1);
				if (err == GSTAT_OK)
				{
					err = DDGConnect(&descr, NULL, NULL, &(tmp->ddg_session));
					tmp->next = NULL;
					wps_doc_nb_session++;
				}
			}
		}
	}
	if (err == GSTAT_OK)
		*session = tmp;
return(err);
}

