/*
** Copyright (c) 1985, 2009 Ingres Corporation All Rights Reserved.
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <cm.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm1m.h>
#include    <dm1c.h>
#include    <dm1r.h>
#include    <dmftrace.h>
#include    <dmucb.h>
#include    <dmpecpn.h>
#include    <dmppn.h>

# define DMPE_NULL_TCB	"\0\0\0\0\0\0\0\0"

/**
**
**  Name: DM1C.C - Common routines to manipulate records and data pages.
**
**  Description:
**      This file contains all the routines needed to manipulate
**      records and data pages.
**	Note that dm1cc.c and dm1cn.c contain accessors for page
**	manipulation. Many of the operations that used to be performed by
**	routines in this file are now performed by accessors.
**
**      The routines defined in this file are:
**	dm1c_sys_init 	    - Initialize a value of a system_maintained datatype
**	dm1c_badtid	    - Error report bad tid
**
**  History:
**      08-nov-85 (jennifer)
**          Changed for Jupiter.
**	12-dec-88 (greg)
**	    Integrate Jeff's BYTE_ALIGN changes
**	29-feb-89 (mikem)
**	    Logical key development.  Added dm1c_sys_init().
**	17-may-89 (mikem)
**	    Changes necessary to make a table_key 8 bytes rather than 6.
**	29-may-89 (rogerk)
**	    Added checks for valid data values when [un]compressing data.
**	    Took out ifdef bytalign stuff and used I2ASSIGN_MACRO instead.
**	12-jun-89 (rogerk)
**	    Made sure that I2ASSIG_MACRO calls are made with 2 byte entities.
**	17-aug-1989 (Derek)
**	    Added dm1a_add() to assist in creating the core catalogs.
**	25-Jan-1990 (fred)
**	    Added support for peripheral datatypes
**	    (dm1c_p{delete,get,put,replace}())
**	12-feb-91 (mikem)
**	    Changes made to dm1c_sys_init() to return logical key to client.
**	    Changed interface to pass back which types of logical keys were
**	    assigned, and to accept an already initialized object key as input.
**	7-mar-1991 (bryanp)
**	    Added support for Btree index compression: implemented new routines
**	    dm1cpcomp_rec and dm1cpuncomp_rec to be called by the dm1cidx.c
**	    layer, which has a list of pointers to attributes. Also, added
**	    additional sanity checks in dm1c_get() & dm1c_uncomp_rec() to
**	    detect and complain about scribbling over memory. Made dm1c_oldget()
**	    and dm1c_old_uncomp_rec() static and implemented buffer overflow
**	    detection there as well.
**	14-jun-1991 (Derek)
**	    Changed dm1c_allocate to not change the page, and have dm1c_put
**	    allocate a new line number if needed at insert time.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	04-jun-92 (kwatts)
**	    Migrated lots of code to dm1cc.c and dm1cn.c for MPF project.
**	17-sep-1992 (jnash)
**	    Reduced logging project.
**	    --  Include system catalog accessor setup.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	30-Oct-1992 (fred)
**	    Fixed bugs in dm1c_p{delete, replace} code where they were being
**	    ineffective.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**      24-Jun-1993 (fred)
**          Added include of <dmpecpn.h> to pick up dmpe prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	22-nov-1994 (shero03)
**	    Use LZW (HICOMPRESS) for standard, non-index, non-catalog
**	    tuples.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Pass page size parameter to dm1c_getaccessors() to determine
**		which page level vector to use based on page size.
**	12-may-1995 (shero03)
**	    Add Key Level Vectors
**      15-Jul-1996 (ramra01 for bryanp)
**          Alter Table project: Peripheral objects associated with
**          add / drop columns.
**	24-sep-1996 (shero03)
**	    Add object type and hilbertsize to the KLV
**	15-apr-1997 (shero03)
**	    Add dimension size and perimeter to the KLV
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. Modified dm1c_pput().
**	28-jul-1998 (somsa01)
**	    Tell dm1c_pput() and dmpe_move() if we are bulk loading blobs
**	    into extension tables or not via a new parameter.  (Bug #92217)
**	22-sep-1998 (somsa01)
**	    We need to properly set pop_temporary in dm1c_preplace() and
**	    dm1c_pdelete().  (Bug #93465)
**	17-feb-1999 (somsa01)
**	    In dm1c_preplace(), if dmpe_replace() returned an OK status yet
**	    the pop_error.err_code was set to E_DM0154_DUPLICATE_ROW_NOTUPD,
**	    send this back up (as there were no peripheral tuples
**	    updated).
**	15-apr-1999 (somsa01)
**	    In places where we allocate an ADP_POP_CB, initialize the
**	    pop_info field to a NULL pointer.
**      12-aug-99 (stial01)
**          Redeem/Couponify instead of dmpe_move peripheral data if 
**          source and destination segments sizes differ.
**      31-aug-99 (stial01)
**          VPS etab support
**      10-sep-99 (stial01)
**          dm1c_preplace() Send table info in pop_info before dmpe_replace
**      08-dec-99 (stial01)
**          Backout of previous changes for vps etab
**      21-dec-99 (stial01)
**          Added support for V3 page accessors
**      10-jan-2000 (stial01)
**          Added support for 64 bit RCB pointer in DMPE_COUPON
**      12-jan-2000 (stial01)
**         Byte swap per_key (Sir 99987)
**      27-mar-2000 (stial01)
**         Backout unecessary code for vps etab
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      25-may-2000 (stial01)
**         Use Normal Accessor routines for system catalogs
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          V1 use dm1cn*, V2,V3.. use dm1cnv2* (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      01-may-2001 (stial01)
**          Init adf_ucollation
**      20-mar-2002 (stial01)
**          Updates to changes for 64 bit RCB pointer in DMPE_COUPON   
**          Removed att_id from coupon, making room for 64 bit RCB ptr in
**          5 word coupons. (b103156)
**      26-nov-2002 (stial01)
**          Added dm1c_get_tlv
**      15-sep-2003 (stial01)
**          Blob optimization for prepared updates (b110923)
**	15-Jan-2004 (schka24)
**	    Logical keys have to come from the partitioned master.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      30-mar-2004 (stial01)
**          Ignore tracepoint DM721 (set svcb_page_type) when creating etabs
**      25-jun-2004 (stial01)
**          Added dm1c_get for processing compressed, altered, or segmented rows
**      8-jul-2004 (stial01)
**          Zero tcb ptr in coupon before copying into base table row (b112591)
**          Zero tcb ptr in dm1c_pget, since it is no longer used by dmpe_get
**      8-jul-2004 (stial01)
**          Changes for DMPP_VPT_PAGE_HAS_SEGMENTS and readnolock
**     06-oct-2004 (stial01)
**          dm1c_getpgtype() status E_DB_ERROR -> page_type TCB_PG_INVALID
**      1-dec-2004 (stial01)
**          Added dm1c_pvalidate() to validate coupons, activated with 
**          server trace point dm722
**	16-dec-04 (inkdo01)
**	    Added various collID's.
**     07-jan-2005 (stial01)
**          dm1c_getpgtype() Enforce same maxreclen for all page types
**          valid_page_type_size() Enforce same maxreclen for all page types
**      10-jan-2005 (stial01)
**          dm1c_get() Fixed lock failure error handling. (b113716)
**	20-feb-2008 (kschendel) SIR 122739
**	    Reorganized row accessor stuff, invent DMP_ROWACCESS.
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1r_? functions converted to DB_ERROR *
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm1c_? functions converted to DB_ERROR *,
**	    use new form uleFormat, macroized dm1c_badtid to call
**	    dm1cBadTidFcn.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add V6, V7 page types, prototype changes
**	    to page accessors.
**	
*/


/*
**  External data references. These are to the accessor structures defined
**  in dm1cc.c and dm1cn.c for the different page structures.
*/

GLOBALREF DMPP_ACC_PLV	dm1cn_plv;
GLOBALREF DMPP_ACC_PLV	dm1cnv2_plv;

GLOBALREF DMP_RACFUN_CMP dm1c_compress_v[];
GLOBALREF DMP_RACFUN_UNCMP dm1c_uncompress_v[];
GLOBALREF DMP_RACFUN_XPN dm1c_compexpand_v[];
GLOBALREF DMP_RACFUN_CSETUP dm1c_cmpcontrol_setup_v[];
GLOBALREF DMP_RACFUN_CSIZE dm1c_cmpcontrol_size_v[];

/*
**  Forward and/or External function references.
*/
static bool valid_page_type_size(
			i4 page_type,
			i4 page_size,
			i4 create_flags,
			i4 rec_len);

