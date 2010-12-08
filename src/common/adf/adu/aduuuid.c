/*
** Copyright (c) 1999, 2005 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <st.h>
#include    <id.h>
#include    <me.h>
#include    <iicommon.h>
#include    <ulf.h>
#include    <adf.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <nm.h>

/**
**
**  Name: ADUUUID.C - Routines related to UUIDs. 
**
**  Description:
**      This file contains routines related to UUIDs.  A UUID is a 
**	Universally Unique ID based on the Internet-Draft by Paul J.
**	Leach (Microsoft) and Rich Salz (Certco) of the Internet Engineering
**	Task Force (IETF) and dated 4-Feb-1998.  See cl!clf!id!iduuid.c for
**	more details.
**
**	This file defines the following externally visible routines:
**
**          adu_uuid_create()  - Return the next UUID
**          adu_uuid_to_char(u) - Return the character form of this UUID
**          adu_uuid_from_char(c)  - Return the UUID from its character form
**          adu_uuid_compare(u1,u2) - Compare 2 UUIDs -1:u1>u2, 0:u1=u2, 1:u1>u2  
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      25-Jan-1999 (shero03)
**          Initial creation.
**      21-Jan-2000 (stial01)
**          B100105, align/pad/truncate UUID data if needed, fix rdv assignment
**	15-feb-2001 (kinte01)
**	    add me.h to pick up mecopy prototypes
**	02-feb-2005 (raodi01)
**	    Added handling of creating of MAC based uuids depending on the 
**	    value of the environment variable II_UUID_MAC.  
**	28-apr-2005 (raodi01)
**	    Added the check_uuid_mac function declaration 
**	17-MAy-2005 (raodi01)
**	    Added check in check_uuid_mac, if II_UUID_MAC is not set,
**	    the function returns FALSE. 
**	23-may-2005 (abbjo03)
**	    Correct check_uuid_mac handling of the result from NMgtAt.
**	30-nov-2010 (gupsh01) SIR 124685.
**	    Prototype cleanup.
**
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/

/*
**  Definition of static variables and forward static functions.
*/
	
/*
[@static_variable_or_function_definition@]...
*/
static VOID
uuid_format(
DB_DATA_VALUE   *dv,
UUID		*uuid);
bool check_uuid_mac(void);


/*
** Name: adu_uuid_create() - ADF function instance to return a new UUID
**
** Description:
**      This function calls IDuuid_create to get the next UUID
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      25-Jan-1999 (shero03)
**          Initial creation.
*/

DB_STATUS
adu_uuid_create(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *rdv)
{
    UUID    		d;
    DB_STATUS		db_stat;
    /* Verify the result is a 16 byte binary */
    if ((rdv->db_datatype != DB_BYTE_TYPE) ||
        (rdv->db_length != sizeof(UUID)) )
    {
       db_stat = adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "uuid_create wrong type/len");
       return(db_stat);
    }
 	if (check_uuid_mac())  
		db_stat = IDuuid_create_seq(&d);
	else 
		db_stat = IDuuid_create(&d);
	/* Ignore possible return codes, e.g. RPC_S_UUID_LOCAL_ONLY */

    /* Copy the UUID into the result area */
    MEcopy(&d, rdv->db_length, rdv->db_data);

    return(E_DB_OK);
}
bool check_uuid_mac()
{
	char *uuid_mac=NULL;
	NMgtAt("II_UUID_MAC", &uuid_mac);
	if ((uuid_mac != (char *)NULL && *uuid_mac != EOS) && 
        (STcompare(uuid_mac,"TRUE") == 0))
        {
		return TRUE;        
	}
	else
	{
		return FALSE;
	}
}

/*
** Name: adu_uuid_to_char() - ADF function instance to return char form of UUID
**
** Description:
**      This function calls IDuuid_to_string to format the UUID into its 
**	character form. aaaaaaaa-bbbb-cccc-dddd-eeeeeeee
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the UUID
**					operand.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the character representation
**					of the input UUID.
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      25-Jan-1999 (shero03)
**          Initial creation.
**	14-Oct-2008 (gupsh01)
**	    Pass in the type of intpu string to adu_movestring 
**	    routine. 
*/

DB_STATUS
adu_uuid_to_char(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    char		d[100];
    DB_STATUS		db_stat;
    UUID                u1;
    UUID                *uuid;

    /* Verify the operand is a UUID */
    if (dv1->db_datatype != DB_BYTE_TYPE)
    {
       db_stat = adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0 ,"uuid_to_char type");
       return(db_stat);
    }

    if ((ME_ALIGN_MACRO((PTR)dv1->db_data, sizeof(ALIGN_RESTRICT))
		!= (PTR)dv1->db_data) ||
		(dv1->db_length != sizeof(UUID)))
    {
	uuid = &u1;
	uuid_format(dv1, uuid);
    }
    else
	uuid = (UUID *)dv1->db_data;

    db_stat = IDuuid_to_string(uuid, d);

    if (db_stat = adu_movestring(adf_scb, (u_char *)d,(i4) STlength(d), dv1->db_datatype, rdv))
       return (db_stat);

    return(E_DB_OK);
}


