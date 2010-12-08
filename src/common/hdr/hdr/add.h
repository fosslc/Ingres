/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: ADD.H - Abstract Datatype Definition Header
**
** Description:
**      This file contains those special symbols which are used only for the
**	dynamic adding of datatypes to the INGRES system.  The datatypes defined
**	in this file are used primarily at program [of whatever variety] startup
**	time.  That is, these structures are used to pass datatype definition
**	information to ADF at the time the program, be it a DBMS server, /STAR
**	server, gateway, or application, starts.
**
**	This file presupposes the previous inclusion of the following files.
**		<compat.h>
**		<dbms.h>
**		<adf.h>
**
** History: 
**      24-Jan-89 (fred)
**          Created as part of the TERMINATOR project.
**      13-Apr-89 (fred)
**	    Overhauled.  Changed model so that all information is passed in at
**	    during the original callback.  The system has been altered so that
**	    the user code contains all information about the datatypes -- none
**	    is contained in the INGRES system catalogs.  Given this, it is less
**	    complex to have a single call back, rather than one per definition,
**	    etc.  Thus, a single structure passed in will contain the
**	    appropriate information.
**      10-apr-1990 (fred)
**          Added ADD_TRACE_FILE_MASK, ADD_ERROR_LOG_MASK, and ADD_FRONTEND_MASK
**	    for use in sending user trace messages around.
**      09-dec-1992 (stevet)
**          Changed ADD_FI_DFN structure to support user defined result
**          length routine.
**      21-dec-1992 (stevet)
**          Added function prototypes.
**      22-Jan-1993 (fred)
**          Added capability for large object types and some
**          additional status bits for the datatypes.
**      31-aug-1993 (stevet)
**          Added ADD_HIGH_CLSOBJ, ADD_LOW_CLSOBJ, ADD_CLSOBJ_FISTART and
**          ADD_CLSOBJ_OPSTART.  Rename ADD_FISTART to ADD_USER_FISTART
**          and ADD_OPSTART to ADD_USER_OPSTART.  These changes are made to
**          support class object on top of UDT.
**      12-Oct-1993 (fred)
**          Added dtd_seglen_addr.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Apr-2004 (stial01)
**          Define add_length as SIZE_TYPE.
**      23-Nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
**/
/*
**	NOT CURRENTLY USED -- BUT LEFT AS PLACEHOLDERS
**	
[@forward_type_references@]
[@forward_function_references@]
[@global_variable_references@]
**
**
*/

/*
**  Defines of other constants.
*/
#ifndef ADF_ADD_HDR_INCLUDED
#define ADF_ADD_HDR_INCLUDED
/*
**      Lock key for use in managing User defined datatypes
*/
#define                 ADD_LOCK_KEY    "II_DATATYPE_LEVEL"

/*
**	Mask values for sending trace message from user code.
**	Used in (at least) scaadddt.c\sca_trace;  defined
**	for users in IIADD.H.
*/

#define                 ADD_FRONTEND_MASK   0x1
#define			ADD_TRACE_FILE_MASK 0x2
#define			ADD_ERROR_LOG_MASK  0x4

/*}
** Name: ADD_CALLBACKS - List of callbacks into the INGRES system
**
** Description:
**      This structure defines the list of call back functions which are
**	available from the INGRES system.  Each callback may be present, in
**	which case is should be used as described.  However, callbacks may be
**	selectively absent, in which case there value will be zero.  Users must
**	be aware of the presence or absence of the callback functions.
**
** History:
**      10-apr-1990 (fred)
**          Created.
**      22-Jan-1993 (fred)
**          Added call back points for use by large object designers.
**          These callbacks are used to read/write large object
**          segments, and to filter large object segments through a
**          slave function.  Since large objects are composed of
**          segment of some underlying type, many operations on a
**          large object can be replaced by a series of similar
**          operations on the segments.  The filter function
**          facilitates such operations (for example, uppercase'ing a
**          long string).
**     26-Jul-1993 (fred)
**          Fixed callback definition so that pointer matches function
**          actually being called.
[@history_template@]...
*/
typedef DB_STATUS ADD_ERROR_WRITER(PTR		scb,
				   i4		err_num,
				   char		*text);

