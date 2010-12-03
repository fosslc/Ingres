/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <erglf.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <sp.h>
#include    <st.h>
#include    <mo.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qsfint.h>
/*
**
**  Name: QSFIMA.C - Provide QSF interface to Management Objects (MO).
**
**  Description:
**      This file holds all of the MO functions for QSF.
**
**          qsf_mo_attach()   - Attaches QSF MO objects
**	    qsf_mo_setdecay() - Sets the decay factor via an MO object.
**
**
**  History:
**      07-jan-1993 (rog)
**          Initial creation.
**	29-jun-1993 (rog)
**	    Include <cv.h> to pick up prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	05-apr-1994 (rog)
**	    Adding MO information to access information about query plans.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	08-nov-2001 (kinte01)
**	    Remove extra comma from routine call to STncpy as there are
**	    only 3 arguments and not 4
**	01-May-2002 (bonro01)
**	    Fix memory overlay in name[] variable.
**	30-may-03 (inkdo01)
**	    Added classes for dbp decaying usage and lock state.
**	24-Sep-2003 (hanje04)
**	    Cleaned up bad X-integ of inkdo01 change to replace long_int 
**  	    with longlong.
**	23-Feb-2005 (schka24)
**	    Alias ID now lives in the master header.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	29-Oct-2010 (jonj) SIR 120874
**	    Conform to use new uleFormat, DB_ERROR constructs.
*/


/* Forward and/or external definitions */
static STATUS qsf_mo_setdecay	( i4  offset, i4  luserbuf, char *userbuf,
				  i4  objsize, PTR object );
static STATUS qsf_name_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_text_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_owner_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_udbid_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_size_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_usage_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_dusage_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );
static STATUS qsf_lstate_get	( i4  offset, i4  size, PTR object, i4  lsbuf,
				  char *sbuf );

/* MO object class definitions */
static MO_CLASS_DEF QSF_Qsr_classes[] =
{
    {
	0, "exp.qsf.qsr.qsr_memtot",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_memtot), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_memtot), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_memleft",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_memleft), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_memleft), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_named_requests",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_named_requests), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_named_requests), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_decay_factor",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_decay_factor), MO_READ | MO_SERVER_WRITE,
	0, CL_OFFSETOF(QSR_CB, qsr_decay_factor), MOintget, qsf_mo_setdecay,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_nsess",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_nsess), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_nsess), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mxsess",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mxsess), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mxsess), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_nobjs",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_nobjs), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_nobjs), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mxobjs",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mxobjs), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mxobjs), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_bmaxobjs",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_bmaxobjs), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_bmaxobjs), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_bkts_used",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_bkts_used), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_bkts_used), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mxbkts_used",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mxbkts_used), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mxbkts_used), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_nbuckets",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_nbuckets), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_nbuckets), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_no_unnamed",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_no_unnamed), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_no_unnamed), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mx_unnamed",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mx_unnamed), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mx_unnamed), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_no_named",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_no_named), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_no_named), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mx_named",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mx_named), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mx_named), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_no_index",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_no_index), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_no_index), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mx_index",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mx_index), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mx_index), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mx_size",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mx_size), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mx_size), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_mx_rsize",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_mx_rsize), MO_READ, 0,
	CL_OFFSETOF(QSR_CB, qsr_mx_rsize), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    {
	0, "exp.qsf.qsr.qsr_no_destroyed",
	MO_SIZEOF_MEMBER(QSR_CB, qsr_no_destroyed), MO_READ,
	0, CL_OFFSETOF(QSR_CB, qsr_no_destroyed), MOintget, MOnoset,
	(PTR) 0, MOcdata_index
    },
    { 0 }
};


static char rqp_index_class[] = "exp.qsf.qso.rqp.index";

