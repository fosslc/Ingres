/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <qu.h>

# include <sl.h>
# include <iicommon.h>
# include <gca.h>
# include <gcocomp.h>
# include <gcoint.h>

/*
** Name: GCOOD.C - Definitions for Object Descriptors.
**
** History:
**	25-Jan-90 (seiwald)
**	   All valid GCA messages have descriptors: messages which carry
**	   no data use the GCA_EMPTY_DESC.  Tuple data now has a parent
**	   VARIMPAR object to eliminate special case looping code.
**	12-Feb-90 (seiwald)
**	    Moved het support in from gccpl.h.
**	27-Mar-90 (seiwald)
**	    Added description of GCA_EVENT message.
**	26-Mar-91 (seiwald)
**	    Changed description of GCA_RE_DATA to match Mikem's change
**	    in gca.h for logical keys.
**	20-May-91 (seiwald)
**	    Fixed bad count for GCA_RE_DATA.
**	13-Jun-91 (seiwald)
**	    Excised much fluff:
**	    -	A complex GCA_ELEMENT has a TPL and an od_ptr, though once
**	        the od_ptr is known, the TPL is not needed.  For complex
**	        GCA_ELEMENTs defined in this file, the TPL is now {0,0,0},
**		and all GCAT_ and GCTR_ defines have been removed.  This 
**	        saves the trouble of making up unused type numbers.
**	    -	All GC_..._TR defines have been replaced with their actual
**		values (a TPL initializer).
**	    -	Removed gca_objects[] - no one ever references GCA messages
**		by their (entirely arbitrary) TPL.
**	    -	Removed user_objects[] - we don't generate them yet, so why
**		bother.
**	    -	Use GCA_VARxxxx from gca.h instead of constants for arr_stat.
**	    -	Redefined OD_INST so that some casting can be avoided.
**	    -	Removed unenlightening comments.
**	20-Jun-91 (seiwald)
**	    Added descriptions for Name Server messages, so that Name Server
**	    operations can be carried out across het-net.  This is for 
**	    testing only: any actual use will have to be keyed to a change
**	    in the GCA_PROTOCOL_LEVEL.
**	17-Jul-91 (seiwald) bug #37564
**	    New messages GCA1_C_FROM, GCA1_C_INTO with corrected copy 
**	    row domain descriptor structures GCA1_CPRDD.  The old descriptor
**	    used a character array as the 'with null' nulldata, even though
**	    its type varied.  The null value is now a GCA_DATA_VALUE.
**	    Introduced at (new) GCA_PROTOCOL_LEVEL_50.
**	12-Aug-91 (seiwald)
**	   A few adjustments to GCA1_CPRDD requested by the front ends.
**	28-Jan-92 (seiwald)
**	    Support for installation passwords: new messages GCN_NS_AUTHORIZE
**	    and GCN_AUTHORIZED to request and receive authorization tickets
**	    from remote Name Servers.
**	6-Aug-92 (seiwald)
**	    Moved data definitions out of gccod.h and into gccod.c.
**	14-Aug-92 (seiwald)
**	    GLOBALDEF, GLOBALDEF, GLOBALDEF.  Can't READONLY object 
**	    descriptors right now, because their arr_stat's can get 
**	    modified in gcc_comp_od.
**	20-Aug-92 (seiwald)
**	    Temporary home for descriptions of ADF datatypes.
**	01-Dec-92 (brucek)
**	    Added descriptors for GCM messages.
**	02-Dec-92 (brucek)
**	    Added descriptor for GCN_NS_2_RESOLVE.
**	16-Dec-92 (seiwald)
**	    New descriptors for long varchar and nullable long varchar, the
**	    first of the blob types (that use GCA_VARSEGAR).
**	09-Mar-93 (seiwald)
**	    Add descriptor for GCA1_DL_DATA as part of GCA_PROTOCOL_LEVEL_60.
**      15-Apr-1993 (fred)
**          Added descriptor for byte, byte varying, and long byte
**          datatypes. 
**	27-apr-93 (sweeney)
**	    Changed gca_msg_ods to be type GLOBALCONSTDEF blah to match
**	    the GLOBALCONSTREF in gccod.h - the previous combo wouldn't
**	    compile on my box (gca_msg_ods redefined).
**	21-Apr-93 (fpang)
**	    Add descriptors for GCA1_IP_DATA and GCA_XA_DIS_TRAN_ID, both 
**	    as part of GCA_PROTOCOL_LEVEL_60.
**	    Also set the descriptor for GCA1_DELETE which was defined, but
**	    not set.
**	3-Jun-93 (seiwald)
**	    Eliminate the age old GCA_TRIAD_OD - when trying to find the
**	    GCA_OBJECT_DESC for a complex type, we now just directly scan 
**	    the table handed back by adc_gcc_decompose().
**	13-Jun-93 (seiwald)
**	    Nullable decimal was missing its null flag - added it.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	20-sep-93 (seiwald)
**	    Revert to GC_PAD_ATOMIC's having an explicit array status
**	    GCA_VARPADAR, which tells gcc_comp_el() that the size of the
**	    pad is already on the stack from the previous GCA_VARVCHAR.
**	    The previous logic assumed that only GC_CHAR_ATOMIC's could
**	    precede pads, and this was broken with the new "nullable
**	    byte varying" type.
**	21-sep-93 (seiwald) bug #54553
**	    Length of nullable decimal was bogus: should be TPL length
**	    minus 1 (VAR1TPLAR), rather than TPL length (VARTPLAR).
**	    Also, fixed name of "byte" type.
**	22-dec-93 (robf)
**          Add GCA_PROMPT, GCA_PRREPLY, DB_SEC_TYPE
**      24-Sep-93 (iyer)
**          Change the object descriptor for the GCA_XA_SECURE message.
**          The message structure now has two additional members. 
**          The two members have been added to the end of the structure
**          are of type "nat".
**	14-Mar-94 (seiwald)
**	    Renamed to gcdod.c from gccod.c.
**	24-mar-94 (peeje01)
**	    Cross integration of doublebyte changes for 6500db_su4_us42
**	25-May-95 (gordy)
**	    Long datatype segments now include the segment present indicator
**	    as first element, possibly followed by length and data.  Changed
**	    GCA_VARSEGAR to continue processing until internal indicator is
**	    0.  Added GCA_VARVSEGAR for segment data (similar to GCA_VARVCHAR)
**	    since can't use GCA_VAREXPAR in a GCA_ELEMENT.
**	14-Jun-95 (gordy)
**	    GCA_SECURE messages use the GCA_TX_DATA message.  Updated
**	    GCD_XA_DIS_TRAN_ID object descriptor to match structure
**	    in gcd.h.  Added object descriptor for GCD_XA_TX_DATA.
**	16-Nov-95 (gordy)
**	    Add GCA1_FETCH message.
**	19-Dec-95 (gordy)
**	    Fixing definitions for GCA_PROMPT_DATA and GCA_PRREPLY_DATA.
**	29-Feb-96 (gordy)
**	    Converted VAREXPAR arrays which occur at start of top level
**	    data objects to VARIMPAR so that large messages may be easily
**	    split at element boundaries.  Created a new data object,
**	    od_tdescr identical to original od_td_data to be used in
**	    the copy map structures.  Removed od_gcn_nm_data which was
**	    simply a VAREXPAR and replaced its single usage by a VARIMPAR.
**	19-Mar-96 (gordy)
**	    Since the tuple object is just a VARIMPAR array of tuples,
**	    which is just a VARIMPAR array of characters (of uncertain
**	    length!), reduce the complexity by using the top level object
**	    gcd_od_tuple as the key object to trigger compilation of tuples.
**	    This effectively removes the VARIMPAR compilation normally done
**	    for tuples.  The compilation routines replace this with an input
**	    loop around the tuple and a VARIMPAR treatement of each element.
**	21-Jun-96 (gordy)
**	    Added GCA_VARMSGAR for messages which are processed in bulk
**	    instead of formatted elements (tuple, trace, empty messages).  
**	    During HET processing tuples are processed by column, while
**	    the formatted GCA interface processes tuples in bulk.  The
**	    OP_CALLTUP operator handles the distinction.
**	30-sep-1996 (canor01)
**	    Add gcd_tuple_prog from gcdcomp.c.
**	 5-Feb-97 (gordy)
**	    DBMS datatypes now represented by internal GCD datatypes and
**	    object descriptors which can be pre-compiled.  Simplified the
**	    array status definitions.  BLOB segment lists now represented 
**	    by their own array status type.  Compressed variable length 
**	    datatypes now have their own object descriptors rather than 
**	    conditional compilation based on array type.  GCA_IGNLENGTH 
**	    flag is now used to determine if the TPL length should be
**	    pushed on the compilation stack prior to calling the pre-
**	    compiled program.  Descriptors with objects having array
**	    types of VARTPLAR or VARVCHAR should not set GCA_IGNLENGTH.
**	    Reworked the global structures for datatype object descriptors.  
**	    Cleaned up the handling of tuple messages for all processing 
**	    (convert/encode/decode).  
**	29-May-97 (gordy)
**	    Fixed object descriptors for GCN_RESOLVED messages.  Added
**	    object descriptors for new GCN2_RESOLVED message.
**	17-Oct-97 (rajus01)
**	    Added auth, emode, emech objects in the GCN2_RESOLVED message.
**	 4-Dec-97 (rajus01)
**	    Changed subscriptor range to 11 for GCN2_RESOLVED message. 
**	12-Jan-98 (gordy)
**	    Added local host to GCN_LCL_ADDR.
**	31-Mar-98 (gordy)
**	    Made GCN2_D_RESOLVED extensible with variable array of values.
**	 3-Sep-98 (gordy)
**	    XA transaction IDs are binary not character.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 1-Mar-01 (gordy)
**	    All TPLs now have length specified.  Added object definitions
**	    for new Unicode datatypes.
**       06-07-01 (wansh01) 
**          Added GLOBALDEF gcd_trace_level.
**	21-Dec-01 (gordy)
**	    Replaced PTR in DB_DATA_VALUE and GCA_ELEMENT with i4.
**	22-Mar-02 (gordy)
**	    Cleanup of GCA definitions.  Simplified ATOMIC types.
**	 9-Sep-02 (gordy)
**	    Fix 'nullable long text' object definition.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	15-Mar-04 (gordy)
**	    Addes support for i8's.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
**	31-May-06 (gordy)
**	    Added definitions for ANSI date/time types.
**	21-Jun-2006 (bonro01)
**	    Fix compile problem caused by last change.
**	    structure allocation should match number of initializers.
**	30-Jun-06 (gordy)
**	    Extend XA support.
**	12-Jul-06 (gordy)
**	    Fixed element counts on XA and ANSI date/time definitions.
**	24-Oct-06 (gordy)
**	    Added definitions for Blob/Clob locator types.
**	12-Mar-07 (gordy)
**	    Added definition for GCA2_FT_DATA for GCA2_FETCH.
**	 1-Oct-09 (gordy)
**	    Added definition for GCA2_IP_DATA for GCA2_INVPROC.
**	 9-Oct-09 (gordy)
**	    Added definitions for Boolean type.
**	22-Jan-10 (gordy)
**	    Cleaned up multi-byte charset processing.  GCA_VARSEGAR
**	    now used strictly for segmented arrays.  GCA_VARVCHAR 
**	    now used for unpadded variable arrays while padded
**	    variable arrays use new GCA_VARPADAR array designation.
**	    New object descriptor flag GCA_VARLENGTH is used for
**	    truly variable length types: segmented and unpadded 
**	    variable arrays.  GCA_IGNLENGTH flag no longer applied 
**	    to unpadded variable array types since max length is
**	    is needed to limit expansion of data during charset
**	    converion.
*/

