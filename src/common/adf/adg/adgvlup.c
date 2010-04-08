/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <sl.h>
#include <iicommon.h>
#include <adf.h>

/*}
** Name: adg_vlup_setnull - Move NULL byte of DB parameter from
**                          .db_length to the length of string + 1 according
**                          to the two byte count field for variable length
**                          text types.
**
** 
** Input:
**      dv                              Pointer to DB_DATA_VALUE
**
** Output:
**      dv                              Pointer to DB_DATA_VALUE
**
** Return:
**      none
**
** History:
**	08-apr-1993 (stevet)
**          Initial creation.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      15-Jul-1993 (fred)
**          Added DB_VBYTE_TYPE to the list of VLUP's
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/
VOID
adg_vlup_setnull(  
DB_DATA_VALUE *dv)
{
	i2  tcount;

	if(dv->db_datatype == -DB_VCH_TYPE ||
	   dv->db_datatype == -DB_TXT_TYPE ||
	   dv->db_datatype == -DB_VBYTE_TYPE ||
	   dv->db_datatype == -DB_LTXT_TYPE)
	{
	    /* Can be called directly from SCF so may not be align */
	    I2ASSIGN_MACRO(((DB_TEXT_STRING *)dv->db_data)->db_t_count,
			   tcount);
	    if( tcount < dv->db_length)
	    {
		/* Move null byte */
		u_char  *tdata=((DB_TEXT_STRING *)dv->db_data)->db_t_text;
		tdata[tcount] = tdata[dv->db_length - DB_CNTSIZE - 1];
	    }
	 }
}