static DB_STATUS dm1c_pvalidate(
	char		*where,
	DB_ATTS           *atts,
	i4		   att_count,
	DMP_RCB            *rcb,
	char		   *record,
	DB_ERROR	   *dberr);


/*{
** Name: dm1c_get_plv - Map relpgtype to page accessor
**
** Description:
**	Given a page type, this (trivial) routine returns a pointer to
**	the page level accessor vector (DMPP_ACC_PLV).
**
** Inputs:
**	relpgtype		Page type of table.
**
** Outputs:
**      plv    -- sets this to point at the PLV vector
**
** Returns:
**	OK
**
** Side Effects:
**	none
** History:
**      08-nov-85 (jennifer)
**          Converted for Jupiter.
**	29-may-89 (rogerk)
**	    Added safety check in checking for amount of data to move when
**	    compressing out deleted space, since the check is likely to be free.
**      08-aug-85 (jas)
**          Created for CAFS project.
**	04-jun-92 (kwatts)
**	    Total restructure for 6.5 project
**	17-sep-1992 (jnash)
**	    Reduced logging project.
**	    --  Include system catalog accessor.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Created new page level vector. Added relpgsize parameter.
**	29-may-1998 (nanpr01)
**	    Set up the accessors correctly so that we can avoid the page
**	    size comparison in macros.
**	13-Feb-2008 (kschendel) SIR 122739
**	    Split off obsolete old-style compression.
*/

VOID
dm1c_get_plv(i4 relpgtype, DMPP_ACC_PLV **plv)
{
    if (relpgtype == TCB_PG_V1)
        *plv = &dm1cn_plv;
    else
        *plv = &dm1cnv2_plv;
}

/*
** Name: dm1c_cmpcontrol_size - Return size of compression control array
**
** Description:
**
**	This routine returns an upper limit on the size of the compression
**	control array.  This array, if used at all by the supplied compression
**	type, typically depends on the attributes involved, their types
**	and nullability, and whether row versioning has to be checked.
**	Unfortunately, at the time we need the size info, we don't yet
**	know the attributes (usually).  So we do the best we can with
**	an attribute count and an indicator of whether versioning is
**	in play.
**
**	This routine is just a dispatcher, it calls a compression specific
**	routine to do the work.
**
** Inputs:
**	compression_type		TCB_C_xxx compression type
**	att_count			Number of attributes in row
**	relversion			Zero if no row versioning needed,
**					else nonzero.
**
** Outputs:
**	Returns upper limit on size (bytes) of the control array.
**
** History:
**	20-Feb-2008 (kschendel) SIR 122739
**	    Written as part of row-access restructuring.
*/

i4
dm1c_cmpcontrol_size(i4 compression_type,
	i4 att_count, i4 relversion)
{

    /* NULL dispatch means no control array used */
    if (dm1c_cmpcontrol_size_v[compression_type] == NULL)
	return (0);

    return (*dm1c_cmpcontrol_size_v[compression_type])(att_count,
		relversion);

} /* dm1c_cmpcontrol_size */

/*
** Name: dm1c_compexpand - Calculate worst-case expansion
**
** Description:
**	For the worst-case growth of a compressed row, one normally looks
**	in the DMP_ROWACCESS structure where it's precomputed.  However
**	there are a very few cases (e.g. modify) where we want to figure
**	the worst-case against a different compression type, perhaps to
**	decide whether the operation can succeed.  Anyway, this routine
**	takes a compression type, attribute pointer array, and count;
**	and returns the worst-case expansion in bytes.  (Possibly zero.)
**
** Inputs:
**	compression_type	TCB_C_xxx compression type
**	att_ptrs		Array of pointers to attributes
**	att_count		Number of attributes
**
** Outputs:
**	Returns worst-case expansion in bytes.
**
** History:
**	25-Feb-2008 (kschendel) SIR 122739
**	    New wrapper for modify's use.
*/

i4
dm1c_compexpand(i4 compression_type, DB_ATTS **att_ptrs, i4 att_count)
{
    if (dm1c_compexpand_v[compression_type] == NULL)
	return (0);
    else
	return (
		(*dm1c_compexpand_v[compression_type])(att_ptrs,
							att_count)
		);
} /* dm1c_compexpand */

/*
** Name: dm1c_rowaccess_setup - Finish ROWACCESS structure setup
**
** Description:
**
**	After the basic info in a DMP_ROWACCESS struct has been
**	initialized (atts array and count, compression type), this
**	routine is called to finish the "setup" of the rowaccess
**	info.  This includes the compress / decompress function call
**	vectors, the control array (if needed), the worst-case expansion
**	amount, and whether the compression compresses coupons.
**
** Inputs:
**	rac			Pointer to DMP_ROWACCESS to set up
**	    .att_ptrs		Array of attribute pointers
**	    .att_count		Number of attributes
**	    .cmp_control	Pointer to place to put control array
**	    .control_count	Pass in size of area in bytes so that
**				setup can verify that it's large enough
**	    .compression_type	The compression type to use
**
** Outputs:
**	rac
**	    .cmp_control	Control array is filled in, if used
**	    .control_count	Number of control entries is filled in
**	    .worstcase_expansion  Worst case expansion, in bytes
**	    .dmpp_[un]compress	Compression dispatchers
**	    .compresses_coupons	TRUE if compression affects coupons
**
**	Returns E_DB_OK or E_DB_xxx error status
**
** History:
**	21-Feb-2008 (kschendel) SIR 122739
**	    Written for overhauled row-accessor structuring
*/

DB_STATUS
dm1c_rowaccess_setup(DMP_ROWACCESS *rac)
{
    DB_STATUS status;

    rac->dmpp_compress = dm1c_compress_v[rac->compression_type];
    rac->dmpp_uncompress = dm1c_uncompress_v[rac->compression_type];
    if (dm1c_compexpand_v[rac->compression_type] == NULL)
	rac->worstcase_expansion = 0;
    else
	rac->worstcase_expansion =
		(*dm1c_compexpand_v[rac->compression_type])(rac->att_ptrs,
							 rac->att_count);
    if (dm1c_cmpcontrol_setup_v[rac->compression_type] != NULL)
    {
	status = (*dm1c_cmpcontrol_setup_v[rac->compression_type])(rac);
	if (status != E_DB_OK)
	    return (status);
    }

    /* Ideally this would be encapsulated into the compression handlers,
    ** but for now I'm going to be a bit lazy...
    */
    rac->compresses_coupons = (rac->compression_type == TCB_C_HICOMPRESS);

    return (E_DB_OK);
} /* dm1c_rowaccess_setup */

