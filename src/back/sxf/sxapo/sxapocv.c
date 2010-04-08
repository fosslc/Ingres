/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <cs.h>
# include    <st.h>
# include    <er.h>
# include    <ex.h>
# include    <tr.h>
# include    <lk.h>
# include    <me.h>
# include    <sa.h>
# include    <adf.h>
# include    <scf.h>
# include    <tm.h>
# include    <dmf.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    "sxapoint.h"

i4		 sxapo_acctype_to_msgid(SXF_ACCESS acctype);
i4		 sxapo_evtype_to_msgid(SXF_EVENT evtype);
VOID		 sxapo_msgid_default(i4 msgid,DB_DATA_VALUE  *desc);
VOID 	 	 sxapo_priv_to_str(i4 priv,DB_DATA_VALUE *pstr,bool term);

static DB_STATUS sxapo_alloc_msgdesc();
static VOID sxapo_incr( CS_SEMAPHORE *sem, i4 *counterp );
static DB_STATUS sxapo_add_msgid(i4 msgid, DB_DATA_VALUE  *desc);
static STATUS   sxa_cvt_handler( EX_ARGS *ex_args );
static STATUS 	sxa_check_adferr( ADF_CB *adf_scb );

static CS_SEMAPHORE SXapomsg_sem ZERO_FILL; /* Semaphore to protect 
					       message list*/
static CS_SEMAPHORE SXapo_sem ZERO_FILL; /* Semaphore to protect 
						general increments*/
static SXAPOSTATS  SXapostats;
/*
**	This is the root of the message id cache
*/
static SXAPO_MSG_DESC *mroot=NULL;
/*
** Name: SXAPOCVT.C	- Conversion routines for SXAPO
**
** Description:
**	This file contains the routines which do the conversion between
**	SXF data and the formats for SA.
**
** History:
**	24-dec-93 (stephenb)
**	    Converted from gwsxacv.c, stole most of the routines "as is" and
**	    renamed them, following history is from that file.
**    	    17-sep-92 (robf)
**	    	Created
**	    20-nov-92 (robf)
**	    	Added handling for objectowner column. 
**	    17-dec-92 (robf)
**	    	Updated error handling.
**	    14-jul-93 (ed)
**	    	replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	24-sep-1997 (hanch04)
**	    cast NULL to (CL_ERR_DESC *) remove compile errors
*/

/*
** Name: sxapo_msgid_to_desc - converts a message id to text
**
** Description:
**	The main reason for this routine is to provide caching of
**	commonly used messages to increasing performance (we could use
**	"fast" messages in ER but initially it was decided to do the
**	caching in the SXF instead.)
**
**	If the message id can't be found an error is issued ONCE, then
**	a default message is provided for subsequent usage
**
**	This uses the SXapomsg_sem semaphore to make sure that
**	another session doesn't change the underlying tree while walking
**	or updating it.
**
** Inputs:
**	msgid	The SXF message id that needs to be converted
**
** Outputs:
**	desc	The converted description
**
** Returns:
**	E_DB_OK if sucessful
**
** History:
**	17-sep-92 (robf)
**	    Created
**	4-jan-94 (stephenb)
**	    Renamed sxapo_msgid_to_desc to be used in sxapo.
*/
DB_STATUS
sxapo_msgid_to_desc(i4 msgid, DB_DATA_VALUE *desc)
{
	DB_STATUS status=E_DB_OK;
	STATUS    semstat;
	i4 local_error;

	/*
	**	Get exclusive access to the message heap
	*/
	semstat = CSp_semaphore(TRUE, &SXapomsg_sem);
	if(semstat!=OK)
	{
		ule_format(semstat, (CL_ERR_DESC*) NULL,
			ULE_LOG, NULL, 0, 0, 0, &local_error, 0);
		return E_DB_FATAL;
	}
	/*
	**	Increment lookup counter
	*/
	sxapo_incr(&SXapo_sem, &SXapostats.sxapo_num_msgid_lookup);
	/*
	**	First try to lookup the message id in the cache
	*/
	status=sxapo_find_msgid(msgid,desc);

	if (status!=E_DB_OK)
	{
		status=sxapo_add_msgid(msgid,desc);
	}
	/*
	**	Release semaphore
	*/
	semstat = CSv_semaphore( &SXapomsg_sem);
	if(semstat!=OK)
	{
		ule_format(semstat, (CL_ERR_DESC*) NULL,
			ULE_LOG, NULL, 0, 0, 0, &local_error, 0);
		return E_DB_FATAL;
	}
	return E_DB_OK;
}