/*
**
LIBRARY = IMPGCFLIBDATA
**
*/

/*
** The GCA_OBJECT_DESCR structure is variable length and
** can't be used directly to statically declare object
** descriptors.  The variable length nature of descriptors
** is avoided by declaring a structure matching the fixed 
** part of GCA_OBJECT_DESCR and a macro to declare a fixed 
** size structure with the desired number of elements.
** Note that GCA_ELEMENT can't be used directly either
** because it references GCA_OBJECT_DESCR.
*/

typedef struct
{
    char            data_object_name[GCA_OBJ_NAME_LEN]; /* Object name */
    i4		    flags;		/* Flags, defined below. */
    i4		    el_count;		/* Number of elements.*/
} _GCO_OD_HDR;


# define OD_INST( len )			\
static struct				\
{					\
    _GCO_OD_HDR		desc;		\
    struct 				\
    {					\
	GCA_TPL		tpl;		\
	_GCO_OD_HDR	*od_ptr;	\
	i4		arr_stat;	\
    } ela[ len ];			\
}


/*
** Definitions of Message objects
*/

OD_INST(1) od_empty_msg =
{ "GCA_EMPTY_DESC", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARIMPAR },	/* whole message */
    }
};

OD_INST(2) od_name = 
{ "GCA_NAME",		GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_name */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR }	/* gca_name */
    }
};

OD_INST(2) od_id = 
{ "GCA_ID",		GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, 2 },			/* gca_index */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_MAXNAME }		/* gca_name */
    }
};

OD_INST(4) od_data_value =
{ "GCA_DATA_VALUE", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_type */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_precision */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_value */
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VAREXPAR }	/* gca_value */
    }
};

OD_INST(4) od_dbms_value =
{ "GCA_DBMS_VALUE", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* db_data pointer */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* db_length */
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },	/* db_datatype */
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY }		/* db_precision */
    }
};