static MO_CLASS_DEF QSF_qso_rqp_classes[] =
{
    {
	MO_INDEX_CLASSID|MO_CDATA_INDEX, rqp_index_class,
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, 0,
	CL_OFFSETOF(QSO_OBID, qso_handle), MOptrget, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.rqp.name",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, rqp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_name_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.rqp.udbid",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, rqp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_udbid_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.rqp.size",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, rqp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_size_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.rqp.usage",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, rqp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_usage_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.rqp.text",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, rqp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_text_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    { 0 }
};


static char dbp_index_class[] = "exp.qsf.qso.dbp.index";

static MO_CLASS_DEF QSF_qso_dbp_classes[] =
{
    {
	MO_INDEX_CLASSID|MO_CDATA_INDEX, dbp_index_class,
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, 0,
	CL_OFFSETOF(QSO_OBID, qso_handle), MOptrget, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.name",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_name_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.owner",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_owner_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.udbid",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_udbid_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.size",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_size_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.usage",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_usage_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.dusage",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_dusage_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    {
	MO_CDATA_INDEX, "exp.qsf.qso.dbp.lstate",
	MO_SIZEOF_MEMBER(QSO_OBID, qso_handle), MO_READ, dbp_index_class,
	CL_OFFSETOF(QSO_OBID, qso_handle), qsf_lstate_get, MOnoset,
	(PTR) 0, MOidata_index
    },
    { 0 }
};


/*{
** Name: qsf_mo_attach - Management Object initialization for QSF
**
** Description:
**      This function attaches QSF's Management Object instances
**        
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	OK				Everything was attached successfully
**	other				An MO-specific error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	07-jan-1992 (rog)
**          Initial creation.
*/

STATUS
qsf_mo_attach( void )
{
    STATUS	status;
    i4	err_code;
    i4		i;

    for (i = 0;
	 i < (sizeof(QSF_Qsr_classes) / sizeof(QSF_Qsr_classes[0]));
	 i++)
    {
	QSF_Qsr_classes[i].cdata = (PTR) Qsr_scb;
    }

    status = MOclassdef( MAXI2, QSF_Qsr_classes);
    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }

    status = MOclassdef( MAXI2, QSF_qso_rqp_classes);
    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }

    status = MOclassdef( MAXI2, QSF_qso_dbp_classes);
    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
    }

    return (status);
}

/*{
**  Name: qsf_mo_setdecay - set qsf_decay_factor with input checking
**
**  Description:
**
**	Set the QSF LRU decay factor to the caller's input string
**	after checking that it contains a valid decay factor.
**
**	The offset is treated as a byte offset to the input object .
**	They are added together to get the object location.
**	
** Inputs:
**	offset
**		Value from the class definition, taken as byte offset to the
**		object pointer where the data is located; ignored.
**	luserbuf
**		The length of the user buffer; ignored.
**	userbuf
**		The user string to convert.
**	objsize
**		The size of the integer, from the class definition; ignored.
**	object
**		A pointer to the integer; ignored.
** Outputs:
**	None
**
** Side Effects:
**	Calls qsf_set_decay() which changes the value of the qsr_decay_factor
**	element of in the Qsr_scb.
**
** Returns:
**	OK
**		if the operation succeseded
**	MO_BAD_SIZE
**		if the object size isn't something that can be handled.
**	other
**		conversion-specific failure status as appropriate.
**  History:
**	07-jan-1993 (rog)
**	    Created from MOintset().
*/

static STATUS 
qsf_mo_setdecay( i4  offset, i4  luserbuf, char *userbuf, i4  objsize,
		 PTR object )
{
    STATUS status;
    i4 ival;
    
    status = CVal(userbuf, &ival);
    if (status == OK)
    {
	status = qsf_set_decay(ival);
	if (status != OK)
	{
	    status = MO_BAD_SIZE;
	}
    }
    return( status );
}

/*{
**  Name:	qsf_size_get - Extract the size of a sharable object
**
**  Description:
**	Extract the size of a sharable query plan and return it as a character
**	string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created from MOintget().
**      08-Nov-2001 (inifa01) INGSRV 1590 bug 106318
**          IMA tables 'ima_qsf_rqp' and 'ima_qsf_dbp' were
**          returning no rows because MO_BAD_SIZE was returned
**          here on 64 bit AXP OSF.  
**          Added case for generic long with new datatype long_int.
*/