/*
** Name: dm1c_getpgtype - Initialize page type for a table or index
**
** Description:
**	This routine is called to determine what page type to use
**
**      If page size is != 2048, TCB_PG_V1 page type should only be used
**      internally.... for 4k blob extension tables so that we will be able
**      to hold 2 etab records
**
** Inputs:
**	page size	    - page size for this table. One of:
**				2048, 4096, 8192, 16384, 32768, 65536
**	create_flags        - create flags
**      rec_len		    - record size
**
** Returns:
**      page type
**
** History:
**      09-mar-2001 (stial01)
**          Created for Variable Page Size - Variable Page Type support    
**	04-May-2006 (jenjo02)
**	    Clustered primary btrees can't be V1 as they store DMPP_CLUSTERED
**	    page stat which is lost in V1 page stat.
*/
DB_STATUS
dm1c_getpgtype(
		i4 page_size, 
		i4 create_flags,
		i4 rec_len,
		i4 *page_type)
{
    i4	config_pgtypes = dmf_svcb->svcb_config_pgtypes;
    i4  tperpage;

    if (dmf_svcb->svcb_page_type && 
	(create_flags & 
	(DM1C_CREATE_CORE | DM1C_CREATE_CATALOG | DM1C_CREATE_ETAB | DM1C_CREATE_CLUSTERED)) == 0)
    {
	/* Using tracepoint DM721 to set page type can cause errors */
	if (valid_page_type_size(dmf_svcb->svcb_page_type, page_size, 
			create_flags, rec_len))
	{
	    *page_type = dmf_svcb->svcb_page_type;
	    return (E_DB_OK);
	}
	/* Ignore invalid page type, fall through and get valid one */
    }

    if (create_flags & DM1C_CREATE_ETAB)
    {
	/*
	** Blob extension table:
	** 2K	page type MUST be V1, to hold ONE 2000 byte etab record
	**          even if this page type has been disabled for this server
	** 4K	page type V1 or V4, to hold TWO 2000 byte etab records
	** other    Should not use V1. V4 is the most appropriate page type,
	**          since we do not do deferred update processing, or alter table
	**          for etabs, but we DO use row locking protocols on etabs.
	**          Note we can also use page type V2 and still be able to fit
	**          the maximum possible number of rows.
	*/
	if (page_size == DM_COMPAT_PGSIZE)
	    *page_type = TCB_PG_V1;
	else if (config_pgtypes & SVCB_CONFIG_V6)
	    *page_type = TCB_PG_V6;
	else if (config_pgtypes & SVCB_CONFIG_V4)
	    *page_type = TCB_PG_V4;
	else if (page_size == 4096)
	    *page_type = TCB_PG_V1;
	else
	    *page_type = TCB_PG_V2;

	tperpage = (page_size - DMPPN_VPT_OVERHEAD_MACRO(*page_type))/
		(rec_len + DM_VPT_SIZEOF_LINEID_MACRO(*page_type)
		+ DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(*page_type));

	if (tperpage != page_size/2048)
	    TRdisplay(
	    "WARNING etab create with page size %d page type %d tperpage %d\n",
	    page_size, *page_type, tperpage);
    }
    else if (create_flags & (DM1C_CREATE_INDEX | DM1C_CREATE_TEMP))
    {
	/*
	** indexes,temps:   (no alter table support needed)
	** For 2k page size, use page type V1 if enabled for the server
	** For page size >= 4K use V4 for indexes if enabled for the server
	*/
	if (page_size == DM_COMPAT_PGSIZE && (config_pgtypes & SVCB_CONFIG_V1) &&
			(create_flags & DM1C_CREATE_CLUSTERED) == 0)
	    *page_type = TCB_PG_V1;
	else if (config_pgtypes & SVCB_CONFIG_V6)
	    *page_type = TCB_PG_V6;
	else if (config_pgtypes & SVCB_CONFIG_V4)
	    *page_type = TCB_PG_V4;
	else if (page_size == DM_COMPAT_PGSIZE && (create_flags & DM1C_CREATE_CLUSTERED) == 0)
	    *page_type = TCB_PG_V1;
	else
	    *page_type = TCB_PG_V2;
    }
    else
    {
	/*
	** DM1C_CREATE_DEFAULT, DM1C_CREATE_CORE comes here
	*/ 
	if (page_size == DM_COMPAT_PGSIZE && (config_pgtypes & SVCB_CONFIG_V1) &&
			(create_flags & DM1C_CREATE_CLUSTERED) == 0)
	    *page_type = TCB_PG_V1;
	else if ((config_pgtypes & SVCB_CONFIG_V7) &&
	    (create_flags & DM1C_CREATE_CORE) == 0 &&
	    (create_flags & DM1C_CREATE_CATALOG) == 0)
	    *page_type = TCB_PG_V7;
	else if (config_pgtypes & SVCB_CONFIG_V6)
	    *page_type = TCB_PG_V6;
	else if (config_pgtypes & SVCB_CONFIG_V3)
	    *page_type = TCB_PG_V3;
	else if (page_size == DM_COMPAT_PGSIZE && (create_flags & DM1C_CREATE_CLUSTERED) == 0)
	    *page_type = TCB_PG_V1;
	else
	    *page_type = TCB_PG_V2;
    }

    if ((create_flags & DM1C_CREATE_DEFAULT) ||
	(create_flags & DM1C_CREATE_TEMP) )
    {
	if (rec_len > dm2u_maxreclen(*page_type, page_size) 
		&& (config_pgtypes & SVCB_CONFIG_V7))
	    *page_type = TCB_PG_V7;
    }

    if (valid_page_type_size(*page_type, page_size, create_flags, rec_len))
	return (E_DB_OK);
    else
    {
	*page_type = TCB_PG_INVALID;
	return (E_DB_ERROR);
    }
}

/*
** Name: valid_page_size_type - Is this a valid page size/type combination
**
** Description:
**	This routine is called to validate page type - page size
**
**      If page size is != 2048, TCB_PG_V1 page type should only be used
**      internally.... for 4k blob extension tables so that we will be able
**      to hold 2 etab records
**
** Inputs:
**      page type           - page type for this table
**	page size	    - page size for this table. One of:
**				2048, 4096, 8192, 16384, 32768, 65536
**
** Returns:
**      TRUE/FALSE
**
** History:
**      09-mar-2001 (stial01)
**          Created for Variable Page Size - Variable Page Type support    
*/
static bool
valid_page_type_size(
		i4 page_type,
		i4 page_size,
		i4 create_flags,
		i4 rec_len)
{

    if (page_size != 2048   && page_size != 4096  &&
	page_size != 8192   && page_size != 16384 &&
	page_size != 32768  && page_size != 65536)
	return (FALSE);

    if (!DM_VALID_PAGE_TYPE(page_type))
	return (FALSE);

    /*
    ** We cannot support TCB_PG_V1 if page size > 8192, since the
    ** line_id offset mask is 3fff, and the maximum offset for a tuple
    ** is 0x3fff=16383
    */
    if (page_type == TCB_PG_V1 && page_size > 8192)
	return (FALSE);

    if (create_flags & DM1C_CREATE_ETAB)
    {
	if (page_size == 2048 && page_type != TCB_PG_V1)
	    return (FALSE);
    }
    
    if (page_type != TCB_PG_V7 && 
	rec_len > dm2u_maxreclen(page_type, page_size))
	return (FALSE);

    return (TRUE);

}

/*{
** Name: dm1c_getklv - Map Data Type and Dimension to Key Level Vectors
**
** Description:
**	Given the field of a tuple in the iirelation relation,
**      this routine returns a pointer to the key level vectors:-
**	NBR(obj, box)->nbr; HILBERT(nbr)->binary(); NbrONbr(nbr, nbr)->I2
**
** Inputs:
**	*adf_cb	     Pointer to ADF_Control Block
**
**	dimension    Dimension of the object data type
**
**	obj_dt_id   Object data type
**
**	*klv         Pointer to a KLV data structure.
**
** Outputs:
**      klv          Fills in the KLV structure with the addresses to
**                   the routines.
**
** Returns:
**	OK
**
** Side Effects:
**	none
** History:
**      18-may-1995 (shero03)
**          Created for RTree.
**	23-apr-1996 (shero03)
**	    Added NBR U NBR and NBR N NBR
**	06-may-1996 (shero03
**	    Moved hilbert to adf/ads.
**	24-sep-1996 (shero03)
**	    Added hilbertsize and object data type
**	15-apr-1997 (shero03)
**	    Added dimension and perimeter to the KLV, 
**	    Removed dimension as an input parm. 
**	    Moved the code to adi_dt_rtree.
*/
DB_STATUS
dm1c_getklv(
            ADF_CB	  *adf_scb,
	    DB_DT_ID	  obj_dt_id,
	    DMPP_ACC_KLV  *klv)
{
    ADI_DT_RTREE	rtree_blk;	
    DB_STATUS		status;

    klv->obj_dt_id = (DB_DT_ID)abs(obj_dt_id);
    klv->box_dt_id = DB_NODT;
    klv->nbr_dt_id= DB_NODT;
    klv->dmpp_nbr = NULL;
    klv->dmpp_unbr = NULL;
    klv->dmpp_hilbert = NULL;
    klv->dmpp_overlaps = NULL;
    klv->dmpp_inside = NULL;

    /*
    ** Initialize "null" nbr
    ** Since an nbr can't have the ll > ur, this creates a unique, null value
    */
    MEfill(DB_MAXRTREE_NBR, 0x00, (PTR)&klv->null_nbr);
    klv->null_nbr[0] = 0x01;

    /* 
    ** The adf_qlang field has not been set up properly yet
    ** so, adi_opid doesn't think "nbr" is a valid operator
    ** The adf block should have been initialized better before
    ** now.  Short term - we know this is SQL only so force it.
    */
    adf_scb->adf_qlang = DB_SQL;

    /*
    ** Call adi_dt_rtree to obtain info about the object data type
    ** and its related functions necessary for R-tree.
    */

    status = adi_dt_rtree(adf_scb, obj_dt_id, &rtree_blk);
    if (DB_FAILURE_MACRO(status))
	return(status);

    klv->dimension = rtree_blk.adi_dimension;
    klv->hilbertsize = rtree_blk.adi_hilbertsize;
    klv->nbr_dt_id = rtree_blk.adi_nbr_dtid;
    klv->box_dt_id = rtree_blk.adi_range_dtid;
    klv->range_type = rtree_blk.adi_range_type;
    klv->dmpp_nbr = rtree_blk.adi_nbr;
    klv->dmpp_hilbert = rtree_blk.adi_hilbert;
    klv->dmpp_overlaps = rtree_blk.adi_nbr_overlaps;
    klv->dmpp_inside = rtree_blk.adi_nbr_inside;
    klv->dmpp_unbr = rtree_blk.adi_nbr_union;

    return(status);
}

