/*
**  Copyright (c) 2004,2010 Ingres Corporation
*/

#ifndef _ADP_H_INCLUDE
#define _ADP_H_INCLUDE
/*
**  Name: ADP.H - ADF Peripheral datatype structure definitions
**
**  Description:
**      This file contains the C types and define's necessary for ADF's
**	manipulations of peripheral types.
**
**  History:
**      23-Jan-1990 (fred)
**          Created.
**      23-Mar-1993 (fred)
**          Removed DB_TAB_ID reference from ADP_POP_CB to support
**          splitting of <dbms.h>.  Field was set only -- never used.
**       6-Jul-1993 (fred)
**          Added func extern/prototype for adu_peripheral -- the
**          fexi-based function called by users to do peripheral
**          manipulation. 
**      12-Aug-1993 (fred)
**          Added prototype for adu_lo_delete() -- to delete LO temp's.
**      15-Apr-1994 (fred)
**          Altered prototype for adu_free_objects.  Now takes a
**          sequence number as well.
**	28-mar-1996 (toumi01)
**	    Increased size of ADP_COUPON from 20 bytes to 24 bytes for
**	    axp_osf to allow for 8-byte pointer.  Added dummy ALIGN_RESTRICT
**	    "Align" variable to ADP_PERIPHERAL union to force alignment
**	    to accommodate pointer in coupon.
**	31-jul-1996 (kch)
**	    Replaced ADP_FREE_OBJECTS (remove temp objects) with
**	    ADP_FREE_NONSESS_OBJS (remove all non sess temp objects) and
**	    ADF_FREE_ALL_OBJS (remove all temp objects, including sess temps
**	    objects). This change fixes bug 78030.
**      08-aug-1996 (i4jo01)
**          Add define for ADP_CLOSE_TABLE to allow varchar function to close
**          the extension table it happens to be using. This was done to fix
**          bug #77189.
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - Added ADP_POP_FROM_COPY.
**	14-nov-1998 (somsa01)
**	    In ADP_POP_CB, added pop_xcb_ptr to hold the xcb pointer.
**	    (Bug #79203)
**	07-dec-1998 (somsa01)
**	    Re-worked the fix for bug #79203. The previous fix compromised
**	    spatial objects (unless a change was made to iiadd.h, which we
**	    are hesitant to make).  (Bug #79203)
**	10-may-99 (stephenb)
**	    Add define for ADP_CLEANUP
**      23-mar-1999 (hweho01)
**          Extended the changes for axp_osf to ris_u64 (AIX 64-bit platform).
**	16-apr-1999 (hanch04)
**	    For LP64 add cpn_id[5] to debugging prints for expanded
**	    64-bit coupon.
**      08-Sep-2000 (hanje04)
**          Extended the changes for axp_osf to axp_lnx (Alpha Linux)
**      12-jun-2001 (stial01)
**          Added ADW_L_BYTES, ADW_L_CPNBYTES
**      20-mar-2002 (stial01)
**          ADP_COUPON is 6 words only when there is no 32 - 64 bit upgrade 
**          path.
**	24-Oct-2001 (thaju02)
**	    Added shd_exp_action of ADW_FLSH_SEG_NOFIT_LIND to signify
**	    that we must flush segment due to adc_lvch_xform() being unable
**	    to fit 2 byte segment length indicator into result buffer.
**	    (B104122) 
**	10-sep-2002 (somsa01)
**	    Going along with Alison's change, "Align" should only be used
**	    only when there is no 32-64 bit upgrade path.
**      02-oct-2002 (stial01)
**          Added ADW_FLUSH_INCOMPLETE_SEGMENT for DB_LNVCHR_TYPE's
**          We may need to flush output buffer before it is full.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	16-oct-2003 (stial01, gupsh01)
**	    Added ADW_HALF_DBL_FLOC for indicating half of 
**	    unicode value is stored in adc_chars. 1/2 of unicode 
**	    is stored in adc_chars if a unicode value splits up in 
**	    input buffers.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**	7-May-2004 (schka24)
**	    Get rid of CLOSE TABLE, superseded by CLEANUP.
**	    Simplify all the crazy short temp/long temp stuff.
**      11-Aug-2006 (stial01)
**          Added new routines for substring, position for blobs.
**      16-oct-2006 (stial01)
**          Changes for blob locators.
**      08-dec-2006 (stial01)
**          Added additional position functions.
**      09-feb-2007 (stial01)
**          Added ADP_FREE_LOCATOR, adu_free_locator()
**      17-may-2007 (stial01)
**          Changed prototype for adu_free_locator
**	21-Mar-2009 (kiria01) SIR121788
**	    Added prototype for extended adu_lo_setup_workspace and
**	    adu_long_coerce
**	05-May-2009 (kiria01) b122030
**	    Added prototype for adu_long_unorm
*/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _ADP_POP_CB      ADP_POP_CB;
typedef	struct _ADP_LO_WKSP     ADP_LO_WKSP;
typedef struct _ADP_ADC_PRIVATE ADP_ADC_PRIVATE;
typedef struct _ADP_FI_PRIVATE  ADP_FI_PRIVATE;
typedef struct _ADP_SHARED      ADP_SHARED;
typedef u_i4			ADP_LOCATOR;

