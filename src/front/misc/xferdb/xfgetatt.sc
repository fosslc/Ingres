/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<me.h>
# include	<iicommon.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<st.h>
# include	<cv.h>
# include	<er.h>
# include       <si.h>
# include       <lo.h>
# include	<fe.h>
# include	<ug.h>
# include       <ui.h>
# include       <uigdata.h>
# include	<adf.h>
# include	<add.h>
# include	<afe.h>
EXEC SQL INCLUDE <xf.sh>;
# include	"erxf.h"

/*
** Fool MING, which doesn't know about EXEC SQL INCLUDE
# include <xf.qsh>
*/

/**
** Name:	xfgetatt.sc - fill array of attribute structs for a relation.
**
** Description:
**	This file defines:
**		xfgetcols - 	fill column-description structures.
**
** History:
**	25-aug-1987 (rdesmond)
**		written.
**	18-aug-88 (marian)
**		Changed retrieve statement to reflect column name changes in
**	22-feb-89 (marian)	bug 4789
**		Add a sort clause to the retrieve from iicolumns so the
**		create statement is generated in the same order as the
**		original table.
**	16-may-89 (marian)
**		Add support for logical keys.  Add a new query to get
**		information about logical keys from iicolumns.  This is
**		done as 2 separate retrieves because the standard catalogs
**		will only be updated for TERMINATOR.
**	08-jan-90 (marian)	bug 9148
**		Handle UDT columns correctly.  Store information in the
**		attinfo structure for the column_internal_datatype to be 
**		retrieved later.  Retrieve column_internal_ingtype to
**		determine if this is a UDT or logical key.  If the -c flag 
**		is used to unload a UDT column, generate error 
**		E_XF0029_Warn_udt_and_c_flag.  Add new portable parameter 
**		to determine if -c flag specified.
**	29-jan-90 (marian)
**		Pass NULL to afe_ctychk() since it has been changed to support
**		packed decimal.  This will need to be changed when unloaddb/
**		copydb is modified to support packed decimal.  For now, there
**		is no packed decimal support so NULL needs to be passed.
**      05-mar-1990 (mgw)
**              Changed #include <erxf.h> to #include "erxf.h" since this is
**              a local file and some platforms need to make the destinction.
**	26-mar-90 (marian)	bug 20272
**		Return FAIL when FEafeerr is called.  Since FAIL was not
**		being returned, unloaddb/copydb were not aborting which was
**		causing other errors to be returned.
**	04-may-90 (billc)
**		major rewrite.  Conversion to SQL, avoidance of extra queries,
**		list strategy to avoid up-front allocation requiring count
**		queries, etc.  Now returns linked list of XF_COLINFO nodes.
**      17-jan-92 (billc)
**              Fix bug 40139 - wasn't testing for nullable UDTs.
**      30-feb-92 (billc)
**		Major rewrite to support FIPS in 6.5.
**      27-jul-1992 (billc)
**              Rename from .qsc to .sc suffix.
**	26-Aug-1993 (fredv)
*8		Included <me.h>.
**	13-sep-1995 (kch)
**		Ignore extended tables when filling the column-description
**		structures. This change fixes bug b71126.
**	14-feb-1996 (kch)
**		When retrieving index column info in xfgeticols(), order by
**		iiphysical_columns.column_sequence rather than
**		iiphysical_columns.key_sequence. This change fixes bug 74591.
**	 1-jul-1997 (hayke02)
**		The function xfgetcols() is now called with an argument: the
**		name of the table for which the XF_COLINFO list with column
**		info is required. This change fixes bug 83350.
**	21-jul-1997 (i4jo01)
**		When retrieving column information for an index, in
**		xfgeticols() the default_value for a column was not being
**		initialized.
**      23-Jul-1997 (wanya01)
**              The function xfgetcols() is now called with 2 arguments: 
**              table_name and table_owner. The query which retrieves table
**              columns is restricted on relowner now. This change fixes 
**              bug 87905. 
**	26-Feb-1999 (consi01) Bug 92350 Problem INGDBA 28
**		Block the RTREE hilbert column from index column list
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	23-may-2001 (gupsh01)
**	    Removed the static declaration from function attnode.
**	19-Oct-2001 (hanch04)
**	    Changed static function fill_xf_colinfo to global xffillcolinfo
**      20-jun-2002 (stial01)
**          Unloaddb changes for 6.4 databases (b108090)
**	07-Sep-2005 (thaju02) B115166
**	    Addition of global with_tbl_list.
**      19-Mar-2008 (Martin Bowes) Bug 120254
**          Alter repeated query (xfgetatt) to drop the superflouous 
**          ''=:table_owner. This has effect of dramatically improving query 
**          response time.
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for db objects, DB_IITYPE_LEN for iitypename() result
**	24-Nov-2009 (drivi01)
**	    Add security_audit_key column from iicolumns table to the query
**	    to ensure that audit_key field in the column structure reflects
**	    accurate infromation.
**	    Also clear out the allocated buffer for column structure to avoid
**	    false positives resulting from the residual garbage in the buffer.
**      17-Feb-2010 (coomi01) b122954
**          Port previous change for use with 65 catalogs and above.
**	21-apr-2010 (toumi01) SIR 122403
**	    Add encryption support and With_r1000_catalogs.
**	27-Jul-2010 (troal01)
**	    Add geospatial support
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
GLOBALREF bool	With_log_key;	/* determine if logical keys are available */
GLOBALREF bool	With_UDT;	/* determine if UDTs are available */
GLOBALREF bool  Portable;
GLOBALREF bool  With_physical_columns; /* determine if physical columns are available*/
GLOBALREF i4    Objcount ;
GLOBALREF bool	with_tbl_list;
GLOBALREF bool	with_no_rep;