/*
** Name: sxapo_find_msgid - finds a message id in the cache.
**
** Description:
**	Searches the cache for a message id, if found fills in desc
**	with the description of the message.
**
** Inputs:
**	msgid	The SXF message id that needs to be converted
**
** Outputs:
**	desc	The converted description
**
** Returns:
**	E_DB_OK    if sucessful
**	E_DB_WARN if not found
**
** History:
**	17-sep-92 (robf)
**	    Created
**	4-jan-94 (stephenb)
**	    Renamed sxapo_find_msgid to be used in sxapo.
*/
DB_STATUS
sxapo_find_msgid(i4 msgid,DB_DATA_VALUE *desc)
{
	SXAPO_MSG_DESC *m;

	m=mroot;
	while(m!=NULL)
	{
		if (m->msgid==msgid)
			break;
		else if (msgid<m->msgid)
			m=m->left;
		else 
			m=m->right;
	}
	if (m==NULL)
	{
		/*
		**	Not found, so return
		*/
		return E_DB_WARN;
	}
	/*
	**	Message Id found in the cache so convert the
	**	data.
	*/
	desc->db_length=m->db_length;
	MEcopy(m->db_data,m->db_length,desc->db_data);
	return E_DB_OK;
}

/*
** Name: sxapo_add_msgid - adds a message id to the cache.
**
** Description:
**	Adds a new message id to the cache. If ERslookup can't find
**	the message id, then it issues a single diagnostic and creates
**	a default message for later usage.
**
** Inputs:
**	msgid	The SXF message id that needs to be converted
**
** Outputs:
**	desc	The converted description
**
** Returns:
**	E_DB_OK    if sucessful
**	E_DB_WARN if not found
**
** History:
**	17-sep-92 (robf)
**	    Created
**	26-oct-92 (andre)
**	    replaced calls to ERlookup() with calls to ERslookup();
**	    instead of generic error, ERslookup() returns sqlstate
**	4-jan-94 (stephenb)
**	    renamed sxapo_add_msgid to be used in sxapo.
*/
static DB_STATUS
sxapo_add_msgid(i4 msgid, DB_DATA_VALUE  *desc)
{
	SXAPO_MSG_DESC *m;
	SXAPO_MSG_DESC *mnew;
	char	msgdesc[81];
	VOID sxapo_msg_defaut();
	i4		ulecode;
	i4		msglen;
	DB_STATUS	status;
	STATUS	        erstatus;
	CL_ERR_DESC sys_err;
	/*
	**	First walk the tree to find the place where the
	**	data would go
	*/
	m=mroot;
	while(m!=NULL)
	{
		if (m->msgid==msgid)
			break;
		else if (msgid<m->msgid)
		{
			if (m->left==NULL)
				break;
			else
				m=m->left;
		}
		else 
		{
			if (m->right==NULL)
				break;
			else
				m=m->right;
		}
	}
	/*
	**	Sanity check to see if found already
	*/
	if (m!=NULL && m->msgid==msgid)
	{
		desc->db_length=m->db_length;
		desc->db_data=m->db_data;
		return E_DB_OK;
		
	}
	/*
	**	Look up the message text
	*/
	erstatus = ERslookup(msgid, (CL_ERR_DESC *)NULL,
		ER_TEXTONLY, (char *) NULL,
		msgdesc, sizeof(msgdesc), -1,
		&msglen,&sys_err,0,NULL);

	/*
	** If ERslookup failed, get a default text for the message
	** and continue. Always issue a diagnostic just in case.
	*/
	if (erstatus != OK)
	{
		TRdisplay("SXA Gateway: Unable to lookup message id %d (X%x), using default.\n",msgid,msgid);
		sxapo_msgid_default(msgid,desc);
		MEcopy(desc->db_data,sizeof(msgdesc),msgdesc);
	        sxapo_incr(&SXapo_sem, &SXapostats.sxapo_num_msgid_fail);
	}
	sxapo_incr(&SXapo_sem, &SXapostats.sxapo_num_msgid);
	/*
	**	Allocate space for new message
	*/
	status=sxapo_alloc_msgdesc(&mnew);
	if ( status!=E_DB_OK)
	{
		/*
		**	No more memory right now.
		**	so what we do is fill in a "default" value in the
		**	data and return
		*/
		sxapo_msgid_default(msgid,desc);
		return E_DB_OK;
	}
	/*
	**	Save the data in the new pointer and hook into the
	**	tree.
	*/
	MEcopy(msgdesc, sizeof(mnew->db_data), mnew->db_data);
	mnew->msgid=msgid;
	mnew->db_length=STtrmwhite(mnew->db_data);
	mnew->left=NULL;
	mnew->right=NULL;
	if (mroot==NULL)
	{
		mroot=mnew;
	}
	else if (msgid<m->msgid)
	{
		/*
		**	Goes on the left
		*/
		m->left=mnew;
	}
	else 
	{
		/*
		**	Goes on the right
		*/
		m->right=mnew;
	}
	/*
	**	Return the new saved value
	*/
	desc->db_length=mnew->db_length;
	MEcopy(mnew->db_data,desc->db_length,desc->db_data);
	return E_DB_OK;
}
/*
** Name: sxapo_alloc_msgdesc - allocates memory for a new message
**	 description.
**
** Outputs:
**	m 	points to the new memory.
**
** Returns:
**	E_DB_OK    if sucessful
**	E_DB_ERROR if something went wrong
**
** History:
**	17-sep-92 (robf)
**	    Created
**	4-jan-94 (stephenb)
**	    renamed sxapo_alloc_msgdesc to be used in sxapo.
*/
static DB_STATUS
sxapo_alloc_msgdesc(SXAPO_MSG_DESC **m)
{
	DB_STATUS status;
	ULM_RCB		ulm_rcb;

	/*
	**	Allocate a new value from the pool
	*/
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_streamid_p = &Sxf_svcb->sxf_svcb_mem;
	ulm_rcb.ulm_psize=sizeof(SXAPO_MSG_DESC);
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	status=ulm_palloc(&ulm_rcb);

	if (status != E_DB_OK)
	{
		i4		err_code;
		i4		local_err;

		if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		    err_code = E_SX1002_NO_MEMORY;
		else
		    err_code = E_SX106B_MEMORY_ERROR;
		_VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		_VOID_ ule_format(err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		return E_DB_ERROR;

	}
	*m= (SXAPO_MSG_DESC*)ulm_rcb.ulm_pptr;
	return E_DB_OK;
}
/*
** Name: sxapo_msgid_default - Generates a default message for
**       a message id
**	 description.
**
** Description:
**	Generates a substitute message text when for some reason the
**	message description is not available.
**
** Inputs:
**	msgid message id
** Outputs:
**	desc formatted output.
**
** Returns:
**	None
**
** History:
**	17-sep-92 (robf)
**	    Created
**	4-jan-94 (stephenb)
**	    Renamed sxapo_msgid_default to be used in sxapo.
*/
VOID
sxapo_msgid_default(i4 msgid,DB_DATA_VALUE  *desc)
{
	char cmsgid[25];
	STprintf(desc->db_data,"Audit description id %d (X%x)",msgid,msgid);
}


/*
** Name: sxapo_evtype_to_msgid - converts an SXF event type to a msgid for
**	 text string lookup.
**
** Description:
**	This takes an SXF event type and returns an appropriate
**	msgid which can then be used as a key to get a textual
**	description of the event
**
** Inputs:
**	evtype 
**
** Returns:
**	Message ID
**
** History:
**	21-sep-92 (robf)
**	    Created
**	3-feb-94 (stephenb)
**	    Renamed sxapo_evtype_to_msgid for use in sxapo.
*/
i4
sxapo_evtype_to_msgid(SXF_EVENT evtype)
{
	i4 i;
	static struct {
		SXF_ACCESS evtype;
		i4    msgid;
	} evtypes[]= {
		SXF_E_DATABASE,	I_SX2901_DATABASE,
		SXF_E_APPLICATION,	I_SX2902_APPLICATION,
		SXF_E_PROCEDURE,	I_SX2903_PROCEDURE,
		SXF_E_TABLE,	I_SX2904_TABLE,
		SXF_E_LOCATION,	I_SX2905_LOCATION,
		SXF_E_VIEW,	I_SX2906_VIEW,
		SXF_E_RECORD,	I_SX2907_RECORD,
		SXF_E_SECURITY,	I_SX2908_SECURITY,
		SXF_E_USER,	I_SX2909_USER,
		SXF_E_LEVEL,	I_SX2910_LEVEL,
		SXF_E_ALARM,	I_SX2911_ALARM,
		SXF_E_RULE,	I_SX2912_RULE,
		SXF_E_EVENT,	I_SX2913_EVENT,
		SXF_E_ALL,	I_SX2914_ALL,
		SXF_E_INSTALLATION,	I_SX2915_INSTALLATION,
		SXF_E_RESOURCE,		I_SX2916_RESOURCE,
		SXF_E_QRYTEXT,		I_SX2917_QRYTEXT,
		0,		I_SX2900_BAD_EVENT_TYPE
		
	};
	for (i=0; evtypes[i].evtype!= 0; i++)
	{
		if ( evtypes[i].evtype==evtype)
			break;
	}
	return evtypes[i].msgid;
}

/*
** Name: sxapo_acctype_to_msgid - converts an SXF object access type 
**	 to a msgid for text string lookup.
**	This takes an SXF object access type and returns an appropriate
**	msgid which can then be used as a key to get a textual
**	description of the event
**
** Inputs:
**	evtype 
**
** Returns:
**	Message ID
**
** History:
**	21-sep-92 (robf)
**	    Created
**	27-nov-92 (robf)
**	    Added handling for SXF_A_SETUSER 
**	16-jun-93 (robf)
**	     Conditionally include old accesstypes if defined in SXF
**	3-feb-94 (stephenb)
**	    Renamed sxapo_acctype_to_msgid to be used in sxapo.
**	14-feb-94  (robf)
**          Added SXF_A_CONNECT/DISCONNECT
*/
i4
sxapo_acctype_to_msgid(SXF_ACCESS acctype)
{
	i4 i;
	static struct {
		SXF_ACCESS acctype;
		i4    msgid;
	} acctypes[]= {
		SXF_A_SELECT, 	I_SX2801_SELECT,
		SXF_A_INSERT,	I_SX2802_INSERT,
		SXF_A_DELETE,	I_SX2803_DELETE,
		SXF_A_UPDATE,	I_SX2804_UPDATE,
		SXF_A_COPYINTO,	I_SX2805_COPYINTO,
		SXF_A_COPYFROM,	I_SX2806_COPYFROM,
		SXF_A_SCAN,	I_SX2807_SCAN,
		SXF_A_ESCALATE,	I_SX2808_ESCALATE,
		SXF_A_EXECUTE,	I_SX2809_EXECUTE,
		SXF_A_EXTEND,	I_SX2810_EXTEND,
		SXF_A_CREATE,	I_SX2811_CREATE,
		SXF_A_MODIFY,	I_SX2812_MODIFY,
		SXF_A_DROP,	I_SX2813_DROP,
		SXF_A_INDEX,	I_SX2814_INDEX,
		SXF_A_ALTER,	I_SX2815_ALTER,
		SXF_A_RELOCATE,	I_SX2816_RELOCATE,
		SXF_A_CONTROL,	I_SX2817_CONTROL,
		SXF_A_AUTHENT,	I_SX2818_AUTHENT,
# ifdef SXF_A_READ
		SXF_A_READ,	I_SX2819_READ,
		SXF_A_WRITE,	I_SX2820_WRITE,
		SXF_A_READWRITE,	I_SX2821_READWRITE,
# endif
		SXF_A_TERMINATE,	I_SX2822_TERMINATE,
		SXF_A_CONNECT,		I_SX2835_CONNECT,
		SXF_A_DISCONNECT,	I_SX2836_DISCONNECT,
#ifdef SXF_A_DOWNGRADE
		SXF_A_DOWNGRADE,	I_SX2823_DOWNGRADE,
#endif
		SXF_A_BYTID,	I_SX2824_BYTID,
		SXF_A_BYKEY,	I_SX2825_BYKEY,
		SXF_A_MOD_DAC,	I_SX2826_MOD_DAC,
		SXF_A_REGISTER,	I_SX2827_REGISTER,
		SXF_A_REMOVE,	I_SX2828_REMOVE,
		SXF_A_RAISE,	I_SX2829_RAISE,
		SXF_A_SETUSER,	I_SX282A_SETUSER,
		SXF_A_REGRADE,	I_SX282B_REGRADE,	
		SXF_A_WRITEFIXED,I_SX282C_WRITEFIXED,	
		SXF_A_QRYTEXT,	I_SX2830_QRYTEXT,	
		SXF_A_SESSION,	I_SX2831_SESSION,	
		SXF_A_QUERY,	I_SX2832_QUERY	,	
		SXF_A_LIMIT,	I_SX2833_LIMIT	,	
		SXF_A_USERMESG,	I_SX2834_USERMESG,	
		0,		I_SX2800_BAD_ACCESS_TYPE
	};
	for (i=0; acctypes[i].acctype!= 0; i++)
	{
		if ( acctypes[i].acctype==acctype)
			break;
	}
	return acctypes[i].msgid;
}
/*
** Name: sxapo_priv_to_string - converts a privilege mask to a string
**       of Y and -
**
** Description:
**	This takes an internal priv mask and converts it bit-by-bit
**	to a privilege string (Y & -). Currently there is no knowledge
**	of the "meaning" of each bit. 
**      
**	It is assumed that pstr points to something sensible
**
**	If 'term' is set to TRUE then a NULL terminator is added,
**	otherwise not.
**
** Inputs:
**	priv	the internal privilege mask 
**
** Outputs:
**	pstr    string holding the privileges
**
** History:
**	21-sep-92 (robf)
**	    Created
**	4-jan-94 (stephenb)
**	    renamed sxapo_priv_to_string to be used in sxapo.
*/
VOID
sxapo_priv_to_str( i4 priv, DB_DATA_VALUE *pstr, bool term)
{
	int i;
	for (i=0;i<pstr->db_length;i++)
	{
		if (priv& (1<<i))
			pstr->db_data[i]='Y';
		else
			pstr->db_data[i]='-';
	}
	if(term==TRUE)
		pstr->db_data[pstr->db_length]=EOS;
}

/*
**	Name: sxapo_incr - increment a counter protected by a semaphore
**
**	Description:
**	    This provides a standard way to increment statistics 
**	    counter variables in a multi-user environment.
**
**	History:
**	   06-oct-92 (robf)
**		  Created.
**	   04-jan-94 (stephenb)
**	    	  Renamed sxapo_incr to be used in sxapo.
*/
VOID
sxapo_incr( CS_SEMAPHORE *sem, i4 *counterp )
{
    STATUS stat;

    stat = CSp_semaphore(TRUE, sem);
    if (stat == OK)
    {
        (*counterp)++;
        stat = CSv_semaphore( sem);
    }
    if( stat!=OK)
    {
	TRdisplay("SXAPO_INCR: Semaphore error\n");
    }
}