/*{
** Name: dm1c_sys_init - Initialize a value of a system_maintained datatype
**
** Description:
**	Set the value of a system_maintained datatype.  Value to be inserted
**	depends on the datatype.
**
**	Currently this routine only supports the DB_LOGKEY_TYPE, DB_TABKEY_TYPE
**	datatypes, it is expected that when the sequence
**	datatype is added support for it will be added here.
**
**	Value of the datatype is put into record provided.
**
**	A logical_key data type is initialized with values inputed to the
**	routine.  These values, all included in the objkey input, include a
**	database id, a table id, and an 8 byte unique to a table id
**	(represented as a high and low i4).
**
** Inputs:
**	objkey				Object key to be used to assign logkeys.
**					The object key is already initialized by
**					the caller with the key values to be
**					assigned.
**      atts                      	Pointer to an array of attribute
**                                      descriptors.
**      atts_count                      Number of entries in atts_array.
**
**      rec                             Pointer to record to insert into.
**
** Outputs:
**	logkey_type			mask indicating type of logical key
**					that was inserted.
**					  RCB_TABKEY - This bit is asserted if
**						a system maintained table_key
**						was inserted.
**					  RCB_OBJKEY - This bit is asserted if
**						a system maintained object_key
**						was inserted.
**
**
**	Returns:
**	    OK		success.
**	    currently cannot fail.
**
** History:
**      28-feb-89 (mikem)
**	    Created.
**	17-may-89 (mikem)
**	    Changes necessary to make a table_key 8 bytes rather than 6.
**	12-feb-91 (mikem)
**	    Changes made to dm1c_sys_init() to return logical key to client.
**	    Changed interface to pass back which types of logical keys were
**	    assigned, and to accept an already initialized object key as input.
*/
DB_STATUS
dm1c_sys_init(
DB_OBJ_LOGKEY_INTERNAL	*objkey,
DB_ATTS       		*atts,
i4        		atts_count,
char           		*rec,
i4			*logkey_type,
DB_ERROR		*dberr)
{
    DB_ATTS		*att;
    char        	*dest_tup_ptr;
    i4     	i;
    STATUS		status = E_DB_OK;
    union {
	DB_TAB_LOGKEY_INTERNAL	tab_logical_key;
	DB_OBJ_LOGKEY_INTERNAL	obj_logical_key;
    }			logical_key;

    CLRDBERR(dberr);

    *logkey_type = 0;
    dest_tup_ptr = rec;

    for (i = atts_count, att = &atts[1]; --i >= 0; att++)
    {
	if ((att->type == DB_LOGKEY_TYPE || att->type == DB_TABKEY_TYPE) &&
	    (att->flag & ATT_SYS_MAINTAINED))
	{
	    if (att->type == DB_LOGKEY_TYPE)
	    {
		/* a object_key that we must fill in */
		*logkey_type |= RCB_OBJKEY;

		STRUCT_ASSIGN_MACRO((*objkey), logical_key.obj_logical_key);
	    }
	    else if (att->type == DB_TABKEY_TYPE)
	    {
		/* only other possibility is a table_key/seclabel_id */
		*logkey_type |= RCB_TABKEY;

		logical_key.tab_logical_key.tlk_low_id	=
				(u_i4) objkey->olk_low_id;
		logical_key.tab_logical_key.tlk_high_id	=
				(u_i4) objkey->olk_high_id;

	    }

	    MEcopy((PTR) &logical_key, att->length, dest_tup_ptr);
	}

	/* position ptr at the next attribute */

	dest_tup_ptr += att->length;
    }

    return(status);
}

/*{
** Name: dm1c_pdelete	- Delete peripheral objects from tuple
**
** Description:
**      This routine deletes all peripheral objects associated with a particular
**	tuple.  This is accomplished by searching the attributes.  For each
**	attribute which is peripheral, call the dmpe_delete() routine to remove
**	it.
**
** Inputs:
**      atts                            Pointer to array of DB_ATTS describing
**					the tuple
**      att_count                       Count of elements in atts
**	rcb				Pointer to RCB of table being deleted
**					from
**      record                          Ptr to tuple being for which this
**					operation is necessary
**	error				Ptr to place to put error
**
** Outputs:
**      *error				Filled with error (if any) from
**					dmpe_delete()
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Jan-1990 (fred)
**          Created.
**	30-Oct-1992 (fred)
**	    Fixed up handling of peripherals so that tuples to be deleted
**	    were found.
**	15-Jul-1996 (ramra01 for bryanp)
**	    Alter Table project: Peripheral objects associated with
**	    add / drop columns.
**	20-nov-1996 (nanpr01)
**	    If the version is correct, then only try to delete it. Otherwise
**	    forget it.
**	22-sep-1998 (somsa01)
**	    We need to properly set pop_temporary.  (Bug #93465)
**	15-apr-1999 (somsa01)
**	    Initialize pop_info to NULL.
**	11-May-2004 (schka24)
**	    Delete is from "permanent" etab.
[@history_template@]...
*/
DB_STATUS
dm1c_pdelete(
	DB_ATTS           *atts,
	i4            att_count,
	DMP_RCB		   *rcb,
	char               *record,
	DB_ERROR	   *dberr)
{
    DB_STATUS           status = E_DB_OK;
    ADP_POP_CB		pop_cb;
    DB_DATA_VALUE	cpn_dv;
    union {
        ADP_PERIPHERAL	    periph;
	char		    buf[sizeof(ADP_PERIPHERAL) + 1];
    }			pdv;
    DMPE_COUPON		*cpn;
    i4		i;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    pop_cb.pop_type = ADP_POP_TYPE;
    pop_cb.pop_length = sizeof(pop_cb);
    pop_cb.pop_coupon = &cpn_dv;
    pop_cb.pop_underdv = (DB_DATA_VALUE *) 0;
    pop_cb.pop_segment = (DB_DATA_VALUE *) 0;
    pop_cb.pop_continuation = 0;
    pop_cb.pop_temporary = ADP_POP_PERMANENT;
    pop_cb.pop_user_arg = (PTR) 0;
    pop_cb.pop_info = (PTR) 0;

    for (i = 1; i <= att_count; i++)
    {
	if ( (atts[i].flag & ATT_PERIPHERAL) &&
	     (atts[i].ver_dropped == 0) &&
	     (rcb->rcb_row_version >= atts[i].ver_added) )
	{
	    cpn_dv.db_datatype = atts[i].type;
	    cpn_dv.db_length = atts[i].length;
	    cpn_dv.db_prec = atts[i].precision;
	    cpn_dv.db_collID = atts[i].collID;

	    MECOPY_VAR_MACRO(&record[atts[i].offset],
				cpn_dv.db_length, pdv.buf);

	    cpn_dv.db_data = (PTR) pdv.buf;
	    cpn = (DMPE_COUPON *) &pdv.periph.per_value.val_coupon;
	    DMPE_TCB_ASSIGN_MACRO(rcb->rcb_tcb_ptr, cpn->cpn_tcb);
	    status = dmpe_delete(&pop_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
	        *dberr = pop_cb.pop_error;
	    }
	}
    }
    if (DMZ_REC_MACRO(3))
    {
	if (DB_FAILURE_MACRO(status))
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
	status = E_DB_OK;
	CLRDBERR(dberr);
    }
    return(status);
}

