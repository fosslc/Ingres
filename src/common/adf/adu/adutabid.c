/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/**
**
**  Name: ADUTABID.C - Routines dealing with INGRES table IDs.
**
**  Description:
**      This file contains all of the low level ADF routines required to perform
**      any of the INGRES functions that deal with INGRES table IDs.
**
**          adu_di2tb() - Generates DI file name from INGRES table ID.
**          adu_tb2di() - Generates the unique part of an INGRES table ID
**			  from a DI file name.
[@func_list@]...
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:    $Log-for RCS$
**      30-sep-87 (thurston)
**          Initial creation.
**      05-jan-1993 (stevet)
**          Added function prototypes.
[@history_template@]...
**/


/*
**  Definition of static variables and forward static functions.
*/

static	char        ad0_fnchars[16] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
					'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
				      };


/*{
** Name: adu_tb2di() - Generate DI file name from INGRES table ID.
**
** Description:
**      This routine performs the INGRES function `ii_tabid_di(i,i)', 
**      which generates a DI file name from the supplied INGRES table ID. 
**      The table ID is a pair of integers (usually i4's) which represent
**      a base table ID and an index ID.  If the index ID is non-zero, that 
**      number is used to generate the file name.  If it is zero, then the 
**      base table ID is used.  Only the file name portion of the file spec
**	will be generated -- no file extension.  (i.e. "aaaaaakj", not
**	"aaaaaakj.t00")
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-sep-87 (thurston)
**          Initial creation.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	12-Feb-2004 (schka24)
**	    Produce correct name for partitions.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring routine, 
**	    in order to allow for binary data to be copied untainted. Since the 
**	    character input to movestring was created in this routine itself 
**	    we pass in DB_NODT to this routine.
[@history_template@]...
*/

DB_STATUS
adu_tb2di(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *base_dv,
DB_DATA_VALUE	    *idx_dv,
DB_DATA_VALUE	    *rdv)
{
    i4			i;
    i4			tabid;
    char		c[8];


    if (    base_dv->db_datatype != DB_INT_TYPE
	||   idx_dv->db_datatype != DB_INT_TYPE
       )
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));


    switch (idx_dv->db_length)
    {
      case 8:
	tabid = (i4) (*(i8 *)idx_dv->db_data);
	break;
      case 4:
	tabid = *(i4 *)idx_dv->db_data;
	break;
      case 2:
	tabid = *(i2 *)idx_dv->db_data;
	break;
      case 1:
	tabid = I1_CHECK_MACRO(*(i1 *)idx_dv->db_data);
	break;
      default:
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	break;
    }

    if (tabid == 0)
    {
	switch (base_dv->db_length)
	{
	  case 8:
	    tabid = (i4) (*(i8 *)idx_dv->db_data);
	    break;
	  case 4:
	    tabid = *(i4 *)base_dv->db_data;
	    break;
	  case 2:
	    tabid = *(i2 *)base_dv->db_data;
	    break;
	  case 1:
	    tabid = I1_CHECK_MACRO(*(i1 *)base_dv->db_data);
	    break;
	  default:
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    break;
	}
    }

    tabid &= DB_TAB_MASK;
    i = 8;
    while (--i >= 0)
    {
	c[i] = ad0_fnchars[tabid & 0x0f];
	tabid >>= 4;
    }

    return (adu_movestring(adf_scb, (u_char *) c, (i4) 8, DB_NODT, rdv));
}


/*{
** Name: adu_di2tb() - Generates the unique part of an INGRES table ID
**		       from a DI file name.
**
** Description:
**      This routine performs the INGRES function `ii_di_tabid(char)', 
**      which generates an i4 that is the unique portion of an INGRES table ID,
**	from a DI file name.  A table ID is a pair of integers (usually i4's)
**	Which represent a base table ID and an index ID.  If the index ID is
**      non-zero, that number is the unique portion.  If it is zero, then the
**      base table ID is the unique portion. 
**
**	Note that given an INGRES table ID, there is exactly one DI file name
**	that can be derived from it.  However, given a DI file name, the only
**	thing that can be derived about the INGRES table ID is the value of the
**	i4 that was the unique ID.  You cannot tell if that represented the base
**	table ID, or the index ID, and obviously you cannot derive the other i4.
**	You also cannot know if the table was a partition, so the returned ID
**	will assume that it wasn't.
**
**	The DI file name is supplied as only the file name portion of the file
**	spec -- no extension.  (i.e. "aaaaaakj", not "aaaaaakj.t00")
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-sep-87 (thurston)
**          Initial creation.
[@history_template@]...
*/

DB_STATUS
adu_di2tb(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *diname_dv,
DB_DATA_VALUE	    *rdv)
{
    i4			i;
    i4			len;
    register i4	next;
    i4			*tabid;
    char		*c;
    DB_STATUS		db_stat;


    /* Result must be an i4:						    */

    if (rdv->db_datatype != DB_INT_TYPE  ||  rdv->db_length != 4)
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

    tabid = (i4 *) rdv->db_data;
    *tabid = 0;
    

    /* Check name to make sure it is the right length			    */

    if (db_stat = adu_lenaddr(adf_scb, diname_dv, &len, &c))
	return (db_stat);
    while (len >= 8  &&  c[len - 1] == ' ')
	len--;
	
    if (len != 8)
    {
	return (adu_error(adf_scb, E_AD5005_BAD_DI_FILENAME, 2,
			  (i4) len, (i4 *) c));
    }


    /* Now calculate the i4 based on these eight characters:		    */

    for (i = 0; i < 8; i++)
    {
	for (next = 0; next < 16; next++)
	{
	    if (ad0_fnchars[next] == c[i])
		break;
	}

	if (next > 0x0f)
	{
	    return (adu_error(adf_scb, E_AD5005_BAD_DI_FILENAME, 2,
			      (i4) len, (i4 *) c));
	}

	*tabid = (*tabid << 4) | next;
    }

    return (E_DB_OK);
}

/*
[@function_definition@]...
*/