/*
**  Defines of other constants.
*/

/*
**      Operations defined on peripheral objects.
**
**	These are prefixed with ADF's facility code (2).
*/
#define                 ADP_INFORMATION 0x0201	    /*
						    ** Obtain information about
						    ** runtime environment.
						    */
#define                 ADP_GET         0x0202	    /*
						    ** Get the next segment
						    */
#define			ADP_PUT		0x0203	    /*
						    ** Put the next segment
						    */
#define			ADP_DELETE	0x0204	    /*
						    ** Delete an entire
						    ** peripheral object.
						    */
#define			ADP_COPY	0x0205	    /*
						    ** Copy a coupon from one
						    ** place to another
						    */
#define		ADP_FREE_NONSESS_OBJS	0x0206      /*
						    ** Remove temp. objects
						    ** that are not sess temps
						    */
#define		ADP_CPN_TO_LOCATOR	0x0207	    /* input cpn, output loc */
#define		ADP_LOCATOR_TO_CPN	0x0208	    /* input loc, output cpn */

#define		ADP_CLEANUP		0x0209	    /*
						    ** POP is finished
						    ** Clean up DMF temp memory
						    ** and close ext table
						    */
#define		ADP_FREE_LOCATOR	0x020A      /* free locator(s) */

/*
**  Test value for faking varchar elements when returning blobs before
**  FE's are ready.
*/
#define                 ADP_TST_VLENGTH 100
#define			ADP_TST_SEG_CNT   4

/*}
** Name: ADP_COUPON - Coupon for later exchange for a peripheral type.
**
** Description:
**      This structure is used by the system for the storage of peripheral
**	types.  Peripheral types are not stored, by the DBMS, inline with the
**	tuple.  Instead, this coupon is stored.  Later, when the actual data for
**	the peripheral type is desired, this coupon may be `redeemed' for the
**	actual data.
**	
**      The data in the coupon is really not controlled by ADF, however, ADF
**	must understand its format.  Therefore, there is some cooperation in
**	design between the tuple storer (DMF in the DBMS case) and ADF.
**
**      DMF will use the fields contained herein to store the peripheral
**	location of the data represented by this coupon.  This will be used
**	later, by DMF, to materialize the actual data.
**
** History:
**      07-nov-1989 (fred)
**          Created.
[@history_template@]...
*/
typedef struct _ADP_COUPON
{
#if defined(axp_osf) || defined(ris_u64) || defined(i64_win) ||  \
    defined(i64_hp2) || defined(i64_lnx) || defined(axp_lnx) ||  \
    defined(a64_win)
    i4         cpn_id[6];		/* Some sort of identification */
#else
    i4         cpn_id[5];		/* Some sort of identification */
#endif
}   ADP_COUPON;

