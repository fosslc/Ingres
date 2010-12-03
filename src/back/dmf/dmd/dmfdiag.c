/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <di.h>
#include    <sr.h>
#include    <ex.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0p.h>
#include    <dm0m.h>
#include    <dm1b.h>
#include    <dm0pbmcb.h>
#include    <dmucb.h>
#include    <dmse.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm0llctx.h>
#include    <dm0l.h>
#include    <dmd.h>
#include <evset.h>

/**
**  Name: dmfdiag - DMF diagnostic routines
**
**  Description:
**     This module contains code for providing DMF specific analysis of core
**     files. The following routines are supplied:-
**
**     dmf_diag_dump_tables   - Dump details of active tables
**
**  History:
**	22-feb-1996 - Initial port to Open Ingres
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	15-Aug-2005 (jenjo02)
**	    Bind DMF memory "types" to real values. Count and display
**	    them all.
**    25-Oct-2005 (hanje04)
**        Add prototype for dump_tcb() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	02-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype dump_atts(), dump_modify(), trim() properly.
*/

static void dump_atts(
	void (*output)(),
	DB_ATTS **atts_ptr,
	i4 number);
static void dump_modify(
	void (*output)(),
	DMP_TCB *tcb_ptr);
static PTR trim(
	PTR string,
	i4  len);
static dump_tcb(
	DMP_TCB *tcb_ptr,
	void (*output)(),
	void (*error)());


static VOID
count_cb(
i4            count[DM0M_MAX_TYPE+1][3],
DM_OBJECT	   *obj)
{
    i4		type = obj->obj_type & ~(DM0M_LONGTERM | DM0M_SHORTTERM);

    if ( type > 0 && type <= DM0M_MAX_TYPE )
    {
	/*  Increment count of objects and size used. */

	count[0][0]++;
	count[0][1] += obj->obj_size;

	/* Increment count and size by type, stuff "tag". */

	count[type][0]++;
	count[type][1] += obj->obj_size;
	count[type][2] = obj->obj_tag;
    }
}

/*{
**  Name: dmf_diag_dmp_pool - Dump all dmf memeory pools
**
**  Description:
**     This routine dumps all the dmf memeory pools
**
**  History:
**	13-Mch-1996 (prida01)
**	    Created
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototyped.
*/
VOID
dmf_diag_dmp_pool(void)
{
    i4                 count[DM0M_MAX_TYPE+1][3];
    i4                 err_code;
    DB_STATUS               status;
    i4                 one = 1;
    i4		    flag = 1;
    i4		    i;
    DB_ERROR	    local_dberr;

    TRdisplay("\n\n%10* SUMMARY OF DMF CONTROL BLOCKS at %@.\n\n");
    TRdisplay("%10* Type  Tag     Count     Total Size\n\n");

    /*  Calculate a control block summary. */

    MEfill(sizeof(count), 0, (char *)count);
    status = dm0m_search((DML_SCB*)NULL, 0, count_cb, (i4 *)count, &local_dberr);
    for ( i = 1; i <= DM0M_MAX_TYPE; i++ )
    {
	if ( count[i][0] )
	    TRdisplay("%10* %4d %.4s %9d     %10d\n",
		    i, &count[i][2], count[i][0], count[i][1]);
    }

    TRdisplay("%10*    TOTALS %9d     %10d\n\n",
			count[0][0], count[0][1]);
 
    dmd_fmt_cb(&one, (DM_OBJECT *)dmf_svcb);
 
    /*  Scan the pool dumping control blocks. */
 
    status = dm0m_search((DML_SCB*)NULL, 0, dmd_fmt_cb, &flag, &local_dberr);
    if (status != E_DB_OK)
        TRdisplay("Error searching the memory pool.");
    return;

}
/*{
**  Name: dmf_diag_dump_tables - Dump details of open tables
**
**  Description:
**     This routine dumps details of open user tables. It attempts to dump
**     the information in a form as close as possible to the SQL required to
**     recreate the tables.
**
**     Only base tables and indices are currently supported
**
**  Inputs:
**     void TRdisplay(PTR format,...)      Output function
**     void (*error)(PTR format,...)       Error function
**
**  History:
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototyped.
*/

VOID
dmf_diag_dump_tables( void (*output)(), void (*error)() )
{


        /* The SVCB contains a series of hash buckets each of which is the */
        /* head of a chain of TCBs - follow each of these in turn          */

        DMP_HCB *hcb_ptr;
        int c;
    if (dmf_svcb)
    {
        hcb_ptr = dmf_svcb->svcb_hcb_ptr;

        for(c=0;c<HCB_HASH_ARRAY_SIZE;++c)
        {
            DMP_TCB *tcb_ptr;

            tcb_ptr = hcb_ptr->hcb_hash_array[c].hash_q_next;

            while(tcb_ptr !=(DMP_TCB *) &hcb_ptr->hcb_hash_array[c])
            {
                dump_tcb(tcb_ptr,output,error);
                tcb_ptr=tcb_ptr->tcb_q_next;
            }
            
        }
    }
    return;
}