FUNC_EXTERN     ADF_CB  *FEadfcb();

/* static's */
static XF_COLINFO	*Columnlist = NULL;
static ADF_CB	*Adf_cb = NULL;

/*{
** Name:	xfgetcols - fill array of column-description structures.
**
** Description:
**	Fills array of XF_COLINFO structures with attribute information 
**	necessary to write create and copy statements.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**		none.
**
**	Exceptions:
**		none.
**
** Side Effects:
**
** History:
**	25-aug-1987 (rdesmond)
**	13-sep-1995 (kch)
**		Ignore extended tables when filling the column-description
**		structures. This change fixes bug b71126.
**	 1-jul-1997 (hayke02)
**		This function is now called with an argument: table_name, the
**		name of the table for which the XF_COLINFO list with column
**		info is required. This change fixes bug 83350.
**      23-Jul-1997 (wanya01)
**              The function xfgetcols() is now called with 2 arguments:
**              table_name and table_owner. The query which retrieves table
**              columns is restricted on relowner now. This change fixes
**              bug 87905.
**	7-Jun-2004 (schka24)
**	    Caller is ignoring etabs (and partitions!), don't do it here.
**	    Fixes user complaint of slowness (no bug # that I know of,
**	    though).
**	30-dec-04 (inkdo01)
**	    Add support of column level collations.
**	13-oct-05 (inkdo01)
**	    Collations only apply to 3.0.2 and higher.
**	14-feb-2007 (gupsh01)
**	    For 3.0.2 and previous versions if the 'date' keyword is used for
**	    defining column datatypes, convert it to ingresdates in xfgetcols
**	    and xffillcolinfo. 
**	10-Jul-2007 (kibro01) b118702
**	    Get date_alias from the setting generated in FEadfcb
**	14-nov-2008 (dougi)
**	    Load identity column flags.
**      17-Feb-2010 (coomi01) b122954
**          Add security_audit_key column from iicolumns table to the query
**          to ensure that audit_key field in the column structure reflects
**          accurate infromation.
*/