/* FUNC_EXTERN ADD_ERROR_WRITER adu_ome_error; */

FUNC_EXTERN DB_STATUS adu_ome_error(ADF_CB	*scb,
				    i4		 e_number,
              			    char	*e_text);

typedef DB_STATUS ADD_LO_HANDLER(i4  op_code,
				 PTR  pop_cb);

typedef DB_STATUS ADD_INIT_FILTER(PTR    	scb,
				  DB_DATA_VALUE *dv_lo,
				  DB_DATA_VALUE *dv_work);

typedef DB_STATUS ADD_FILTER_FCN (PTR   	  scb,
				  DB_DATA_VALUE   *dv_in,
				  DB_DATA_VALUE   *dv_out,
				  PTR 	    	  *work);

typedef DB_STATUS ADD_LO_FILTER(PTR            scb,
				DB_DATA_VALUE  *dv_in,
				DB_DATA_VALUE  *dv_out,
				ADD_FILTER_FCN *fcn,
				PTR            work,
				i4            continuation);

typedef struct _ADD_CALLBACKS
{
    i4              add_t_version;      /* The version available */
#define                 ADD_T_V1        0x1
#define                 ADD_T_V2        0x2
	/* ADD_T_V1 has only trace functions */
        /* ADD_T_V2 added large object support */
	
    DB_STATUS       (*trace_function)(); /*
					 ** Allow users to send trace messages
					 ** Called as
					 **	status = trace_function(
					 **		    dispose_mask,
					 **		    length,
					 **		    string);
					 **	where dispose_mask is taken
					 **	from the masks above,
					 **	and length is the length of the
					 **	character string pointed to by
					 **	string.
					 */
    ADD_ERROR_WRITER *add_error_fcn;     /*
					 ** Places error information
					 ** in the scb appropriately.
					 ** This function removes
					 ** dependence upon the
					 ** internal ADF_CB format
					 ** from the user's domain.
					 */
    ADD_LO_HANDLER   *add_lo_handler_fcn;/*
					 ** handles large object
					 ** segments.
					 */
    ADD_INIT_FILTER  *add_init_filter_fcn;/* Set up for filter function */
    ADD_LO_FILTER    *add_filter_fcn;
#define                 ADD_T_NOT_AVAILABLE 0x0
}   ADD_CALLBACKS;