OD_INST(3) od_col_att = 
{ "GCA_COL_ATT", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_dbms_value.desc, GCA_NOTARRAY },	/* gca_attdbv */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_attname */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR }	/* gca_attname */
    }
};

OD_INST(12) od_cprdd = 
{ "GCA_CPRDD",		GCA_IGNPRCLEN, 12,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_MAXNAME },	/* gca_domname_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_type_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_length_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_user_delim_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_delim_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, 2 },			/* gca_delim_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_tupmap_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_cvid_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_cvlen_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_withnull_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_nullen_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR }	/* gca_nuldata_cp */
    }
};

OD_INST(14) od1_cprdd = 
{ "GCA1_CPRDD",		GCA_IGNPRCLEN, 14,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_MAXNAME },	/* gca_domname_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_type_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_precision_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_length_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_user_delim_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_delim_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, 2 },			/* gca_delim_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_tupmap_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_cvid_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_cvprec_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_cvlen_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_withnull_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_nullvalue_cp */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VAREXPAR }	/* gca_nullvalue_cp */
    }
};

OD_INST(7) od_e_element = 
{ "GCA_E_ELEMENT", GCA_IGNPRCLEN, 7,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_id_error */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_id_server */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_server_type */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_severity */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_local_error */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_error_parm */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VAREXPAR }	/* gca_error_parm */
    }
};

OD_INST(3) od_p_param = 
{ "GCA_P_PARAM", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_parname */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_parm_mask */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_NOTARRAY }	/* gca_parvalue */
    }
};

OD_INST(5) od_tup_descr = 
{ "GCA_TUP_DESCR", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_tsize */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_result_modifier*/
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_id_tdscr */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_col_desc */
    { { 0, 0, 0 }, &od_col_att.desc, GCA_VAREXPAR }	/* gca_col_desc */
    }
};

OD_INST(6) od_xa_dis_tran_id =
{ "GCA_XA_DIS_TRAN_ID", GCA_IGNPRCLEN, 6,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* formatID */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gtrid_length */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* bqual_length */
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_XA_XID_MAX },	/* data */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* branch_seqnum */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* branch_flag */
    }
};

OD_INST(2) od_user_data = 
{ "GCA_USER_DATA", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_p_index */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_NOTARRAY }	/* gca_p_value */
    }
};

OD_INST(1) od_ak_data =
{ "GCA_AK_DATA", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* gca_ak_data */
    }
};

OD_INST(1) od_at_data = 
{ "GCA_AT_DATA", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* gca_itype */
    }
};

OD_INST(10) od_cp_map = 
{ "GCA_CP_MAP",		GCA_IGNPRCLEN, 10,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_status_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_direction_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_maxerrs_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_fname_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* gca_fname_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_logname_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* gca_logname_cp */
    { { 0, 0, 0 }, &od_tup_descr.desc, GCA_NOTARRAY },	/* gca_tup_desc_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_row_desc_cp */
    { { 0, 0, 0 }, &od_cprdd.desc, GCA_VARIMPAR }	/* gca_row_desc_cp */
    }
};

OD_INST(10) od1_cp_map = 
{ "GCA1_CP_MAP",	GCA_IGNPRCLEN, 10,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_status_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_direction_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_maxerrs_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_fname_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* gca_fname_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_logname_cp */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* gca_logname_cp */
    { { 0, 0, 0 }, &od_tup_descr.desc, GCA_NOTARRAY },	/* gca_tup_desc_cp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_row_desc_cp */
    { { 0, 0, 0 }, &od1_cprdd.desc, GCA_VARIMPAR }	/* gca_row_desc_cp */
    }
};

OD_INST(2) od_dl_data = 
{ "GCA_DL_DATA", GCA_IGNPRCLEN, 2,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_cursor_id */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY }	/* gca_table_name */
    }
};

OD_INST(3) od_dl1_data = 
{ "GCA1_DL_DATA", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_cursor_id */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_owner_name */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY }	/* gca_table_name */
    }
};

OD_INST(2) od_er_data = 
{ "GCA_ER_DATA", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_e_element */
    { { 0, 0, 0 }, &od_e_element.desc, GCA_VARIMPAR }	/* gca_e_element */
    }
};

OD_INST(6) od_ev_data =
{ "GCA_EV_DATA", GCA_IGNPRCLEN, 6,
    {
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_evname */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_evowner */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_evdb */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_NOTARRAY },	/* gca_evtime */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_evvalue */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VARIMPAR },	/* gca_evvalue */
    }
};

OD_INST(2) od1_ft_data =
{ "GCA1_FT_DATA", GCA_IGNPRCLEN, 2,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_cursor_id */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* gca_rowcount */
    }
};

OD_INST(4) od2_ft_data =
{ "GCA2_FT_DATA", GCA_IGNPRCLEN, 4,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_cursor_id */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_rowcount */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_anchor */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* gca_offset */
    }
};

OD_INST(3) od_ip_data = 
{ "GCA_IP_DATA", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_id_proc */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_proc_mask */
    { { 0, 0, 0 }, &od_p_param.desc, GCA_VARIMPAR }	/* gca_param */
    }
};

OD_INST(4) od1_ip_data = 
{ "GCA1_IP_DATA", GCA_IGNPRCLEN, 4,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_id_proc */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_proc_own */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_proc_mask */
    { { 0, 0, 0 }, &od_p_param.desc, GCA_VARIMPAR }	/* gca_param */
    }
};

OD_INST(4) od2_ip_data = 
{ "GCA2_IP_DATA", GCA_IGNPRCLEN, 4,
    {
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_proc_name */
    { { 0, 0, 0 }, &od_name.desc, GCA_NOTARRAY },	/* gca_proc_own */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_proc_mask */
    { { 0, 0, 0 }, &od_p_param.desc, GCA_VARIMPAR }	/* gca_param */
    }
};

OD_INST(2) od_iv_data = 
{ "GCA_IV_DATA", GCA_IGNPRCLEN, 2,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_qid */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VARIMPAR }	/* gca_qparm */
    }
}; 

OD_INST(5) od_prompt_data =
{ "GCA_PROMPT_DATA", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_prflags */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_prtimeout */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_prmaxlen */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_prmesg */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VAREXPAR },	/* gca_prmesg */
    }
};

OD_INST(3) od_prreply_data =
{ "GCA_PRREPLY_DATA", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_prflags */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_prvalue */
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VAREXPAR },	/* gca_prvalue */
    }
};

OD_INST(3) od_q_data = 
{ "GCA_Q_DATA", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_language_id */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_query_modifier*/
    { { 0, 0, 0 }, &od_data_value.desc, GCA_VARIMPAR }	/* gca_qdata */
    }
};

OD_INST(11) od_re_data = 
{ "GCA_RE_DATA", GCA_IGNPRCLEN, 11,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_rid */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_rqstatus */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_rowcount */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_rhistamp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_rlostamp */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_cost */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_errd0 */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_errd1 */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_errd4 */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_errd5 */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, 16 },			/* gca_logkey */
    }
};