/*}
** Name: ADP_PERIPHERAL - Structure describing a peripheral datatype
**
** Description:
**      This data structure "describes" the representation of a peripheral
**	datatype.  A datatype's declaration as peripheral indicates that the
**	data may not be stored in actual tuple on disk.  Instead, there may be
**	an ADP_COUPON which will indicate that the data is elsewhere.  This
**	data structure represents what will actually be present on the disk in
**	either case.  This structure represents a union between the ADP_COUPON
**	mentioned above, and a byte stream of indeterminate form.  Also included
**	is enough information to tell the difference.
**
** History:
**      07-nov-1989 (fred)
**          Created.
**	24-jun-2009 (gupsh01)
**	    Remove definition of val_locator. Locator values are expected to be 
**	    at a constant offset of ADP_HEADER_SIZE by API etc and on 64 bit 
**	    platforms per_value will get aligned to the ALIGN_RESTRICT boundary.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Define max tag value for validity checking.
*/
typedef struct _ADP_PERIPHERAL
{
    i4               per_tag;            /*
					** Tag as to whether this is a coupon or
					** the real thing
				        */
# define                ADP_P_DATA      0   /* Actual data */
# define                ADP_P_COUPON    1   /* Coupon is present */
# define		ADP_P_GCA	2   /* Data present in GCA format */
# define		ADP_P_GCA_L_UNK 3   /* 
					    ** Data present, GCA format, but
					    ** unknown length.
					    */
# define		ADP_P_LOCATOR	4   /* Locator. */
# define		ADP_P_LOC_L_UNK 5   /* Locator, length unknown */
					    /* Note: When locator tags are set,
					    ** value of locator is expected at
					    ** an offset of ADP_HDR_SIZE in this 
					    ** structure. 
					    ** Refrain from accessing it using the 
					    ** per_value field as per_value is 
					    ** aligned at ALIGN_RESTRICT boundary. 
					    */

# define		ADP_P_TAG_MAX	5   /* UPDATE ME if you add tags! */

    u_i4	    per_length0;	/* First half of length */
    u_i4	    per_length1;	/* Second half */
    union {
	ADP_COUPON	    val_coupon;	/* The coupon structure */
	char		    val_value[1];
	i4                 val_nat;    /* test for segment ind */
#if defined(axp_osf) || defined(ris_u64) || defined(i64_win) ||  \
    defined(i64_hp2) || defined(i64_lnx) || defined(axp_lnx) ||  \
    defined(a64_win)
	ALIGN_RESTRICT	    Align;	/* force alignment for coupon PTR */
#endif
    }
		    per_value;
# define                ADP_HDR_SIZE         (sizeof(i4) \
					      + (2 * sizeof(u_i4)))
# define                ADP_NULL_PERIPH_SIZE (ADP_HDR_SIZE \
					      + sizeof(i4) + 1)
# define                ADP_LOC_PERIPH_SIZE (ADP_HDR_SIZE + sizeof(i4))
}   ADP_PERIPHERAL;

/*
** }
** Name: ADP_POP_CB - CB used to invoke peripheral operations (POP's)
**
** Description:
**      This control block is used for ADF to invoke the peripheral operations
**	necessary to allow ADF to manipulate peripheral datatypes.  Internally,
**	ADF divides peripheral datatypes into smaller units of some
**	non-peripheral datatype.  By so doing, the DBMS can store the segments
**	as tuples in non-peripheral tables (table extensions in DMF
**	terminology).
**
**	This control block is used to invoke the necessary operations of get,
**	put, and delete.  All of these operations involve the use of a coupon,
**	as described above.  All but delete are transfering data from or to a
**	segment.  Thus, this control block, in addition to the standard headers
**	necessary in the DBMS, defines the two data elements as a segment and a
**	coupon.  Other administrative information is present to describe in more
**	detail the operation being performed.  The operation codes which are
**	described by this CB are described above.
**
**	ADF internally expects to call a routine whose standard interface is
**	
**	    status = (*routine)(i4 operation, ADP_POP_CB* pop_cb);
**
**	where status is one of
**	
**		ADP_INFORMATION	    Obtain information about
**				    runtime environment.
**
**		ADP_GET		    Get the next segment
**
**		ADP_PUT		    Put the next segment
**
**		ADP_DELETE	    Delete an entire peripheral object.
**
**		ADP_COPY	    Copy a coupon from one
**				    place to another
**
** History:
**      23-Jan-1990 (fred)
**          Created.
**      23-Mar-1993 (fred)
**          Removed DB_TAB_ID reference from ADP_POP_CB to support
**          splitting of <dbms.h>.  Field was set only -- never used.
**      27-Oct-1993 (fred)
**          Added pop_segno{0,1}.  These are used to request an
**          ADP_GET operation on a specific segment.  They are valid
**          only during an ADP_GET operation.  To have an effect,
**          these must be accompanied by setting the ADP_C_RANDOM_MASK
**          in the continuation field.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**          In this instance owner was missing from the 'standard header'.
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - Added ADP_POP_FROM_COPY.
**      29-Jun-98 (thaju02)
**          Added #define for ADP_GLBL_SESS_TEMP_ETAB. (B91469)
**	14-nov-1998 (somsa01)
**	    Added pop_xcb_ptr to hold the xcb pointer.  (Bug #79203)
**	07-dec-1998 (somsa01)
**	    Re-worked the fix for bug #79203. The previous fix compromised
**	    spatial objects (unless a change was made to iiadd.h, which we
**	    are hesitant to make).  (Bug #79203)
**	10-jan-99 (stephenb)
**	    Add pop_info field to avoid the use of temp tables.
**	11-May-2004 (schka24)
**	    Simplify temp flags.
**	26-Feb-2010 (troal01)
**	    Add ADP_C_GET_MASK to ensure that cpn_tcb isn't filled with a
**	    bogus value in dmpe_allocate, this flag is very short-lived:
**	    it is set before dmpe_allocate is called in dmpe_get and is removed
**	    within dmpe_allocate right after it has been checked.
[@history_template@]...
*/
struct _ADP_POP_CB
{
    ADP_POP_CB      *pop_next;          /* Standard header for all DBMS objects */
    ADP_POP_CB      *pop_prev;
    SIZE_TYPE	    pop_length;
    i2		    pop_type;
#define                 ADP_POP_TYPE    0x2001
    i2		    pop_s_reserved;
    PTR    	    pop_l_reserved;
    PTR    	    pop_l_owner;
    i4	    pop_ascii_id;
#define                 ADP_POP_ASCII_ID CV_C_CONST_MACRO('#', 'P', 'O', 'P')
	/* EO Standard header */
    DB_ERROR	    pop_error;		/* Result of this operation */
    i4		    pop_continuation;	/* status of this operation */
#define                 ADP_C_BEGIN_MASK    0x1 /* First time invoked */
#define			ADP_C_END_MASK	    0x2 /* Last time */
#define                 ADP_C_RANDOM_MASK   0x4 /* Want a specific
    	    	    	    	    	    	** segment
    	    	    	    	    	    	*/
#define			ADP_C_GET_MASK	    0x8 /* Indicates an ADP_GET, needed for SRID */
#define			ADP_C_MOVE_MASK	    0x10 /* Indicates a dmpe_move, do
                                                  * not overwrite the SRID */