static STATUS 
qsf_size_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    longlong	 ival = 0;
    QSO_OBJ_HDR	*obj;

    obj = (QSO_OBJ_HDR *) object + offset;

    switch( size )
    {
	case sizeof(i1):
	    ival = (i1) obj->qso_size;
	    break;
	case sizeof(i2):
	    ival = (i2) obj->qso_size;
	    break;
	case sizeof(i4):
	    ival = (i4) obj->qso_size;
	    break;
	case sizeof(longlong):
	    ival = (longlong) obj->qso_size;
	    break;
	default:
	    stat = MO_BAD_SIZE;
	    break;
    }
    
    if( stat == OK )
	stat = MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );

    return( stat );
}

/*{
**  Name:	qsf_usage_get - Extract the usage of a sharable object
**
**  Description:
**	Extract the usage of a sharable query plan and return it as a character
**	string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created from MOintget().
**	08-Nov-2001 (inifa01) INGSRV 1590 bug 106318
**	    IMA tables 'ima_qsf_rqp' and 'ima_qsf_dbp' were
**	    returning no rows because MO_BAD_SIZE was returned
**	    here on 64 bit AXP OSF.  Size of 'long' on 64 bit 
**	    platform (8 bytes) differs from size on 32 bit platform (4 bytes).
**          Added case for generic long with new datatype long_int.
**	30-Sep-2002 (inifa01) 
**          Reworked 08-Nov-2001 fix to bug  106318 to use longlong
**          definition which now exists in 2.6 instead of long_int.
**          definition of long_int to be removed from compat.h
*/

static STATUS 
qsf_usage_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    longlong	 ival = 0;
    QSO_OBJ_HDR	*obj;
    obj = (QSO_OBJ_HDR *) object + offset;

    switch( size )
    {
	case sizeof(i1):
	    ival = (i1) obj->qso_real_usage;
	    break;
	case sizeof(i2):
	    ival = (i2) obj->qso_real_usage;
	    break;
	case sizeof(i4):
	    ival = (i4) obj->qso_real_usage;
	    break;
	case sizeof(longlong):
            ival = (longlong) obj->qso_real_usage;
            break;
	default:
	    stat = MO_BAD_SIZE;
	    break;
    }
    
    if( stat == OK )
	stat = MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );

    return( stat );
}

/*{
**  Name:	qsf_dusage_get - Extract the decaying usage of a sharable object
**
**  Description:
**	Extract the decaying usage of a sharable query plan and return it as 
**	a character string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	30-may-03 (inkdo01)
**	    Cloned from qsf_usage_get. 
**	30-may-03 (inkdo01)
**	    Manually change long_int to longlong to satisfy 2.6. 
**	5-june-03 (inkdo01)
**	    Re-add i4 case lost when long_int changed to longlong (grrr!).
**	24-Sep-2003 (hanje04)
**	    Cleaned up bad X-integ of inkdo01 change to replace long_int 
**  	    with longlong.
*/

static STATUS 
qsf_dusage_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    longlong	 ival = 0;
    QSO_OBJ_HDR	*obj;
    obj = (QSO_OBJ_HDR *) object + offset;

    switch( size )
    {
	case sizeof(i1):
	    ival = (i1) obj->qso_decaying_usage;
	    break;
	case sizeof(i2):
	    ival = (i2) obj->qso_decaying_usage;
	    break;
	case sizeof(i4):
	    ival = (i4) obj->qso_decaying_usage;
	    break;
	case sizeof(longlong):
            ival = (longlong) obj->qso_decaying_usage;
            break;
	default:
	    stat = MO_BAD_SIZE;
	    break;
    }
    
    if( stat == OK )
	stat = MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );

    return( stat );
}