void
xfgetcols(table_name, table_owner, seqvalpp)
EXEC SQL BEGIN DECLARE SECTION;
char		*table_name;
char            *table_owner;
EXEC SQL END DECLARE SECTION;
char		**seqvalpp;
{
EXEC SQL BEGIN DECLARE SECTION;
    char	owner_name[DB_MAXNAME + 1];
    char	datatype[DB_IITYPE_LEN + 1];
    char	int_datatype[DB_IITYPE_LEN + 1];
    char	default_value[XF_DEFAULTLINE + 1];
    i4		int_ingtype;
    i4		extern_len;
    i4		intern_len;
    i2		scale;
    i2		null_ind;
    char	buf[100] = {0};
    XF_COLINFO	*cp;
EXEC SQL END DECLARE SECTION;
    auto DB_DATA_VALUE	dbdv;
    auto DB_USER_TYPE	utype;
    char		len_buf[12];
    char		scale_buf[12];
    STATUS		stat = OK;
    char		*dtemp;
    i4			len;


    auto XF_TABINFO	*tp;

    /* get first node to fill in */
    cp = attnode();

    if (Adf_cb == NULL)
    {
	/* Get date_alias from FEadfcb's setting of it (kibro01) b118702 */
	Adf_cb = FEadfcb();

        /* If copy is from any of the versions older than 2006r2 */
        if (With_r930_catalogs | With_r302_catalogs | With_r3_catalogs | 
              With_20_catalogs | With_65_catalogs | With_64_catalogs )
        {
            Adf_cb->adf_date_type_alias = AD_DATE_TYPE_ALIAS_INGRES;
        }
    }


    if (!strcmp(table_owner, "")) /* No table_owner was supplied. */
    {
    if (With_r1000_catalogs)
    {
    EXEC SQL REPEATED SELECT c.table_owner,
            c.column_name, c.column_datatype,
            c.column_internal_datatype, c.column_internal_ingtype,
            c.column_scale, c.column_length, c.column_internal_length,
            c.column_nulls, c.column_defaults,
            c.column_default_val,   -- user defaults in 6.5
            c.column_has_default,
            c.column_sequence, c.key_sequence, 
            c.column_system_maintained,
            c.column_collid,
            c.column_always_ident, c.column_bydefault_ident,
            c.security_audit_key,
            c.column_encrypted, c.column_encrypt_salt,
            ifnull(g.srid, -1)
        INTO    :owner_name,
            :cp->column_name, :datatype,
            :int_datatype, :int_ingtype,
            :scale, :extern_len, :intern_len,
            :cp->nulls, :cp->defaults,
            :default_value:null_ind, :cp->has_default, 
            :cp->col_seq, :cp->key_seq, :cp->sys_maint,
            :cp->collID, :cp->always_ident, :cp->bydefault_ident,
            :cp->audit_key,
            :cp->column_encrypted, :cp->column_encrypt_salt,
            :cp->srid
        FROM iicolumns c LEFT OUTER JOIN geometry_columns g
        ON c.table_name = g.f_table_name 
        AND c.table_owner = g.f_table_schema
        AND c.column_name = g.f_geometry_column,  iitables t
        WHERE c.table_name = :table_name
                AND c.table_owner = t.table_owner
            AND c.table_name = t.table_name
            AND t.table_type = 'T'
        ORDER BY c.table_owner, c.column_sequence desc;
    EXEC SQL BEGIN;
	{
	    if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
		    int_ingtype, extern_len, intern_len, scale, cp, 
		    (null_ind == -1 ? (char *) NULL : default_value)))
	    {
		/* First see if we must save identity default. */
		if (cp->always_ident[0] == 'Y' ||
		    cp->bydefault_ident[0] == 'Y')
		    *seqvalpp = cp->default_value;

		/* get another empty node */
		cp = attnode();
	    }
	}
	EXEC SQL END;
    }
    else if (With_r930_catalogs)
    {
	EXEC SQL REPEATED SELECT c.table_owner,
		    c.column_name, c.column_datatype,
		    c.column_internal_datatype, c.column_internal_ingtype,
		    c.column_scale, c.column_length, c.column_internal_length,
		    c.column_nulls, c.column_defaults,
		    c.column_default_val,   -- user defaults in 6.5
		    c.column_has_default,
		    c.column_sequence, c.key_sequence, 
		    c.column_system_maintained,
		    c.column_collid,
		    c.column_always_ident, c.column_bydefault_ident,
		    c.security_audit_key
	    INTO	:owner_name,
		    :cp->column_name, :datatype,
		    :int_datatype, :int_ingtype,
		    :scale, :extern_len, :intern_len,
		    :cp->nulls, :cp->defaults,
		    :default_value:null_ind, :cp->has_default, 
		    :cp->col_seq, :cp->key_seq, :cp->sys_maint,
		    :cp->collID, :cp->always_ident, :cp->bydefault_ident,
		    :cp->audit_key
	    FROM iicolumns c, iitables t
	    WHERE c.table_name = :table_name
	    	    AND c.table_owner = t.table_owner
		    AND c.table_name = t.table_name
		    AND t.table_type = 'T'
	    ORDER BY c.table_owner, c.column_sequence desc;
	EXEC SQL BEGIN;
	{
	    if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
		    int_ingtype, extern_len, intern_len, scale, cp, 
		    (null_ind == -1 ? (char *) NULL : default_value)))
	    {
		/* First see if we must save identity default. */
		if (cp->always_ident[0] == 'Y' ||
		    cp->bydefault_ident[0] == 'Y')
		    *seqvalpp = cp->default_value;

		/* get another empty node */
		cp = attnode();
	    }
	}
	EXEC SQL END;
    }
    else if (With_r302_catalogs)
    {
	cp->always_ident[0] = 'N';
	cp->bydefault_ident[0] = 'N';
	EXEC SQL REPEATED SELECT c.table_owner,
		    c.column_name, c.column_datatype,
		    c.column_internal_datatype, c.column_internal_ingtype,
		    c.column_scale, c.column_length, c.column_internal_length,
		    c.column_nulls, c.column_defaults,
		    c.column_default_val,   -- user defaults in 6.5
		    c.column_has_default,
		    c.column_sequence, c.key_sequence, 
		    c.column_system_maintained,
		    c.column_collid,
		    c.security_audit_key
	    INTO    :owner_name,
		    :cp->column_name, :datatype,
		    :int_datatype, :int_ingtype,
		    :scale, :extern_len, :intern_len,
		    :cp->nulls, :cp->defaults,
		    :default_value:null_ind, :cp->has_default, 
		    :cp->col_seq, :cp->key_seq, :cp->sys_maint,
		    :cp->collID,
		    :cp->audit_key
	    FROM iicolumns c, iitables t
	    WHERE c.table_name = :table_name
	    	    AND c.table_owner = t.table_owner
		    AND c.table_name = t.table_name
		    AND t.table_type = 'T'
	    ORDER BY c.table_owner, c.column_sequence desc;
	EXEC SQL BEGIN;
	{
	    if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
		    int_ingtype, extern_len, intern_len, scale, cp, 
		    (null_ind == -1 ? (char *) NULL : default_value)))
	    {
		/* get another empty node */
		cp = attnode();
	    }
	}
	EXEC SQL END;
    }
    else if (With_65_catalogs)
    {
	cp->always_ident[0] = 'N';
	cp->bydefault_ident[0] = 'N';
	EXEC SQL REPEATED SELECT c.table_owner,
		    c.column_name, c.column_datatype,
		    c.column_internal_datatype, c.column_internal_ingtype,
		    c.column_scale, c.column_length, c.column_internal_length,
		    c.column_nulls, c.column_defaults,
		    c.column_default_val,   -- user defaults in 6.5
		    c.column_has_default,
		    c.column_sequence, c.key_sequence, 
		    c.column_system_maintained, -1,
		    c.security_audit_key
	    INTO	:owner_name,
		    :cp->column_name, :datatype,
		    :int_datatype, :int_ingtype,
		    :scale, :extern_len, :intern_len,
		    :cp->nulls, :cp->defaults,
		    :default_value:null_ind, :cp->has_default, 
		    :cp->col_seq, :cp->key_seq, :cp->sys_maint,
		    :cp->collID,
		    :cp->audit_key
	    FROM iicolumns c, iitables t
	    WHERE c.table_name = :table_name
	    	    AND c.table_owner = t.table_owner
		    AND c.table_name = t.table_name
		    AND t.table_type = 'T'
	    ORDER BY c.table_owner, c.column_sequence desc;
	EXEC SQL BEGIN;
	{
	    if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
		    int_ingtype, extern_len, intern_len, scale, cp, 
		    (null_ind == -1 ? (char *) NULL : default_value)))
	    {
		/* get another empty node */
		cp = attnode();
	    }
	}
	EXEC SQL END;
    }
    else
    {
	cp->always_ident[0] = 'N';
	cp->bydefault_ident[0] = 'N';
	EXEC SQL REPEATED SELECT c.table_owner,
		    c.column_name, c.column_datatype,
		    c.column_internal_datatype, c.column_internal_ingtype,
		    c.column_scale, c.column_length, c.column_internal_length,
		    c.column_nulls, c.column_defaults,
		    '', '',		 -- no user defaults before 6.5
		    c.column_sequence, c.key_sequence, 
		    c.column_system_maintained, -1,
		    'N'
	    INTO	:owner_name,
		    :cp->column_name, :datatype,
		    :int_datatype, :int_ingtype,
		    :scale, :extern_len, :intern_len,
		    :cp->nulls, :cp->defaults,
		    :default_value, :cp->has_default,
		    :cp->col_seq, :cp->key_seq, :cp->sys_maint,
		    :cp->collID,
		    :cp->audit_key
	    FROM iicolumns c, iitables t
	    WHERE c.table_name = :table_name
	    	    AND c.table_owner = t.table_owner
		    AND c.table_name = t.table_name
		    AND t.table_type = 'T'
	    ORDER BY c.table_owner, c.column_sequence desc;
	EXEC SQL BEGIN;
	{
	    if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
		    int_ingtype, extern_len, intern_len, scale,
		    cp, (char*) NULL))
	    {
		/* get another empty node */
		cp = attnode();
	    }
	}
	EXEC SQL END;
    }
    }
    else /* A table_owner was supplied */
    {
        if (With_r1000_catalogs)
        {
        EXEC SQL REPEATED SELECT c.table_owner,
                c.column_name, c.column_datatype,
                c.column_internal_datatype, c.column_internal_ingtype,
                c.column_scale, c.column_length, c.column_internal_length,
                c.column_nulls, c.column_defaults,
                c.column_default_val,   -- user defaults in 6.5
                c.column_has_default,
                c.column_sequence, c.key_sequence, 
                c.column_system_maintained,
                c.column_collid,
		c.column_always_ident, c.column_bydefault_ident,
		c.security_audit_key,
		c.column_encrypted, c.column_encrypt_salt,
		ifnull(g.srid, -1)
            INTO    :owner_name,
                :cp->column_name, :datatype,
                :int_datatype, :int_ingtype,
                :scale, :extern_len, :intern_len,
                :cp->nulls, :cp->defaults,
                :default_value:null_ind, :cp->has_default, 
                :cp->col_seq, :cp->key_seq, :cp->sys_maint,
		:cp->collID, :cp->always_ident, :cp->bydefault_ident,
		:cp->audit_key,
		:cp->column_encrypted, :cp->column_encrypt_salt,
		:cp->srid
        FROM iicolumns c LEFT OUTER JOIN geometry_columns g
        ON c.table_name = g.f_table_name 
        AND c.table_owner = g.f_table_schema
        AND c.column_name = g.f_geometry_column,  iitables t
            WHERE c.table_name = :table_name
                AND c.table_owner = t.table_owner
                AND c.table_name = t.table_name
                AND c.table_owner = :table_owner
                AND t.table_type = 'T'
            ORDER BY c.table_owner, c.column_sequence desc;
        EXEC SQL BEGIN;
        {
            if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
                int_ingtype, extern_len, intern_len, scale, cp, 
                (null_ind == -1 ? (char *) NULL : default_value)))
            {
		/* First see if we must save identity default. */
		if (cp->always_ident[0] == 'Y' ||
		    cp->bydefault_ident[0] == 'Y')
		    *seqvalpp = cp->default_value;

            /* get another empty node */
            cp = attnode();
            }
        }
        EXEC SQL END;
        }
        else if (With_r930_catalogs)
        {
        EXEC SQL REPEATED SELECT c.table_owner,
                c.column_name, c.column_datatype,
                c.column_internal_datatype, c.column_internal_ingtype,
                c.column_scale, c.column_length, c.column_internal_length,
                c.column_nulls, c.column_defaults,
                c.column_default_val,   -- user defaults in 6.5
                c.column_has_default,
                c.column_sequence, c.key_sequence, 
                c.column_system_maintained,
                c.column_collid,
		c.column_always_ident, c.column_bydefault_ident,
		c.security_audit_key
            INTO    :owner_name,
                :cp->column_name, :datatype,
                :int_datatype, :int_ingtype,
                :scale, :extern_len, :intern_len,
                :cp->nulls, :cp->defaults,
                :default_value:null_ind, :cp->has_default, 
                :cp->col_seq, :cp->key_seq, :cp->sys_maint,
		:cp->collID, :cp->always_ident, :cp->bydefault_ident,
		:cp->audit_key
            FROM iicolumns c, iitables t
            WHERE c.table_name = :table_name
                AND c.table_owner = t.table_owner
                AND c.table_name = t.table_name
                AND c.table_owner = :table_owner
                AND t.table_type = 'T'
            ORDER BY c.table_owner, c.column_sequence desc;
        EXEC SQL BEGIN;
        {
            if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
                int_ingtype, extern_len, intern_len, scale, cp, 
                (null_ind == -1 ? (char *) NULL : default_value)))
            {
		/* First see if we must save identity default. */
		if (cp->always_ident[0] == 'Y' ||
		    cp->bydefault_ident[0] == 'Y')
		    *seqvalpp = cp->default_value;

            /* get another empty node */
            cp = attnode();
            }
        }
        EXEC SQL END;
        }
        else if (With_r302_catalogs)
        {
	cp->always_ident[0] = 'N';
	cp->bydefault_ident[0] = 'N';
        EXEC SQL REPEATED SELECT c.table_owner,
                c.column_name, c.column_datatype,
                c.column_internal_datatype, c.column_internal_ingtype,
                c.column_scale, c.column_length, c.column_internal_length,
                c.column_nulls, c.column_defaults,
                c.column_default_val,   -- user defaults in 6.5
                c.column_has_default,
                c.column_sequence, c.key_sequence, 
                c.column_system_maintained,
                c.column_collid,
		c.security_audit_key
            INTO    :owner_name,
                :cp->column_name, :datatype,
                :int_datatype, :int_ingtype,
                :scale, :extern_len, :intern_len,
                :cp->nulls, :cp->defaults,
                :default_value:null_ind, :cp->has_default, 
                :cp->col_seq, :cp->key_seq, :cp->sys_maint,
                :cp->collID,
		:cp->audit_key
            FROM iicolumns c, iitables t
            WHERE c.table_name = :table_name
                AND c.table_owner = t.table_owner
                AND c.table_name = t.table_name
                AND c.table_owner = :table_owner
                AND t.table_type = 'T'
            ORDER BY c.table_owner, c.column_sequence desc;
        EXEC SQL BEGIN;
        {
            if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
                int_ingtype, extern_len, intern_len, scale, cp, 
                (null_ind == -1 ? (char *) NULL : default_value)))
            {
            /* get another empty node */
            cp = attnode();
            }
        }
        EXEC SQL END;
        }
        else if (With_65_catalogs)
        {
	cp->always_ident[0] = 'N';
	cp->bydefault_ident[0] = 'N';
        EXEC SQL REPEATED SELECT c.table_owner,
                c.column_name, c.column_datatype,
                c.column_internal_datatype, c.column_internal_ingtype,
                c.column_scale, c.column_length, c.column_internal_length,
                c.column_nulls, c.column_defaults,
                c.column_default_val,   -- user defaults in 6.5
                c.column_has_default,
                c.column_sequence, c.key_sequence, 
                c.column_system_maintained, -1,
		c.security_audit_key
            INTO    :owner_name,
                :cp->column_name, :datatype,
                :int_datatype, :int_ingtype,
                :scale, :extern_len, :intern_len,
                :cp->nulls, :cp->defaults,
                :default_value:null_ind, :cp->has_default, 
                :cp->col_seq, :cp->key_seq, :cp->sys_maint,
                :cp->collID,
		:cp->audit_key
            FROM iicolumns c, iitables t
            WHERE c.table_name = :table_name
                AND c.table_owner = t.table_owner
                AND c.table_name = t.table_name
                AND c.table_owner = :table_owner
                AND t.table_type = 'T'
            ORDER BY c.table_owner, c.column_sequence desc;
        EXEC SQL BEGIN;
        {
            if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
                int_ingtype, extern_len, intern_len, scale, cp, 
                (null_ind == -1 ? (char *) NULL : default_value)))
            {
            /* get another empty node */
            cp = attnode();
            }
        }
        EXEC SQL END;
        }
        else
        {
	cp->always_ident[0] = 'N';
	cp->bydefault_ident[0] = 'N';
        EXEC SQL REPEATED SELECT c.table_owner,
                c.column_name, c.column_datatype,
                c.column_internal_datatype, c.column_internal_ingtype,
                c.column_scale, c.column_length, c.column_internal_length,
                c.column_nulls, c.column_defaults,
                '', '',         -- no user defaults before 6.5
                c.column_sequence, c.key_sequence, 
                c.column_system_maintained, -1,
		'N'
            INTO    :owner_name,
                :cp->column_name, :datatype,
                :int_datatype, :int_ingtype,
                :scale, :extern_len, :intern_len,
                :cp->nulls, :cp->defaults,
                :default_value, :cp->has_default,
                :cp->col_seq, :cp->key_seq, 
		:cp->sys_maint, :cp->collID,
		:cp->audit_key
            FROM iicolumns c, iitables t
            WHERE c.table_name = :table_name
                AND c.table_owner = t.table_owner
                AND c.table_name = t.table_name
                AND c.table_owner = :table_owner
                AND t.table_type = 'T'
            ORDER BY c.table_owner, c.column_sequence desc;
        EXEC SQL BEGIN;
        {
            if (xffillcolinfo( table_name, owner_name, datatype, int_datatype,
                int_ingtype, extern_len, intern_len, scale,
                cp, (char*) NULL))
            {
            /* get another empty node */
            cp = attnode();
            }
        }
        EXEC SQL END;
        }
    } 
}