OD_INST(2) od_rp_data = 
{ "GCA_RP_DATA", GCA_IGNPRCLEN, 2,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_id_proc */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* gca_retstat */
    }
};

OD_INST(2) od_session_parms = 
{ "GCA_SESSION_PARMS", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_user_data */
    { { 0, 0, 0 }, &od_user_data.desc, GCA_VARIMPAR }	/* gca_user_data */
    }
};

OD_INST(5) od_td_data = 
{ "GCA_TD_DATA", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_tsize */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_result_modifier*/
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_id_tdscr */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_l_col_desc */
    { { 0, 0, 0 }, &od_col_att.desc, GCA_VARIMPAR }	/* gca_col_desc */
    }
};

OD_INST(1) od_tr_data = 
{ "GCA_TR_DATA", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARIMPAR }	/* gca_tr_data */
    }
};

OD_INST(1) od_tu_data = 
{ "GCA_TU_DATA", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARIMPAR }	/* gca_tu_data */
    }
};

OD_INST(2) od_tx_data = 
{ "GCA_TX_DATA", GCA_IGNPRCLEN, 2,
    {
    { { 0, 0, 0 }, &od_id.desc, GCA_NOTARRAY },		/* gca_tx_id */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* gca_tx_type */
    }
};

OD_INST(4) od_xa_tx_data =
{ "GCA_XA_TX_DATA", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_xa_type */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_xa_precision */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_xa_l_value */
    { { 0, 0, 0 }, &od_xa_dis_tran_id.desc, GCA_NOTARRAY } /* gca_xa_id */
    }
};

OD_INST(2) od_xa_data =
{ "GCA_XA_DATA", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gca_xa_flags */
    { { 0, 0, 0 }, &od_xa_dis_tran_id.desc, GCA_NOTARRAY } /* gca_xa_xid */
    }
};

OD_INST(3) od_gca_tpl =
{ "GCA_TPL", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },	/* type */
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },	/* precision */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* length */
    }
};

OD_INST(3) od_gca_element =
{ "GCA_ELEMENT", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_gca_tpl.desc, GCA_NOTARRAY },	/* tpl */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* od_ptr */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }		/* arr_stat */
    }
};

OD_INST(4) od_obj_desc =
{ "GCA_OBJECT_DESC", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_OBJ_NAME_LEN },	/* data_object_name */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* flags */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* el_count */
    { { 0, 0, 0 }, &od_gca_element.desc, GCA_VAREXPAR }	/* ela */
    }
};

OD_INST(2) od_gcn_val = 
{ "GCN_VAL", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_l_item */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR }	/* gcn_value */
    }
};

OD_INST(4) od_gcn_req_tuple =
{ "GCN_REQ_TUPLE", GCA_IGNPRCLEN, 4,
    {
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_type */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_uid */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_obj */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY }	/* gcn_val */
    }
};

OD_INST(5) od_gcn_d_oper =
{ "GCN_D_OPER", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_flag */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_opcode */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_install */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_tup_cnt */
    { { 0, 0, 0 }, &od_gcn_req_tuple.desc, GCA_VARIMPAR } /* gcn_tuple */
    }
};

OD_INST(3) od_gcn_oper_data =
{ "GCN_OPER_DATA", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_op */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_tup_cnt */
    { { 0, 0, 0 }, &od_gcn_req_tuple.desc, GCA_VARIMPAR } /* gcn_tuple */
    }
};

OD_INST(3) od_gcn_lcl_addr =
{ "GCN_LCL_ADDR", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_host */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_addr */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY }	/* gcn_auth */
    }
};

OD_INST(3) od_gcn_rmt_addr =
{ "GCN_RMT_ADDR", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_node */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_protocol */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY }	/* gcn_port */
    }
};

OD_INST(2) od_gcn_rmt_data =
{ "GCN_RMT_DATA", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_data_type */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_data_value */
    }
};

OD_INST(5) od_gcn_d_ns_resolve =
{ "GCN_D_NS_RESOLVE", GCA_IGNPRCLEN, 5,
    {
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_install */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_uid */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_ext_name */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_user */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY }	/* gcn_passwd */
    }
};

OD_INST(7) od_gcn_d_ns_2_resolve =
{ "GCN_D_NS_2_RESOLVE", GCA_IGNPRCLEN, 7,
    {
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_install */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_uid */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_ext_name */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_user */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_passwd */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_rem_username */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY }	/* gcn_rem_passwd */
    }
};

OD_INST(8) od_gcn1_d_resolved =
{ "GCN1_D_RESOLVED", GCA_IGNPRCLEN, 8,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_rmt_count */
    { { 0, 0, 0 }, &od_gcn_rmt_addr.desc, GCA_VAREXPAR},/* gcn_rmt_addr */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_username */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_password */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_rmt_dbname */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_server_class */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_lcl_count */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_VAREXPAR }	/* gcn_lcl_addr */
    }
};

OD_INST(7) od_gcn2_d_resolved = 
{ "GCN2_D_RESOLVED", GCA_IGNPRCLEN, 7,
    {
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_username */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_l_local */
    { { 0, 0, 0 }, &od_gcn_lcl_addr.desc, GCA_VAREXPAR},/* gcn_local */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_l_remote */
    { { 0, 0, 0 }, &od_gcn_rmt_addr.desc, GCA_VAREXPAR },/* gcn_remote */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_l_data */
    { { 0, 0, 0 }, &od_gcn_rmt_data.desc, GCA_VAREXPAR }/* gcn_data */
    }
};

OD_INST(5) od_gcn_d_ns_authorize =
{ "GCN_D_NS_AUTHORIZE", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_auth_type */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_username */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_client_realm */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_NOTARRAY },	/* gcn_server_realm */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }	/* gcn_rq_count */
    }
};

OD_INST(3) od_gcn_d_authorized =
{ "GCN_D_AUTHORIZED", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_auth_type */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* gcn_l_tickets */
    { { 0, 0, 0 }, &od_gcn_val.desc, GCA_VARIMPAR }	/* gcn_tickets */
    }
};

OD_INST(4) od_gcm_identifier =
{ "GCM_VAR_IDENTIFIER", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* l_object_class */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* object_class */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* l_object_instance */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* object_instance */
    }
};

OD_INST(2) od_gcm_value =
{ "GCM_VAR_BINDING", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* l_object_value */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* object_value */
    }
};

OD_INST(3) od_gcm_var =
{ "GCM_VAR_BINDING", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_gcm_identifier.desc, GCA_NOTARRAY }, /* var_name */
    { { 0, 0, 0 }, &od_gcm_value.desc, GCA_NOTARRAY },	/* var_value */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* var_perms */
    }
};

OD_INST(6) od_gcm_msg_hdr =
{ "GCM_MSG_HDR", GCA_IGNPRCLEN, 6,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* error_status */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* error_index */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* future 1 */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* future 2 */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* client_perms */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* row_count */
    }
};