/*{
**  Name:	qsf_lstate_get - Extract the lock state of a sharable object
**
**  Description:
**	Extract the lock state of a sharable query plan and return it as 
**	a character string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	30-may-03 (inkdo01)
**	    Cloned from qsf_usage_get. 
**	30-may-03 (inkdo01)
**	    Manually change long_int to longlong to satisfy 2.6. 
**	5-june-03 (inkdo01)
**	    Re-add i4 case lost when long_int changed to longlong (grrr!).
**	24-Sep-2003 (hanje04)
**	    Cleaned up bad X-integ of inkdo01 change to replace long_int 
**  	    with longlong.
*/

static STATUS 
qsf_lstate_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    longlong	 ival = 0;
    QSO_OBJ_HDR	*obj;
    obj = (QSO_OBJ_HDR *) object + offset;

    switch( size )
    {
	case sizeof(i1):
	    ival = (i1) obj->qso_lk_state;
	    break;
	case sizeof(i2):
	    ival = (i2) obj->qso_lk_state;
	    break;
	case sizeof(i4):
	    ival = (i4) obj->qso_lk_state;
	    break;
	case sizeof(longlong):
            ival = (longlong) obj->qso_lk_state;
            break;
	default:
	    stat = MO_BAD_SIZE;
	    break;
    }
    
    if( stat == OK )
	stat = MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf );

    return( stat );
}

/*{
**  Name:	qsf_udbid_get - Extract the udbid of a sharable object
**
**  Description:
**	Extract the udbid of a sharable query plan and return it as a character
**	string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created from MOintget().
*/

static STATUS 
qsf_udbid_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    i4	 ival;
    QSO_OBJ_HDR	*obj;
    QSO_OBID	 obid;
    i4		*udbid;

    obj = (QSO_OBJ_HDR *) object + offset;
    obid = obj->qso_mobject->qsmo_aliasid;
    udbid = (i4 *) ((char *) obid.qso_name + obid.qso_lname - sizeof(i4));

    /*
    ** Ignore the size.  It should always be an i4.  Also, do an 
    ** MEcopy because we don't know whether the alignment is okay.
    */
    MECOPY_CONST_MACRO(udbid, sizeof(i4), &ival);
    
    return(MOlongout( MO_VALUE_TRUNCATED, ival, lsbuf, sbuf ));
}

/*{
**  Name:	qsf_owner_get - Extract the owner of a sharable object
**
**  Description:
**	Extract the owner of a sharable query plan and return it as a character
**	string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created.
*/

static STATUS 
qsf_owner_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    QSO_OBJ_HDR	*obj;
    QSO_OBID	 obid;
    char	 owner[DB_OWN_MAXNAME + 1];

    obj = (QSO_OBJ_HDR *) object + offset;
    obid = obj->qso_mobject->qsmo_aliasid;

    /* Repeat queries don't have an owner. */
    if (obid.qso_lname <= (sizeof(DB_CURSOR_ID) + sizeof(i4)))
    {
	stat = MO_BAD_SIZE;
    }

    if (stat == OK)
    {
	STncpy(owner, (char *) obid.qso_name + sizeof(DB_CURSOR_ID), 
		DB_OWN_MAXNAME);
	owner[DB_OWN_MAXNAME] = '\0';
	MOstrout( MO_VALUE_TRUNCATED, owner, lsbuf, sbuf );
    }
    
    return(stat);
}

/*{
**  Name:	qsf_name_get - Extract the name of a sharable object
**
**  Description:
**	Extract the name of a sharable query plan and return it as a character
**	string.
**
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or sizeof(i4).
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created.
**	01-May-2002 (bonro01)
**	    Fix memory overlay in name[] variable.
*/

