/*
**  Copyright (c) 2004 Ingres Corporation
*/
/*
NO_OPTIM = su4_u42 su4_cmw
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <cv.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
** Name:  ADUPERM.C -    Routines for managing permissions
**
** Description:
**	  This file defines routines for managing permissions
**
**	This file defines:
**
**		adu_perm_type()		- convert permission bit to text
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**	25-june-1996 (angusm)
**		add adu_perm_type().
**	23-oct-1996 (kamal03)
**		Added static attribute to definition of "permtab[]"         
**		to fix the shareable image linking problem on VAX VMS.
**	22-aug-1997 (hayke02)
**		Added DB_CAN_ACCESS (0x8000) entry in permtab[] for views.
**		This change fixes bug 84757.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Sep-2008 (jonj)
**	    SIR 120874: Add adfint.h include to pick up adu_error() prototype.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/* Forward/static declarations */

/*{
** Name: adu_perm_type
**
** Description:
**		Takes a permission (bitmask) field, returns text.
**
**		This assumes that the permit field will contain
**		no more than two bits set (one of which may be
**		DB_GRANT_OPTION, i,e, "WITH GRANT OPTION".
**
**		Only known exception to this is SELECT which also
**		includes privileges 'SCAN' and 'AGGREGATE', neither
**		of which is available externally.
**	
**		Second parameter determines if function is in 'local'
**		or 'SQL92/FIPS' mode : all permission types supported
**		have a text string returned in 'local' mode: in
**		'SQL92/FIPS' mode, non-standard (INGRES-specific)
**		permissions are returned as 'OTHER'
**
**		If an unrecognised bit is set, value will be
**		returned as 'OTHER'.
**
** Inputs:
**
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**
**      dv1             Ptr to DB_DATA_VALUE for first param.
**      .db_datatype        Datatype; assumed to be DB_INT_TYPE.
**      .db_length          Length.
**      .db_data            Ptr to the data of first param.
**
**		This is assumed to be an integer with a valid
**		INGRES permit mask set.
**		
**      dv2             Ptr to DB_DATA_VALUE for second param.
**      .db_datatype        Datatype; assumed to be DB_CHA_TYPE.
**      .db_length          Length.
**      .db_data            Ptr to the data of second param.
**
**		This is a mode indicator: 
**
**		-	0	-> INGRES internal permissions
**		-	1   -> SQL92-compliant permissions only
**					(SELECT, INSERT, UPDATE, DELETE, REFERENCES)
** Outputs:
**
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**	Returns:
**	adf_scb				Pointer to an ADF session control block.
**
**      rdv             Ptr to DB_DATA_VALUE for result 
**      .db_datatype        Datatype; assumed to be DB_CHA_TYPE.
**      .db_length          Length.
**      .db_data            Ptr to the data of result 
**
**	Side-effects:
**		nil
**	History:
**		25-june-1996 (angusm)
**		Added for ODBC v 3 support project. This implements
**		function 'iipermittype()'
**		18-jul-1996 (angusm)
**		Tighten validation on input params.
**		22-aug-1997 (hayke02)
**		Added DB_CAN_ACCESS (0x8000) entry in permtab[] for views.
**		This change fixes bug 84757.
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	26-Jun-2008 (kiria01) b120556
**	    Complete fix b84757 for case where out of range parameters
**	    will cause loop to exit leaving access to data beyond
**	    array and resultant SEGV. Also compliete the i8 handling of P1.
*/

/*
**  The following constants are for use with the bit maps stored in the
**  iiprotect and iisecalarm catalog, for telling what operations are 
**  permitted on a column.
**
**	since dbdbms.h which contains this is in back!hdr, duplicate here.
*/

#define                 DB_RETRIEVE	0x01L	/* Retrieve bit */
#define			DB_RETP		   0	/* Bit position of retrieve */
#define			DB_REPLACE	0x02L	/* Replace bit */
#define			DB_REPP		   1	/* Bit position of replace */
#define			DB_DELETE	0x04L	/* Delete bit */
#define			DB_DELP		   2	/* Bit position of delete */
#define			DB_APPEND	0x08L	/* Append bit */
#define			DB_APPP		   3	/* Bit position of append */
#define			DB_TEST		0x10L	/* Qual. test bit */
#define			DB_TESTP	   4	/* Bit position of qual test */
#define			DB_AGGREGATE	0x20L	/* Aggregate bit */
#define			DB_AGGP		   5	/* Bit position of aggregate */
#define			DB_EXECUTE	0x40L	/* Execute bit (dbproc only) */
#define			DB_EXECP	   6	/* Bit pos. of execute */
#define                 DB_ALARM        0x80L   /* Alarm bit (C2/B1 only) */
#define			DB_ALAP	           7	/* Bit pos. of alarm */
#define			DB_EVREGISTER 0x0100L	/* REGISTER EVENT permission */
#define			DB_EVRGP	   8	/* Bit pos of REGISTER permit */
#define			DB_EVRAISE    0x0200L	/* RAISE EVENT permission */
#define			DB_EVRAP	   9	/* Bit pos of RAISE permit */
#define                 DB_ALLOCATE   0x0400L   /* ALLOCATE permission */
#define                 DB_ALLOP          10    /* Bit pos of ALLOCATE permit */
#define                 DB_SCAN       0x0800L   /* SCAN permission */
#define                 DB_SCANP          11    /* Bit pos of SCAN permit */
#define                 DB_LOCKLIMIT  0x1000L   /* LOCK_LIMIT permission */
#define                 DB_LOCKP          12    /* Bit pos of LOCKLIM permit */
#define			DB_GRANT_OPTION 0x2000L /* permit WITH GRANT OPTION */
#define			DB_GROPTP	  13	/* bit pos of WGO indicator */
#define			DB_REFERENCES	0x4000L	/* REFERENCES permission */
#define			DB_REFP		  14	/* bit position of above */
#define			DB_CAN_ACCESS 0x8000L	/* object can be accessed by grantee */
#define			DB_CAN_ACCESSP	  15	/* bit pos. of above */