/*
** }
** Name: ADD_DT_DFN - Information defining a datatype to ADF
**
** Description:
**      This structure contains the basic information necessary of ADF to
**	define a new datatype to the system.  Note that this information
**	represents only that information which is normally stored in the INGRES
**	system catalogs.  The majority of the datatype definition information is
**	actually obtained by a call the the user routines which define the
**	datatype.  For more information, see the "Product Specification for User
**	Defined Datatypes."
**
**	The system catalogs store the information about the datatype which must
**	be preserved across program invocations.  This includes
**
**	    dtd_name -- The name of the datatype
**	    dtd_id   -- The identification number for the datatype
**
** History:
**      24-Jan-1989 (fred)
**          Created.
**	28-mar-90 (jrb)
**	    Added ADD_O_INTERNDT as new definition type.  This flag means that 
**	    the datatypes being added are internal only so it's ok to have null
**	    names and null function pointers.
**	31-jul-90 (linda)
**	    Added the "isminmax" function.  Used by RMS Gateway to interpret
**	    values for positioning and scan limits.
**      4-mar-90 (linda)
**          Take out "isminmax" function.  It isn't needed by UDT users, just
**          by GWF, so can remain an internal ADF function.
**      22-Jan-1993 (fred)
**          Added dtd_attributes field and dtd_xform() field.  The
**          dtd_xform() function is used in handling large objects.
**          It is necessary only when the attributes field specifies
**          that this object is peripheral.  The attributes field also
**          allows the user to specify all of the dt_status bits
**          allowed for in adf.h (and available via the adi_dtstatus()
**          call.
**      12-Oct-1993 (fred)
**          Added dtd_seglen_addr -- used to determine length of
**          underlying type.
**	16-may-1996 (shero03)
**	    Add compress/expand routine
[@history_template@]...
*/
typedef struct _ADD_DT_DFN
{
    i4		    dtd_object_type;	    /* Check that block type is	    */
#define                 ADD_O_DATATYPE  0x0210
#define                 ADD_O_INTERNDT	0x9999
    ADI_DT_NAME	    dtd_name;		    /* The name of the datatype */
    DB_DT_ID	    dtd_id;		    /* Id number for the datatype */
#define                 ADD_LOW_CLSOBJ  ((DB_DT_ID) 8192)
#define                 ADD_HIGH_CLSOBJ ((DB_DT_ID) (8192+127))
#define                 ADD_LOW_USER    ((DB_DT_ID) 16384)
#define                 ADD_HIGH_USER   ((DB_DT_ID) (16384+127))
    DB_DT_ID        dtd_underlying_id;      /* Underlying DT if */
					    /* peripheral */
    i4              dtd_attributes;         /* Datatype attributes */
    ADP_LENCHK_FUNC     *dtd_lenchk_addr;   /* Ptr to lenchk function. */ 
    ADP_COMPARE_FUNC    *dtd_compare_addr;  /* Ptr to compare function. */
    ADP_KEYBLD_FUNC     *dtd_keybld_addr;   /* Ptr to keybld function. */
    ADP_GETEMPTY_FUNC   *dtd_getempty_addr; /* Ptr to getempty function. */
    ADP_KLEN_FUNC       *dtd_klen_addr;     /* Ptr to klen function. */
    ADP_KCVT_FUNC       *dtd_kcvt_addr;     /* Ptr to kcvt function. */
    ADP_VALCHK_FUNC     *dtd_valchk_addr;   /* Ptr to valchk function. */
    ADP_HASHPREP_FUNC   *dtd_hashprep_addr; /* Ptr to hashprep function. */
    ADP_HELEM_FUNC      *dtd_helem_addr;    /* Ptr to helem function. */
    ADP_HMIN_FUNC       *dtd_hmin_addr;     /* Ptr to hmin function. */
    ADP_HMAX_FUNC       *dtd_hmax_addr;     /* Ptr to hmax function. */
    ADP_DHMIN_FUNC	*dtd_dhmin_addr;    /* Ptr to "default" hmin function*/
    ADP_DHMAX_FUNC	*dtd_dhmax_addr;    /* Ptr to "default" hmax function*/
    ADP_HG_DTLN_FUNC    *dtd_hg_dtln_addr;  /* Ptr to hg_dtln function. */
    ADP_MINMAXDV_FUNC   *dtd_minmaxdv_addr; /* Ptr to minmaxdv function. */
    DB_STATUS	        (*dtd_tpls_addr)(); /* Ptr to tpls function. */
    DB_STATUS	        (*dtd_decompose_addr)();/* Ptr to decompose function.*/
    ADP_TMLEN_FUNC      *dtd_tmlen_addr;    /* Ptr to tmlen function. */
    ADP_TMCVT_FUNC      *dtd_tmcvt_addr;    /* Ptr to tmcvt function. */
    ADP_DBTOEV_FUNC     *dtd_dbtoev_addr;   /* Ptr to dbtoev function. */
    ADP_DBCVTEV_FUNC    *dtd_dbcvtev_addr;  /* Ptr to dbcvtev function. */
    ADP_XFORM_FUNC      *dtd_xform_addr;    /* Ptr to LO transformation fcn */
    ADP_SEGLEN_FUNC     *dtd_seglen_addr;   /* Ptr to seglen fcn */
    ADP_COMPR_FUNC      *dtd_compr_addr;    /* Ptr to compr/expd function. */
}   ADD_DT_DFN;