/*{
**  Name: dump_tcb - Dump a single TCB and related index TCBs
**
**  Description:
**      This routine dumps a single TCB and details of all related index TCBs
**
**  Inputs:
**     u_i4 address                       Address of TCB to dump
**     void TRdisplay(PTR format,...)      Output function
**     void (*error)(PTR  format,...)      Error function
**
**  History:
** 	13-Mch-1996 (prida01)
**	    Modify to print out system catalogs.
**	20-Oct-2009 (kschendel) SIR 122739
**	    Changes for new rowaccessor scheme.
*/

static
dump_tcb( DMP_TCB *tcb_ptr, void (*output)(), void (*error)() )
{
    i4  c;


    /*
    ** We are only interested in valid TCBs for non temporary user tables
    ** and are not interested in normal or extended catalogues (we know about
    ** them already)
    */

    if(tcb_ptr->tcb_status & TCB_VALID &&
       tcb_ptr->tcb_temporary==TCB_PERMANENT &&
       tcb_ptr->tcb_sysrel==TCB_USER_REL)
    {
        /* Output standard create table header */
	(*output)("-----");
	if (tcb_ptr->tcb_rel.relstat &TCB_CATALOG)
	    (*output)("SYSTEM CATALOG ");
	else
	    (*output)("USER ");
	if (tcb_ptr->tcb_rel.relstat &TCB_VIEW)
	    (*output)(" VIEW");
	else
	    (*output)("TABLE");
	(*output)("-----\n");
        (*output)("create table %s (\n",
            trim(tcb_ptr->tcb_rel.relid.db_tab_name,
            sizeof(DB_TAB_NAME)));

        /* Dump attributes */

        dump_atts(output,tcb_ptr->tcb_data_rac.att_ptrs,
            tcb_ptr->tcb_rel.relatts);

        /* Close off create table */

        (*output)(")\\p\\g\n");

        /* Output modify information */

        dump_modify(output,tcb_ptr);

        /* Output details of any secondary indices */

        if(tcb_ptr->tcb_index_count)
        {
            DMP_TCB *index_tcb;
            i4  c;

            index_tcb = tcb_ptr->tcb_iq_next;

            for(c=0;c<tcb_ptr->tcb_index_count;++c)
            {
                DB_ATTS **atts_ptr;
                i2 *key_atts;
                i4  number;
                PTR unique="";
                i4  cc;

                if(index_tcb->tcb_unique_index)
                {
                    unique="unique ";
                }

                /* Output create index header */

                (*output)("create %sindex %s\n",unique,
                    trim(index_tcb->tcb_rel.relid.db_tab_name,
                    sizeof(DB_TAB_NAME)));
                (*output)("on %s (\n",
                    trim(tcb_ptr->tcb_rel.relid.db_tab_name,
                    sizeof(DB_TAB_NAME)));

                /* Dump key attributes - except the last which is the tidp */

                atts_ptr=index_tcb->tcb_data_rac.att_ptrs;
                number=index_tcb->tcb_rel.relatts-1;

                for(cc=0;cc<number;++cc)
                {
                    DB_ATTS *attribute;

                    attribute=atts_ptr[cc];

                    (*output)("  %s%c\n",attribute->attnmstr,
                        cc==(number-1)?' ':',');
                }

                /* Complete create index */

                (*output)(") with key=");

                key_atts=index_tcb->tcb_att_key_ptr;

                for(cc=0;cc<index_tcb->tcb_keys-1;++cc)
                {
                    DB_ATTS *attribute;
                    i4  key_att=key_atts[cc];

                    attribute=atts_ptr[key_att-1];

                    (*output)("%c%s",cc==0?'(':',', attribute->attnmstr);
                }

                (*output)(")\\p\\g\n");

                /* Output modify information */

                dump_modify(output,index_tcb);

                index_tcb=index_tcb->tcb_q_next;
            }
        }
    }

}

/*{
**  Name: dump_atts - Dump attribute information
**
**  Description:
**     This routine is passed the address of an array of DB_ATTS pointers
**     and the number of entries in the array. It displays details of each
**     attribute in turn.
**
**  Inputs:
**     void TRdisplay(char *fmt,...)    Routine to call with output
**     i4  address                      Address of DB_ATTS array
**     i4  number                       Number of attribute entries in array
**
**  History:
**	03-aug-2006 (gupsh01)
**	    Added new ANSI date/time types.
*/