/*{
** Name:	xffillcolinfo - fill a column-description structure.
**
** Description:
**	Fills an XF_COLINFO structure with attribute information 
**	retrieved from the catalogs.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**		none.
**
** History:
**	10-Jul-2007 (kibro01) b118702
**	    Get date_alias from the setting generated in FEadfcb
**      11-Apr-2008 (hanal04) 
**          ASCII copy will now include a "set decimal" statement.
**          We need replace any values from iicolumns that were generated
**          under a different II_DECIMAL value.
**       9-Jan-2008 (hanal04) Bug 121484
**          Pass scale to afe_ctychk() for the TIME, TIMESTAMP and
**          INTERVAL DAY TO SECOND datatypes. We need to store this data 
**          so that precision details can be generated.
*/

bool
xffillcolinfo(
    char	*table_name,
    char	*owner_name,
    char	*datatype,
    char	*int_datatype,
    i4		int_ingtype,
    i4		extern_len,
    i4		intern_len,
    i2		scale,
    XF_COLINFO	*cp,
    char	*default_value
)
{
EXEC SQL BEGIN DECLARE SECTION;
    char	buf[100] = {0};
EXEC SQL END DECLARE SECTION;
    auto DB_DATA_VALUE	dbdv;
    auto DB_USER_TYPE	utype;
    char		len_buf[12];
    char		scale_buf[12];
    STATUS		stat = OK;
    char		*dtemp;
    i4			len;

    auto XF_TABINFO	*tp;

    if (Adf_cb == NULL)
    {
	/* Get date_alias from FEadfcb's setting (kibro01) b118702 */
	Adf_cb = FEadfcb();

        /* If copy is from any of the versions older than 2006r2 */
        if (With_r302_catalogs | With_r3_catalogs | With_20_catalogs |
              With_65_catalogs | With_64_catalogs )
        {
            Adf_cb->adf_date_type_alias = AD_DATE_TYPE_ALIAS_INGRES;
	}
    }

    xfread_id(table_name);
    xfread_id(owner_name);
    xfread_id(cp->column_name);

    /* determine internal length and datatype */
    utype.dbut_kind = (cp->nulls[0] == 'Y') ? DB_UT_NULL : DB_UT_NOTNULL;

    /*
    ** bug 9148
    ** if the internal datatype is of type 'logical_key', or a UDT you
    ** must pass in the internal datatype, and internal length
    ** otherwise, pass in datatype and extern_len.
    */
    (void) STtrmwhite(datatype);
    (void) STtrmwhite(int_datatype);

    cp->log_key = FALSE;

    /* Is this a logical_key type? */
    if (STbcompare(int_datatype, 0, ERx("table_key"), 0, TRUE) == 0
	 || STbcompare(int_datatype, 0, ERx("object_key"), 0, TRUE) == 0)
    {
	cp->log_key = TRUE;
	if (cp->sys_maint[0] != 'Y')
	{
	    /* 
	    ** user-maintained logical keys, the feature from hell.
	    ** Warn the user that disaster looms.
	    */
	    IIUGerr(E_XF014A_Log_key_warn, UG_ERR_ERROR, 
			3, cp->column_name, owner_name, table_name);
	}
    }

    if (STcompare(int_datatype, datatype) == 0)
    {
	CVlower(datatype);
	STlcopy(datatype, utype.dbut_name, (i4) sizeof(utype.dbut_name));
	CVla((i4) extern_len, len_buf); 
    }
    else
    {
	CVlower(int_datatype);
	STlcopy(int_datatype, utype.dbut_name, 
			    (i4) sizeof(utype.dbut_name));
	CVla((i4) intern_len, len_buf); 
    }

    switch (abs(int_ingtype))
    {
        case DB_DEC_TYPE:
        case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
        case DB_TSWO_TYPE:
        case DB_TSW_TYPE:
        case DB_INDS_TYPE:
        case DB_TME_TYPE:
        case DB_TSTMP_TYPE:
            CVla((i4) scale, scale_buf);
            break;

        default:
            scale_buf[0] = EOS;
	    break;
    }

    if (afe_ctychk(Adf_cb, &utype, len_buf, scale_buf, &dbdv) != OK)
    {
	/*
	** bug 9148
	** Check to see if this is a UDT.  If it is, ignore the error
	** and set the length and datatype.
	**
	** bug 40139
	** Check for nullable UDTs, too!
	*/
	if (IS_UDT(int_ingtype))
	{
	    dbdv.db_length = intern_len;
	    dbdv.db_datatype = int_ingtype;
	    STcopy(int_datatype, cp->col_int_datatype);
	    if (Portable)
	    {
		/*
		** Generate a warning message if the -c flag
		** is specified and a UDT column exists.
		*/
		IIUGerr(E_XF014B_Warn_udt_and_c_flag, UG_ERR_ERROR, 3,
			    cp->column_name, owner_name, table_name);
	    }
	}
	else
	{
	    FEafeerr(Adf_cb);
	    return (FALSE);
	}
    }

    cp->adf_type = dbdv.db_datatype;
    cp->precision = dbdv.db_prec;
    cp->intern_length = dbdv.db_length;

    if (default_value != NULL)
    {
        if(Portable)
        {
            /* Default values pulled from iicolumn may have a mix of
            ** II_DECIMAL vlaues. Make sure current value is applied
            ** to the "with default" statements.
            */
            char	*dcp;
    
            if (Adf_cb == NULL)
            {
                Adf_cb = FEadfcb();
            }

            switch (abs(int_ingtype))
            {
                case DB_DEC_TYPE:
                case DB_FLT_TYPE:
                case DB_MNY_TYPE:
                {
                    switch (Adf_cb->adf_decimal.db_decimal)
                    {
                        case ',':
                        {
                            dcp = STindex(default_value, ".",
                                          STlength(default_value));
                            break;
                        }
                        case '.':
                        {
                            dcp = STindex(default_value, ",",
                                          STlength(default_value));
                            break;
                        }
                    }
                    if(dcp)
                    {
                        *dcp = Adf_cb->adf_decimal.db_decimal;
                    }
                    break;
                }
            }
        }
	cp->default_value = STalloc(default_value);
    }
    else
	cp->default_value = NULL;

    cp->col_next = Columnlist;
    Columnlist = cp;

    if (cp->col_seq == 1)
    {
	/*
	** This is the first column in the table.  (We're getting
	** the columns in reverse order, so it's the last column that
	** we retrieved.)  So, look up the table in
	** our hash-table and attach the column definitions to it.
	*/
	if ((tp = xf_getobj(owner_name, table_name)) != NULL)
	    tp->column_list = Columnlist;
	Columnlist = NULL;
    }

    return(TRUE);
}