/*{
** Name: dm1c_pget	- Fill in coupons for gotten tuples
**
** Description:
**      This routine fills in the short term information in gotten tuples.
**	This is accomplished by searching the tuple for peripheral attributes,
**	then, for each one found, substituting the appropriate information.
**
**	At this time, the only information necessary is the rcb address.
**
** Inputs:
**      atts                            Pointer to array of DB_ATTS describing
**					the tuple
**      att_count                       Count of elements in atts
**	rcb				Pointer to RCB of table being gotten
**					from
**      record                          Ptr to tuple being for which this
**					operation is necessary
**	error				Ptr to place to put error
**
** Outputs:
**      *error				Filled with error (if any).
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Jan-1990 (fred)
**          Created.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      15-Jul-1996 (ramra01 for bryanp)
**          Alter Table project: Peripheral objects associated with
**          add / drop columns.
**	20-nov-1996 (nanpr01)
**	    If the version is correct, then only try to delete it. Otherwise
**	    forget it.
**	27-Jul-2005 (toumi01) BUG 114933
**	    If we are projecting key columns into the exchange buffer
**	    don't init non-existent coupon fields and trash memory.
**	    Previously deleted RCB_NO_CPN test has been restored.
**      01-may-2008 (huazh01)
**          Remove the variable 'cpn', the address for the coupon within a 
**          peripheral attributes might not be aligned. Use the expression
**          "(...val_coupon)->cpn_tcb" in the assign macro instead.
**          bug 120329.
[@history_template@]...
*/
DB_STATUS
dm1c_pget(
	DB_ATTS           *atts,
	i4		   att_count,
	DMP_RCB            *rcb,
	char		   *record,
	DB_ERROR	   *dberr)
{
    i4		i;

    CLRDBERR(dberr);

    if (rcb->rcb_state & RCB_NO_CPN)
	return (E_DB_OK);

    /*
    ** The short term tcb pointer in DMPE_COUPON is no longer needed during get
    ** because dmpe_allocate gets context by calling CSget_sid, GET_DML_SCB.
    **
    ** Zero out the tcb pointer in the DMPE_COUPON
    ** Even when called by modify
    */
    for (i = 1; i <= att_count; i++)
    {
	if ( (atts[i].flag & ATT_PERIPHERAL) &&
	     (atts[i].ver_dropped == 0) &&
	     (rcb->rcb_row_version >= atts[i].ver_added) )
	{
	    /*
	    ** Don't know that the coupon is aligned, so must account
	    ** for moving a pointer into it.
	    */

	    DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, 
                   ((DMPE_COUPON *) &((ADP_PERIPHERAL *)
                    &record[atts[i].offset])->per_value.val_coupon)->cpn_tcb);
	}
    }

    return(E_DB_OK);
}

/*{
** Name: dm1c_pput	- Put peripheral objects in a tuple
**
** Description:
**	This routine inserts the peripheral objects for all peripheral objects
**	associated with a particular tuple.  This is accomplished by searching
**	the attributes.  For each attribute which is peripheral, call the
**	dmpe_move() routine to insert it.
**
** Inputs:
**	load_blob			This is set if we are bulk loading a
**					blob into an extension table.
**	high				High half of the table key used to key
**					extended tables.
**	low				Low half of same.
**      atts                            Pointer to array of DB_ATTS describing
**					the tuple
**      att_count                       Count of elements in atts
**	rcb				Pointer to RCB of table being deleted
**					from
**      record                          Ptr to tuple being for which this
**					operation is necessary
**	error				Ptr to place to put error
**
** Outputs:
**      *error				Filled with error (if any) from
**					dmpe_delete()
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Jan-1990 (fred)
**          Created.
**      15-Jul-1996 (ramra01 for bryanp)
**          Alter Table project: Peripheral objects associated with
**          add / drop columns.
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. If tcb_temporary then set
**	    pop_temporary to ADP_POP_LONG_TEMP.
**      29-Jun-98 (thaju02)
**          Regression bug: with autocommit on, temporary tables created
**          during blob insertion, are not being destroyed at statement
**          commital, but are being held until session termination.
**          Regression due to fix for bug 87880. (B91469)
**	28-jul-1998 (somsa01)
**	    Tell dm1c_pput() if we are bulk loading blobs into extension
**	    tables or not via a new parameter.  (Bug #92217)
**	feb-mar-99 (stephenb)
**	    add code to allow peripheral inserts to load directly into the
**	    target table
**	15-apr-1999 (somsa01)
**	    Initialize pop_info to NULL.
**	26-apr-99 (stephenb)
**	    Check for nulls before de-refferencing the original coupon RCB,
**	    when the peripheral field contains a null value, the RCB pointer
**	    may be invalid.
**	14-oct-99 (stephenb)
**	    Check the SCB to see if the blob storage optimization has been used
**	    since checking the table name is not 100% reliable.
**	10-May-2004 (schka24)
**	    Blob put optimization now flagged in the coupon, not the SCB.
**	    Put is to final etab.
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag.
**	11-May-2007 (gupsh01)
**	    Added support for UTF8 character set.
**      19-Mar-2008 (coomi01) b119852
**          Whether or not the column is nullable
**          preserve the coupon details where the per_len is zero.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**      01-Dec-2008 (coomi01) b121301
**          If in a dbproc we do not throw away temp blob tables
**          immediately. They might be referenced by local variables.
**          
[@history_template@]...
*/
DB_STATUS
dm1c_pput(
	i4		   load_blob,
	u_i4		   high,
	u_i4		   low,
	DB_ATTS           *atts,
	i4            att_count,
	DMP_RCB		   *rcb,
	char               *record,
	DB_ERROR	   *dberr)
{
    DB_STATUS		status = E_DB_OK;
    ADP_POP_CB		pop_cb;
    DB_DATA_VALUE	cpn_dv;
    DB_DATA_VALUE	orig_dv;
    DMPE_COUPON		*cpn;
    DMPE_COUPON		*orig_cpn;
    union {
	ADP_PERIPHERAL	p;
	char		buf[sizeof(ADP_PERIPHERAL) + 1];
    }			orig;
    union {
	ADP_PERIPHERAL	p;
	char		buf[sizeof(ADP_PERIPHERAL) + 1];
    }			new_one;
    union {
	ADP_PERIPHERAL	p;
	char		buf[sizeof(ADP_PERIPHERAL) + 1];
    }			old_one;
    i4		i;
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    DB_BLOB_WKSP        blob_work;

    CLRDBERR(dberr);

    if (t->tcb_rel.relstat2 & TCB2_BSWAP)
    {
	char cp = *((char *)&low);
	*((char *)&low) = *((char *)&low + 3);
	*((char *)&low + 3) = cp;
    }

    pop_cb.pop_type = ADP_POP_TYPE;
    pop_cb.pop_length = sizeof(pop_cb);
    pop_cb.pop_coupon = &cpn_dv;
    pop_cb.pop_underdv = (DB_DATA_VALUE *) 0;
    pop_cb.pop_segment = &orig_dv;
    pop_cb.pop_continuation = 0;
    pop_cb.pop_temporary = ADP_POP_PERMANENT;
    pop_cb.pop_user_arg = (PTR) 0;
    pop_cb.pop_info = (PTR) 0;

    for (i = 1; i <= att_count; i++)
    {
	if ( (atts[i].flag & ATT_PERIPHERAL) &&
	     (atts[i].ver_dropped == 0) )
	{
	    cpn_dv.db_datatype =
		orig_dv.db_datatype = atts[i].type;
	    cpn_dv.db_length =
		orig_dv.db_length = atts[i].length;
	    cpn_dv.db_prec =
		orig_dv.db_prec = atts[i].precision;
	    cpn_dv.db_collID =
		orig_dv.db_collID = atts[i].collID;
	    MEcopy(&record[atts[i].offset], orig_dv.db_length,
					orig.buf);
	    MECOPY_CONST_MACRO(orig.buf, sizeof(orig), old_one.buf);
	    MECOPY_CONST_MACRO(orig.buf, sizeof(orig), new_one.buf);
	    orig_dv.db_data = (PTR) &orig;
	    cpn_dv.db_data = (PTR) &new_one;

	    cpn = (DMPE_COUPON *) &new_one.p.per_value.val_coupon;
	    DMPE_TCB_ASSIGN_MACRO(rcb->rcb_tcb_ptr, cpn->cpn_tcb);
	    cpn->cpn_log_key.tlk_low_id = (u_i4) low;
	    cpn->cpn_log_key.tlk_high_id = (u_i4) high;
	    orig_cpn = (DMPE_COUPON *) &old_one.p.per_value.val_coupon;

	    /*
	    ** If column is nullable, or empty, no data to move.
	    */
	    if (pop_cb.pop_segment->db_datatype < 0)
	    {
		i4	is_null;
		ADF_CB	adf_cb;

		MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);
		adf_cb.adf_maxstring = DB_MAXSTRING;

        	if (CMischarset_utf8())
          	  adf_cb.adf_utf8_flag = AD_UTF8_ENABLED;
        	else
          	  adf_cb.adf_utf8_flag = 0;

		status = adc_isnull(&adf_cb, pop_cb.pop_segment, &is_null);
		if (status)
		{
		    pop_cb.pop_error= adf_cb.adf_errcb.ad_dberror;
		    break;
		}

		if (is_null)
		{
		    ((ADP_PERIPHERAL *) 
			pop_cb.pop_coupon->db_data)->per_length0 = 
			old_one.p.per_length0 = 0;
		    ((ADP_PERIPHERAL *) 
			pop_cb.pop_coupon->db_data)->per_length1 = 
			old_one.p.per_length1 = 0;
		    orig_cpn->cpn_log_key.tlk_low_id = (u_i4) low;
		    orig_cpn->cpn_log_key.tlk_high_id = (u_i4) high;
		    DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, orig_cpn->cpn_tcb);
		    MEcopy(old_one.buf, atts[i].length,
			   &record[atts[i].offset]);
		    continue;
		}
	    }

	    /* 
	    ** BUG : b119852
	    **
	    ** Whether or not the column is nullable, the per_len may be zero.
	    ** - (This test used only to be in the nullable 'test & branch' above)
	    ** We must transfer the coupon none the less, for it contains
	    ** the per_key information, which otherwise will be lost.
	    **
	    */
	    if (((ADP_PERIPHERAL *)
		 pop_cb.pop_coupon->db_data)->per_length0 == 0 &&
		((ADP_PERIPHERAL *)
		 pop_cb.pop_coupon->db_data)->per_length1 == 0)
	    {
		orig_cpn->cpn_log_key.tlk_low_id = (u_i4) low;
		orig_cpn->cpn_log_key.tlk_high_id = (u_i4) high;
		old_one.p.per_length0 = 0;
		old_one.p.per_length1 = 0;
		DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, orig_cpn->cpn_tcb);
		MEcopy(old_one.buf, atts[i].length,
		       &record[atts[i].offset]);

		continue;
	    }

	    /* If sequencer was able to tell blob putter where the final
	    ** blob destination is, coupon is OK, no need to move data.
	    */
	    if (! DMPE_TCB_COMPARE_NE_MACRO(orig_cpn->cpn_tcb, DMPE_TCB_PUT_OPTIM))
	    {
		DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, orig_cpn->cpn_tcb);
		MEcopy(old_one.buf, atts[i].length, &record[atts[i].offset]);
	    }
	    else
	    {
		/* Init pop_info with target table info */
		blob_work.flags = BLOBWKSP_ATTID;
		blob_work.base_attid = i;
		blob_work.source_dt = t->tcb_atts_ptr[i].type;
		pop_cb.pop_info = (PTR)&blob_work;


		/*
		** Move can delete source if it's an anon-temp
		** - Unless we are in a dbproc, when the tidy up
		**   comes at end of dbproc as part of adu_free_objects().
		*/
		status = dmpe_move(&pop_cb, load_blob, (rcb->rcb_dsh_isdbp == 0));

		if (status)
		{
		    break;
		}
		DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, cpn->cpn_tcb);
		MEcopy(new_one.buf, atts[i].length, &record[atts[i].offset]);
	    }
	}
    }
    if (status)
	*dberr = pop_cb.pop_error;

    if (DMZ_TBL_MACRO(22) && status == E_DB_OK)
    {
	DB_STATUS lstatus;
	DB_ERROR  ldberr;
	lstatus = dm1c_pvalidate("INSERT  ", atts, att_count, rcb, record, &ldberr);
    }

    return(status);
}