    /* pop_temporary specifies what sort of target the POP should place
    ** the result value in, if it's a long object and if there is a result.
    ** If there's no long result (e.g. just a GET), pop_temporary is without
    ** meaning.
    ** "permanent" means permanent in the sense of a real etab.  A
    ** permanent result might be a session temporary etab if the base table
    ** is itself a session temp!
    ** "temporary" implies an anonymous temporary holding tank.  These
    ** are short-lived and are automagically tossed as soon as the value
    ** leaves the holding tank;  or if temporaries are explicitly cleared;
    ** or as a last resort at (parent) session end.
    ** Note that ADP_POP_TEMPORARY is the same as the old "short temp" flag,
    ** so existing OME clients should continue to work.
    */
    i4		    pop_temporary;	/* Is this a real table or temp */
#define                 ADP_POP_PERMANENT   0x0
#define			ADP_POP_TEMPORARY   0x1
    i4         pop_segno0;
    i4         pop_segno1;         /* segment # to get */
    DB_DATA_VALUE   *pop_coupon;	/* data value holding coupon */
    DB_DATA_VALUE   *pop_segment;	/* and the segment ... */
    DB_DATA_VALUE   *pop_underdv;	/*
					** Data value (no data part)
					** describing the underlying data for
					** table creations.
					*/
    PTR		    pop_user_arg;	/* Place for service to store data
					** NOTE that for multi-pass calls this
					** should be saved and restored.
					** The name is not very helpful as it is
					** really DMF info, at times a DMPE_PCB
					** or a DML_ODCB */
    PTR		    pop_info;		/* Extra information usefull to the
					** peripheral operation. for backend
					** storage, this will contain info
					** about the target table */
};


