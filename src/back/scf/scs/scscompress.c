/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**  LEVEL1_OPTIM = axp_osf
*/

/**
**
**  Name: SCSCOMPRESS.C - Compress data before sending to front end
**
**  Description:
**      This file contains routines responsible for compressing data
**      sent from the SCF to the front end.
**
**  History:
**      05-apr-96 (loera01)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Nov-2000 (hweho01)
**          Added LEVEL1_OPTIM (-O1) for axp_osf.  
**          With option "-O2" ("-O"), the compiler generates incorrect   
**          codes that cauase unaligned access error during ingstart/ 
**          createdb.
**          Compiler version: Compaq C V6.1-019 -- V6.1-110.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Add include of sc0e.h, use new form sc0ePut().
**  16-Jun-2009 (thich01)
**      Treat GEOM type the same as LBYTE.
**  20-Aug-2009 (thich01)
**      Treat all spatial types the same as LBYTE.
*/

/*
** Name: scs_compress - Compress tuples containing varchars
**
** Description:
**      This routine accepts a set of tuples, and removes all garbage
**      data from the varchars.
**
**      scs_compress searches through the tuple descriptor, looking for 
**      columns that contain varchars.  If it finds that there are no varchars
**      in the row, it simply returns without modifying anything.
**    
**      If scs_compress determines there are varchars in the tuple, it
**      searches through each row for varchars.  When it finds a varchar,
**      scs_compress uses the string length contained in the first two bytes 
**      and the nullability of the associated datatype to determine how many 
**      chars it can remove from the tuple.  It then repositions the null flag 
**      at the termination point of the string, if the column is nullable, and 
**      maps the next column or row to the next character.
**
**      This routine is called from scs_scsqncr, just before it
**      sends tuple data to the front end.  
**
**	The result is a data buffer that does not map directly to the
**	tuple descriptor.  The front end must reverse the process
**      described above, in order to process queries correctly.
**
**	Note that this routine is unique to queries only.  It has no
**      relationship to varchar compression routines that apply to the 
**      modification of tables in compressed format.  This routine is
**      not invoked when sending data from the front to the back end.
** 
** Inputs:
**      scb	    SCF Session control block for this session
**      qe_ccb      QEF Request control block
**
** Outputs:
**      total_dif   The number of chars removed
**      scb
**	    .scb->scb_cscb.cscb_tuples	The compressed tuple data
**
**	Returns:
**	    E_DB_OK  Everything ok
**	    E_SC039B_VAR_LEN_INCORRECT problem found with length
**	Exceptions:
**	    none
**
** Side Effects:
**	    Invalidates the tuple descriptor by compressing the data.
**          Any client of the data should be prepared to decompress the 
**          data buffer before further processing.
**          
**
** History:
**	18-aug-95  (loera01)
**	   Created
**      14-jun-96  (loera01) 
**         Modified the extraction of datatype for unaligned platforms so that 
**         the true datatype value was extracted, rather than the absolute value 
**         of the datatype.
**      04-nov-1996 (loera01)
**         Changed handling of null flag so that ADF_NVL_BIT was referenced
**         instead of a literal 1.
**      14-may-2001 (loera01)
**         Added support for Unicode varchars (DB_NVCHR_TYPE).  Note that
**         for DB_NVCHR_TYPE, the byte size of the varchar is double
**         the embedded varchar length (length * DB_ELMSZ).        
**	12-dec-2001 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent "createdb" from causing
**	    the DBMS server to SEGV.
**	23-jun-2003 (somsa01)
**	    Updated i64_win such that, instead of a NO_OPTIM we just turn off
**	    using the memcpy() intrinsic.
**	22-Feb-2008 (kiria01) b119976
**	    Defend against corrupted varchar length field corruption.
**	    Rename length to size for VARxxxx distinction and removed
**	    unused variables. Also scs_compress now returns an error
**	    if the corrupt data is seen.
**	29-Feb-2008 (kiria01) b119976
**	    Correction to above change - NULL checking must precede length
**	    validation as buffer might have been otherwise uninitialised.
**	30-Sep-2009 (wanfr01) b122659
**	    Can't access attlen directly or you will get a SIGBUS.
*/
#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <st.h>
#include    <sl.h>
#include    <er.h>
#include    <ex.h>
#include    <cs.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <adp.h>
#include    <gca.h>

