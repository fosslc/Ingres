/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
+*  Name: iirowdesc.h - Row descriptor for embedded run-time libraries.
**
**  Defines:
**	ROW_COLNAME - Varying length column name returned from DBMS.
**	ROW_DESC    - Row descriptor maintained by LIBQ.
**
**  Notes:
**	1. For filling usage see LIBQ routine IIrdDescribe.
-*
**  History:
**	25-mar-1987	- Written (ncg)
**	20-may-1988	- Modified DBV array for INGRES/NET changes. (ncg)
**	04-aug-1988	- Added RD_MAX_COLS (ncg)
**	24-apr-1990	- Added cursor-performance flags (ncg)
**	23-feb-1993 - Added tags to structures for making automatic prototypes.(mohan)
**	12-mar-93 (fraser)
**	    Changed structure tag names so that they don't begin with
**	    underscore.
**	05-apr-1996 	- Added RD_CVCH flag to support compressed variable-
**			  length varchars.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*}
+*  Name: ROW_COLNAME - Varying length column name of a row descriptor.
**
**  Description:
**	This data structure contains a varying length column name of a row
**	descriptor as returned by the DBMS on a DESCRIBE-like query. The
**	name is varying length (ie, has a 2-byte count field) but is at most
**	RD_MAXNAME bytes long.  This is like a text string created from
**	VARCHAR(RD_MAXNAME). The name is not null terminated.
-*
**  History:
**	25-mar-1987	- Written (ncg)
**	23-feb-1988	- Extended length to RD_MAXNAME (ncg)
**	05-Dec-2003 (yeema01) Bug# 111404
**	    OpenROAD crashed in DataStream.SetCols method  due to 
**	    pointer size problem.
**	    Need to explicitly define pointer size as 64-bits pointer since
**	    OpenROAD is built in 32-bits pointer size context.
*/

#if defined(xde) && defined(axp_osf)
#pragma pointer_size save
#pragma pointer_size long
#endif

# define	RD_MAXNAME	32

typedef struct s_ROW_COLNAME {
    i2  	rd_nmlen;		/* 0 <= Length of name <= RD_MAXNAME */
    char	rd_nmbuf[RD_MAXNAME];
} ROW_COLNAME;


/*}
+*  Name: ROW_GCA - Row descriptor that is equivalent to that of GCA.
**
**  Description:
**	This data structure contains a descriptor that has the same layout
**	of the GCA tuple descriptor, except that it never has a varying-
**	length column name.  Users of this file may only reference
**	the DB data value component of the inner varying-length array.
**	All other fields are maintained as fillers so that the presentation
**	layer of GCF (for INGRES/NET) may be passed the same data structure
**	that clients of this file use to process simple columns.  Most other
**	filler fields are set to zero.
**
**	The DB data value is set when the descriptor is initially read
**	from GCA.
**
**	Whenever the GCA_TD_DATA structure is changed you must also
**	change ROW_GCA accordingly.  The GCA data structure is virtual and
**	cannot be handled by simple indexing.  This is a more useful data
**	structure than can be referenced in the normal way.
**
**  Note to Users:
**	Outside of LIBQ no one should use the fields rd_gc_xx.  The macro:
**		RD_DBVS_MACRO
**	is defined to get at the DB data values (which are the only item
**	needed from this data structure).  This macro is defined later.
**	You do NOT want any dependencies on GCA.
-*
**  History:
**	20-may-1988	- Written for INGRES/NET (ncg)
**	22-jun-1988	- Added descriptor id for INGRES/NET (ncg)
*/

