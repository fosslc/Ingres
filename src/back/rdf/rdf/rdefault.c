/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<ddb.h>
#include	<ulf.h>
#include        <cs.h>
#include	<scf.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<adfops.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<tr.h>
#include	<psfparse.h>
#include	<rdftree.h>
#include	<rdftrace.h>

/* prototype definitions */

static VOID
rdu_dname(	RDF_GLOBAL  *global,
		DB_DEF_ID   *id,
		char	    *name,
		i4	    *len);

static DB_STATUS 
rdd_gdefault(	RDF_GLOBAL   *global,
		DMT_ATT_ENTRY *col_ptr,
		DB_IIDEFAULT *def);

/**
**  Name: RDEFAULT.C - Perform operations for DEFAULT cache
**
**  Description:
**	This file contains the routines to request/build a default cache entry.
**
**	This module contains the following routines:
**	    rdd_defsearch   - searches default cache for default and builds an
**			      entry if one is not found
**	    rdu_dname	    - builds a ULH hash table key for a default
**	    rdd_gdefault    - reads an entry from the iidefaults catalog
**
**	The defaults entries reside on the RDF cache as follows:
**
**	    RDF_INFO->rdr_attr is the attribute descriptor for a table.  It is
**	    of type DMT_ATT_ENTRY.  The last field in DMT_ATT_ENTRY is
**		PTR             att_default;	-- of type RDD_DEFAULT --
**
**	    The RDD_DEFAULT structure is:
**	    {
**		PTR	rdf_def_object;	-- PTR to RDF_DE_ULHOBJECT --
**		u_i2	rdf_deflen;	-- num of chars in default string --
**		char	*rdf_default;	-- PTR to default char string --
**	    } RDD_DEFAULT;
**
**	    The RDF_DE_ULHOBHECT looks like:
**	    {
**		RDF_STATE   rdf_state;	  -- state of access to object, this
**                                        -- variable must be updated and looked
**                                        -- under protection of a ULH semaphore
**		ULH_OBJECT  *rdf_ulhptr;  -- this is the ulh object used to
**                                        -- release the stream --
**		PTR	    rdf_pstream_id;-- the private memory pool stream id
**					  -- Currently Not used --
**		RDD_DEFAULT *rdf_attdef;  -- ptr to RDD_DEFAULT structure --
**		u_i2	    rdf_ldef;	  -- length of default value --
**		char	    *rdf_defval;  -- contains default value as char str
**	    }   RDF_DE_ULHOBJECT;
**
**	So, the RDF Default cache structures look like and map like this:
**
**	     RDR_INFO
**	    :-----------:
**	    :   :  :    :
**	    : rdr_attr  :---+
**	    :   :  :    :   :
**	    :-----------:   :
**			    :
**	    DMT_ATT_ENTRY   :
**	    :-----------:   :
**	    :   :  :    :<--+
**	+---:att_default:
**	:   :-----------:
**	:	    
**	:     RDD_DEFAULT		     RDF_DE_ULHOBJECT
**	:   :----------------:		    :---------------:
**	+-->:		     :		    : rdf_state	    :
**	    : rdf_def_object :------------->:		    :<----------+
**	    : rdf_deflen     :		    : rdf_ulhptr    :-------+	:
**	    :		     :		    : rdf_pstream_id:	    :	:
**	    :                :<-------------: rdf_attdef    :	    :	:
**	    :		     :		    : rdf_ldef	    :	    :	:
**	+---: rdf_default    :		    : rdf_defval    :---+   :	:
**	:   :----------------:		    :---------------:	:   :	:
**	:							:   :	:
**	:	    :-------------------------------:		:   :	:
**	+---------->: var length character string   :<----------+   :	:
**		    :(contains ASCII representation :		    :	:
**		    : of default value)		    :		    :	:
**		    :-------------------------------:		    :	:
**								    :	:
**	     CS_SEMAPHORE	     ULH_OBJECT			    :	:
**	    :------------:	    :-----------------------:	    :	:
**	    :		 :	    : ulh_id		    :<------+	:
**	    : (used to 	 :	    : ulh_objname	    :		:
**	    : get	 :	    : ulh_namelen	    :		:
**	    : semaphores :<---------: ulh_usem		    :		:
**	    : from SCF)	 :	    : ulh_uptr		    :-----------+
**	    :------------:	    :-----------------------:
**
**  History:
**	16-feb-93 (teresa)
**	    initial creation
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	19-may-93 (teresa)
**	    fix instance where def_p was uninitialized (if a default was
**	    specified but contained a zero length default value).
**	20-may-93 (teresa)
**	    rename rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 
**	    8 characters from rdf_de_ulhobject.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**      14-jul-93 (rblumer)
**          in rdd_gdefault:
**          a defaultID of (0,0) is legal, so remove the piece of the
**          parameter-checking code that disallows that value.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**/