/*}
** Name: ADD_FO_DFN - Information defining an operator or function
**
** Description:
**      This structure is analogous to the ADD_DT_DFN structure above,
**	except it specifies information about functions (F) and operators (O).
**	Again, the information provided here is only that information which is
**	stored in the catalogs, and thus is only the information which must be
**	preserved over time.
**
**	This information includes
**
**	    fod_name -- the name of the function/operator
**	    fod_id   -- the identification number of the function/operator
**	    fod_type -- whether this object is a function or operator
**
** History:
**      24-Jan-1989 (fred)
**          Created.
[@history_template@]...
*/
typedef struct _ADD_FO_DFN
{
    i4		    fod_object_type;	/* Check for correct block type	    */
#define                 ADD_O_OPERATOR  0x0211
    ADI_OP_NAME     fod_name;           /* Func/Op name */
    ADI_OP_ID	    fod_id;		/* Func/Op Identifier */
#define			ADD_NO_OPERATOR	(ADI_OP_ID) -1
#define                 ADD_CLSOBJ_OPSTART   (ADI_OP_ID) 8192
#define                 ADD_USER_OPSTART     (ADI_OP_ID) 16384
    i4		    fod_type;		/* Function or Operator */
		/*	 See #define's in adf.h -- ADI_FI_DESC	*/
}   ADD_FO_DFN;

/*}
** Name: ADD_FI_DFN - Information about a function instance.
**
** Description:
**      This data structure, analogous to be two previous structures, provides
**	information to define a function instance (See ADF Specification).  And,
**	once again, the information provided here is only that information which
**	must be preserved across program invocations.  This information
**	corresponds to that information which is stored in the system catalogs
**	in the IIDBDB database, obtained at program/adf startup.
**
**	This information includes
**
**	    fid_id	-- Function instance identification number
**	    fid_opid	-- Operator id associated with this function instance
**	    fid_arg1	-- Datatype of first argument/operand
**	    fid_arg2	-- Datatype of second argument/operand
**	    fid_result	-- Datatype of function instance result
**
** History:
**      24-Jan-1989 (fred)
**          Created.
**      09-dec-1992 (stevet)
**          Added lenspec_length to ADD_FI_DFN structure to support user 
**          result length routine.
[@history_template@]...
*/
typedef struct _ADD_FI_DFN
{
    i4		    fid_object_type;	/* Check to verify block type	    */
#define                 ADD_O_FUNCTION_INSTANCE 0x0212
    ADI_FI_ID       fid_id;             /* FID */
#define			ADD_NO_FI	(ADI_FI_ID) -1
#define                 ADD_CLSOBJ_FISTART     (ADI_FI_ID) 8192
#define                 ADD_USER_FISTART       (ADI_FI_ID) 16384
    ADI_FI_ID       fid_cmplmnt;	/* FID of compliment */
    ADI_OP_ID	    fid_opid;		/* Associated operator		    */
    i4		    fid_optype;
    i4              fid_attributes;     /* FI Attributes */
    i4         fid_wslength;       /* Length of workspace if */
					/* req'd */
    i4		    fid_numargs;
    DB_DT_ID	    *fid_args;		/* Datatype for arguments */
    DB_DT_ID	    fid_result;		/* ...  for result */
    i4		    fid_rltype;		/* How to compute result length	    */
#define			ADD_FI_FIXED	7
    i4	    fid_rlength;	  /* ... result length itself	    */
    i4		    fid_rprec;		/* ... Precision and scale	    */
#define			ADD_FI_PSVALID	0
    DB_STATUS	    (*fid_routine)();	/* Routine to perform the FI	    */
    DB_STATUS	    (*lenspec_routine)(); /* User defined lenspec routine   */
}   ADD_FI_DFN;