/*}
** Name: ADP_FI_PRIVATE - Function instance's workspace arena
**
** Description:
**      This section is data used by the function instances to
**      manage their state.  This should not be used by adc_()
**      routines.
** History:
**  	24-Nov-1992 (fred)
**          Created as part of restructuring ADP_LO_WKSP.
[@history_template@]...
*/
struct _ADP_FI_PRIVATE
{
    i4              fip_state;          /* State of operation */
# define               ADW_F_INITIAL           0
# define               ADW_F_STARTING          1
# define               ADW_F_RUNNING           2
# define               ADW_F_DONE_NEED_NULL    3
    i4              fip_null_bytes;
    i4              fip_l0_value;       /* Length of the large object */
    i4              fip_l1_value;
    i4         fip_test_mode;      /* are we in test mode ? */
    i4         fip_test_length;
    i4         fip_test_sent;
    i4              fip_done;           /* Is fetch operation complete */
    i4              fip_seg_ind;        /* Current segment indicator */
    i4              fip_seg_needed;     /* bytes needed for above */
    DB_DATA_VALUE   fip_other_dv;	/* Free DB_DATA_VALUE */
    DB_DATA_VALUE   fip_seg_dv;		/* Data Segment */
    DB_DATA_VALUE   fip_cpn_dv;		/* Coupon Segment */
    DB_DATA_VALUE   fip_under_dv;	/* Underlying datatype for blob */
    DB_DATA_VALUE   fip_work_dv; 	/* Unformated workspace provided */
    ADP_POP_CB	    fip_pop_cb;		/* Periperal Op. Ctl Block */
    ADP_PERIPHERAL  fip_periph_hdr;     /* Header area */
};

/*}
** Name: ADP_SHARED - Shared workspace arena
**
** Description:
**      This section is data which is used to communicate
**      between the function instance routines and the adc_()
**      routines.  These areas may be set or examined by either
**      side, typically by agreement between the two parties.
**
** History:
**  	24-Nov-1992 (fred)
**          Created as part of restructuring ADP_LO_WKSP.
**      14-Jul-2009 (hanal04)
**          Warning cleanup for Janitor's project.
[@history_template@]...
*/
struct _ADP_SHARED
{
    /*
    ** This is the datatype for which the operation is being
    ** performed.
    */

    DB_DT_ID       shd_type;
    /*
    ** These two values indicated whether the input or output values
    ** are segmented (respectively).  Segmented means that the values
    ** are to be sequences of legal segments of the underlying data
    ** type.  For large objects in the form of [ADP_P_] coupons or GCA
    ** representations, these values will be set.  If the data is to
    ** be specified as a the data in [relatively] raw form, these will
    ** be cleared.  Note that for a given call, neither, either, or
    ** both of these values may be set.  The adc_lo_xform() routine uses
    ** these values to determine how to do its job.
    */

    i4	    	    shd_inp_segmented;
    i4	    	    shd_out_segmented;

    i4	    shd_inp_tag;
    i4 	    shd_out_tag;        /* More detailed ... */

    /*
    ** These two values are the lengths computed by the adc_()
    ** routines.  These will be set by the adc_lo_xform() routines, and
    ** subsequently used by the function instance routines to set up
    ** the output coupon and/or to check the validity of the object.
    ** Failure to correctly set these values will result in either the
    ** creation or reporting of invalid/corrupted large objects.
    ** Either of these is bad.
    **
    ** The adc_lo_xform() routine is not necessarily called at the end of
    ** the action (i.e. after the last section has been presented),
    ** and thus is not necessarily aware of when it has been called
    ** for the last time on a particular large object.  Consequently,
    ** it is incumbent upon this routine to keep the shd_l{0,1}_check
    ** variable up to date -- accurate as of the time that the
    ** adc_lo_xform() routine returns.
    */

    i4 	    shd_l0_check;
    i4         shd_l1_check; 

    /*
    ** The expected action indicator communicates is used to convey
    ** information between the adc_lo_xform() routine and the function
    ** instance routine.  It is set to ADW_START before the first
    ** adc_lo_xform() call.  Thereafter, it is examined at the return of
    ** the adc_lo_xform() call to determine the next action necessary
    ** for the function instance routine to perform.  If the value is
    ** set (by adc_lo_xform(), on its return) to ADW_GET_DATA, then the
    ** function instance routine will obtain the next section of data
    ** to be processed by adc_lo_xform().  If it is set to
    ** ADW_FLUSH_SEGMENT, then the function instance routine is
    ** expected to dispose of the current (presumably "full") output
    ** segment, and provide a new, empty one for the adc_lo_xform()
    ** routine to deal with.  Note here that if adc_lo_xform() returns
    ** ADW_GET_DATA, and there is no more data to get, the function
    ** instance routine will 1) dispose of the current segment, and 2)
    ** assume that the work of adc_lo_xform() is complete.  adc_lo_xform()
    ** will not be called again.
    **
    ** This sequencing mandates that the adw_l{0,1}_chk values be
    ** correct (as noted above).  It further mandates that the output
    ** segment always be a valid segment as it stands.
    */