/*{
** Name: dm1c_preplace	- Replace any peripheral objects that need replacing
**
** Description:
**	This routine replaces the peripheral objects for all peripheral objects
**	associated with a particular tuple.  This is accomplished by searching
**	the attributes.  For each attribute which is peripheral, call the
**	dmpe_replace() routine to replace it.
**
** Inputs:
**      atts                            Pointer to array of DB_ATTS describing
**					the tuple
**      att_count                       Count of elements in atts
**	rcb				DMP_RCB * for record being updated.
**      old				Ptr to old tuple (being replaced)
**	new				Ptr to new tuple
**	error				Ptr to place to put error
**
** Outputs:
**      *error				Filled with error (if any) from
**					dmpe_delete()
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-Jan-1990 (fred)
**          Created.
**	30-Oct-1992 (fred)
**	    Fixed up handling of peripherals so that tuples to be replaced
**	    were found.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      15-Jul-1996 (ramra01 for bryanp)
**          Alter Table project: Peripheral objects associated with
**          add / drop columns.
**	20-nov-1996 (nanpr01)
**	    dm1c_preplace was assuming before that a peripheral object is
**	    present. However because of the row_version, the object is not
**	    actually present. So treat it as insert.
**	28-jul-1998 (somsa01)
**	    Tell dmpe_move() if we are bulk loading blobs into extension
**	    tables or not via a new parameter.  (Bug #92217)
**	22-sep-1998 (somsa01)
**	    We need to properly set pop_temporary.  (Bug #93465)
**	17-feb-1999 (somsa01)
**	    If dmpe_replace() returned an OK status, yet the
**	    pop_error.err_code was set to E_DM0154_DUPLICATE_ROW_NOTUPD,
**	    send this back up (as there were no peripheral tuples
**	    updated).
**	15-apr-1999 (somsa01)
**	    Initialize pop_info to NULL.
**	10-apr-2002 (thaju02)
**	    Update of a blob column preceded by alter table add long
**	    varchar/byte column and modify, results in duplicate logical
**	    keys within coupon. Select query fails with E_AD7004, 
**	    E_SC0216, E_SC0206. (b107428, INGSRV1738)
**	10-May-2004 (schka24)
**	    Blob put optimization now flagged in the coupon, not the SCB.
**	    Put is to final etab, set pop-temporary accordingly.
**	13-Feb-2007 (kschendel)
**	    dml scb fetch no longer needed due to above, kill it.
**	16-Oct-2008 (coomi01) b121046
**          When updating a peripeheral of zero length we may not
**          presume the incoming coupon is consistent with outgoing
**          one. We therefore ignore the DMPE_TCB_PUT_OPTIM flag and
**          force the update through rather than optimize it away.
**      11-Nov-2008 (coomi01) b121046
**          Move the declarator for old_periph to head of routine.
**
[@history_template@]...
*/
DB_STATUS
dm1c_preplace(
	DB_ATTS           *atts,
	i4            att_count,
	DMP_RCB		   *rcb,
	char		   *old,
	char               *new,
	DB_ERROR	   *dberr)
{
    DB_STATUS		status = E_DB_OK;
    ADP_POP_CB		pop_cb;
    DB_DATA_VALUE	old_dv;
    DB_DATA_VALUE	new_dv;
    DMP_RCB         	*r = rcb;
    DMPE_COUPON		*old_cpn;
    DMPE_COUPON		*new_cpn;
    union {
	    ADP_PERIPHERAL	periph;
	    char		buf[sizeof(ADP_PERIPHERAL) + 1];
    }			old_value;
    union {
	    ADP_PERIPHERAL	periph;
	    char		buf[sizeof(ADP_PERIPHERAL) + 1];
    }			new_value;
    i4		i;
    bool		updated = FALSE;
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    DB_BLOB_WKSP        blob_work;
    bool		need_to_insert = FALSE;
    ADP_PERIPHERAL      *old_periph;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    pop_cb.pop_type = ADP_POP_TYPE;
    pop_cb.pop_length = sizeof(pop_cb);
    pop_cb.pop_coupon = &new_dv;
    pop_cb.pop_underdv = (DB_DATA_VALUE *) 0;
    pop_cb.pop_segment = &old_dv;
    pop_cb.pop_continuation = 0;
    pop_cb.pop_temporary = ADP_POP_PERMANENT;
    pop_cb.pop_user_arg = (PTR) 0;
    pop_cb.pop_info = (PTR) 0;

    for (i = 1; i <= att_count; i++)
    {
	if ( (atts[i].flag & ATT_PERIPHERAL) && (atts[i].ver_dropped == 0) )
	{
	    /* Non-dropped blob column */
	    need_to_insert = FALSE; /* init for every column */
	    old_dv.db_datatype =
		new_dv.db_datatype = atts[i].type;
	    old_dv.db_length =
		new_dv.db_length = atts[i].length;
	    old_dv.db_prec =
		new_dv.db_prec = atts[i].precision;
	    new_dv.db_data = (PTR) &new_value;
	    old_dv.db_collID =
		new_dv.db_collID = atts[i].collID;
	    MECOPY_VAR_MACRO( (PTR) &new[atts[i].offset],
				new_dv.db_length,
				(PTR) new_value.buf);
	    old_dv.db_data = (PTR) old_value.buf;
	    new_cpn = (DMPE_COUPON *) &new_value.periph.per_value.val_coupon;
	    if ( r->rcb_row_version >= atts[i].ver_added )
	    {
		/* Row contains old coupon, but check that the coupon is
		** real and not something bogus introduced by add column
		** followed by modify
		*/
		MECOPY_VAR_MACRO( (PTR) &old[atts[i].offset],
				old_dv.db_length,
				(PTR) old_value.buf);

		old_cpn = (DMPE_COUPON *)
				&old_value.periph.per_value.val_coupon;
		if ( (old_cpn->cpn_log_key.tlk_low_id == 0) &&
		     (old_cpn->cpn_log_key.tlk_high_id == 0) )
		    need_to_insert = TRUE;
		else
		{
		    DMPE_TCB_ASSIGN_MACRO(rcb->rcb_tcb_ptr, old_cpn->cpn_tcb);
		    
		    /* Bug 121046
		    **
		    **   When the old peripheral had zero length we may not
		    **   take the optim route. The old coupon is not in the
		    **   old etab itself (data is zero length). We have then to go 
		    **   through, delete and update the master table to maintain
		    **   consistency. Not doing so potentially leaves us with
		    **   duplicate keys in the master table.
		    */
                    old_periph = (ADP_PERIPHERAL *)old_dv.db_data;

		    if ( 
			 ! (old_periph->per_length0 == 0 && old_periph->per_length1 == 0 ) && 
			 ! DMPE_TCB_COMPARE_NE_MACRO(new_cpn->cpn_tcb, DMPE_TCB_PUT_OPTIM) )
		    {
			pop_cb.pop_coupon = &old_dv;
			status = dmpe_delete(&pop_cb);
			/* The coupon in new is already correct */
			continue;
		    }
		    else
		    {
 			/* Send table information in pop_info */
			blob_work.flags = BLOBWKSP_ATTID;
			blob_work.base_attid = i;
			blob_work.source_dt = t->tcb_atts_ptr[i].type;
			pop_cb.pop_info = (PTR)&blob_work;
			/* Delete old, move new to old, update old coupon */
			status = dmpe_replace(&pop_cb);
		    }
		    if (status)
		    {
			if (DMZ_REC_MACRO(3))
			{
			    if (DB_FAILURE_MACRO(status))
			    {
				uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
			    }
			    status = E_DB_OK;
			    CLRDBERR(dberr);
			}
			else
			{
			    break;
			}
		    }
		    else if (pop_cb.pop_error.err_code != E_DM0154_DUPLICATE_ROW_NOTUPD)
		    {
			updated = TRUE;
		    }
		    new_cpn->cpn_etab_id = old_cpn->cpn_etab_id;
		    new_cpn->cpn_log_key.tlk_low_id = (u_i4)
					old_cpn->cpn_log_key.tlk_low_id;
		    new_cpn->cpn_log_key.tlk_high_id = (u_i4)
					old_cpn->cpn_log_key.tlk_high_id;

		    DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, new_cpn->cpn_tcb);
		    MECOPY_VAR_MACRO( (PTR) new_value.buf,
				    new_dv.db_length,
				    (PTR) &new[atts[i].offset]);
		}
	    } /* if row contained att */
	    if ( r->rcb_row_version < atts[i].ver_added || need_to_insert )
	    {
		u_i4 high, low;

		if (! DMPE_TCB_COMPARE_NE_MACRO(new_cpn->cpn_tcb, DMPE_TCB_PUT_OPTIM))
		{
		    /* The coupon in new is already correct */
		    continue;
		}

		status = dm1c_get_high_low_key(rcb->rcb_tcb_ptr, rcb, &high, &low,
			&pop_cb.pop_error);
		if (status != E_DB_OK)
		    break;
		MECOPY_CONST_MACRO(new_value.buf, sizeof(new_value), old_value.buf);
 
		DMPE_TCB_ASSIGN_MACRO(rcb->rcb_tcb_ptr, new_cpn->cpn_tcb);
		new_cpn->cpn_log_key.tlk_low_id = (u_i4) low;
		new_cpn->cpn_log_key.tlk_high_id = (u_i4) high;

		/*
		** Init pop_info with target table info
		*/
		blob_work.flags = BLOBWKSP_ATTID;
		blob_work.base_attid = i;
		blob_work.source_dt = t->tcb_atts_ptr[i].type;
		pop_cb.pop_info = (PTR)&blob_work;
		/* Move can delete source if it's an anon-temp */
		status = dmpe_move(&pop_cb, 0, TRUE);

		if (status)
		{
		    if (DMZ_REC_MACRO(3))
		    {
			if (DB_FAILURE_MACRO(status))
			{
			    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
			}
			status = E_DB_OK;
			CLRDBERR(dberr);
		    }
		    else
		    {
			break;
		    }
		}
		else if (pop_cb.pop_error.err_code != E_DM0154_DUPLICATE_ROW_NOTUPD)
		{
		    updated = TRUE;
		}
		DMPE_TCB_ASSIGN_MACRO(DMPE_NULL_TCB, new_cpn->cpn_tcb);
		MEcopy(new_value.buf, atts[i].length, &new[atts[i].offset]);
	    }
	} /* if blob */
    } /* for each att */
    if (status)
	*dberr = pop_cb.pop_error;
    else if (!updated)
	SETDBERR(dberr, 0, E_DM0154_DUPLICATE_ROW_NOTUPD);

    if (DMZ_REC_MACRO(3))
    {
	if (DB_FAILURE_MACRO(status))
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
	status = E_DB_OK;
	CLRDBERR(dberr);
    }

    if (DMZ_TBL_MACRO(22) && status == E_DB_OK)
    {
	DB_STATUS lstatus;
	DB_ERROR  ldberr;
	lstatus = dm1c_pvalidate("REPL old", atts, att_count, rcb, old, &ldberr);
	lstatus = dm1c_pvalidate("REPL new", atts, att_count, rcb, new, &ldberr);
    }

    return(status);
}
VOID
dm1cBadTidFcn(
DMP_RCB	    *rcb,
DM_TID	    *tid,
PTR	    FileName,
i4	    LineNumber)
{
    i4	    	    local_err;
    DB_ERROR	    local_dberr;

    local_dberr.err_code = E_DM938A_BAD_DATA_ROW;
    local_dberr.err_data = 0;
    local_dberr.err_file = FileName;
    local_dberr.err_line = LineNumber;

    /*
    ** DM1C_GET returned an error - this indicates that
    ** something is wrong with the tuple at the current tid.
    */
    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
	NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 4,
	sizeof(DB_DB_NAME),
	&rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,
	sizeof(DB_TAB_NAME), &rcb->rcb_tcb_ptr->tcb_rel.relid,
	sizeof(DB_OWN_NAME), &rcb->rcb_tcb_ptr->tcb_rel.relowner,
	0, tid->tid_i4);
}