static STATUS 
qsf_name_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    QSO_OBJ_HDR	*obj;
    QSO_OBID	 obid;
    i4		 int1, int2;
    /*
    ** This is complicated: a DB_CURSOR_ID contains two i4's.  When they are
    ** printed, they may expand up to 10 bytes, so we add 6 bytes for each
    ** one plus the byte for null at the end -- hence 13.
    */
    char	 name[sizeof(DB_CURSOR_ID) + 13];

    obj = (QSO_OBJ_HDR *) object + offset;
    obid = obj->qso_mobject->qsmo_aliasid;
    MECOPY_CONST_MACRO(obid.qso_name, sizeof(i4), &int1);
    MECOPY_CONST_MACRO((char *) obid.qso_name + sizeof(i4), sizeof(i4), &int2);

    STprintf(name, "%-10d%-10d", int1, int2);
    STncpy( name + 20, (char *) obid.qso_name + (2 * sizeof(i4)), DB_MAXNAME);
    name[sizeof(name)-1] = '\0';
    return(MOstrout( MO_VALUE_TRUNCATED, name, lsbuf, sbuf ));
}

/*{
**  Name:	qsf_text_get - Extract text from a sharable object
**
**  Description:
**	If this is a text object, return as much text as will fit in the
**	given buffer.
**
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	5-dec-2007 (dougi)
**	    Cloned from qsf_name_get().
*/

static STATUS 
qsf_text_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS	 stat = OK;
    QSO_OBJ_HDR	*obj;
    QSO_OBID	 obid;
    i4		 tsize;
    char	 text[1000];

    obj = (QSO_OBJ_HDR *) object + offset;
    obid = obj->qso_mobject->qsmo_aliasid;

    if (obj->qso_obid.qso_type == QSO_QTEXT_OBJ)
    {
	tsize = (obj->qso_size > 999) ? 999 : obj->qso_size;
	if (tsize > 0)
	    MEcopy(obj->qso_root, tsize, &text);
    }
    else tsize = 0;

    text[tsize] = '\0';	/* terminate string */
    return(MOstrout( MO_VALUE_TRUNCATED, text, lsbuf, sbuf ));
}

/*{
**  Name:	qsf_attach_inst - Attach an instance of a sharable QP
**
**  Description:
**	Attach a sharable QP's alias to the MO structures.
**
**  Inputs:
**	 obid	A QSO_OBID that describes this object.
**
**  Outputs:
**	 None.
**
**  Returns:
**	 OK
**		if the operation succeseded
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created.
**      17-feb-2009 (wanfr01) b121543
**          Add new qsf_type parameter which indicates if the QSF object is
**	    a repeated query or a database procedure.
*/

STATUS 
qsf_attach_inst( QSO_OBID obid, i2 qsf_type )
{
    STATUS	 status;
    char	 buf[DB_MAXNAME + 1];
    char	*index;

    if (qsf_type != QSOBJ_DBP_TYPE)
    {
	index = rqp_index_class;
    }
    else
    {
	index = dbp_index_class;
    }

    status = MOptrout( MO_VALUE_TRUNCATED, obid.qso_handle, sizeof(buf), buf );

    if (status == OK)
    {
	status = MOattach( MO_INSTANCE_VAR, index, buf, obid.qso_handle );
    }

    return(status);
}

/*{
**  Name:	qsf_detach_inst - Detach an instance of a sharable QP
**
**  Description:
**	Detach a sharable QP's alias from the MO structures.
**
**  Inputs:
**	 obid	A QSO_OBID that describes this object.
**
**  Outputs:
**	 None.
**
**  Returns:
**	 OK
**		if the operation succeseded
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	26-apr-1994 (rog)
**	    Created.
**	30-sep-1996 (angusm)
**	    Conditional diags w.r.bug 76728
**      17-feb-2009 (wanfr01) b121543
**          Add new qsf_type parameter which indicates if the QSF object is
**	    a repeated query or a database procedure.
*/

STATUS 
qsf_detach_inst( QSO_OBID obid, i2 qsf_type)
{
    STATUS	 status;
    char	 buf[DB_MAXNAME + 1];
    char	*index;

    if (qsf_type != QSOBJ_DBP_TYPE)
    {
	index = rqp_index_class;
    }
    else
    {
	index = dbp_index_class;
    }

    status = MOptrout( MO_VALUE_TRUNCATED, obid.qso_handle, sizeof(buf), buf );

    if (status == OK)
    {
	status = MOdetach( index, buf );
    }

    return(status);
}