/*}
** Name: ADD_DEFINTION - Union structure of definitions for passing into ADF
**
** Description:
**      This defines the union of the ADD_DT_DFN, ADD_FO_DFN, and ADD_FI_DFN.
**	This union is used to pass in all the datatype definition information at
**	program startup time.
**
**	In addition to the union structure, there are 3 counters passed in.
**	These represent the count of aforementioned structures, respectively.
**	These structures are expected to be `bunched;'  that is, there will
**	first be a bunch of datatype definitions, followed by a bunch of
**	function/operator definitions, followed by a bunch of function instance
**	definitions.
**
** History:
**      24-Jan-1989 (fred)
**          Created.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
[@history_template@]...
*/
typedef	union _ADD_UNION
{
    ADD_DT_DFN	add_dt_dfn;	/* datatype definition... */
    ADD_FO_DFN	add_fo_dfn;
    ADD_FI_DFN	add_fi_dfn;
} ADD_UNION;
typedef struct _ADD_DEFINITION	ADD_DEFINITION;
struct _ADD_DEFINITION
{
    ADD_DEFINITION  *add_next;           /* Next in the list -- std header */
    ADD_DEFINITION  *add_prev;		/* Previous in the list -- std header */
    SIZE_TYPE	    add_length;         /* length of this control block */
    i2		    add_type;           /* the type of the control block */
#define			ADD_DFN_TYPE	    (i2) 0x0201
#define                 ADD_DFN_TYPE_V2     (i2) 0x0202
#define                 ADD_CLSOBJ_TYPE     (i2) 0x0301
    i2              add_s_reserved;
    PTR		    add_l_reserved;
    PTR		    add_owner;
    i4		    add_ascii_id;
#define                 ADD_ASCII_ID    CV_C_CONST_MACRO('@','a','d', 'd')
    i4		    add_risk_consistency;
					/* boolean which, if set, says to   */
					/* ignore database consistency	    */
					/* issues and use the datatypes	    */
					/* regardless of whether the	    */
					/* recovery system agrees.	    */
					/* THIS IS VERY DANGEROUS:	    */
					/*   SETTING THIS FLAG REMOVES	    */
					/*   RESPONSIBILITY FROM THE INGRES */
					/*   RECOVERY SYSTEM.		    */
					/* USE THIS FLAG WITH EXTREME	    */
					/* CAUTION.			    */
#define			ADD_CONSISTENT	    0
#define			ADD_INCONSISTENT    1
    i4	    add_major_id;	/* Major identifier */
#define			ADD_INGRES_ORIGINAL 0x80000000L
    i4	    add_minor_id;	/* Minor identifier */
    i4		    add_l_ustring;	/* Length of ... */
    char	    *add_ustring;	/* Arbitrary string to be printed   */
					/* in log file when stuff is used.  */
    i4		    add_trace;		/* Boolean indicating whether	    */
					/* II_DBMS_LOG tracing is desired   */
#define                 ADD_T_LOG_MASK	0x1 /* Print trace messages */
#define			ADD_T_FAIL_MASK	0x2 /* Server startup fail on error */
    i4		    add_count;		/* total number of objects to be    */
					/* added			    */
    i4              add_dt_cnt;		/* Count of ADD_DT_DFN's */
    ADD_DT_DFN	    *add_dt_dfn;	/* and Ptr to array of ADD_DT_DFN's */
    i4		    add_fo_cnt;		/* Count of ....FO...... */
    ADD_FO_DFN	    *add_fo_dfn;	/* and Ptr to array of ADD_FO_DFN's */
    i4		    add_fi_cnt;		/* Count of function instances	    */
    ADD_FI_DFN	    *add_fi_dfn;	/* Ptr to array of their dfn's	    */
};



/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS adg_augment(ADD_DEFINITION  *new_objects,
				  i4	  table_size,
				  PTR             table_space,
				  DB_ERROR        *error);

FUNC_EXTERN i4   adg_sz_augment(ADD_DEFINITION     *new_objects,
				     DB_ERROR           *error);

#endif /* define ADF_ADD_HDR_INCLUDED */