/*
** Name: dm1c_get_high_low_key
**
** Description:
**	This utility retrieves a logical key value, generating more key
**	values if necessary.  The "high low key" name is somewhat misleading,
**	and simply refers to the fact that a logical key is at least 8
**	bytes.  The caller's RCB is set up with the full 16 byte object
**	key in case that's what the user wants.
**
**	If there aren't any more logical key values available to hand out,
**	dm2t is called to get more.  This will involve a mini-transaction
**	and an update of the iirelation row.
**
**	If the RCB is for a partition, all the work is done against the
**	master.  Logical keys are handed out table-wide, not partition
**	by partition.
**
** Inputs:
**	tcb		Table to get key value for
**	rcb		RCB to set up, should point to tcb!
**	high		An output
**	low		An output
**	err_code	An output
**
** Outputs:
**	Returns the usual DB_STATUS
**	high, low	Set to the next table key value
**	err_code	Specific error code if not E_DB_OK
**	Also sets up rcb_logkey in the rcb with the full object key.
**
** Side Effects:
**	Takes [master] tcb mutex, may do mini-transaction on iirelation
**
** History:
**	(unknown)
**	15-Jan-2004 (schka24)
**	    Made external for others to use;  work against master always.
*/

DB_STATUS
dm1c_get_high_low_key(
DMP_TCB		*tcb,
DMP_RCB		*rcb,
u_i4	*high,
u_i4	*low,
DB_ERROR	*dberr)
{
    DB_STATUS           status = E_DB_OK;
    DMP_TCB		*t = tcb->tcb_pmast_tcb;	/* Master always! */
    DMP_RCB		*r = rcb;

    CLRDBERR(dberr);

    /* get the unique logical key */
 
    dm0s_mlock(&t->tcb_mutex);
 
    status = E_DB_OK;
 
    if ((t->tcb_low_logical_key % TCB_LOGICAL_KEY_INCREMENT) == 0)
    {
	/* on success the tcb is guaranteed to include a usable
	** logical key
	*/
	status = dm2t_update_logical_key(r, dberr);

	/* TCB mutex is re-taken on return, success or failure */
    }
 
    if (status == E_DB_OK)
    {
	*high = t->tcb_high_logical_key;
	*low  = t->tcb_low_logical_key;
	t->tcb_low_logical_key++;
 
	dm0s_munlock(&t->tcb_mutex);
 
	/* initialize saved copy of "last" logical key assigned */
 
	r->rcb_logkey.olk_low_id = *low;
	r->rcb_logkey.olk_high_id = *high;
	r->rcb_logkey.olk_rel_id = t->tcb_rel.reltid.db_tab_base;
	r->rcb_logkey.olk_db_id = t->tcb_dcb_ptr->dcb_id;

	/* After storing last logical key in rcb, do the byte swap */ 
	if (t->tcb_rel.relstat2 & TCB2_BSWAP)
	{
	    char cp = *((char *)low);
	    *((char *)low) = *((char *)low + 3);
	    *((char *)low + 3) = cp;
	}
    }
    else
    {
	/* Error, but don't hold the mutex! */
	dm0s_munlock(&t->tcb_mutex);
    }
    return(status);
} 