/*{
** Name:	xfgeticols - fill array of column-description structures.
**
** Description:
**	For each interesting index we generate a linked list of XF_COLINFO
**	structs describing the columns.  We then look up the XF_TABINFO
**	struct for that index and attach the list of XF_COLINFOs to it.
**
** Inputs:
**	basetable - table whose index information is to be retrieved.
**		    This parameter is null if you want all indices in the db
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**		none.
**
** Side Effects:
**	Attaches list of XF_COLINFO structs to descriptions of all indexes
**	that interest us.
**
** History:
**	2-feb-1992 (billc)	Written.
**	17-nov-93 (robf)
**         Include iihidden_columns to pick up any hidden key columns
**	17-dec-93 (robf)
**         Rework iihidden_columns  to iiphysical_columns
**	14-feb-1996 (kch)
**	   When retrieving index column info, order by
**	   iiphysical_columns.column_sequence rather than
**	   iiphysical_columns.key_sequence. This change fixes bug 74591.
**	21-jul-1997 (i4jo01)
**	   Initialize default_value field for column info.
**	26-Feb-1999 (consi01) Bug 92350 Problem INGDBA 28
**	   Block the RTREE hilbert column from index column list
**	06-nov-2000 (gupsh01)
**	   Modified the STbcompare call to STcasecmp. Also fixed 
**	   the bug (#103126) for checking for hilbert. This was causing
**	   no index columns names to be printed for create index statement. 
**	15-Apr-2005 (thaju02)
**	   copydb performance enhancement: if table list specified, build IN
**	   clause. Use execute immediate. (B114375)
**	07-Sep-2005 (thaju02) B115166
**	    Concat session.tbl_list subquery if with_tbl_list.
**	24-Aug-2009 (wanfr01) b122319
**	    select columns only for the specified tables to improve speed
*/