/*
** Name: adu_uuid_from_char() - ADF function instance to return UUID from char
**
** Description:
**      This function calls IDuuid_from_string to format the UUID from its 
**	character form. aaaaaaaa-bbbb-cccc-dddd-eeeeeeee
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the varchar
**					operand.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE describing the UUID.
**	    .db_data			Ptr to the UUID.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      25-Jan-1999 (shero03)
**          Initial creation.
**	21-Oct-2004 (shaha03)
**	    return type for IDuuid_from_string is captured to support 
**	    failure cases in UUID.
**	11-apr-05 (inkdo01)
**	    Return reasonable error for bad IDuuid_from_string() calls.
**	30-mar-06 (toumi01) BUG 115911
**	    Protect stack against long input string corruption.
*/

DB_STATUS
adu_uuid_from_char(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    UUID    		d;
    i4			lensrc;
    char		e[100];
    char		*srcptr;
    DB_STATUS		db_stat;

    /* Verify the result is 16 byte binary UUID */
    if ((rdv->db_datatype != DB_BYTE_TYPE) ||
        (rdv->db_length != sizeof(UUID)) ) 
    {
       db_stat = adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "uuid_from_char wrong type/len");
       return(db_stat);
    }

    if (db_stat = adu_lenaddr(adf_scb, dv1, &lensrc, &srcptr))
	return (db_stat);
    if(lensrc > sizeof(e)-1)
    {
       db_stat = adu_error(adf_scb, E_AD50A0_BAD_UUID_PARM, 0);
       return(db_stat);
    }
    /* Make a string */
    MEcopy(srcptr, lensrc, &e);
    e[lensrc] = 0x00;

    if((db_stat = IDuuid_from_string(&e[0], &d)) != OK)
    {
       db_stat = adu_error(adf_scb, E_AD50A0_BAD_UUID_PARM, 0);
       return(db_stat);
    }

    /* Copy the UUID into the result area */
    MEcopy(&d, rdv->db_length, rdv->db_data);

    return(E_DB_OK);
}


/*
** Name: adu_uuid_compare() - ADF function instance to compare two UUIDs
**
** Description:
**      This function calls IDuuid_compare with the two input UUIDs.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the first
**					UUID operand.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**      dv2                             DB_DATA_VALUE describing the second
**					UUID operand.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
**
** Outputs:
**	rdv				DB_DATA_VALUE  
**					-1:  dv1 < dv2
**					 0:  dv1 = dv2
**					 1:  dv1 > dv2
**	    .db_data			Ptr to the varchar.
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9999_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      25-Jan-1999 (shero03)
**          Initial creation.
**      21-Jan-2000 (stial01)
**          B100105, align/pad/truncate UUID data if needed, fix rdv assignment
*/

DB_STATUS
adu_uuid_compare(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
    i4    		d;
    DB_DATA_VALUE	exp;
    DB_STATUS		db_stat;
    UUID                u1, u2;
    UUID                *uuid1, *uuid2;

    /* Verify the result is an integer and both operands are UUIDs */
    if ((dv1->db_datatype != DB_BYTE_TYPE) ||
	(dv2->db_datatype != DB_BYTE_TYPE) ||
	(rdv->db_datatype != DB_INT_TYPE))
    {
       db_stat = adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "uuid_compare type");
       return(db_stat);
    }

    if ((ME_ALIGN_MACRO((PTR)dv1->db_data, sizeof(ALIGN_RESTRICT))
		!= (PTR)dv1->db_data) ||
		(dv1->db_length != sizeof(UUID)))
    {
	uuid1 = &u1;
	uuid_format(dv1, uuid1);
    }
    else
	uuid1 = (UUID *)dv1->db_data;

    if ((ME_ALIGN_MACRO((PTR)dv2->db_data, sizeof(ALIGN_RESTRICT))
		!= (PTR)dv2->db_data) ||
		(dv2->db_length != sizeof(UUID)))
    {
	uuid2 = &u2;
	uuid_format(dv2, uuid2);
    }
    else
	uuid2 = (UUID *)dv2->db_data;

    d = IDuuid_compare(uuid1, uuid2);

    if (rdv->db_datatype == DB_INT_TYPE && rdv->db_length == 4)
	*(i4 *)rdv->db_data = d;
    else if (rdv->db_datatype == DB_INT_TYPE && rdv->db_length == 2)
	*(i2 *)rdv->db_data = d;
    else
    {
	exp.db_datatype = DB_INT_TYPE;
	exp.db_length = 4;
	exp.db_data = (PTR) &d;
	if ((db_stat = adu_1int_coerce(adf_scb, rdv, &exp)) != E_DB_OK)
	   return (db_stat);
    }

    return(E_DB_OK);
}

static VOID
uuid_format(
DB_DATA_VALUE   *dv,
UUID		*uuid)
{
    /* Pad or Truncate byte to sizeof(UUID) and align */
    if (dv->db_length != sizeof(UUID))
    {
	if (dv->db_length >= sizeof(UUID))
	{
	    MECOPY_CONST_MACRO((char *)dv->db_data, sizeof(UUID), (char *)uuid);
	}
	else
	{
	    MEcopy((char *)dv->db_data, dv->db_length, (char *)uuid);
	    MEfill(sizeof(UUID) - dv->db_length, '\0', 
		(char *)uuid + dv->db_length);
	}
    }
    else
    {
	/* Alignment */
	MECOPY_CONST_MACRO((char *)dv->db_data, sizeof(UUID), (char *)uuid);
    }
}
/*
[@function_definition@]...
*/