    enum shd_exp_action_enum
    {
	ADW_CONTINUE =			0,
	ADW_START =   			1,
	ADW_GET_DATA =			2,
	ADW_FLUSH_SEGMENT =		3,
	ADW_FLUSH_STOP =		4,
	ADW_NEXT_SEGMENT =		5,
	ADW_STOP =			6,
	ADW_FLSH_SEG_NOFIT_LIND	=	7,
	ADW_FLUSH_INCOMPLETE_SEGMENT =	8
    } shd_exp_action;

    /*
    ** The adw_i_area is a generic pointer to the area holding
    ** the current input segment;  the adw_o_area similarly
    ** describes the output segment.
    */

    i4	    shd_i_used;	        /* Number of bytes used so far */
    i4         shd_i_length;
    PTR             shd_i_area;
    i4         shd_o_used;         /* Number of bytes used so far */
    i4         shd_o_length;
    PTR     	    shd_o_area;
};

/*}
** Name: ADP_ADC_PRIVATE - Adc workspace arena
**
** Description:
**      The final section is an area of data for use by the adc_()
**      routines.  It will be neither examined nor set by the function
**      instance level routines during the course of processing.  It
**      can be trusted to be preserved across calls bounded by the
**      continuation - completed call sequence.
**
** History:
**  	24-Nov-1992 (fred)
**          Created as part of restructuring ADP_LO_WKSP.
**	16-Oct-2003 (stial01, gupsh01)
**	    Added ADW_C_HALF_DBL_FLOC to indicate if half
**	    a unicode/dbl byte value is set in the DBL flag 
**	    as first half of a two byte unicode value can be 0. 
**       7-Oct-2005 (hanal04) Bug 113093 INGSRV2973
**          Added ADW_S_BYTES. In SPuslong_xform() we may copy part of a
**          POINT or IPOINT because of input buffer boundaries. We need
**          to know how many whole points have been transfered and
**          the ADW_S_BYTES short is used to ensure the subsequent call
**          includes the partial (I)POINT's bytes in the amount transferred.
**	24-May-2007 (gupsh01)
**	    Added HALF_DBL_MAXCNT for storing count of the partial multibyte
**	    characters stored.
**
[@history_template@]...
*/
struct _ADP_ADC_PRIVATE
{
    i4		    adc_state;		/* Current state for machine */
    i4	    adc_need;		/* Number of bytes to complete state */
    i4	    adc_longs[16];	/* Collection of integers */
# define                ADW_L_COUNTLOC 0 /* Where in o_area is the */
					 /* count for the segment */
# define		ADW_L_BYTES    1 /* BYTE count for the segment */
# define		ADW_L_CPNBYTES 2 /* BYTE count for the blob */
# define                ADW_L_LRCOUNT  0 /* Count for left/right ops */
# define		ADW_L_SUBSTR_POS 0 /* position for substring op */
# define		ADW_L_SUBSTR_CNT 1 /* count for substring */
    i2              adc_shorts[16];     /* ...both long and short */
# define                ADW_S_COUNT    0 /* varchar count, current seg */
# define                ADW_S_BYTES    1 /* Count of the last POINT's bytes */
                                         /* transferred in previous call */
    char            adc_chars[32];       /* Collection of characters */
# define                ADW_C_HALF_DBL 1 /* Place for 1/2 a double */
					 /* byte character -- used in */
					 /* long varchar or half a */
					 /* unicode long nvarchar */
# define                ADW_C_HALF_DBL_FLOC 0 /* location in adc_chars */
					      /* to indicate that 1/2  */ 
					      /* of a dbl byte/unicode */ 
					      /* or part of a multibyte */
					      /* char stored in adc_chars */ 	
# define                ADW_C_HALF_DBL_CLEAR 0 /* Default flag */
				               /* nothing is stored */ 
# define                ADW_C_HALF_DBL_MAXCNT  3 /* Value indicates max */ 
					      /* allowed partials chars */ 
					      /*  of a unicode or dbl char */
					      /* is stored */
    char            adc_memory[32];     /* Generic memory buffer for */
					/* forming objects. */
};