#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <ddb.h> 

#include    <me.h>
#include    <mecl.h>
#include    <qsf.h> 

#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <scf.h>
#include    <dudbms.h>

#include    <sc.h>
#include    <scc.h> 
#include    <scs.h>
#include    <scd.h> 
#include    <sc0e.h> 

#ifdef i64_win
/* Turn off the usage of the memcpy() intrinsic. */
#pragma function(memcpy)	/* Turn off usage of memcpy() intrinsic. */
#endif

DB_STATUS
scs_compress(GCA_TD_DATA *tup_desc, char *local_row_buf, 
    i4  row_count, i4  *total_dif)

{
    DB_STATUS		status = E_DB_OK;	/* Default to success */
    u_i2 		local_length=0;		/* Length of varchar */ 
    i4  		adj_length;		/* Adjusted varchar length */
    i4  		max_length;		/* Max length allowed */
    u_char 		null_flag;		/* Nullness of varchar */
    i4  		j,k,l,m;		/* Loop counters */
    GCA_COL_ATT 	*cur_att;		/* Column attributes */
    GCA_COL_ATT 	*err_col = NULL;	/* Column attributes of error column */
    i4  		dif;			/* Dif of db_length and */
                      				/* length of varchar */
    DB_DT_ID 		type;                   /* Datatype of cur. column */
    i4			size;			/* Size of column in bytes */
    i4  		total_len=0;            /* Total len of buffer */
    char 		*compress_buf;		/* Compressed buffer */

    /*
    **  Initialize local variables from the SCB control block.
    */

    *total_dif = 0;
    compress_buf = local_row_buf; 

    for (l = 0; l < row_count; l++) 
    { 
       cur_att = (GCA_COL_ATT *) &tup_desc->gca_col_desc[0];
       for (k = 0; k < tup_desc->gca_l_col_desc; k++)
       {
	  i4 element_size = sizeof(char);

#ifdef BYTE_ALIGN
	  MECOPY_CONST_MACRO((PTR)&cur_att->gca_attdbv.db_datatype,
	      sizeof(DB_DT_ID), (PTR)&type);
	  MECOPY_CONST_MACRO(&cur_att->gca_attdbv.db_length,
	      sizeof(i4), &size);
#else
          type = (DB_DT_ID) cur_att->gca_attdbv.db_datatype;
	  size = cur_att->gca_attdbv.db_length;
#endif
          switch (abs(type))
          {
             case DB_LVCH_TYPE:
	     case DB_LBYTE_TYPE:
             case DB_GEOM_TYPE:
             case DB_POINT_TYPE:
             case DB_MPOINT_TYPE:
             case DB_LINE_TYPE:
             case DB_MLINE_TYPE:
             case DB_POLY_TYPE:
             case DB_MPOLY_TYPE:
             case DB_GEOMC_TYPE:
             {
                ADP_PERIPHERAL periph;
                MEcopy((PTR)local_row_buf,ADP_HDR_SIZE, (PTR)&periph);
                if (periph.per_length1 == 0)
                {
                   null_flag = 1;
                }
                else
                {
                   null_flag = 0;
                }

                /* 
                ** Copy blob header.
                */
                MEcopy((PTR)local_row_buf,ADP_HDR_SIZE+sizeof(i4),
                  ((PTR)compress_buf+total_len));
                total_len += ADP_HDR_SIZE + sizeof(i4);
                local_row_buf += ADP_HDR_SIZE + sizeof(i4);
   
                /*
                ** Find the length of the BLOB in this tuple.
                */
                MECOPY_CONST_MACRO((PTR)local_row_buf, sizeof(u_i2),
                   &local_length); 
                if (null_flag && (type < 0))
                {
                   adj_length = sizeof(char);
                }
                else if (null_flag && (type > 0))
                {
                   adj_length = 0;
                }
                else if (!null_flag && (type > 0))
		{
                   adj_length = local_length + sizeof(i2) + sizeof(i4);
		}
		else
                {
                   adj_length = local_length + sizeof(i2) + sizeof(char) +
		       sizeof(i4);
                }
                MEcopy((PTR)local_row_buf,adj_length,
                   ((PTR)compress_buf + total_len));
                total_len += adj_length;
                local_row_buf += adj_length;
                break;
             }

	     case DB_NVCHR_TYPE:
		element_size = DB_ELMSZ;
		/* FALLTHROUGH */
             case DB_VCH_TYPE:
             case DB_TXT_TYPE:
             case DB_VBYTE_TYPE:
             case DB_LTXT_TYPE:

		/* Always 2 byte VAR len */
		adj_length = DB_CNTSIZE;
		if (type < 0) /* nullable column */
		{
		   /* A NULL marker byte is present */
		   adj_length++;
		   /* Pick up NULL marker from last byte in the column data */
		   null_flag = ((u_char *)local_row_buf)[size-1] & ADF_NVL_BIT;
		   if (null_flag)
		   {
		      /* We do have a null here - make sure VAR len is 0 */
		      local_length = 0;
		      MECOPY_CONST_MACRO(&local_length, sizeof(u_i2),
			(PTR)local_row_buf);
		   }
		}
		/* Pick up and validate VAR len */
		max_length = (size - adj_length)/element_size;
		/* Pick up and validate VAR len */
		MECOPY_CONST_MACRO((PTR)local_row_buf, DB_CNTSIZE,
			&local_length);
		if (local_length > max_length)
		{
		   /* We have a problem - the VAR len is inconsistent with
		   ** the column size. To protect the dbms from massive data
		   ** corruption we limit the data length at this point.
		   ** In this context there is little we can do to flag the
		   ** problem. For the moment output and indication to the
		   ** dbms log indicating the column name affected and how.
		   */
		   status = E_SC039B_VAR_LEN_INCORRECT;
		   if (!err_col)
		   {
			i4 attlen;

			/* Report first occurance */
			err_col = cur_att;
			MEcopy(&(err_col->gca_l_attname), sizeof(i4), &attlen);
			sc0ePut(NULL, status, NULL, 3,
				attlen, (PTR)err_col->gca_attname,
				sizeof(local_length), &local_length,
				sizeof(max_length), &max_length);
			sc0e_uput(status, 0, 3,
				attlen, (PTR)err_col->gca_attname,
				sizeof(local_length), &local_length,
				sizeof(max_length), &max_length,
				0, (PTR)0,
				0, (PTR)0,
				0, (PTR)0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		   }
		   local_length = max_length;
		   MECOPY_CONST_MACRO(&local_length, DB_CNTSIZE, (PTR)local_row_buf);
		   /* Set the status and note first error column but otherwise
		   ** continue processing as the error has been fixed up in some
		   ** measure
		   */
		}
		adj_length += local_length * element_size;
		if (type < 0) /* nullable column */
		{
		   /* Copy the NULL marker to the end of the varchar data
		   ** so that it is copied as part of the later MEcopy */
		   ((u_char *)local_row_buf)[adj_length-1] = null_flag;
		}
		/* Add the reduction */
                *total_dif += size - adj_length;

                MEcopy((PTR)local_row_buf, adj_length, ((PTR)compress_buf+total_len));
                total_len += adj_length;
                local_row_buf += size; 
                break;

             default:
                MEcopy((PTR)local_row_buf, size, ((PTR)compress_buf+total_len));
                total_len += size;
                local_row_buf += size;
                break;
          }

# ifdef BYTE_ALIGN
          {
              i4  l_attname;

              MEcopy(&cur_att->gca_l_attname, sizeof(l_attname),&l_attname);
              if (l_attname == 0)
              {
                  cur_att = (GCA_COL_ATT *) &tup_desc->gca_col_desc[k+1];
              }
              else
              {
   	          cur_att = (GCA_COL_ATT *) &cur_att->gca_attname[l_attname];
              }
          }  
# else
          {
                if (cur_att->gca_l_attname == 0)
                {
                   cur_att = (GCA_COL_ATT *) &tup_desc->gca_col_desc[k+1];
                }
                else
                {
                   cur_att = (GCA_COL_ATT *) 
                      &cur_att->gca_attname[cur_att->gca_l_attname];
                }
          }
# endif

       }
    }
    return status;
}