OD_INST(5) od_gcm_trap_hdr =
{ "GCM_TRAP_HDR", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* trap_reason */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* trap_handle */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* mon_handle */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* l_trap_address */
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VAREXPAR },	/* trap_address */
    }
};

OD_INST(3) od_gcm_query_msg =
{ "GCM_QUERY_MSG", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_gcm_msg_hdr.desc, GCA_NOTARRAY },  /* gcm_msg_hdr */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* element_count */
    { { 0, 0, 0 }, &od_gcm_var.desc, GCA_VARIMPAR }	/* gcm_var_binding */
    }
};

OD_INST(4) od_gcm_trap_msg =
{ "GCM_TRAP_MSG", GCA_IGNPRCLEN, 4,
    {
    { { 0, 0, 0 }, &od_gcm_msg_hdr.desc, GCA_NOTARRAY },  /* gcm_msg_hdr */
    { { 0, 0, 0 }, &od_gcm_trap_hdr.desc, GCA_NOTARRAY }, /* gcm_trap_hdr */
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },	/* element_count */
    { { 0, 0, 0 }, &od_gcm_var.desc, GCA_VARIMPAR }	/* gcm_var_binding */
    }
};


/*
** Global references to local structures.
*/
 
GLOBALDEF i4     gco_trace_level=0; 
GLOBALDEF GCA_OBJECT_DESC *gco_od_data_value= (GCA_OBJECT_DESC *)&od_data_value;


/* 
** Array used to perform MESSAGE type to Object Descriptor mapping. 
*/

GLOBALDEF GCA_OBJECT_DESC *gco_msg_ods[] =
{
    0,						/* 0x00 Internal */
    0,						/* 0x01 Internal */
    (GCA_OBJECT_DESC *) &od_session_parms,	/* GCA_MD_ASSOC */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_ACCEPT */
    (GCA_OBJECT_DESC *) &od_er_data,		/* GCA_REJECT */
    (GCA_OBJECT_DESC *) &od_er_data,		/* GCA_RELEASE */
    (GCA_OBJECT_DESC *) &od_tx_data,		/* GCA_Q_BTRAN */
    (GCA_OBJECT_DESC *) &od_re_data,		/* GCA_S_BTRAN */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_ABORT */
    (GCA_OBJECT_DESC *) &od_tx_data,		/* GCA_SECURE */
    (GCA_OBJECT_DESC *) &od_re_data,		/* GCA_DONE */
    (GCA_OBJECT_DESC *) &od_re_data,		/* GCA_REFUSE */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_COMMIT */
    (GCA_OBJECT_DESC *) &od_q_data,		/* GCA_QUERY */
    (GCA_OBJECT_DESC *) &od_q_data,		/* GCA_DEFINE */
    (GCA_OBJECT_DESC *) &od_iv_data,		/* GCA_INVOKE */
    (GCA_OBJECT_DESC *) &od_id,			/* GCA_FETCH */
    (GCA_OBJECT_DESC *) &od_dl_data,		/* GCA_DELETE */
    (GCA_OBJECT_DESC *) &od_id,			/* GCA_CLOSE */
    (GCA_OBJECT_DESC *) &od_at_data,		/* GCA_ATTENTION */
    (GCA_OBJECT_DESC *) &od_id,			/* GCA_QC_ID */
    (GCA_OBJECT_DESC *) &od_td_data,		/* GCA_TDESCR */
    (GCA_OBJECT_DESC *) &od_tu_data,		/* GCA_TUPLES */
    (GCA_OBJECT_DESC *) &od_cp_map,		/* GCA_C_INTO */
    (GCA_OBJECT_DESC *) &od_cp_map,		/* GCA_C_FROM */
    (GCA_OBJECT_DESC *) &od_tu_data,		/* GCA_CDATA */
    (GCA_OBJECT_DESC *) &od_ak_data,		/* GCA_ACK */
    (GCA_OBJECT_DESC *) &od_re_data,		/* GCA_RESPONSE */
    (GCA_OBJECT_DESC *) &od_er_data,		/* GCA_ERROR */
    (GCA_OBJECT_DESC *) &od_tr_data,		/* GCA_TRACE */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_Q_ETRAN */
    (GCA_OBJECT_DESC *) &od_re_data,		/* GCA_S_ETRAN */
    (GCA_OBJECT_DESC *) &od_ak_data,		/* GCA_IACK */
    (GCA_OBJECT_DESC *) &od_at_data,		/* GCA_NP_INTERRUPT */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_ROLLBACK */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_Q_STATUS */
    (GCA_OBJECT_DESC *) &od_q_data,		/* GCA_CREPROC */
    (GCA_OBJECT_DESC *) &od_q_data,		/* GCA_DRPPROC */
    (GCA_OBJECT_DESC *) &od_ip_data,		/* GCA_INVPROC */
    (GCA_OBJECT_DESC *) &od_rp_data,		/* GCA_RETPROC */
    (GCA_OBJECT_DESC *)	&od_ev_data,		/* GCA_EVENT */
    (GCA_OBJECT_DESC *) &od1_cp_map,		/* GCA1_C_INTO */
    (GCA_OBJECT_DESC *) &od1_cp_map,		/* GCA1_C_FROM */
    (GCA_OBJECT_DESC *) &od_dl1_data,		/* GCA1_DELETE */
    (GCA_OBJECT_DESC *) &od_xa_tx_data,		/* GCA_XA_SECURE */ 
    (GCA_OBJECT_DESC *) &od1_ip_data,		/* GCA1_INVPROC */ 
    (GCA_OBJECT_DESC *) &od_prompt_data,	/* GCA_PROMPT */ 
    (GCA_OBJECT_DESC *) &od_prreply_data,	/* GCA_PRREPLY */ 
    (GCA_OBJECT_DESC *) &od1_ft_data,		/* GCA1_FETCH */
    (GCA_OBJECT_DESC *) &od2_ft_data,		/* GCA2_FETCH */
    (GCA_OBJECT_DESC *) &od2_ip_data,		/* GCA2_INVPROC */

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* Up to 0x40 */

    (GCA_OBJECT_DESC *) &od_gcn_d_ns_resolve,	/* GCN_NS_RESOLVE */
    (GCA_OBJECT_DESC *) &od_gcn1_d_resolved,	/* GCN_RESOLVED */
    (GCA_OBJECT_DESC *) &od_gcn_d_oper,		/* GCN_NS_OPER */
    (GCA_OBJECT_DESC *) &od_gcn_oper_data,	/* GCN_RESULT */
    (GCA_OBJECT_DESC *) &od_gcn_d_ns_authorize,	/* GCN_NS_AUTHORIZE */
    (GCA_OBJECT_DESC *) &od_gcn_d_authorized,	/* GCN_AUTHORIZED */
    (GCA_OBJECT_DESC *) &od_gcn_d_ns_2_resolve,	/* GCN_NS_2_RESOLVE */
    (GCA_OBJECT_DESC *) &od_gcn2_d_resolved,	/* GCN2_RESOLVED */

    0, 0, 0, 0, 0, 0, 0, 0, 			/* Up to 0x50 */

    (GCA_OBJECT_DESC *) &od_td_data,		/* GCA_IT_DESCR (0x50) */
    (GCA_OBJECT_DESC *) &od_obj_desc,		/* GCA_TO_DESCR (0x51) */
    (GCA_OBJECT_DESC *) &od_empty_msg,		/* GCA_GOTOHET (0x52) */

    0, 0, 0, 0, 0, 0, 0, 			/* Up to 0x5A */

    (GCA_OBJECT_DESC *) &od_xa_data, 		/* GCA_XA_START (0x5A) */
    (GCA_OBJECT_DESC *) &od_xa_data,		/* GCA_XA_END (0x5B) */
    (GCA_OBJECT_DESC *) &od_xa_data, 		/* GCA_XA_PREPARE (0x5C) */
    (GCA_OBJECT_DESC *) &od_xa_data, 		/* GCA_XA_COMMIT (0x5D) */
    (GCA_OBJECT_DESC *) &od_xa_data,		/* GCA_XA_ROLLBACK (0x5E) */

    0,						/* Up to 0x60 */

    (GCA_OBJECT_DESC *) &od_gcm_query_msg,	/* GCM_GET (0x60) */
    (GCA_OBJECT_DESC *) &od_gcm_query_msg,	/* GCM_GETNEXT (0x61) */
    (GCA_OBJECT_DESC *) &od_gcm_query_msg,	/* GCM_SET (0x62) */
    (GCA_OBJECT_DESC *) &od_gcm_query_msg,	/* GCM_RESPONSE (0x63) */
    (GCA_OBJECT_DESC *) &od_gcm_trap_msg,	/* GCM_SET_TRAP (0x64) */
    (GCA_OBJECT_DESC *) &od_gcm_trap_msg,	/* GCM_TRAP_IND (0x65) */
    (GCA_OBJECT_DESC *) &od_gcm_trap_msg 	/* GCM_UNSET_TRAP (0x66) */
};

