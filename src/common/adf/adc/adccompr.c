/*
** Copyright (c) 2004 Ingres Corporationc
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADCCOMPR.C - Compress or expand a datatype.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF calls "adc_compr()" which
**      compress or expand a column value.                            
**
**
**
**
**  History:
**  16-may-1996 (shero03)
**      Initial creation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/



/*{
** Name: adc_compr() -   Compress or expand a data value for a datatype
**
** Description:
**      This function is the external ADF call "adc_compr()" which
**      compresses or expands the data value for the supplied datatype.
**
** Inputs:
**  adf_scb            Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**    .ad_ebuflen      The length, in bytes, of the buffer
**                       pointed to by ad_errmsgp.
**    .ad_errmsgp      Pointer to a buffer to put formatted
**                       error message in, if necessary.
**  adc_dv             DB_DATA_VALUE describing the data value
**                       to compress or expand.                 
**      .db_datatype   The datatype to compress or expand
**      .db_prec       The precision/scale to compress or expand
**      .db_length     Length of the data value which
**                       is being compressed or expanded          
**  adc_flag           Flag which denotes compression (TRUE) 
**                       or expansion (FALSE)          
**  adc_buf            Pointer to the buffer where the expansed or
**                       contracted value will be produced
**
** Outputs:
**  adf_scb            Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**                       error occurs the following fields will
**                       be set.  NOTE: if .ad_ebuflen = 0 or
**                       .ad_errmsgp = NULL, no error message
**                       will be formatted.
**    .ad_errcode      ADF error code for the error.
**    .ad_errclass     Signifies the ADF error class.
**    .ad_usererr      If .ad_errclass is ADF_USER_ERROR,
**                       this field is set to the corresponding
**                       user error which will either map to
**                       an ADF error code or a user-error code.
**    .ad_emsglen      The length, in bytes, of the resulting
**                       formatted error message.
**    .adf_errmsgp     Pointer to the formatted error message.
**  adc_outlen         The amount of data written to the buffer.  
**                       If there was an error this is set to 0.     
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**  16-may-1996 (sher03)
**      Initial creation.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adc_compr(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dv,
i4                  adc_flag,
char                *adc_buf,
i4             *adc_outlen)
# else
DB_STATUS
adc_compr( adf_scb, adc_dv, adc_flag, adc_buf, adc_outlen)
ADF_CB              *adf_scb;
DB_DATA_VALUE       *adc_dv;
i4                  adc_flag;
char                *adc_buf;
i4             *adc_outlen;
# endif
{
    DB_STATUS      db_stat = E_DB_OK;
    i4			bdt     = abs((i4) adc_dv->db_datatype);
    i4			mbdt;

    mbdt = ADI_DT_MAP_MACRO(bdt);

    for (;;)
    {
       if ( mbdt <= 0 ||
            mbdt > ADI_MXDTS ||
            Adf_globs->Adi_dtptrs[mbdt] == NULL )
       {
         db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
         break;
       }



	/*
	** Now call the low-level routine to do the work.  Note that if the
	** datatype is nullable, we use a temp DV_DATA_VALUE which gets set
	** to the corresponding base datatype and length.
	*/

       if (adc_dv->db_datatype > 0)     /* non-nullable */
       {
         db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
                      adi_dt_com_vect.adp_compr_addr)
                      (adf_scb, adc_dv, adc_flag, adc_buf, adc_outlen);
       }
       else                 /* nullable */
       {
         DB_DATA_VALUE  tmp_dv;

         tmp_dv.db_datatype = bdt;
         tmp_dv.db_data     = adc_dv->db_data;
         tmp_dv.db_prec     = adc_dv->db_prec;
         tmp_dv.db_length   = adc_dv->db_length - 1;

         db_stat = (*Adf_globs->Adi_dtptrs[mbdt]->
                      adi_dt_com_vect.adp_compr_addr)
                      (adf_scb, &tmp_dv, adc_flag, adc_buf, adc_outlen);
       }

       break;
    }	    /* end of for stmt */

    return(db_stat);
}