/*{
** Name: rdu_defsearch	- search defaults cache by database id, default id.
**
** Description:
**      This routine searches the defaults cache for a specified default.  If
**	the default was not found on the cache, it queries the iidefaults
**	catalog and builds a cache object for the default.
**
**	There are subtle concurancy issues here, so PLEASE take care if you
**	decide to modify this routine:
**
**	    RDF can have more than one thread trying to create the cache object
**	    at the same time.  The first one in does the following:
**		1.  look for object on cache -- gets back empty object
**		2.  take a semaphore and verify object is really empty (cuz
**		    no memory has been allocated to hold the object.
**		3.  Set RDF_STATE to RDF_SUPDATE indicating this object is
**		    in the process of being built.
**		4.  allocate memory for RDD_DEFAULT, II_DEFAULT tuple and
**		    RDF_DE_ULHOBJECT structures and do all necessary mapping.
**		    NOTE: it is critical that under concurrant conditions, only
**			  one thread actually allocate the memory.  Else we
**			  get memory leaks.  This is why memory is only 
**			  allocated under semaphore protection.
**		5.  Release semaphore.
**		6.  Read tuple from iidefault into stack memory.  If there is 
**		    an error reading the tuple or the tuple is not found, then 
**		    retake semaphore,  set state to RDD_SBAD, release semaphore 
**		    and take an error exit.
**		7.  Assuming the catalog read is successful: take semaphore.
**		    Check state.  If it's RDF_SSHARED, some other thread has
**		    compleated building the object while you were reading the
**		    tuple. The object is usable, so just return a pointer to it.
**		8.  If state remains RDF_SUPDATE under semaphore protection,
**		    then copy the tuple from stack memory into cache memory
**		    and update state to RDF_SSHARED.
**		9.  Release semaphore.
**	       10.  Return a ptr to the RDD_DEFAULT structure.
**
**	    On race conditions, another thread may try to access a default cache
**	    object  before it is fully built:
**		1.  look for object on cache -- get back an empty object.
**		    Another possibility is that the object is not empty, but
**		    the state is RDF_SUPDATE. Both of these cases are handled
**		    the same.
**		2.  Take a semaphore and verify that its really empty -- it
**		    wont be because the earlier thread got semaphore and 
**		    allocated memory.
**		3.  Check state.  If state is RDF_SUPDATE, then this thread
**		    needs to try and populate the object. If state is 
**		    RDF_SSHARED, then the object is fully built.  Just return
**		    the ptr to the RDD_DEFAULT structure.  If state is RDD_SBAD,
**		    then the earlier thread encountered an error trying to build
**		    the object.  There's either some resource problem, like not
**		    enough memory, or the default is not in the catalog or 
**		    whatever.  Rather than trying to untangle it, just return
**		    an error building the default (the other thread will give
**		    a more comprehensive error message) and take an error exit.
**		4.  Release semaphore.
**		5.  Read tuple from iidefault into stack memory.  If there is 
**		    an error reading the tuple or the tuple is not found, then 
**		    retake semaphore,  set state to RDD_SBAD, release semaphore 
**		    and take an error exit.
**		6.  Assuming the catalog read is successful: take semaphore.
**		    Check state.  If it's RDF_SSHARED, some other thread has
**		    compleated building the object while you were reading the
**		    tuple. The object is usable, so just return a pointer to it.
**		7.  If state remains RDF_SUPDATE under semaphore protection,
**		    then copy the tuple from stack memory into cache memory
**		    and update state to RDF_SSHARED.
**		8.  Release semaphore.
**	        9.  Return a ptr to the RDD_DEFAULT structure.
**
**	    Once the object is on the cache, the state will be RDD_SSHARED and
**	    memory for this object will be be allocated.  If both of these
**	    conditions are true from ulh_create, then just use the pointer to
**	    to object.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**	col_ptr				ptr to DMT_ATT_ENTRY that needs a default
**	def_ptr				Address of variable to hold ptr to new
**					RDD_DEFAULT that RDF will build
**
** Outputs:
**	def_ptr				pointer to default descriptor
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-feb-93 (teresa)
**	    initial creation
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	19-may-93 (teresa)
**	    fix instance where def_p was uninitialized (if a default was
**	    specified but contained a zero length default value).
**	20-may-93 (teresa)
**	    rename rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 
**	    8 characters from rdf_de_ulhobject.
**	11-nov-1998 (nanpr01)
**	    Allocate a single chunk rather than allocating small pieces of
**	    memory separately.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**	21-Mar-2003 (jenjo02)
**	    rdu_ferror() pointing to wrong error code, rdf_dist_ulhcb,
**	    instead of rdf_def_ulhcb.
*/