GLOBALDEF i4	    gco_l_msg_ods = sizeof(gco_msg_ods) / sizeof(*gco_msg_ods);
GLOBALDEF GCO_OP    **gco_msg_map = NULL;

/*
** Compiled program for tuples from od_tu_data.
**
** The message compiler produces a CALL_TUP loop
** for tuple messages (GCA_TUPLES, GCA_CDATA)
** which then calls the appropriate program for
** the current tuples.  A gco client can compile
** a program for a tuple set from a GCA_TDESCR
** message to be used in the CALL_TUP loop.  If
** such a program is not provided, the generic
** program provided below will be used instead.
*/

GLOBALDEF GCO_OP        *gco_tuple_prog = NULL;


/* 
** Definition of DBMS datatypes 
*/

OD_INST(1) od_int1 =
{ "i1", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_int1_n =
{ "nullable i1", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_int2 =
{ "i2", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_int2_n =
{ "nullable i2", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_int4 =
{ "i4", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_int4_n =
{ "nullable i4", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_int8 =
{ "i8", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 8 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_int8_n =
{ "nullable i8", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 8 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_flt4 =
{ "f4", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_FLOAT, 0, 4 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_flt4_n =
{ "nullable f4", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_FLOAT, 0, 4 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_flt8 =
{ "f8", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_FLOAT, 0, 8 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_flt8_n =
{ "nullable f8", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_FLOAT, 0, 8 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_dec =
{ "decimal", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_dec_n =
{ "nullable decimal", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_money =
{ "money", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_FLOAT, 0, 8 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_money_n =
{ "nullable money", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_FLOAT, 0, 8 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_bool =
{ "boolean", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_bool_n =
{ "nullable boolean", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_c =
{ "c", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_c_n =
{ "nullable c", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_char =
{ "char", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_char_n =
{ "nullable char", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_vchr =
{ "varchar", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARPADAR },
    }
};

OD_INST(3) od_vchr_n =
{ "nullable varchar", GCA_IGNPRCSN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARPADAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_vchr_c =
{ "compressed varchar", GCA_IGNPRCSN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARVCHAR },
    }
};

OD_INST(3) od_vchr_cn =
{ "nullable compressed varchar", GCA_IGNPRCSN | GCA_VARLENGTH, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARVCHAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_blob_header =
{ "blob header", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 4 }, 0, 2 }
    }
};

OD_INST(2) od_lchr_segment =
{ "long varchar segment", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARSEGAR }
    }
};

OD_INST(2) od_lchr_list =
{ "long varchar segment list", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_lchr_segment.desc, GCA_NOTARRAY },
    }
};

OD_INST(2) od_lchr =
{ "long varchar", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_lchr_list.desc, GCA_VARLSTAR }
    }
};

OD_INST(3) od_lchr_n =
{ "nullable long varchar", GCA_IGNPRCLEN | GCA_VARLENGTH, 3,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_lchr_list.desc, GCA_VARLSTAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_loc =
{ "long locator", GCA_IGNPRCLEN, 2,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(3) od_loc_n =
{ "nullable long locator", GCA_IGNPRCLEN, 3,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_byte =
{ "byte", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_byte_n =
{ "nullable byte", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_vbyt =
{ "varbyte", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARPADAR },
    }
};

OD_INST(3) od_vbyt_n =
{ "nullable varbyte", GCA_IGNPRCSN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARPADAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_vbyt_c =
{ "compressed varbyte", GCA_IGNPRCSN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARVCHAR },
    }
};

OD_INST(3) od_vbyt_cn =
{ "nullable compressed varbyte", GCA_IGNPRCSN | GCA_VARLENGTH, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARVCHAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_lbyt_segment =
{ "long byte segment", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARSEGAR }
    }
} ;

OD_INST(2) od_lbyt_list =
{ "long byte segment list", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_lbyt_segment.desc, GCA_NOTARRAY },
    }
} ;

OD_INST(2) od_lbyt =
{ "long byte", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_lbyt_list.desc, GCA_VARLSTAR }
    }
};

OD_INST(3) od_lbyt_n =
{ "nullable long byte", GCA_IGNPRCLEN | GCA_VARLENGTH, 3,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_lbyt_list.desc, GCA_VARLSTAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_text =
{ "text", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARPADAR },
    }
};

OD_INST(3) od_text_n =
{ "nullable text", GCA_IGNPRCSN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARPADAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_text_c =
{ "compressed text", GCA_IGNPRCSN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARVCHAR },
    }
};

OD_INST(3) od_text_cn =
{ "nullable compressed text", GCA_IGNPRCSN | GCA_VARLENGTH, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARVCHAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_ltxt =
{ "long text", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARPADAR },
    }
};

OD_INST(3) od_ltxt_n =
{ "nullable long text", GCA_IGNPRCSN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARPADAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_ltxt_c =
{ "compressed long text", GCA_IGNPRCSN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARVCHAR },
    }
};

OD_INST(3) od_ltxt_cn =
{ "nullable compressed long text", GCA_IGNPRCSN | GCA_VARLENGTH, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARVCHAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_qtxt =
{ "query text", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_qtxt_n =
{ "nullable query text", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_CHAR, 0, 1 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_nchar =
{ "nchar", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_nchar_n =
{ "nullable nchar", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_nvchr =
{ "nvarchar", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARPADAR },
    }
};

OD_INST(3) od_nvchr_n =
{ "nullable nvarchar", GCA_IGNPRCSN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARPADAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_nvchr_c =
{ "compressed nvarchar", GCA_IGNPRCSN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARVCHAR },
    }
};

OD_INST(3) od_nvchr_cn =
{ "nullable compressed nvarchar", GCA_IGNPRCSN | GCA_VARLENGTH, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARVCHAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_nlchr_segment =
{ "long nvarchar segment", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARSEGAR }
    }
};

OD_INST(2) od_nlchr_list =
{ "long nvarchar segment list", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_nlchr_segment.desc, GCA_NOTARRAY },
    }
};

OD_INST(2) od_nlchr =
{ "long nvarchar", GCA_IGNPRCLEN | GCA_VARLENGTH, 2,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_nlchr_list.desc, GCA_VARLSTAR }
    }
};

OD_INST(3) od_nlchr_n =
{ "nullable long nvarchar", GCA_IGNPRCLEN | GCA_VARLENGTH, 3,
    {
    { { 0, 0, 0 }, &od_blob_header.desc, GCA_NOTARRAY },
    { { 0, 0, 0 }, &od_nlchr_list.desc, GCA_VARLSTAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_nqtxt =
{ "UCS-2 query text", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_nqtxt_n =
{ "nullable UCS-2 query text", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(3) od_idate =
{ "ingres date", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 2 }, 0, 3 },
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(4) od_idate_n =
{ "nullable ingres date", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 2 }, 0, 3 },
    { { GCO_ATOM_INT, 0, 4 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_adate =
{ "ansi date", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    }
};

OD_INST(3) od_adate_n =
{ "nullable ansi date", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_time =
{ "time", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    }
};

OD_INST(3) od_time_n =
{ "nullable time", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(4) od_ts =
{ "timestamp", GCA_IGNPRCLEN, 4,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 4 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    }
};

OD_INST(5) od_ts_n =
{ "nullable timestamp", GCA_IGNPRCLEN, 5,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 4 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, 2 },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(2) od_intym =
{ "interval y/m", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY },
    }
};

OD_INST(3) od_intym_n =
{ "nullable interval y/m", GCA_IGNPRCLEN, 3,
    {
    { { GCO_ATOM_INT, 0, 2 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_intds =
{ "interval d/s", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, 3 },
    }
};

OD_INST(2) od_intds_n =
{ "nullable interval d/s", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_INT, 0, 4 }, 0, 3 },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_lkey =
{ "logical key", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, DB_LEN_OBJ_LOGKEY }
    }
};

OD_INST(2) od_lkey_n =
{ "nullable logical key", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, DB_LEN_OBJ_LOGKEY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_tkey =
{ "table key", GCA_IGNPRCLEN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, DB_LEN_TAB_LOGKEY }
    }
};

OD_INST(2) od_tkey_n =
{ "nullable table key", GCA_IGNPRCLEN, 2,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, DB_LEN_TAB_LOGKEY },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};