/*}
** Name: ADP_LO_WKSP - Workspace for longish peripheral operations
**
** Description:
**      This data structure is used by peripheral operations to maintain
**	their context across calls.  Since peripheral datatypes tend to be
**	larger than is convenient to store in memory, it is often the case
**	that many passes are required for them to complete.  That is,
**	they will often return before their operation is complete, requesting
**	that their caller obtain more data or buffer space in which they can
**	work.
**
**	Given that this is the case and that there may be more than one
**	peripheral operation going on at once,  these routines require that a
**	workspace be provided.  The definition of the fields in this workspace
**	is public -- it must be since the caller must declare and own this
**	space;  the routine being called may specify a public usage for some or
**	all of these fields.  However, unless the called routine specifies that
**	the field may be altered, the caller is expected to preserve the value
**	of all fields across calls.
**
** History:
**      15-may-1990 (fred)
**          Created.
**  	24-Nov-1992 (fred)
**          Redone.  Separated sections by interface function.  Added
**          explanatory material in support of OME large objects.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      21-mar-1994 (johnst)
**          Bug #58635; re-do previous change that didn't seem to make it into
**	    this file:
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT. (In this instance owner was missing from the 
**	    'standard header'.)
**	23-Mar-2009 (kiria01) SIR121788
**	    Separated usage of adw_filter_space into adw_dmf_ctx for persisted
**	    dmf info and adw_filter_space as user-arg for the slave functions.
[@history_template@]...
*/
struct _ADP_LO_WKSP
{
    ADP_LO_WKSP     *adw_next;          /* Standard header for all DBMS objects */
    ADP_LO_WKSP     *adw_prev;
    SIZE_TYPE	    adw_length;
    i2		    adw_type;
#define                 ADP_ADW_TYPE    0x2002
    i2		    adw_s_reserved;
    PTR		    adw_l_reserved;
    PTR    	    adw_l_owner;
    i4	    adw_ascii_id;
#define                 ADP_ADW_ASCII_ID CV_C_CONST_MACRO('#', 'L', 'O', 'W')
	/* EO Standard header */
    PTR             adw_filter_space;   /* storage for filter slave function */
    ADP_FI_PRIVATE  adw_fip;
    ADP_SHARED      adw_shared;
    ADP_ADC_PRIVATE adw_adc;
    PTR		    adw_dmf_ctx;	/* DMF sore persisted across multi-
					** pass calls */
};

/*}
** Name: ADP_FILTER_FCN -- Function used by large object filter instances
**
** Description:
**      This is a datatype which defines the prototype used for
**      large object filtered instance functions.  These are functions
**      which are implemented using the general purpose
**      adu_lo_filter_fcn() routine which will apply some slave
**      function (specifically of the type specified by this typdef)
**      to each segment in the large object.  These instances are, in
**      general, relatively easy to generate, since they typically use
**      existing instances for the base type of the large object
**      applied to each segment.  They use the ADP_LO_WKSP workspace
**      (defined above) to maintain context between the master routine
**      and the individual invocations of the slave routine.
**
**      For more details, see the adu_lo_filter() routine definition.
**
** History:
**      15-Dec-1992 (fred)
**          Created.
*/
typedef DB_STATUS ADP_FILTER_FCN (ADF_CB  	  *scb,
				  DB_DATA_VALUE   *dv_in,
				  DB_DATA_VALUE   *dv_out,
				  ADP_LO_WKSP     *work);


/*
**  Forward and/or External function references.
*/
FUNC_EXTERN ADP_XFORM_FUNC adc_xform;
FUNC_EXTERN ADP_XFORM_FUNC adc_lvch_xform;

FUNC_EXTERN DB_STATUS
adu_0lo_setup_workspace(ADF_CB     	*scb,
			DB_DATA_VALUE   *dv_lo,
			DB_DATA_VALUE 	*dv_work);

FUNC_EXTERN DB_STATUS
adu_lo_setup_workspace(ADF_CB     	*scb,
			DB_DATA_VALUE   *dv_lo,
		        DB_DATA_VALUE   *dv_out,
			DB_DATA_VALUE 	*dv_work,
			i4		ws_buf_ratio);

FUNC_EXTERN DB_STATUS
adu_lo_filter(ADF_CB         *scb,
	      DB_DATA_VALUE  *dv_in,
	      DB_DATA_VALUE  *dv_out,
	      ADP_FILTER_FCN *fcn,
	      ADP_LO_WKSP    *work,
	      i4             continuation,
              ADP_POP_CB     *pop_cb_ptr);


FUNC_EXTERN DB_STATUS
adu_3lvch_upper(ADF_CB      	*scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_work,
		DB_DATA_VALUE   *dv_out);

FUNC_EXTERN DB_STATUS
adu_5lvch_lower(ADF_CB      	*scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_work,
		DB_DATA_VALUE   *dv_out);