DB_STATUS
rdd_defsearch(	RDF_GLOBAL         *global,
		DMT_ATT_ENTRY	   *col_ptr,
		RDD_DEFAULT	   **def_p)
{
    DB_STATUS	    status,t_status;
    char	    key_name[ULH_MAXNAME];
    i4		    key_len;
    DB_IIDEFAULT    default_buf;
#define	DEF_BLK	8

    /* 
    ** set up key into cache for this default object.
    */
    rdu_dname(global, &col_ptr->att_defaultID, key_name, &key_len);

    /* 
    ** initialize default ulhcb 
    */
    if (!(global->rdf_init & RDF_DE_IULH))
    {	/* init the dist ULH control block if necessary, only need the server
        ** level hashid obtained at server startup 
	*/
	global->rdf_def_ulhcb.ulh_hashid = Rdi_svcb->rdv_def_hashid;
	global->rdf_init |= RDF_DE_IULH;
    }

    /* 
    ** now get item from cache, if it exists 
    */
    status = ulh_create(&global->rdf_def_ulhcb, (unsigned char *)&key_name[0],
			key_len, ULH_DESTROYABLE, 0);
    if (DB_SUCCESS_MACRO (status) )
    {
	RDF_DE_ULHOBJECT	    *rdf_ulhobject;

	global->rdf_resources |= RDF_DEULH;	/* mark ulh resource as fixed */

	/*
	** now see if this is a newly created defaults cache object
	*/
	rdf_ulhobject = (RDF_DE_ULHOBJECT *) global->rdf_def_ulhcb.
						    ulh_object->ulh_uptr;
	global->rdf_de_ulhobject = rdf_ulhobject;   /* save for error recovery*/
	/*  We can be here for two cases:
	**	    - we have successfully created an empty 
	**	      ULH object, or
	**	    - we have successfully gained access to an object.  That
	**	      object may be fully or partially created.  If it is fully
	**	      created, the rdf_state will be RDF_SSHARED.
	*/
	if ( !rdf_ulhobject 
	   ||
	     (rdf_ulhobject && (rdf_ulhobject->rdf_state != RDF_SSHARED) )
	   )		
	{
	    do		/* control loop */
	    {   
		/* object newly created or it is not fully built --
		** get a semaphore on the ulh object in order to update
		** the object status 
		*/
		status = rdd_setsem(global, RDF_DEF_DESC);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		/* re-fetch the ULH object under semaphore protection in order 
		** to check if another thread is trying to create this object
		*/
		rdf_ulhobject = (RDF_DE_ULHOBJECT *) global->rdf_def_ulhcb.
				ulh_object->ulh_uptr;
		global->rdf_de_ulhobject = rdf_ulhobject; /* save ptr
				    ** to the resource which is to be updated */
		if (!rdf_ulhobject)
		{   
		    /* another thread is not attempting to create this object
		    ** so allocate some memory for the object descriptor.
		    ** Also, set RDF_DE_RUPDATE to indicate that the ULH object
		    ** has been successfully obtained and will be updated by
		    ** this thread.  Finally, set RDF_DEF_DESC to let rdf_malloc
		    ** know it is allocating memory for the Defaults cache.
		    ** (remember to unset RDF_DEF_DESC after all allocation
		    ** successfully completes, or futher processing will try
		    ** to do rdu_malloc calls to build relation cache objects
		    ** and will fail.
		    */
		    global->rdf_resources |= (RDF_DE_RUPDATE | RDF_DEF_DESC); 
		    status = rdu_malloc(global, 
			(i4)(sizeof(*rdf_ulhobject) + sizeof(RDD_DEFAULT)), 
			(PTR *)&rdf_ulhobject);
		    if (DB_FAILURE_MACRO(status))
			break;
		    /* save ptr to ULH object for unfix commands */
		    rdf_ulhobject->rdf_ulhptr = 
					    global->rdf_def_ulhcb.ulh_object; 
		    /* indicate that this object is being updated */
		    rdf_ulhobject->rdf_state = RDF_SUPDATE; 
		    rdf_ulhobject->rdf_pstream_id = 
				global->rdf_def_ulhcb.ulh_object->ulh_streamid;

		    /*
		    **	allocate buffer to hold the RDD_DEFAULT structure
		    */
		    rdf_ulhobject->rdf_attdef = (RDD_DEFAULT *)
			((char *) rdf_ulhobject + sizeof(*rdf_ulhobject));
		    /* 
		    **	initialize and map allocated memory -- 
		    **  we cant allocate/map the default value because we have 
		    **  not read it from the iidefault catalog yet.
		    */
		    rdf_ulhobject->rdf_attdef->rdf_def_object = 
		      (PTR)rdf_ulhobject;
		    rdf_ulhobject->rdf_attdef->rdf_deflen = 0;
		    rdf_ulhobject->rdf_attdef->rdf_default = NULL;

		    /*
		    ** We've allocated everything we need for the defaults
		    ** cache (except the default string, which we will allocate
		    ** later.  So, clear RDF_DEF_DESC so future allocation 
		    ** calls (which will try to build relation or qtree cache
		    ** objects) do not use default cache memory by accident.
		    */
		    global->rdf_resources &= (~RDF_DEF_DESC); 

		    /* save this object after allocating memory for it. */
		    global->rdf_de_ulhobject = rdf_ulhobject;
		    global->rdf_def_ulhcb.ulh_object->ulh_uptr = 
							    (PTR)rdf_ulhobject;
		}
		else
		{
		    /* some other thread allocated memory while we waited for
		    ** the semaphore.  See if they've finished the whole thing,
		    ** if so, use it unless its bad.
		    */
		    if (rdf_ulhobject->rdf_state != RDF_SUPDATE)
		    {
			/* release semaphore*/
			status = rdd_resetsem(global, RDF_DEF_DESC);
			if (DB_FAILURE_MACRO(status))
			    break;

			/* the other thread is done, so we do not need to 
			** try and build this cache object 
			*/
			if (rdf_ulhobject->rdf_state == RDF_SBAD)
			{
			    /* 
			    ** the other thread failed 
			    */
			    global->rdf_resources |= RDF_BAD_DEFDESC;
			    *def_p = NULL;

			    /* error reading default */
			    rdu_ierror(global, E_DB_ERROR, 
				       E_RD001C_CANT_READ_COL_DEF);
			    return(E_DB_ERROR);
			}
			else
			{
			    /* the memory object is availabe and ready for use*/
			    *def_p = rdf_ulhobject->rdf_attdef;
			    return (status);
			}
		    }
		}

		/*
		** Now populate the default tuple
		*/
		status = rdd_resetsem(global, RDF_DEF_DESC); /* release semaphore*/
		if (DB_FAILURE_MACRO(status))
		    break;

		status = rdd_gdefault(global, col_ptr, &default_buf);
		if (status != E_DB_OK)
		{
		    /* could not read default
		    */
		    rdf_ulhobject->rdf_state = RDF_SBAD;
		    global->rdf_resources |= RDF_BAD_DEFDESC;
		    return (status);
		}

		/* retake the semaphore */
		status = rdd_setsem(global, RDF_DEF_DESC);
		if (DB_FAILURE_MACRO(status))
		    return(status);

		/* some other thread may have fininshed building this
		** object while we were reading the iidefault tuple.  So
		** only update the tuple if the state is still RDF_SUPDATE
		*/
		if (rdf_ulhobject->rdf_state == RDF_SUPDATE)
		{
		    i4	    defsize;
                    
                    if( default_buf.dbd_def_length == 0 )
                    { 
                       default_buf.dbd_def_length = 1;
                    }
		    if (defsize=default_buf.dbd_def_length)
		    {
			/* most default tuples have data, but a few do not.
			** this one does 
			*/
			rdf_ulhobject->rdf_attdef->rdf_deflen = defsize;
			/* round defsize to nearest multiple of 8 bytes */
			defsize = defsize + DEF_BLK - defsize%DEF_BLK;

			/* 
			** allocate buffer to hold defaults tuple 
			*/
			global->rdf_resources |= (RDF_DE_RUPDATE|RDF_DEF_DESC);
			status = rdu_malloc(global, defsize,
					    (PTR *)&rdf_ulhobject->rdf_defval);
			global->rdf_resources &= (~RDF_DEF_DESC); 
			if (DB_FAILURE_MACRO(status))
			    break;

			/* copy iidefault tuple into cache */
			rdf_ulhobject->rdf_ldef = default_buf.dbd_def_length;
			MEcopy( (PTR) default_buf.dbd_def_value,
				rdf_ulhobject->rdf_attdef->rdf_deflen,
				(PTR) rdf_ulhobject->rdf_defval);
			rdf_ulhobject->rdf_attdef->rdf_default = 
						      rdf_ulhobject->rdf_defval;

		    }

		    /* return ptr to RDD_DEFAULT to caller.*/
		    *def_p = rdf_ulhobject->rdf_attdef;

		    /* update status to sharable by all, indicating that the 
		    ** build of this cache object is complete 
		    */
		    rdf_ulhobject->rdf_state = RDF_SSHARED;
		}
		else if (rdf_ulhobject->rdf_state == RDF_SBAD)
		{
                    if (global->rdf_resources & RDF_RSEMAPHORE)
                    {
                        status = rdd_resetsem(global,RDF_DEF_DESC); 
                        if (DB_FAILURE_MACRO(status))
                            break;
                    }
		    global->rdf_resources |= RDF_BAD_DEFDESC;
		    *def_p = NULL;

		    rdu_ierror(global, E_DB_ERROR, 
			       E_RD001C_CANT_READ_COL_DEF);
		    return(E_DB_ERROR);
		}
		else
		{
                   /* the memory object is availabe and ready for use*/
		    *def_p = rdf_ulhobject->rdf_attdef;
		}

	    } while (0);	/* end control loop */

	    /* release semaphore if held */
	    if (global->rdf_resources & RDF_RSEMAPHORE)
	    {
		t_status = rdd_resetsem(global,RDF_DEF_DESC);
		/* keep the most severe error status */
		status = (t_status > status) ? t_status : status;
	    }
	}
	else
	{
	    /* the object was already on the cache, so return ptr to it. */
	    *def_p = rdf_ulhobject->rdf_attdef;
	    /* save ptr for future ref */
	    global->rdf_de_ulhobject = rdf_ulhobject; 
	}
    }
    else    /* ulh_create failed */
	rdu_ferror(global, status, &global->rdf_def_ulhcb.ulh_error,
		   E_RD0045_ULH_ACCESS,0);

    return (status);
}