OD_INST(1) od_secl =
{ "security label", GCA_IGNPRCSN, 1,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARTPLAR }
    }
};

OD_INST(2) od_secl_n =
{ "nullable security label", GCA_IGNPRCSN, 2,
    {
    { { GCO_ATOM_BYTE, 0, 1 }, 0, GCA_VARTPLAR },
    { { GCO_ATOM_INT, 0, 1 }, 0, GCA_NOTARRAY }
    }
};


/* 
** Array used to perform DBMS datatype to Object Descriptor mapping. 
*/

GLOBALDEF GCA_OBJECT_DESC *gco_ddt_ods[] =
{
    /* Numeric types */

    (GCA_OBJECT_DESC *)&od_int1,	/* GCO_DT_INT1    */
    (GCA_OBJECT_DESC *)&od_int1_n,	/* GCO_DT_INT1_N  */
    (GCA_OBJECT_DESC *)&od_int2,	/* GCO_DT_INT2    */
    (GCA_OBJECT_DESC *)&od_int2_n,	/* GCO_DT_INT2_N  */
    (GCA_OBJECT_DESC *)&od_int4,	/* GCO_DT_INT4    */
    (GCA_OBJECT_DESC *)&od_int4_n,	/* GCO_DT_INT4_N  */
    (GCA_OBJECT_DESC *)&od_int8,	/* GCO_DT_INT8    */
    (GCA_OBJECT_DESC *)&od_int8_n,	/* GCO_DT_INT8_N  */
    (GCA_OBJECT_DESC *)&od_flt4,	/* GCO_DT_FLT4    */
    (GCA_OBJECT_DESC *)&od_flt4_n,	/* GCO_DT_FLT4_N  */
    (GCA_OBJECT_DESC *)&od_flt8,	/* GCO_DT_FLT8    */
    (GCA_OBJECT_DESC *)&od_flt8_n,	/* GCO_DT_FLT8_N  */
    (GCA_OBJECT_DESC *)&od_dec,		/* GCO_DT_DEC     */
    (GCA_OBJECT_DESC *)&od_dec_n,	/* GCO_DT_DEC_N   */
    (GCA_OBJECT_DESC *)&od_money,	/* GCO_DT_MONEY   */
    (GCA_OBJECT_DESC *)&od_money_n,	/* GCO_DT_MONEY_N */

    /* String types */

    (GCA_OBJECT_DESC *)&od_c,		/* GCO_DT_C       */
    (GCA_OBJECT_DESC *)&od_c_n,		/* GCO_DT_C_N     */
    (GCA_OBJECT_DESC *)&od_char,	/* GCO_DT_CHAR    */
    (GCA_OBJECT_DESC *)&od_char_n,	/* GCO_DT_CHAR_N  */
    (GCA_OBJECT_DESC *)&od_vchr,	/* GCO_DT_VCHR    */
    (GCA_OBJECT_DESC *)&od_vchr_n,	/* GCO_DT_VCHR_N  */
    (GCA_OBJECT_DESC *)&od_vchr_c,	/* GCO_DT_VCHR_C  */
    (GCA_OBJECT_DESC *)&od_vchr_cn,	/* GCO_DT_VCHR_CN */
    (GCA_OBJECT_DESC *)&od_lchr,	/* GCO_DT_LCHR    */
    (GCA_OBJECT_DESC *)&od_lchr_n,	/* GCO_DT_LCHR_N  */
    (GCA_OBJECT_DESC *)&od_loc,		/* GCO_DT_LCLOC   */
    (GCA_OBJECT_DESC *)&od_loc_n,	/* GCO_DT_LCLOC_N */
    (GCA_OBJECT_DESC *)&od_byte,	/* GCO_DT_BYTE    */
    (GCA_OBJECT_DESC *)&od_byte_n,	/* GCO_DT_BYTE_N  */
    (GCA_OBJECT_DESC *)&od_vbyt,	/* GCO_DT_VBYT    */
    (GCA_OBJECT_DESC *)&od_vbyt_n,	/* GCO_DT_VBYT_N  */
    (GCA_OBJECT_DESC *)&od_vbyt_c,	/* GCO_DT_VBYT_C  */
    (GCA_OBJECT_DESC *)&od_vbyt_cn,	/* GCO_DT_VBYT_CN */
    (GCA_OBJECT_DESC *)&od_lbyt,	/* GCO_DT_LBYT    */
    (GCA_OBJECT_DESC *)&od_lbyt_n,	/* GCO_DT_LBYT_N  */
    (GCA_OBJECT_DESC *)&od_loc,		/* GCO_DT_LBLOC   */
    (GCA_OBJECT_DESC *)&od_loc_n,	/* GCO_DT_LBLOC_N */
    (GCA_OBJECT_DESC *)&od_text,	/* GCO_DT_TEXT    */
    (GCA_OBJECT_DESC *)&od_text_n,	/* GCO_DT_TEXT_N  */
    (GCA_OBJECT_DESC *)&od_text_c,	/* GCO_DT_TEXT_C  */
    (GCA_OBJECT_DESC *)&od_text_cn,	/* GCO_DT_TEXT_CN */
    (GCA_OBJECT_DESC *)&od_ltxt,	/* GCO_DT_LTXT    */
    (GCA_OBJECT_DESC *)&od_ltxt_n,	/* GCO_DT_LTXT_N  */
    (GCA_OBJECT_DESC *)&od_ltxt_c,	/* GCO_DT_LTXT_C  */
    (GCA_OBJECT_DESC *)&od_ltxt_cn,	/* GCO_DT_LTXT_CN */
    (GCA_OBJECT_DESC *)&od_qtxt,	/* GCO_DT_QTXT    */
    (GCA_OBJECT_DESC *)&od_qtxt_n,	/* GCO_DT_QTXT_N  */
    (GCA_OBJECT_DESC *)&od_nchar,	/* GCO_DT_NCHAR   */
    (GCA_OBJECT_DESC *)&od_nchar_n,	/* GCO_DT_NCHAR_N */
    (GCA_OBJECT_DESC *)&od_nvchr,	/* GCO_DT_NVCHR   */
    (GCA_OBJECT_DESC *)&od_nvchr_n,	/* GCO_DT_NVCHR_N */
    (GCA_OBJECT_DESC *)&od_nvchr_c,	/* GCO_DT_NVCHR_C */
    (GCA_OBJECT_DESC *)&od_nvchr_cn,	/* GCO_DT_NVCHR_CN */
    (GCA_OBJECT_DESC *)&od_nlchr,	/* GCO_DT_NLCHR   */
    (GCA_OBJECT_DESC *)&od_nlchr_n,	/* GCO_DT_NLCHR_N */
    (GCA_OBJECT_DESC *)&od_loc,		/* GCO_DT_LNLOC   */
    (GCA_OBJECT_DESC *)&od_loc_n,	/* GCO_DT_LNLOC_N */
    (GCA_OBJECT_DESC *)&od_nqtxt,	/* GCO_DT_NQTXT   */
    (GCA_OBJECT_DESC *)&od_nqtxt_n,	/* GCO_DT_NQTXT_N */

    /* Abstract types */

    (GCA_OBJECT_DESC *)&od_idate,	/* GCO_DT_IDATE   */
    (GCA_OBJECT_DESC *)&od_idate_n,	/* GCO_DT_IDATE_N */
    (GCA_OBJECT_DESC *)&od_adate,	/* GCO_DT_ADATE   */
    (GCA_OBJECT_DESC *)&od_adate_n,	/* GCO_DT_ADATE_N */
    (GCA_OBJECT_DESC *)&od_time,	/* GCO_DT_TIME    */
    (GCA_OBJECT_DESC *)&od_time_n,	/* GCO_DT_TIME_N  */
    (GCA_OBJECT_DESC *)&od_time,	/* GCO_DT_TMWO    */
    (GCA_OBJECT_DESC *)&od_time_n,	/* GCO_DT_TMWO_N  */
    (GCA_OBJECT_DESC *)&od_time,	/* GCO_DT_TMTZ    */
    (GCA_OBJECT_DESC *)&od_time_n,	/* GCO_DT_TMTZ_N  */
    (GCA_OBJECT_DESC *)&od_ts,		/* GCO_DT_TS      */
    (GCA_OBJECT_DESC *)&od_ts_n,	/* GCO_DT_TS_N    */
    (GCA_OBJECT_DESC *)&od_ts,		/* GCO_DT_TSWO    */
    (GCA_OBJECT_DESC *)&od_ts_n,	/* GCO_DT_TSWO_N  */
    (GCA_OBJECT_DESC *)&od_ts,		/* GCO_DT_TSTZ    */
    (GCA_OBJECT_DESC *)&od_ts_n,	/* GCO_DT_TSTZ_N  */
    (GCA_OBJECT_DESC *)&od_intym,	/* GCO_DT_INTYM   */
    (GCA_OBJECT_DESC *)&od_intym_n,	/* GCO_DT_INTYM_N */
    (GCA_OBJECT_DESC *)&od_intds,	/* GCO_DT_INTDS   */
    (GCA_OBJECT_DESC *)&od_intds_n,	/* GCO_DT_INTDS_N */
    (GCA_OBJECT_DESC *)&od_bool,	/* GCO_DT_BOOL    */
    (GCA_OBJECT_DESC *)&od_bool_n,	/* GCO_DT_BOOL_N  */
    (GCA_OBJECT_DESC *)&od_lkey,	/* GCO_DT_LKEY    */
    (GCA_OBJECT_DESC *)&od_lkey_n,	/* GCO_DT_LKEY_N  */
    (GCA_OBJECT_DESC *)&od_tkey,	/* GCO_DT_TKEY    */
    (GCA_OBJECT_DESC *)&od_tkey_n,	/* GCO_DT_TKEY_N  */
    (GCA_OBJECT_DESC *)&od_secl,	/* GCO_DT_SECL    */
    (GCA_OBJECT_DESC *)&od_secl_n,	/* GCO_DT_SECL_N  */

};

GLOBALDEF i4	    gco_l_ddt_ods = sizeof(gco_ddt_ods) / sizeof(*gco_ddt_ods);
GLOBALDEF GCO_OP    **gco_ddt_map = NULL;