/*{
** Name: dm1c_get - Get a record and do additional processing required
**       for compressed, altered and/or segmented rows.
**
** Description:
**      Get the specified record from the page, and if necessary
**      get additional segments, and/or uncompress and/or convert the
**      row to the specified table version.
**      
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
** 	data				data page
**	tid				tid of row to get
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**					E_DM003C_BAD_TID
**					E_DM938B_INCONSISTENT_ROW
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**      24-jul-2004 (stial01)
**          created from existing code.
**	28-Jul-2004 (schka24 for stial01)
**	    If get accessor returns "warn", just accept it.
**	13-Feb-2008 (kschendel) SIR 122739
**	    Revise uncompress call, remove record-type and tid.
*/
DB_STATUS
dm1c_get(
DMP_RCB		*r,
DMPP_PAGE	*data,
DM_TID		*tid,
PTR		record,
DB_ERROR	*dberr)
{
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DB_STATUS		s;
    i4			uncompressed_length;
    char		*rec_ptr;
    i4			record_size;
    i4			row_version = 0;
    i4			*row_ver_ptr;
    DMPP_SEG_HDR	seg_hdr;
    DMPP_SEG_HDR	*seg_hdr_ptr;
    bool		convert_row;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (r->rcb_proj_relversion)
	row_ver_ptr = &row_version;
    else
	row_ver_ptr = NULL;

    if (t->tcb_seg_rows)
	seg_hdr_ptr = &seg_hdr;
    else
	seg_hdr_ptr = NULL;

    record_size = r->rcb_proj_relwid;

    s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
	    t->tcb_rel.relpgsize, data, tid, &record_size,
	    &rec_ptr, row_ver_ptr, NULL, NULL, seg_hdr_ptr);
    if (s == E_DB_ERROR)
	SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
    else if (s == E_DB_WARN && rec_ptr)
	s = E_DB_OK;			/* Accept deleted row */

    r->rcb_row_version = row_version;
    convert_row = (r->rcb_data_rac->compression_type != TCB_C_NONE) ||
		(row_version != r->rcb_proj_relversion);

    if (s == E_DB_OK && seg_hdr_ptr && seg_hdr_ptr->seg_next)
    {
	if (convert_row)
	{
	    s = dm1r_get_segs(r, data, seg_hdr_ptr, rec_ptr, r->rcb_segbuf, dberr);
	    rec_ptr = r->rcb_segbuf;
	}
	else
	{
	    s = dm1r_get_segs(r, data, seg_hdr_ptr, rec_ptr, record, dberr);
	    rec_ptr = record;
	}
    }

    if (s == E_DB_OK && convert_row)
    {
	if (r->rcb_data_rac->compression_type != TCB_C_NONE)
	{
	    s = (*r->rcb_data_rac->dmpp_uncompress)(
		r->rcb_data_rac,
		rec_ptr, record, record_size,
		&uncompressed_length,
		&r->rcb_tupbuf, r->rcb_row_version, r->rcb_adf_cb);
	}
	else
	{
	    s = dm1r_cvt_row(
		r->rcb_data_rac->att_ptrs, r->rcb_data_rac->att_count,
		rec_ptr, record, record_size, &uncompressed_length,
		r->rcb_row_version, r->rcb_adf_cb);
	}

	if ( (s != E_DB_OK) || (uncompressed_length != record_size))
	{
	    if (s != E_DB_OK)
	    {
		uncompressed_length = 0;
		record_size = 0;
	    }

	    if (r->rcb_data_rac->compression_type != TCB_C_NONE)
	    {
		uleFormat(NULL, E_DM942C_DMPP_ROW_UNCOMP,
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, err_code, 7, sizeof(DB_DB_NAME),
		    &t->tcb_dcb_ptr->dcb_name, sizeof(DB_TAB_NAME),
		    &t->tcb_rel.relid, sizeof(DB_OWN_NAME),
		    &t->tcb_rel.relowner, 0,
		    tid->tid_tid.tid_page, 0, tid->tid_i4,
		    0, record_size, 0, uncompressed_length);
	    }
	    else
	    {
		uleFormat(NULL, E_DM9445_DMR_ROW_CVT_ERROR,
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, err_code, 7, sizeof(DB_DB_NAME),
		    &t->tcb_dcb_ptr->dcb_name, sizeof(DB_TAB_NAME),
		    &t->tcb_rel.relid, sizeof(DB_OWN_NAME),
		    &t->tcb_rel.relowner, 0, 0, 0, 0, 0, 0, 0, 0);
	    }
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    s = E_DB_ERROR;
	}
	rec_ptr = record;
    }
    
    if (s == E_DB_OK && rec_ptr != record)
	MEcopy(rec_ptr, record_size, record);

    /*
    ** If E_DB_ERROR, err_code should already be set.
    ** If the row spans pages, lock errors are possible.
    ** Otherwise the row is probably inconsistent (E_DM938B_INCONSISTENT_ROW).
    ** The caller of dm1c_get needs to be able to distinguish 
    ** lock failure errors from inconsistent row errors,
    ** so don't remap the error
    */

    return (s);
}

/* Validate etab id in coupon is an etab for this table */
static DB_STATUS
dm1c_pvalidate(
	char		*where,
	DB_ATTS           *atts,
	i4		   att_count,
	DMP_RCB            *rcb,
	char		   *record,
	DB_ERROR	   *dberr)
{
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMPE_COUPON		*cpn;
    char		*cpn_chr;
    DMPE_COUPON		tmp_cpn;
    i4		i;
    DMP_ET_LIST		*etlist;
    bool		found = FALSE;

    CLRDBERR(dberr);

    /* Validate etab id in coupon is an etab for this table */
    for (i = 1; i <= att_count; i++)
    {
	if ( (atts[i].flag & ATT_PERIPHERAL) &&
	     (atts[i].ver_dropped == 0) &&
	     (rcb->rcb_row_version >= atts[i].ver_added) )
	{
	    /*
	    ** Don't know that the coupon is aligned, so must account
	    ** for moving a pointer into it.
	    */
	    cpn_chr = (char *) &((ADP_PERIPHERAL *)
			&record[atts[i].offset])->per_value.val_coupon;
	    cpn = (DMPE_COUPON *) &((ADP_PERIPHERAL *)
			&record[atts[i].offset])->per_value.val_coupon;
	    if ((char *)cpn != cpn_chr)
		 TRdisplay("DMPE VALIDATE WARNING cpn %x %x\n", cpn, cpn_chr);

	    MEcopy(cpn_chr, sizeof(DMPE_COUPON), &tmp_cpn);

	    for (etlist = rcb->rcb_tcb_ptr->tcb_et_list->etl_next;
			etlist != rcb->rcb_tcb_ptr->tcb_et_list;
				etlist = etlist->etl_next)
	    {
		if (tmp_cpn.cpn_etab_id == etlist->etl_etab.etab_extension)
		{
		    found = TRUE;
		    break;
		}
	    }

	    if (!found)
	    {
		TRdisplay("DMPE VALIDATE WARNING: %s %~t col %d %d etab %d\n",
		    where,
		    32, t->tcb_rel.relid.db_tab_name, i,
		    t->tcb_rel.reltid.db_tab_base, tmp_cpn.cpn_etab_id);
	    }
	}
    }

    return(E_DB_OK);
}