void
xfgeticols(XF_TABINFO *basetable)
{
EXEC SQL BEGIN DECLARE SECTION;
    char	index_name[DB_MAXNAME + 1];
    char	base_name[DB_MAXNAME + 1];
    char	owner_name[DB_MAXNAME + 1];
    XF_COLINFO	*cp;
    char	stmtbuf[512];
EXEC SQL END DECLARE SECTION;
    char	clausebuf[1024];

    auto XF_TABINFO	*ip;
    XF_COLINFO	*clist = NULL;

    /* get first empty XF_COLINFO node to fill in */
    cp = attnode();

    /* initialize default value */
    cp->default_value = NULL;

    STprintf(stmtbuf, "SELECT c.column_sequence, c.key_sequence, \
c.table_owner, i.index_name, i.base_name, c.column_name");

    if (With_physical_columns)
        STcat(stmtbuf, " FROM iiphysical_columns c, iiindexes i");
    else
        STcat(stmtbuf, " FROM iicolumns c, iiindexes i");

    STcat(stmtbuf, " WHERE (i.index_owner = '");
    STcat(stmtbuf, Owner);
    STcat(stmtbuf, "' OR '' = '");
    STcat(stmtbuf, Owner);
    STcat(stmtbuf, "') AND c.table_name = i.index_name \
AND c.table_owner = i.index_owner");

    if (basetable)
    {
        STprintf(clausebuf, " AND i.base_name = '%s' AND i.base_owner = '%s' ",basetable->name,basetable->owner);
        STcat(stmtbuf, clausebuf);
    }

    if (with_tbl_list)
    {
	STcat(stmtbuf, " AND i.base_name IN ( SELECT tabname \
FROM session.tbl_list )");
    }

    if (with_no_rep)
    {
        STcat(stmtbuf, " AND (i.base_name NOT IN ( SELECT relid FROM iirelation WHERE reltidx > 0 AND reltid IN (SELECT reltid FROM iirelation, dd_support_tables WHERE relid = table_name) ))");
    }

    STcat(stmtbuf, " ORDER by c.table_owner, i.index_name, \
c.column_sequence desc");

    EXEC SQL EXECUTE IMMEDIATE :stmtbuf INTO
            :cp->col_seq, :cp->key_seq,
            :owner_name, :index_name, :base_name,
            :cp->column_name;
    EXEC SQL BEGIN;
    {
        xfread_id(cp->column_name);

        /* Don't try to recreate the tidp column. */
        if (STcasecmp(cp->column_name, ERx("tidp")) == 0)
            continue;


        /* Skip internal hilbert column */
        if (STcasecmp(cp->column_name, ERx("hilbert")) == 0)
            continue;

        /* if we're not interested in this table, keep going. */
        xfread_id(base_name);
        if (!xfselected(base_name))
            continue;

        cp->col_next = clist;
        clist = cp;

        if (cp->col_seq == 1)
        {
            /*
            ** This is the first column in the index.  (We're getting
            ** the columns in reverse order, so it's the last column that
            ** we retrieved.)  So, look up the index in
            ** our hash-table and attach the column definitions to it.
           **
            ** If this index is system-generated (for enforcing UNIQUE
            ** constraints) then xf_getobj won't find it.
            */

            xfread_id(index_name);
            xfread_id(owner_name);

            if ((ip = xf_getobj(owner_name, index_name)) != NULL)
                ip->column_list = clist;
            else
                xfattfree(clist);
            clist = NULL;
        }

        /* get another empty XF_COLINFO node */
        cp = attnode();

        /* initialize default value */
        cp->default_value = NULL;
    }
    EXEC SQL END;
}