/*{
** Name: rdu_dname	- build ULH name for defaults cache entry
**
** Description:
**      Build the name for the ULH hash table ID for the defaults cache.
**	This name is comprised of:
**		unique default identifier (i4, i4)
**		database id (i4)
**
** Inputs:
**      global                          ptr to RDF global state variable
**	    rdrcb			RDF Control block
**		rdf_rb			    RDF Request block
**		    rdr_unique_dbid		    unique DB identifier
**	id				Default ID
**	
** Outputs:
**      *name		    hash object name to be used by ULH
**	*len		    number of characters in *name
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-feb-93 (teresa)
**          initial creation for constraints
**	6-Jul-2006 (kschendel)
**	    Don't try to substitute db-id for unique-dbid (totally different).
[@history_template@]...
*/
static VOID
rdu_dname(	RDF_GLOBAL  *global,
		DB_DEF_ID   *id,
		char	    *name,
		i4	    *len)
{
    char	    *def_name = name;

    MEcopy ( (PTR) id, sizeof(DB_DEF_ID), (PTR) def_name);
    def_name += sizeof(DB_DEF_ID);

    MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_unique_dbid, 
	sizeof(i4), 
	(PTR)def_name);
    *len = sizeof(DB_DEF_ID) + sizeof(i4);
}