typedef struct s_ROW_GCA {
    i4		rd_gc_tsize;	/* Dummy: gca_tsize 	   = tsize  */
    i4			rd_gc_res_mod;	/* Dummy: gca_res_modifier = 0      */
    i4		rd_gc_tid;	/* Dummy: gca_id_tdescr    = id     */
    i4		rd_gc_l_cols;	/* Dummy: gca_l_col_desc   = # cols */
    struct _rd_col_att {		/* Dummy: GCA_COL_ATT		    */
	DB_DATA_VALUE	rd_dbv;		/* Dummy: gca_attdbv	   = DBV    */
	i4		rd_gc_l_name;	/* Dummy: gca_l_attname    = 0      */
    } rd_gc_col_desc[1];		/* Bogus array is allocated         */
} ROW_GCA;				/* Dummy: GCA_TD_DATA */

/*
**  Name: RD_GCA_SIZE_MACRO - Get the size of ROW_GCA.
**
**  Description:
**	This macro allows users to allocate the structure without knowing
**	about the varying length field.
**
**  History:
**	20-may-1988	- Written for INGRES/NET. (ncg)
*/

# define	ROW_GCA_SIZE_MACRO(cols)				\
		(sizeof(ROW_GCA) + (cols-1)*sizeof(struct _rd_col_att))

/*
**  Name: RD_DBVS_MACRO - To get at DB data value bypassing the GCA fillers.
**
**  Description:
**	This macro allows users of this file to access the DB data value of
**	a column without knowing about the GCA filler fields defined for
**	ROW_GCA.  The usage of this macro may be (for example):
**
**		row_desc->RD_DBVS_MACRO(i).db_datatype
**
**  History:
**	20-may-1988	- Written for INGRES/NET. (ncg)
*/

# define RD_DBVS_MACRO(col)	rd_gca->rd_gc_col_desc[col].rd_dbv

/*}
+*  Name: ROW_DESC - Full row descriptor.
**
**  Description:
**	This data structure contains enough information process any
**	DESCRIBE-like query.  All functional information is returned from
**	the DBMS as a result of the query.  The structure includes self
**	contained allocation information in order to maintain the usage
**	of previously allocated fields (arrays of DBV's, etc).
-*
**  History:
**	25-mar-1987	- Written (ncg)
**	20-may-1988	- Modified field rd_gca for INGRES/NET. (ncg)
**	24-apr-1990	- Added RD_READONLY & RD_UNBOUND flags for cursor
**			  performance project (neil)
**	04-apr-1996	- Added RD_CVCH flag to support compressed variable-
**			  length varchars.
*/

typedef struct s_ROW_DESC {
    i4			rd_flags;	/* Modifiers on useful info */
# define	RD_DEFAULT	 0x0	/* Default case means no name info is
					** included and no errors reading */
# define	RD_ERROR	 0x1	/* Error reading descriptor from DBMS */
# define	RD_NAMES	 0x2	/* Name field has useful info */
# define	RD_READONLY	 0x4	/* DBMS returned read-only cursor,
					** and thus pre-fetching available.
					*/
# define	RD_UNBOUND	 0x8	/* The referenced row has unbound
					** data included (BLOB type).  This
					** will be used to avoid pre-fetching
					** for enormous rows.
					*/
# define 	RD_CVCH		 0x10	/* The referenced row contains at least
					** one column with compressed varchars.
					*/
    i4			rd_numcols;	/* Number of columns in a row */
    i4			rd_dballoc;	/* Number of DBV's allocated in next
					** member (rd_dballoc >= rd_numcols) */
    ROW_GCA		*rd_gca;	/* Includes all the DBV's */
    i4			rd_nmalloc;	/* Number of names allocated in next
					** array (note rd_nmalloc >= rd_numcols
					** only if RD_NAMES is on) */
    ROW_COLNAME		*rd_names;	/* Pointer to array of column names */
} ROW_DESC;

/*
** RD_MAX_COLS is defined as some reasonable limit of columns.  This is set
** so that if the protocol gets messed up the reader of the tuple descriptor
** does not try to allocate a large hex number of column descriptors.
*/
# define RD_MAX_COLS	(4 * DB_MAX_COLS)

#if defined(xde) && defined(axp_osf)
#pragma pointer_size restore
#endif