/*
** Internals for our column information handling.
*/

static XF_COLINFO *ColFreeList = NULL;
# define XFA_BLOCK       32

/*{
** Name:	xfattfree - free a linked list of XF_COLINFO structs.
**
** Description:
**	Put all the given XF_COLINFO structs back on the free list.
**
** Inputs:
**	list	- linked list to return to the freelist.
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**		none.
**
** Side Effects:
**
** History:
**	2-feb-1992 (billc)	Written.
*/

void
xfattfree(list)
XF_COLINFO	*list;
{
    XF_COLINFO 	*cp;

    if (list == NULL)
	return;

    for (cp = list; cp->col_next != NULL; cp = cp->col_next)
    {
	if (cp->default_value != NULL)
	    MEfree((PTR) cp->default_value);
    }

    cp->col_next = ColFreeList;
    ColFreeList = list;
}

/*{
** Name:	attnode - get an empty XF_COLINFO struct.
**
** Description:
**	Gets an empty XF_COLINFO struct from the free list.
**	If the free list is empty, allocate more structs and put on the list.
**	XF_COLINFOs are allocated XFA_BLOCK-at-a-time to cut down on
**	allocation overhead.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**		none.
**
** Side Effects:
**
** History:
**	2-feb-1992 (billc)	Written.
**      28-Oct-2009 (coomi01) b122786
**         Records should be cleaned before use.
**	02-Dec-2009 (drivi01)
**	   Take out MEfill as the buffer will be
**	   cleared with REQMEM.
*/

XF_COLINFO *
attnode()
{
    XF_COLINFO *tmp;

    if (ColFreeList == NULL)
    {
        i4  i;

	/*
	** Nothing on the freelist, so allocate a bunch of new structs.
	*/

        ColFreeList = (XF_COLINFO *) XF_REQMEM(sizeof(*tmp) * XFA_BLOCK, TRUE);
        if (ColFreeList == NULL)
        {
	    IIUGerr(E_XF0060_Out_of_memory, UG_ERR_FATAL, 0);
	    /* NOTREACHED */
        }

        /* set up the 'next' pointers, linking the empties together. */
        for (i = 0; i < (XFA_BLOCK - 1); i++)
             ColFreeList[i].col_next = &ColFreeList[i+1];
        ColFreeList[XFA_BLOCK - 1].col_next = NULL;
    }
    tmp = ColFreeList;

    ColFreeList = ColFreeList->col_next;
    tmp->col_next = NULL;

    return (tmp);
}