/*{
** Name: rdd_gdefault	- get default value from IIDEFAULT catalog
**
** Description:
**
** Inputs:
**	
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-feb-93 (teresa)
**          initial creation for constraints
**      14-jul-93 (rblumer)
**          a defaultID of (0,0) is legal, so remove the piece of the
**          parameter-checking code that disallows that value.
**          Also removed unused variable key (to get rid of compiler warning).
*/
static DB_STATUS 
rdd_gdefault(	RDF_GLOBAL   *global,
		DMT_ATT_ENTRY *col_ptr,
		DB_IIDEFAULT *def)
{
    bool		eof_found;	/* To pass to rdu_qget */
    QEF_DATA		qefdata;	/* For QEU extraction call */
    RDD_QDATA		rdqef;		/* For rdu_qopen to use */
    RDF_CB		*rdfcb = global->rdfcb;

    DB_STATUS		status, end_status;

    /* Validate parameters to request */
    if (
	 !col_ptr
       ||
         !def
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    qefdata.dt_next = NULL;
    qefdata.dt_size = sizeof(DB_IIDEFAULT);
    qefdata.dt_data = (PTR) def;
    rdqef.qrym_alloc = rdqef.qrym_cnt = 1;
    rdqef.qrym_data = &qefdata;

    /* Access IIDEFAULT catalog by default id */
    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    rdfcb->rdf_rb.rdr_qtuple_count = 0;
    status = rdu_qopen(global, RD_IIDEFAULT, (char *) col_ptr,
		       sizeof(DB_IIDEFAULT), (PTR) &rdqef,
		       0 /* 0 datasize means RDD_QDATA is set up */);
    if (DB_FAILURE_MACRO(status))
	return(status);
    /* Fetch the tuple */
    status = rdu_qget(global, &eof_found);
    if (!DB_FAILURE_MACRO(status))
    {
	if (global->rdf_qeucb.qeu_count == 0 || eof_found)
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD001D_NO_SUCH_DEFAULT);
	}
    }
    end_status = rdu_qclose(global); 	/* Close file */
    if (end_status > status)
	status = end_status;
	
    return(status);
    
}