#define		        DB_DISCONNECT    0x2000000L /* DISCONNECT */
#define                 DB_DISCONNECTP  25  	  /* bit pos. of above */

#define		        DB_CONNECT    0x4000000L /* CONNECT */
#define                 DB_CONNECTP  26  	  /* bit pos. of above */

#define                 DB_COPY_INTO  0x8000000L  /* COPY_INTO priv */
#define                 DB_COPY_INTOP  27  	  /* bit pos. of above */

#define                 DB_COPY_FROM  0x10000000L  /* COPY_FROM priv */
#define                 DB_COPY_FROMP  28  	   /* bit pos. of above */

#define                 DB_ALMFAILURE 0x20000000L /* Alarm on failure bit */
#define			DB_ALPFAILURE	  29	  /* Bit pos. of alarm/failure */
#define                 DB_ALMSUCCESS 0x40000000L /* Alarm on success bit */
#define			DB_ALPSUCCESS	  30	  /* Bit pos. of above */

#define			DB_NEGATE     0x80000000L /* NOprivilege specified
						  ** (internal use only)   */

#define			PERM_LOCAL	0
#define			PERM_SQL92	1

typedef struct bits_to_perms {
	u_i4	flag;
	char	*perm;
} ADU_BTP;

static ADU_BTP		permtab[] = {

		{DB_RETRIEVE, 		"SELECT" }
		, {DB_REPLACE, 		"UPDATE" }
		, {DB_DELETE, 		"DELETE" }
		, {DB_APPEND, 		"INSERT" }
		, {DB_REFERENCES, 	"REFERENCES"}
		, {0, "OTHER"}
		, {DB_COPY_FROM,	"COPY_FROM"}
		, {DB_COPY_INTO,	"COPY_INTO"}

		, {DB_EXECUTE,		"EXECUTE"}

		, {DB_EVREGISTER,	"REGISTER_EVENT"}
		, {DB_EVRAISE,		"RAISE_EVENT"}
		, {DB_CAN_ACCESS,	"VIEW CAN BE ACCESSED BY GRANTEE"}

		, {0, "OTHER"}		/* MUST be last element of array */

};

DB_STATUS
adu_perm_type(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *dv2,
DB_DATA_VALUE      *rdv)
{
	i4		i;
	i4		npermtab = sizeof(permtab) / sizeof (ADU_BTP) - 1;
	u_i4	tf;
	i8		i8temp;

	switch (dv1->db_length)
	{
	    case 1:
		tf = (DB_DT_ID) I1_CHECK_MACRO(*(i1 *)dv1->db_data);
		break;
	    case 2: 
		tf = (DB_DT_ID) *(i2 *)dv1->db_data;
		break;
	    case 4:
		tf = (DB_DT_ID) *(i4 *)dv1->db_data;
		break;
	    case 8:
		i8temp  = *(i8 *)dv1->db_data;

		/* limit to i4 values  */
		if ( i8temp > MAXI4 || i8temp < MINI4LL )
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "perm_type i overflow"));

		tf = (DB_DT_ID) i8temp;
		break;
	    default:
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "perm_type i length"));
	}
	switch (dv2->db_length)
	{
	    case 1:
		i = (DB_DT_ID) I1_CHECK_MACRO(*(i1 *)dv2->db_data);
		break;
	    case 2: 
		i = (DB_DT_ID) *(i2 *)dv2->db_data;
		break;
	    case 4:
		i = (DB_DT_ID) *(i4 *)dv2->db_data;
		break;
	    case 8:
		i8temp  = *(i8 *)dv2->db_data;

		/* limit to i4 values  */
		if ( i8temp > MAXI4 || i8temp < MINI4LL )
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "perm_type i overflow"));

		i = (DB_DT_ID) i8temp;
		break;
	    default:
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "perm_type i length"));
	}


	if ((dv1->db_datatype != DB_INT_TYPE) || 
		(dv2->db_datatype != DB_INT_TYPE) ||
		((i != PERM_LOCAL) && (i != PERM_SQL92)) )
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

	MEfill(2*DB_TYPE_MAXLEN, ' ', rdv->db_data);

	/*
	** second value indicates:
	**	0  - return all permit types
	**	1 - return only SQL92-compliant ones
	*/

	if (i == PERM_SQL92)
		npermtab = 5; /* Must be a slot marked as "OTHER" */

	for (i=0; i < npermtab; i++)
	{
		if (tf & permtab[i].flag)
			break;
	}	
	
	rdv->db_datatype=DB_CHA_TYPE;
	rdv->db_length=STlength(permtab[i].perm);
	MEcopy(permtab[i].perm, rdv->db_length, rdv->db_data);

	if ( (npermtab != 5) && (tf & DB_GRANT_OPTION))
	{
		MEcopy(" WITH GRANT OPTION", 18, (rdv->db_data+rdv->db_length));
		rdv->db_length +=18; 
	}
	return(E_DB_OK);
}