static void
dump_atts( void (*output)(), DB_ATTS **atts_ptr, i4  number )
{
    i4  c;

    for(c=0;c<number;++c)
    {
        DB_ATTS *attribute;
        PTR type="unknown";
        PTR with_null="";
        PTR with_default="";
        i4  length;

        attribute=atts_ptr[c];

        switch(abs(attribute->type))
        {
            case DB_INT_TYPE:
                type="integer%d";
                break;
            case DB_FLT_TYPE:
                type="float%d";
                break;
            case DB_CHR_TYPE:
                type="c%d";
                break;
            case DB_TXT_TYPE:
                type="text(%d)";
                length-=2;
                break;
            case DB_DTE_TYPE:
                type="ingdate";
                break;
            case DB_ADTE_TYPE:
                type="ansidate";
                break;
	    case DB_TMWO_TYPE:
		type="time without time zone";
		break;
	    case DB_TMW_TYPE:
		type="time with time zone";
        	break;
	    case DB_TME_TYPE:
		type="time with local time zone";
		break;
	    case DB_TSWO_TYPE:
		type="timestamp without time zone";
        	break;
	    case DB_TSW_TYPE:
		type="timestamp with time zone";
        	break;
      	    case DB_TSTMP_TYPE:
        	type="timestamp with local time zone";
		break;
	    case DB_INYM_TYPE:
		type="interval year to month";
        	break;
	    case DB_INDS_TYPE:
		type="interval day to second";
        	break;
            case DB_MNY_TYPE:
                type="money";
                break;
            case DB_DEC_TYPE:
                type="decimal(%d,%d)";
                break;
            case DB_LOGKEY_TYPE:
                type="object_key";
                break;
            case DB_TABKEY_TYPE:
                type="table_key";
                break;
            case DB_CHA_TYPE:
                type="char(%d)";
                break;
            case DB_VCH_TYPE:
                length-=2;
                type="varchar(%d)";
                break;
        }

        /* Handle with null clause */

        if(attribute->type < 0)
        {
           with_null=" with null";
           length=attribute->length-1;
        }
        else
        {
            length=attribute->length;
        }

        /* Handle with default clause */
	
/*
        if(attribute->defaultID.db_tab_base!=DB_DEF_NOT_DEFAULT)
        {
            with_default=" with default";
        }
*/

        (*output)("   %s ",attribute->attnmstr);
        (*output)(type,length,attribute->precision);
        (*output)("%s%s%c\n",with_null,with_default,c==(number-1)?' ':',');
    }
}

/*{
**  Name: dump_modify - Generate a modify statement
**
**  Description:
**     This routine generates a modify statement describing the structure of
**     a particular table or index
**
**  Inputs:
**     void TRdisplay(char *fmt,...)     Routine to do output
**     i4   address                       Address of TCB
**
**  History:
*/

static void
dump_modify( void (*output)(), DMP_TCB *tcb_ptr )
{
    DMP_LOCATION *location;
    i4  c;


    (*output)("modify %s to ",trim(tcb_ptr->tcb_rel.relid.db_tab_name,
        sizeof(DB_TAB_NAME)));

    /* Work out structure */

    switch(tcb_ptr->tcb_rel.relspec)
    {
       case TCB_HEAP:
           (*output)("heap with\n");
           break;
       case TCB_ISAM:
           (*output)("isam with fillfactor=%d,\n",
               tcb_ptr->tcb_rel.reldfill);
           break;
       case TCB_HASH:
           (*output)("hash with fillfactor=%d,minpages=%d,maxpages=%d,\n",
               tcb_ptr->tcb_rel.reldfill,tcb_ptr->tcb_rel.relmin,
               tcb_ptr->tcb_rel.relmax);
           break;
       case TCB_BTREE:
           (*output)("btree with fillfactor=%d,leaffill=%d,nonleaffill=%d,\n",
               tcb_ptr->tcb_rel.reldfill,
               tcb_ptr->tcb_rel.rellfill,
               tcb_ptr->tcb_rel.relifill);
           break;
    }

    /* Dump location details */

    (*output)("location=");

/*
    location=tcb_ptr->tcb_table_io.tbio_location_array;

    for(c=0;c<tcb_ptr->tcb_table_io.tbio_loc_count;++c)
    {
        (*output)("%c%s",c==0?'(':',',
            trim(location->loc_name.db_loc_name,sizeof(DB_LOC_NAME)));
        location+=sizeof(DMP_LOCATION);
    }
*/

    (*output)(")\\p\\g\n\n");
}

/*{
**  Name: trim - Trim spaces off a string
**
**  Description:
**     This routine returns a copy of the passed string with all spaces
**     trimmed off the end. A static buffer is used so care must be taken
**     to copy out the data before the next call
**
**  Inputs:
**     PTR string          String to trim
**     i4  len               Length of string
**
**  Returns:
**     PTR                Pointer to trimmed string
**
**  History:
*/

static PTR
trim( PTR string, i4  len )
{
    static char buffer[100];
    i4  c;

    for(c=0;c<len && string[c]!=' ';++c)
       buffer[c]=string[c];

    buffer[c]=EOS;

    return(buffer);
}