FUNC_EXTERN DB_STATUS
adu_7lvch_left(ADF_CB        *scb,
	       DB_DATA_VALUE *dv_in,
	       DB_DATA_VALUE *dv_count,
	       DB_DATA_VALUE *dv_work,
	       DB_DATA_VALUE *dv_out);

FUNC_EXTERN DB_STATUS
adu_9lvch_right(ADF_CB        *scb,
                DB_DATA_VALUE *dv_in,
                DB_DATA_VALUE *dv_count,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_out);

FUNC_EXTERN DB_STATUS
adu_11lvch_concat(ADF_CB        *scb,
		  DB_DATA_VALUE *dv_in,
		  DB_DATA_VALUE *dv_in2,
		  DB_DATA_VALUE *dv_work,
		  DB_DATA_VALUE *dv_out);

FUNC_EXTERN DB_STATUS
adu_13lvch_substr(ADF_CB        *scb,
                DB_DATA_VALUE *dv_in,
                DB_DATA_VALUE *dv_pos,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_out);

FUNC_EXTERN DB_STATUS
adu_14lvch_substrlen(ADF_CB        *scb,
                DB_DATA_VALUE *dv_in,
                DB_DATA_VALUE *dv_pos,
		DB_DATA_VALUE *dv_count,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_out);

FUNC_EXTERN DB_STATUS
adu_15lvch_position(ADF_CB        *scb,
                DB_DATA_VALUE *dv1,
                DB_DATA_VALUE *dv_lo,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_pos);

FUNC_EXTERN DB_STATUS
adu_16lvch_position(ADF_CB        *scb,
                DB_DATA_VALUE *dv1,
                DB_DATA_VALUE *dv_lo,
		DB_DATA_VALUE *dv_from,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_pos);

FUNC_EXTERN DB_STATUS
adu_17lvch_position(ADF_CB        *scb,
                DB_DATA_VALUE *dv1,
                DB_DATA_VALUE *dv2,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_pos);

FUNC_EXTERN DB_STATUS
adu_18lvch_position(ADF_CB        *scb,
                DB_DATA_VALUE *dv1,
                DB_DATA_VALUE *dv2,
		DB_DATA_VALUE *dv_from,
                DB_DATA_VALUE *dv_work,
                DB_DATA_VALUE *dv_pos);

FUNC_EXTERN DB_STATUS
adu_couponify(ADF_CB             *scb,
    	      i4                 continuation,
              DB_DATA_VALUE      *lodv,
              ADP_LO_WKSP        *workspace,
              DB_DATA_VALUE      *cpn_dv);

/* Redeem an ADF_COUPON data structure */
FUNC_EXTERN DB_STATUS
adu_redeem(ADF_CB         *adf_scb,
	   DB_DATA_VALUE  *result_dv,
	   DB_DATA_VALUE  *coupon_dv,
	   DB_DATA_VALUE  *workspace_dv,
	   i4        continuation);

FUNC_EXTERN DB_STATUS
adu_cpn_to_locator(ADF_CB	*adf_scb,
		DB_DATA_VALUE	*coupon_dv,
		DB_DATA_VALUE	*locator_dv);

FUNC_EXTERN DB_STATUS
adu_locator_to_cpn(ADF_CB *adf_scb,
		DB_DATA_VALUE	*locator_dv,
		DB_DATA_VALUE	*coupon_dv);


/* Perform peripheral manipulations. */

FUNC_EXTERN DB_STATUS
adu_peripheral(i4  	    op_code,
	       ADP_POP_CB   *pop_cb);

/* Delete LO temporaries */

FUNC_EXTERN DB_STATUS
adu_lo_delete(ADF_CB     	*scb,
	      DB_DATA_VALUE     *dbdv);

/* Free temporary objects... */

FUNC_EXTERN VOID
adu_free_objects(PTR            storage_location,
		 i4        sequence_number);

/* Free Locator(s) */

FUNC_EXTERN VOID
adu_free_locator(PTR            storage_location,
		 DB_DATA_VALUE        *locator_dv);

FUNC_EXTERN DB_STATUS
adu_long_coerce(ADF_CB      	*adf_scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_work,
		DB_DATA_VALUE   *dv_out);

/* long UNORM - note no work_dv */
FUNC_EXTERN DB_STATUS
adu_long_unorm(ADF_CB      	*adf_scb,
		DB_DATA_VALUE  	*dv_in,
		DB_DATA_VALUE   *dv_out);

#endif /*_ADP_H_INCLUDE*/
