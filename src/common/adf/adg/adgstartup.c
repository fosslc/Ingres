/*
**  Copyright (c) 1986, 2009 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <cv.h>
#include    <bt.h>
#include    <ex.h>
#include    <me.h>
#include    <nm.h>
#include    <tr.h>
#include    <lo.h>
#include    <er.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adftrace.h>
#include    <adfops.h>
#include    <add.h>
#include    <ade.h>
#include    <clfloat.h>
#include    <aduucoerce.h>
#include    "adgfi_defn.h"

/**
**
**  Name: ADGSTARTUP.C - Initialize ADF for the server.
**
**  Description:
**      This file contains all of the routines necessary to perform the 
**      external ADF call "adg_startup()" which initializes the ADF for a
**      server.
**
**	This file defines the following externally visible routines:
**
**          adg_srv_size()   - Calculate size needed for ADF server CB.
**          adg_startup()    - Initialize ADF for the server.
**	    adg_sz_augment() - Calculate size needed for adf to add new
**			       datatypes, functions, and operators
**	    adg_augment()    - Add new datatypes, functions, and operators
**			       to ADF's world.
**
**	... and the following static routines:
**
** 	    ad0_namesame() - Compares two names.
** 	    ad0_dbiinit()  - Init the table of dbmsinfo() requests.
**          ad0_dtinit()   - Init the datatypes and datatype pointers tables.
**	    ad0_fiinit()   - Init the function instance table.
**	    ad0_hfinit()   - Init the floating-point values for
**			     histogram processing.
**
**  Function prototypes for adg_sz_augment and adg_augment in ADD.H, 
**  adg_srv_size and adgstartup in ADF.H
**
**  History:
**      23-jul-86 (thurston)
**          Initial creation, mostly stripped out of ADGINIT.C.
**	25-jul-86 (thurston)
**	    Changed the names of adc__lenchk(), adc__getempty(), and
**	    adc__isnull() to be adc_1lenchk_rti(), adc_1getempty_rti(), and
**	    adc_1isnull_rti(), respectively.
**	01-aug-86 (ericj)
**	    Added references to the networking routines, adc_tonet() and
**	    adc_tolocal() to the ADP_COM_VECT struct of the datatypes table.
**	24-sep-86 (thurston)
**	    I added the new datatypes "char", "varchar", "longtext", and
**	    "abrtsstr" to the datatypes table, and adjusted the datatype ptrs
**	    table to account for them.  Also, in the static routine adg_dtint(),
**          I added all of the BTset() calls to handle the 37 new coercions
**          necessary to support "char", "varchar", and "longtext". 
**	15-oct-86 (thurston)
**	    Added the datatype "fe_query", and documented as reserved for
**	    frontend use.
**	16-oct-86 (thurston)
**	    Added the static routine ad0_opinit() to finish init'ing the
**	    operations table.
**	28-oct-86 (thurston)
**	    Added ult_init_macro() to adg_startup().
**	01-dec-86 (thurston)
**	    Added the dbtoev and dbcvtev routines to the datatypes table.
**	29-dec-86 (thurston)
**	    Had to add the adc_2lenchk_bool() routine to the datatype table
**	    for "boolean", and add the approipriate entry in the datatype ptrs
**	    table so that a boolean result (retruned from an adf_func() call
**	    for a comparison function instance) can have its length checked.
**	07-jan-87 (daved)
**	    add synonyms for datatypes.
**	07-jan-87 (thurston)
**	    In ad0_opinit() I allowed there to be holes in the fitab by having
**	    the fi ID set to ADI_NOFI.  This allows easy removal of fi's from
**	    said table.
**	03-feb-87 (thurston)
**	    Made GLOBAL datatype ptrs table READONLY.
**	03-feb-87 (thurston)
**	    Temporarily (probably for a long time) removed the embedded value
**	    routines, adc_1dbtoev_rti() and adc_2dbcvtev_rti().
**	03-feb-87 (thurston)
**	    Replaced references to adc_date_compare() and adc_money_compare()
**	    with refs to adu_4date_cmp() and adu_6mny_cmp().
**	18-feb-87 (thurston)
**	    Added an extra check to ad0_opinit() to make sure fitab has no
**	    holes in the middle of an op-group.
**	25-feb-87 (thurston)
**	    Changed adg_startup() interface to accept some memory and initialize
**	    it as ADF's server control block, which will take the place of all
**	    ADF GLOBALDEFs that were not READONLY.
**	25-feb-87 (thurston)
**	    Stripped out the datatypes table and datatype pointer table.
**	    Datatypes table is now in ADGDTTAB.ROC.  Datatype pointer table is
**	    now set up by the ad0_dtinit() routine at server startup.  Wrote
**	    adg_srv_size() routine.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	10-mar-87 (thurston)
**	    Added code to ad0_opinit() to set the function instance table
**	    constants in the server CB.
**	18-mar-87 (thurston)
**	    Added some WSCREADONLY to pacify the Whitesmith compiler on MVS.
**	25-mar-87 (thurston)
**	    Fixed bug in ad0_opinit() where .Adi_num_fis was never initialized to 0.
**	25-mar-87 (thurston)
**	    Modified ad0_dtinit() to look through the coercions in the function
**	    instance table in order to set up the coercion bit masks for each
**	    datatype.  Previously, it was hard-coded as to what coercions to
**	    set, which was (a) a pain in the a-- when adding new coercions to
**	    the system, and (b) ripe for inconsistency.
**	10-apr-87 (thurston)
**	    Added the static routine ad0_dbiinit() and code in adg_startup() to
**	    use it, including an extra argument to adg_startup().
**	24-jun-87 (thurston)
**	    Added code to ad0_opinit() to handle new copy coercions.
**	02-sep-87 (thurston)
**	    Fixed bug in the `do while' loop in ad0_opinit().  Was looping one
**          two many times, and was not setting the biggest fi ID properly.
**	28-sep-87 (thurston)
**	    Added ad0_namesame() and new code to ad0_dbiinit() to set the
**	    ADK_MAPs in the server CB properly.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	06-nov-88 (thurston)
**	    Temporarily commented out the WSCREADONLY on the datatypes and op
**	    tables.
**      25-Jan-1989 (fred)
**          Added adg_augment() support for user-defined ADT support.
**	09-mar-89 (jrb)
**	    Removed ad0_opinit and static variable sz_optab.  Added GLOBALREFs
**	    to sizeof globals for dttab, fitab, optab, and filkup.  The
**	    operations table is now completely filled in at compile-time.
**      18-apr-89 (fred)
**	    Added exception handlers to adg_augment() and adg_sz_augment().
**	    If these handlers are reached, it is assumed (usually correctly)
**	    that there was an undiagnosable (or undiagnosed -- but these are
**	    bugs!) problem in the structure passed in by the user.  Since we
**	    need to catch these and not crash, we have exception handlers.
**	    In the event that such an exception is fired, the system will
**	    restore default ADF capability, and return.  It is a higher order
**	    function to determine whether to quit or not...
**	26-may-89 (fred)
**	    Added additional parameter checking to adg_augment() and
**	    adg_sz_augment().
**      15-jun-89 (fred)
**          Added additional options for user defined function instance return
**	    length specification.
**	15-jun-89 (jrb)
**	    Changed GLOBALREF to GLOBALCONSTREF for datatypes and operations
**	    tables.
**      07-July-89 (fred)
**          Added checking that all coercions and all new function instances
**	    involve only known types.  Also removed non-ifdef'd reference to
**	    adb_optab_print() which, apparently, uses ulf, rendering it unusable
**	    in the frontends (esp. createdb, etc.).  Also added better trace
**	    information, specifically the printing of argument types for
**	    function instance creation.
**      14-jul-89 (fred)
**          Added unsupported support for use of UDADT's in QUEL.  This feature
**	    is not intended to be advertised, but should be available for
**	    various users who are not finished converting all of their
**	    applications as of yet...
**      11-aug-89 (fred)
**	    Added support for overloading unary operators MINUS & PLUS OP.
**	29-mar-90 (jrb)
**	    Added support for internal datatypes being added (this means we
**	    don't do certain consistency checks for datatypes).
**	11-sep-90 (linda)
**	    Add constant strings to represent minimum and maximum values for
**	    character and text types.  This is used by routine adc_isminmax()
**	    to tell if a key is minimum or maximum for its datatype; useful for
**	    gateways when positioning.
**	05-feb-91 (stec)
**	    Added ad0_hfinit() routine, modified adg_statrtup(), added include
**	    for <clfloat.h>
**	22-mar-91 (jrb)
**	    Added #include of st.h.
**	25-apr-91 (seng)
**	    Need to add u_char declaration before Chr_min, Chr_max, Cha_min
**	    Cha_max, Txt_max, Vch_min, Vch_max, Lke_min, and Lke_max arrays.
**	    AIX 3.1 needs to have a type declarator in front on RS/6000.
**	    This is a generic change.
**	09-mar-92 (jrb, merged by stevet)
**	    Added consistency check to ensure no DB_ALL_TYPE's are allowed in
**	    coercion section of function instance table (for outer join
**	    project).
**	09-mar-92 (stevet)
**	    Skip datatype checking for DB_ALL_TYPE, this cause problem when
**	    UDTs are loaded.
**	23-sep-1992 (fred)
**	    Add support for BIT types.  Specifically, initialization for 
**	    min/max value arrays.
**      08-dec-1992 (stevet)
**          Added support for LEN_EXTERN support for UDT functions.  When
**          defined, user routine will be called to calculate result length
**          of an expression.
**      21-dec-1992 (stevet)
**          Added function prototype.
**      24-mar-1993 (smc)
**          Cast arguments of STnlength to same as prototype and commented
**          out text after endifs.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**       5-May-1993 (fred)
**          Upgraded to support large OME objects.
**	08-aug-93 (shailaja)
**	    Unnested  comments.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      31-aug-1993 (stevet)
**          Added support for class objects which act very much like UDT
**          except it's DT, FI, OP are in 8392 ragne instead of 16784 range.
**      16-mar-1994 (johnst)
**          Bug #60764
**          Adjusted sz_optab variable in adg_srv_size() to be aligned on
**	    ALIGN_RESTRICT boundary, just like all the other sz_* variables
**          that are used to calculate pointers to objects.
**	11-jan-1995 (shero03)
**	    Fixed bug in function instance table lookup after adding
**	    more holes to the table.
**	16-feb-1995 (shero03)
**	    Bug #65565
**	    Support UDT xform address so that long UDTs can be copied
**	03-jun-1996 (canor01)
**	    Remove MCT semaphores. Resources are initialized during single-
**	    threaded startup.
**	03-jul-96 (inkdo01)
**	    Various changes to support PRED_FUNC operation types.
**	23-sep-1996 (canor01)
**	    Move global data definitions to adgdata.c.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      10-aug_1999 (chash01)
**          Testing of datatype definition in ad0_dtinit() is not written
**          correctly.  See comments in that routine.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**          between compat.h and system limits.h on (sqs_ptx).
**	19-Jan-2001 (jenjo02)
**	    Added ADI_B1SECURE, ADI_C2SECURE Adi_status flags
**	    so that ADF can be more selective when calling
**	    security functions.
**	11-jun-2001 (somsa01)
**	    To save stack space, we now dynamically allocate space for
**	    the min / max arrays.
**	13-dec-2001 (somsa01)
**	    Added LEVEL1_OPTIM for i64_win to prevent a "help table ..."
**	    from returning E_AD1096 Error occurred while determining the
**	    session privileges within ADF".
**	30-sep-2002 (drivi01)
**	    Added an if loop to avoid lookup of deffinitions for operations 
**	    with ADI_DISABLED flag, to insure that the recovery server is
**	    able to startup when it is being linked with udts.
**	26-jun-2003 (somsa01)
**	    Removed LEVEL1_OPTIM for i64_win starting with the GA version
**	    of the Platform SDK.
**      15-aug-2003 (stial01)
**          Check for II_CHARSET UTF8 in adg_startup (b110727)
**	07-jan-2005 (abbjo03)
**	    Until Xerces C++ 2.5 is ported to VMS, we cannot support Unicode
**	    conversion. Bypass the reading of the map files until this
**	    happens.
**	18-Jan-2005 (fanra01)
**	    Bug 113770
**	    Protect string type max. and min. values from being initialized more
**	    than once.
**	31-Jul-2007 (kiria01) b117955
**	    With change to fi_defn.awk and integration into jam
**	    the header adfopfis.h has become local header adgfi_defn.h
**	10-oct-2007 (abbjo03)
**	    Increase the coercion table sizing fudge factor and define a
**	    constant for it.
**      05-feb-2009 (joea)
**          Use CMischarset_doublebyte and CMischarset_utf8, instead of
**          CMget_attr.
**	04-Oct-2010 (kiria01) b124065
**	    Cater for support of eqv operator in coercions and the name
**	    changes for the operator table (now built from fi_defn.txt)
**/


#define ADF_FUDGE_FACTOR        380

/*
** Definition of all global variables used by this file.
*/

GLOBALDEF				i4				adg_startup_instances=0;
GLOBALCONSTREF			ADI_DATATYPE	    Adi_1RO_datatypes[];
GLOBALCONSTREF			ADI_OPRATION	    Adi_3RO_operations[];
GLOBALCONSTREF			ADI_FI_DESC	    Adi_fis[];
GLOBALCONSTREF			ADI_FI_LOOKUP	    Adi_fi_lkup[];
GLOBALCONSTREF			i4		    Adi_dts_size;
GLOBALCONSTREF			i4		    Adi_3RO_ops_size;
GLOBALCONSTREF			i4		    Adi_fis_size;
GLOBALCONSTREF			i4		    Adi_fil_size;


/* [@global_variable_definition@]... */
/*
**  Definitions of minimum and maximum values for string types.
*/

GLOBALREF   u_char	*Chr_min;
GLOBALREF   u_char	*Chr_max;
GLOBALREF   u_char	*Cha_min;
GLOBALREF   u_char	*Cha_max;
GLOBALREF   u_char	*Txt_max;
GLOBALREF   u_char	*Vch_min;
GLOBALREF   u_char	*Vch_max;
GLOBALREF   u_char	*Lke_min;
GLOBALREF   u_char	*Lke_max;
GLOBALREF   u_char	*Bit_min;
GLOBALREF   u_char	*Bit_max;

			
/*
**  Definition of static variables and forward static functions.
*/

static	i4	     number_of_exceptions = 0;	/* number of exceptions	    */
						/* that the handler has had */
						/* to handle.		    */
static  i4      sz_srvhd    = 0;	    /* Size of ADF's server CB header */
static	i4	     sz_fxtab    = 0;	    /* Size of FEXI table */
static	i4      sz_dttab    = 0;	    /* Size of datatypes table */
static	i4      sz_dtptab   = 0;	    /* Size of datatype ptrs table */
static	i4      sz_coerctab = 0;	    /* Size of sorted coercion table */
static  i4      sz_optab    = 0;	    /* Size of ADF's operations	    */
static	i4	     sz_fitab	 = 0;	    /* Sizeof function inst. table  */
static  i4      sz_srvcb  = 0;	    /* Size of ADF's server CB */
    /* The above static variables are set by adg_srv_size(), and then
    ** used by ad0_dtinit() ... all at server startup time.
static	i4	     sz_fitab = 0;	    /* Size of copied fi tab --	    */
					    /* only when using udadts	    */
static	i4	     sz_newadts = 0;

/* Compares two names. */   
static	bool	     ad0_namesame(char *dbi_name,
				  char *kname);	 

/* Initializes the table of dbmsinfo() requests. */
static	DB_STATUS    ad0_dbiinit(ADF_TAB_DBMSINFO *dbi_tab,
				 ADF_SERVER_CB    *scb);

/* Initializes the function instance table. */
static  DB_STATUS    ad0_fiinit(ADF_SERVER_CB     *scb,
				ADD_DEFINITION    *new_objects,
				ADI_FI_DESC       *new_fis,
				ADI_FI_LOOKUP     *new_fi_lkup,
				DB_ERROR	  *error);

/* Validates sequence of entries in default function instance table. */
static DB_STATUS     ad0_fivalidate(
				ADF_SERVER_CB      *scb,
				DB_ERROR	   *error);

/*
** Initializes the operations table by copying the READONLY operations 
** structure into the server CB, and then setting up the pointer to the 
** group of function instances and the number of function instances for each
** operater by looking through the function instance table. 
*/
static	DB_STATUS    ad0_opinit(ADF_SERVER_CB	    *scb,
				ADD_DEFINITION	    *new_objects,
				ADI_OPRATION	    *new_operations,
				ADI_FI_DESC	    *new_fis,
				ADI_FI_LOOKUP	    *new_fi_lkup,
				DB_ERROR	    *error);

/*
** Initializes the datatypes and datatype pointers tables by copying 
** the READONLY datatypes structure into the server CB, doing BTset() 
** calls to set up the legal coercions found in the function instance 
** table, and then setting up the datatype pointers table from the datatypes 
** table. 
*/
static	DB_STATUS    ad0_dtinit(ADF_SERVER_CB	*scb,
				ADD_DEFINITION	*new_objects,
				ADI_DATATYPE	*new_datatypes,
				ADI_DATATYPE	**new_p_datatypes,
				ADI_COERC_ENT	*new_coercions,
				DB_ERROR	*error);

/*
** Exception handler for dealing with (primarily address errors) in user      
** defined datatype code
*/
static	i4	    ad0_handler(EX_ARGS  *args);


/* 
** Initializes floating-point constants ised in adc_hdec()
*/
static	i4	    ad0_hfinit(ADC_HDEC  *adc_hdec);

/*{
** Name: adg_srv_size() - Return size needed for ADF's server control block.
**
** Description:
**	This function should be used prior to the adg_startup() call in order
**	to determine how much memory will be needed to set up ADF's server
**	control block.  Unlike all other externally visible ADF routines, this
**	does not return a DB_STATUS; instead it returns a i4 which
**      represents the number of bytes needed.
**
** Inputs:
**      none
**
** Outputs:
**	none
**
**	Returns:
**	    i4	    Number of bytes needed to set up ADF's server
**			    control block.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-feb-87 (thurston)
**          Initial creation and coding.
**	06-oct-88 (thurston)
**	    Added in code to calculate the size of the sorted coercion table.
**	06-mar-89 (jrb)
**	    Used globals to get sizes for operations and datatypes tables
**	    rather than counting them at runtime.  Removed all code for getting
**	    size of optab and putting into server cb.  This was done to improve
**	    startup time.
**      09-mar-92 (jrb, merged by stevet)
**          Added calculations for FEXI table.
**	28-apr-06 (dougi)
**	    Shameless hack for data type family coercion structures.
*/

i4
adg_srv_size(void)
{
    i4		s = sizeof(ALIGN_RESTRICT);

    if (!sz_srvcb)
    {
	/* Start with defined size of server control block */
	/* ----------------------------------------------- */
	sz_srvhd = sizeof(ADF_SERVER_CB);
	if (sz_srvhd % s)
	    sz_srvhd += (s - (sz_srvhd % s));

	/* Add in room for FEXI table */
	/* -------------------------- */
	sz_fxtab = sizeof(ADI_FEXI) * ADI_SZ_FEXI;
	if (sz_fxtab % s)
	    sz_fxtab += (s - (sz_fxtab % s));

	/* Add in room for the datatypes table */
	/* ----------------------------------- */
	sz_dttab = Adi_dts_size;
	if (sz_dttab % s)
	    sz_dttab += (s - (sz_dttab % s));

	/* Add in room for the datatype ptrs table of pointers */
	/* --------------------------------------------------- */
	sz_dtptab = (ADI_MXDTS+1) * sizeof(ADI_DATATYPE *);
	if (sz_dtptab % s)
	    sz_dtptab += (s - (sz_dtptab % s));

	/* Add in room for the sorted coercion table; we use number of fi   */
	/* tab entries minus the point where the coercions begin (which	    */
	/* gives us one more than we need because of the fi tab sentinel,   */
	/* but we keep the extra one since the sorted coercion table has a  */
	/* sentinel too)						    */
	/* ---------------------------------------------------------------- */
	sz_coerctab = (	Adi_fis_size / sizeof(ADI_FI_DESC)
			- ADZ_COERCION_TY_FIIDX + ADF_FUDGE_FACTOR)
                        * sizeof(ADI_COERC_ENT);
				/* added entries for dtfamily hack */

	if (sz_coerctab % s)
	    sz_coerctab += (s - (sz_coerctab % s));

	/* the server cb's size is the sum of the sizes of its constituents */
	sz_srvcb =	sz_srvhd
		    +	sz_fxtab
		    +	sz_dttab
		    +	sz_dtptab
		    +	sz_coerctab
		    +	sz_coerctab;
	/*
	** Set static for size of operator table -- useful in debugging the
	** adding of new adt's code
	*/
	sz_optab = Adi_3RO_ops_size;
	if (sz_optab % s)
	    sz_optab += (s - (sz_optab % s));
    }

    return (sz_srvcb);
}


/*{
** Name: adg_startup() - Initialize ADF for the server.
**
** Description:
**      This function is the external ADF call "adg_startup()" which
**      initializes the ADF for the server.  This will set up certain global
**	tables of information that ADF will use.  A call to this routine must
**      precede all calls to the adg_init() routine. In the INGRES backend
**      server, adg_startup() will be called once for the life of the server,
**      and adg_init() will be called once for each session.  In a non-server
**      process such as an INGRES frontend these two routines will be called
**      once, probably back-to-back. 
**
** Inputs:
**      adf_srv_cb			    Ptr to memory to be used as ADF's
**					    server control block.
**	adf_size			    Number of bytes reserved at
**					    adf_srv_cb for ADF's server CB.
**	adf_dbi_tab			    Ptr to table of dbmsinfo() requests
**					    that ADF's dbmsinfo() function will
**					    recognize.  This table MUST remain
**					    accessable for the life of the
**					    server.  NOTE:  If there are no
**					    dbmsinfo() requests, this argument
**					    can be specified as NULL.
**	c2secure			    True if C2 security.
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK			    Operation succeeded.
**	    E_DB_FATAL    		    Fatal startup error...ADF cannot
**					    proceed.  Either there was not
**					    enough memory allocated for ADF's
**					    server CB, or some other internal
**					    ADF error occured during server
**					    startup.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The memory pointed to by adf_srv_cb will be initialized to become
**	    ADF's server control block.
**
** History:
**      23-jul-86 (thurston)
**          Initial creation and coding.
**	16-oct-86 (thurston)
**	    Added call to ad0_opinit().
**	28-oct-86 (thurston)
**	    Added ult_init_macro().
**	25-feb-87 (thurston)
**	    Changed the interface to accept some memory and initialize it as
**	    ADF's server control block, which will take the place of all ADF
**	    GLOBALDEFs that were not READONLY.
**	10-apr-87 (thurston)
**	    Added the adf_dbi_tab argument, and call to ad0_dbiinit() to
**	    process it.
**	08-mar-89 (jrb)
**	    Removed call to ad0_opinit and initialized fi related server cb
**	    variables using sizeof globals and using #define'd constants in
**	    new adfopfis.h header.  Also, the operations table is no longer put
**	    into the server cb, but the pointer to it has been left there.
**	05-feb-91 (stec)
**	    Added a call to ad0_hfinit().
**      09-mar-92 (jrb, merged by stevet)
**          Init pointer to FEXI table and initialize table itself.
**	23-Sep-1992 (fred)
**	    Added support for Bit min/max initialization.
**	17-jul-2000 (rucmi01) Part of fix for bug 101768.
**	    Add code to deal with special case where adg_startup is called 
**      more than once in the same process.
**	19-Jan-2001 (jenjo02)
**	    Added ADI_B1SECURE, ADI_C2SECURE Adi_status flags
**	    so that ADF can be more selective when calling
**	    security functions.
**      29-mar-2002 (gupsh01)
**          Initialize the Adu_maptbl.
**	23-jan-2004 (stephenb)
**	    Set double-byte status
**	23-Jan-2004 (gupsh01)
**	    Added code to identify and read the mapping file for unicode
**	    coercion. 
**	04-Feb-2004 (gupsh01)
**	    Added error handling from adu_readmap(). 
**	17-Feb-2004 (gupsh01)
**	    Added check if the read_map fails for the platform converter, 
**	    we should load the default mapping file. This may be called 
**	    from the FEadfcb calls during installation, when we do not 
**	    have too much information available to us.
**	14-Jun-2004 (schka24)
**	    Safe environment variable handling;  check charset == "UTF8",
**	    not just UTF8 as a prefix.
**  18-Jan-2004 (fanra01)
**      If adg_startup is called more than once in a process, as is the case
**      in the icesvr, then the global initialization of the string data max.
**      and min. values can cause a memory leak and exception during
**      adg_shutdown.
**	16-Jun-2005 (gupsh01)
**	    Moved adg_init_unimap() to this file from adgshutdown.c.
**	    Added adg_setnfc 
**	11-Jul-2005 (schka24)
**	    Remove double decl of "env" caused by x-integration.
**	24-oct-2006 (dougi)
**	    Add function to validate sequence of default FI table entries.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
*/

DB_STATUS
adg_startup(
PTR		    adf_srv_cb,
i4		    adf_size,
ADF_TAB_DBMSINFO    *adf_dbi_tab,
i4		    c2secure)
{
    DB_STATUS  		db_stat;
    ADF_SERVER_CB	*scb = (ADF_SERVER_CB *) adf_srv_cb;
    DB_ERROR		error;
    i4			ret_code;
    i4			i;
    char		chname[MAX_LOC];
    char		converter[MAX_LOC];
    char		*env = 0;

    if (adf_size < adg_srv_size())
	return (E_DB_FATAL);

    /*
    ** Bug 101768.
    ** There is one case where adg_startup can be called more than once by
    ** the same process.  This is when OpenROAD starts an application which
    ** uses the OpenAPI. OpenROAD calls FEadbcb() which calls adg_startup.
    ** OpenAPI calls IIapi_initADF() which calls adg_startup. 
    ** 
    ** This causes a crash when OpenAPI exits. IIapi_termADF() calls 
    ** adg_shutdown() and then frees the server cb which it has set Adf_globs
    ** to point to.  OpenROAD uses the same Adf_globs pointer, which is now
    ** pointing to freed memory, and it crashes.
    **
    ** Since both FEadfcb() and IIapi_initADF() initialize the server CB in the
    ** the 2 can successfully share the same Adf_globs.
    ** The code has been changed here so that if adg_startup is called more
    ** than once Adf_globs is not changed and so OpenROAD and OpenAPI will
    ** share the same ADF_SERVER_CB.
    **
    ** This will not affect any other use of this code since nothing changes
    ** unless adg_startup() is called more than once.
    */
    if (!adg_startup_instances++)
        Adf_globs = scb;

    /* Set up standard header portion of server CB */
    /* ------------------------------------------- */
    scb->adf_next = scb->adf_prev = NULL;
    scb->adf_length = adf_size;
    scb->adf_type = ADSRV_TYPE;
/*  scb->adf_owner = ?????  -- Don't set owner; ADF doesn't own this struct */
    scb->adf_ascii_id = ADSRV_ASCII_ID;

    /* set the unicode coercion table pointer to NULL for now */
    /* ------------------------------------------------------ */
    scb->Adu_maptbl = NULL;

#ifndef VMS
    /* Read maptables for the character set              */
    /* ------------------------------------------------------ */
    if ((db_stat = adu_getconverter (converter)) == E_DB_OK)
      db_stat = adu_readmap (converter);

    /* If for some reason we haven't been able to obtain the converter 
    ** normally or have read the converter map file then fall back to 
    ** default.
    */ 
    if (db_stat) 
      db_stat = adu_readmap ("default");

    if (db_stat)
    {
#ifdef xOPTAB_PRINT
	TRdisplay("adg_startup() ]returning with adu_readmap() error: %d\n",
		    (int) db_stat);
#endif
	return (db_stat);
    }

#ifdef xOPTAB_PRINT
    TRdisplay("adg_startup(): read the converter values as %s\n", converter); 
#endif
#endif /* VMS*/
    /* Call it inited */
    /* -------------- */
    scb->Adi_inited = adg_startup_instances;

    scb->Adi_status = 0;
    if ( c2secure )
	scb->Adi_status |= ADI_C2SECURE;

    NMgtAt(ERx("II_INSTALLATION"), &env);
    if (env && *env)
    {
        STprintf(chname, ERx("II_CHARSET%s"), env);
        NMgtAt(chname, &env);
    }
    else
    {
        NMgtAt(ERx("II_CHARSET"), &env);
    }

    if (env && *env)
    {
        if (STbcompare (env, 4, "UTF8", 4, 1) == 0 )
	    scb->Adi_status |= ADI_UTF8;
    }     

    CMget_charset_name(&chname[0]);
    if (STcasecmp (chname, "UTF8") == 0 )
	scb->Adi_status |= ADI_UTF8;

    if (CMischarset_doublebyte())
    {
        scb->Adi_status |= ADI_DBLBYTE;
        if (CMischarset_utf8())
	{	
	  scb->Adi_status |= ADI_UTF8;
	  /* FIX ME - Technically UTF8 is not actually a double byte. But for
	  ** the sake of simplicity we are adding the ADI_DBLBYTE flag so that
	  ** the CM routines are called for UTF8 code and a single byte 
	  ** character is not assumed.
	  */
	  scb->Adi_status |= ADI_DBLBYTE;
	}
    }

    /* Set up the dbmsinfo stuff */
    /* ------------------------- */
    if (db_stat = ad0_dbiinit(adf_dbi_tab, scb))
    {
#ifdef xOPTAB_PRINT
	TRdisplay("adg_startup() ]returning with ad0_dbiinit() error: %d\n",
		    (int) db_stat);
#endif
	return (db_stat);
    }


    /* Init the tracing vector */
    /* ----------------------- */
    ult_init_macro(&scb->Adf_trvect, 128, 0, 8);

    /* Set up the table pointers */
    /* ------------------------- */
    scb->Adi_fexi	= (ADI_FEXI *)	    ((char *)scb + sz_srvhd);
    scb->Adi_datatypes  = (ADI_DATATYPE  *) ((char *)scb + sz_srvhd
							 + sz_fxtab);
    scb->Adi_dtptrs     = (ADI_DATATYPE **) ((char *)scb + sz_srvhd
							 + sz_fxtab
							 + sz_dttab);
    scb->Adi_tab_coerc  = (ADI_COERC_ENT *) ((char *)scb + sz_srvhd
							 + sz_fxtab
							 + sz_dttab
							 + sz_dtptab);

    /* Set up the fitab constants in the server CB */
    /* ------------------------------------------- */

    /* Get number of fis; subtract one for sentinel */
    scb->Adi_num_fis = Adi_fis_size / sizeof(ADI_FI_DESC) - 1;

    /* Get biggest fi id; minus one for sentinel and one since we're	    */
    /* indexing from 0 instead of from 1				    */
    scb->Adi_fi_biggest = Adi_fil_size / sizeof(ADI_FI_LOOKUP) - 2;

    /* Set indexes into fi tab for each fi type	*/
    scb->Adi_comparison_fis	= ADZ_COMPARISON_TY_FIIDX;
    scb->Adi_operator_fis	= ADZ_OPERATOR_TY_FIIDX;
    scb->Adi_agg_func_fis	= ADZ_AGG_FUNC_TY_FIIDX;
    scb->Adi_norm_func_fis	= ADZ_NORM_FUNC_TY_FIIDX;
    scb->Adi_pred_func_fis	= -1;
    scb->Adi_copy_coercion_fis	= ADZ_COPY_COERCION_TY_FIIDX;
    scb->Adi_coercion_fis	= ADZ_COERCION_TY_FIIDX;

    /* Point to the operations table */
    /* ----------------------------- */
    scb->Adi_operations = (ADI_OPRATION *)&Adi_3RO_operations[0];
    scb->Adi_op_size = Adi_3RO_ops_size;
    scb->Adi_fis = (ADI_FI_DESC *)Adi_fis;
    scb->Adi_fi_size = Adi_fis_size;
    scb->Adi_fi_lkup = (ADI_FI_LOOKUP *)Adi_fi_lkup;
    scb->Adi_fil_size = Adi_fil_size;
    scb->Adi_fi_rt_biggest = (Adi_fil_size/(
		    (char *) &scb->Adi_fi_lkup[1] -
			(char *) &scb->Adi_fi_lkup[0])) - 2;
    scb->Adi_fi_int_biggest = scb->Adi_fi_rt_biggest;

	    /*
	    ** Subtract 2 -- one is the NULL entry at the end (signifying
	    ** the end of the list), the other is because the use of this
	    ** field is really as an index offset into the array --
	    ** therefore we must convert it to be consistent with C's use of
	    ** zero-based array addressing.
	    */


#ifdef xOPTAB_PRINT
    {
	TRdisplay("\n\nOperations table:\n");
	adb_optab_print();
    }
#endif /* xOPTAB_PRINT */

    /* Initialize the FEXI table */
    /* ------------------------- */
    for (i = 0; i < ADI_SZ_FEXI; i++)
	scb->Adi_fexi[i].adi_fcn_fexi = NULL;

    /* Set up the datatypes and datatype pointers tables */
    /* ------------------------------------------------- */
    if (db_stat = ad0_dtinit(scb, (ADD_DEFINITION *) 0,
			(ADI_DATATYPE *) scb->Adi_datatypes,
			(ADI_DATATYPE **) scb->Adi_dtptrs,
			(ADI_COERC_ENT *) scb->Adi_tab_coerc,
			&error))
    {
#ifdef xOPTAB_PRINT
	TRdisplay("adg_startup() returning with ad0_dtinit() error: %d\n",
		    (int) db_stat);
#endif
	return (db_stat);
    }

    /* Validate sort sequence of FI table entries. */
    /* ------------------------------------------- */
    if (db_stat = ad0_fivalidate(scb, &error))
    {
#ifdef xOPTAB_PRINT
	TRdisplay("adg_startup() returning with ad0_fivalidate() error: %d\n",
		    (int) db_stat);
#endif
	return (db_stat);
    }

    /* Initialize the MO classes */
    ADFmo_attach_adg();

    /* Initialize data strings for maximum and minimum values. */
    /*
    ** Initialise the data strings only if this is the first time that this
    ** code is executed.
    ** adg_startup can be called multiple times if a threaded backend server
    ** includes client OpenAPI code or embedded code.  This causes and
    ** exception within a second call to adg_shutdown as the memory has
    ** already been freed.
    ** adg_startup_instance was incremented previously in this function
    ** hence the test for 1.
    */
    if (adg_startup_instances == 1)
    {
        Chr_min = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Chr_max = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Cha_min = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Cha_max = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Txt_max = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Vch_min = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Vch_max = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Lke_min = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Lke_max = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Bit_min = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        Bit_max = (u_char *)MEreqmem(0, DB_MAXSTRING + 1, FALSE, NULL);
        MEfill(DB_MAXSTRING, (u_char)MIN_CHAR, (PTR)&Chr_min[0]);
        MEfill(DB_MAXSTRING, (u_char)IIMAX_CHAR_VALUE, (PTR)&Chr_max[0]);
        MEfill(DB_MAXSTRING, (u_char)0, (PTR)&Cha_min[0]);
        MEfill(DB_MAXSTRING, (u_char)0xff, (PTR)&Cha_max[0]);
        MEfill(DB_MAXSTRING, (u_char)AD_MAX_TEXT, (PTR)&Txt_max[0]);
        MEfill(DB_MAXSTRING, (u_char)0, (PTR)&Vch_min[0]);
        MEfill(DB_MAXSTRING, (u_char)0xff, (PTR)&Vch_max[0]);
        MEfill(DB_MAXSTRING, (u_char)0, (PTR)&Lke_min[0]);
        MEfill(DB_MAXSTRING, (u_char)0xff, (PTR)&Lke_max[0]);
        MEfill(DB_MAXSTRING, (u_char)0, (PTR)&Bit_min[0]);
        MEfill(DB_MAXSTRING, (u_char) ~0, (PTR)&Bit_max[0]);
        Chr_min[DB_MAXSTRING] = '\0';
        Chr_max[DB_MAXSTRING] = '\0';
        Cha_min[DB_MAXSTRING] = '\0';
        Cha_max[DB_MAXSTRING] = '\0';
        Txt_max[DB_MAXSTRING] = '\0';
        Vch_min[DB_MAXSTRING] = '\0';
        Vch_max[DB_MAXSTRING] = '\0';
        Lke_min[DB_MAXSTRING] = '\0';
        Lke_max[DB_MAXSTRING] = '\0';
        Bit_min[DB_MAXSTRING] = '\0';
        Bit_max[DB_MAXSTRING] = '\0';
    }
    /*	Initialize floating-point values used in histogram processing. */
    if ((ret_code = ad0_hfinit(&scb->Adc_hdec)) != 0)
    {
#ifdef xOPTAB_PRINT
	TRdisplay("adg_startup() returning with ad0_hfinit() error: %d\n",
		    (int) ret_code);
#endif
	return (db_stat);
    }

    return (E_DB_OK);
}

/*{
** Name: adg_sz_augment	- Determine the necessary memory space to add objects
**
** Description:
**      This routine determines the memory space necessary to add objects.  This
**	is done this way (just like adg_srv_size() above) because ADF runs
**	`transparently' in the DBMS, STAR, or FE Runtime system;  because of
**	this, ADF doesn't know how to allocate memory -- so it relies upon its
**	callers to perform that function. 
**
**      This routine manages by figuring that the internal state is correct --
**	that is that adg_srv_size() has been previously called, and that the
**	state that was left around is correct.  Assuming this, it takes the
**	information provide by the ADD_DEFINITION structure and figuring out the
**	size additions necessary for the new objects. 
**
** Inputs:
**      new_objects                     Pointer to the ADD_DEFINITION structure
**					defining the new objects.
**
** Outputs:
**      *error                           Appropriate error.
**	Returns:
**	    size of memory (in bytes) necessary to add these members.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Jan-1989 (fred)
**          Created.
**      18-apr-89 (fred)
**	    Added exception handler.  See file level comment.
**      27-aug-92 (stevet)
**          Added room for FEXI table.
**	12-july-06 (dougi)
**	    Replicate earlier hack for type family coercion structures.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
[@history_template@]...
*/
i4
adg_sz_augment(
ADD_DEFINITION     *new_objects,
DB_ERROR           *error)
{
    i4		current_size;
    i4		size_object;    
    ADI_FI_DESC		*fi = (ADI_FI_DESC *) &Adf_globs->Adi_fis[0];
    ADI_FI_DESC		*tfi;
    i4		new_size = -1;
    ADI_FI_ID		largest_fi;
    i4			num_fis;
    i4			num_cos;
    EX_CONTEXT		ex_context;
		/*
		** If we fail, ensure failure of some outer routine by
		** returning -1 as the amount of memory to allocate.  If the
		** outer scope does not correctly check the error and tries to
		** allocate the memory, it will fail attempting to allocate 4
		** gigabytes (assuming 32 bit machines) of memory.
		*/

    for (;;)
    {
	if (!error)
	    break;	    /* cannot set error */

	number_of_exceptions = 0;

	if (EXdeclare(ad0_handler, &ex_context))
	{
	    /* Increment so as not to loop infinitely in the case of *error */
	    /* being invalid						    */

	    if (!number_of_exceptions++)
		error->err_code = E_AD8012_UD_ADDRESS;
	    new_size = -1;	/* might be optimized away */
	    break;
	}
	
	error->err_code = E_AD9000_BAD_PARAM;
	if (!new_objects ||
	    (new_objects->add_count !=
		    (new_objects->add_dt_cnt +
		     new_objects->add_fo_cnt +
		     new_objects->add_fi_cnt) )
	   )
	    break;

	error->err_code = E_AD9001_BAD_CALL_SEQUENCE;
	if (!sz_srvcb)
	    break;

	if (new_objects->add_trace & ADD_T_LOG_MASK)
	{
	    TRdisplay(
"Adg_sz_augment: Adding %d. datatypes, %d. functions, & %d. fi's: Total: %d.\n",
		new_objects->add_dt_cnt,
		new_objects->add_fo_cnt,
		new_objects->add_fi_cnt,
		new_objects->add_count);
	}
	new_size = sz_srvhd + sz_fxtab;

	/*
	** Throughout this section, current_size refers to the incremental size
	** change in the table type currently under consideration.  These table
	** types are operations (and functions), datatypes, and function
	** instances.
	**
	** The first to be dealt with (paralleling adg_srv_size()) is the
	** operation table.
	*/
	
	size_object = (char *) &Adi_3RO_operations[1] -
			    (char *) &Adi_3RO_operations[0];
	current_size = new_objects->add_fo_cnt * size_object;
	if (current_size % sizeof(ALIGN_RESTRICT))
	    current_size += (sizeof(ALIGN_RESTRICT) -
			    (current_size % sizeof(ALIGN_RESTRICT)));

#ifdef	xOPTAB_PRINT
	TRdisplay("Adg_sz_augment: adding %d. operators/%x bytes to %x byte \
optab creating new total of %x (%<%d.) bytes\n",
	    new_objects->add_fo_cnt,
	    current_size,
	    sz_optab,
	    sz_optab + current_size);
#endif
	sz_optab += current_size;
	new_size += sz_optab;

	/*
	** Next we deal with the datatype table.  For this table, there are
	** really two tables.  There is an underlying datatype table, and a set
	** of pointers into that table...
	*/
	
	size_object = (char *) &Adi_1RO_datatypes[1] -
			    (char *) &Adi_1RO_datatypes[0];
	current_size = new_objects->add_dt_cnt * size_object;
	if (current_size % sizeof(ALIGN_RESTRICT))
	    current_size += (sizeof(ALIGN_RESTRICT) -
			    (current_size % sizeof(ALIGN_RESTRICT)));

#ifdef	xOPTAB_PRINT
	TRdisplay("Adg_sz_augment: adding %d. datatypes/%x bytes to %x byte \
dttab creating new total of %x (%<%d.) bytes\n",
	    new_objects->add_dt_cnt,
	    current_size,
	    sz_dttab,
	    sz_dttab + current_size);
#endif
	sz_dttab += current_size;
	new_size += sz_dttab;
			    
	current_size = new_objects->add_dt_cnt * sizeof(ADI_DATATYPE *);
	if (current_size % sizeof(ALIGN_RESTRICT))
	    current_size += (sizeof(ALIGN_RESTRICT) -
			    (current_size % sizeof(ALIGN_RESTRICT)));

	sz_dtptab += current_size;
	new_size += sz_dtptab;

	/*
	** Next, we compute the new size for the function instance table
	*/

	tfi = fi;
	num_fis = 2;	/* Count sentinel at end of table... */
	num_cos = 0;
	largest_fi = Adf_globs->Adi_fi_biggest;
	while ((fi++)->adi_finstid != ADFI_ENDTAB)  /* go past last coercion */
	{
	    if (fi->adi_finstid > largest_fi)
		largest_fi = fi->adi_finstid;
	    num_fis++;
	    if (fi->adi_fitype == ADI_COERCION)
		num_cos++;
	}
	
	{
	    /*
	    ** Here, we need to find the largest function instance id number.
	    ** This value is necessary because the lookup table is indexed by
	    ** function instance number -- therefore, it's size is directly
	    ** related to the largest number found.  Otherspaces are empty,
	    ** and will be zero filled by our caller.
	    **
	    ** In the previous loop, the largest value so far will have been
	    ** located.  This number is now compared with all the new values to
	    ** find the new winner...
	    */
	    i4			    i;
	    ADD_FI_DFN		    *ni = new_objects->add_fi_dfn;

	    for (i = 0;
		i < new_objects->add_fi_cnt && ni;
		i++, ni++)
	    {
		if (ni->fid_id >
						largest_fi)
		{
		    largest_fi = ni->fid_id;
		}
	    }
	}

	/* Now map largest_fi into usable array space -- this is done by    */
	/* asking for the LK_MAP macro value for largest_fi		    */

#ifdef	xOPTAB_PRINT
	TRdisplay("Adg_sz_augment: adding %d. fi's -- total of %d. fi's\n",
	    new_objects->add_fi_cnt,
	    num_fis + new_objects->add_fi_cnt - 1);
	TRdisplay("Adg_sz_augment: new largest fi %d. -- mapped to %d. \
-- old largest was %d.\n",
	    largest_fi,
	    ADI_LK_MAP_MACRO(largest_fi),
	    Adf_globs->Adi_fi_rt_biggest);
#endif

	largest_fi = ADI_LK_MAP_MACRO(largest_fi);
	    
	size_object = (char *) &Adi_fis[1] - (char *) &Adi_fis[0];
	current_size = ((char *) fi - (char *) tfi) +
			    (new_objects->add_fi_cnt * size_object);
	if (current_size % sizeof(ALIGN_RESTRICT))
	    current_size += (sizeof(ALIGN_RESTRICT) -
			    (current_size % sizeof(ALIGN_RESTRICT)));
	sz_fitab = current_size;
	new_size += sz_fitab;
#ifdef	xOPTAB_PRINT
	TRdisplay("Adg_sz_augment: New fitab size %x -- original size %x\n",
	    sz_fitab,
	    Adi_fis_size);
#endif

	
	/* Also need size for FI lookup table... */

	size_object = (char *)&Adi_fi_lkup[1] - (char *) &Adi_fi_lkup[0];
	current_size = (largest_fi + 2) * size_object;	/* One for end, one */
							/* 'cause zero	    */
							/* based...	    */
	if (current_size % sizeof(ALIGN_RESTRICT))
	    current_size += (sizeof(ALIGN_RESTRICT) -
			    (current_size % sizeof(ALIGN_RESTRICT)));
	new_size += current_size;
#ifdef	xOPTAB_PRINT
	TRdisplay("Adg_sz_augment: New fi_lkup_tab size %x -- original size %x\n",
	    current_size,
	    Adi_fil_size);
#endif
	
	{
	    i4		    i;
	    ADD_FI_DFN	    *ni = new_objects->add_fi_dfn;

	    for (i = 0;
		    i <	new_objects->add_fi_cnt && ni;
		    i++, ni++)
	    {
		if (ni->fid_optype == ADI_COERCION)
		    num_cos++;
	    }
	}

	num_cos += ADF_FUDGE_FACTOR;	/* replicate hack of adg_srv_size(). */
	current_size = (num_cos + 1) * sizeof(ADI_COERC_ENT);	/* + 1 for  */
								/* sentinel */
	if (current_size % sizeof(ALIGN_RESTRICT))
	    current_size += (sizeof(ALIGN_RESTRICT) -
			    (current_size % sizeof(ALIGN_RESTRICT)));

	sz_coerctab = current_size;
	new_size += sz_coerctab;

	sz_newadts = new_size;
	error->err_code = E_DB_OK;
	break;
    }

    if (number_of_exceptions)
	new_size = -1;
	
    EXdelete();
    return(new_size);
}

/*{
** Name: adg_augment	- Augment ADF with the addition of user-defined
**			    datatypes, functions, and operators.
**
** Description:
**      This routine takes a list of new adf objects and add's them to the known
**	repertoire.  It does this by copying to new space the tables which
**	are stored in memory by adg_startup().  The new space into which to copy
**	these tables must be passed into the call.  Size for this new space can
**	be calculated by using the routine adg_sz_augment() which will return
**	the size of the area needed.
**
**	Once the size is available, this routine is called.  It will take the
**	objects defined in the ADD_DEFINITION block passed in, add them to the
**	already known objects, and produce a new set of tables, changing ADF's
**	server control block to point at the new tables on successful completion.
**
** Inputs:
**      new_objects                     The ADD_DEFINITION block describing the
**					objects to be added.
**	table_size			Size of block of memory given to ADF
**      table_space                     Pointer to an area of memory of the
**					appropriate size.
**
** Outputs:
**	error				Appropriate errors for this routine.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**      This routine changes the underlying
**	tables used by ADF, and, in so doing,
**	changes the ADF session control block.
**	This block and these tables will now be
**	prepared to deal with the new objects
**	through normal ADF calls.  No other
**	special operations are necessary.
**
** History:
**      26-Jan-1989 (fred)
**          Created.
**      07-July-89 (fred)
**          Added checking that all coercions and all new function instances
**	    involve only known types.  Also removed non-ifdef'd reference to
**	    adb_optab_print() which, apparently, uses ulf, rendering it unusable
**	    in the frontends (esp. createdb, etc.).  Also added better trace
**	    information, specifically the printing of argument types for
**	    function instance creation.
**      27-aug-92 (stevet)
**          Added room for FEXI table.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Changed i4 save_i4 to PTR save_ptr as the type of adf_l_reserved
**	    has been changed to PTR.
[@history_template@]...
*/
DB_STATUS
adg_augment(
ADD_DEFINITION     *new_objects,
i4		   table_size,
PTR                table_space,
DB_ERROR           *error)
{
    DB_STATUS		db_stat = E_DB_ERROR;
    ADI_OPRATION        *new_operations;
    ADI_DATATYPE	*new_datatypes;
    ADI_DATATYPE	**new_p_datatypes;
    ADI_COERC_ENT	*new_coercions;
    ADI_FI_DESC		*new_fis;
    ADI_FI_LOOKUP	*new_fi_lkup;
    ADF_SERVER_CB	*scb = Adf_globs;
    ADF_SERVER_CB	*old_scb = Adf_globs;
    EX_CONTEXT		ex_context;

	
    for (;;)
    {
	if (!error)
	    break;
	    
	error->err_code = E_AD9000_BAD_PARAM;
	if (!table_space || !new_objects)
	    break;
	    
	number_of_exceptions = 0;
	
	if (EXdeclare(ad0_handler, &ex_context))
	{
	    if (!number_of_exceptions++)	/* Don't loop infinitely if *error  */
	    {				/* is wrong...			    */
		error->err_code = E_AD8012_UD_ADDRESS;
	    }
	    break;
	}

	error->err_code = E_AD9001_BAD_CALL_SEQUENCE;
	/*
	** It is illegal to call this routine if
	**   adg_sv_size() has not been successfully called,
	**   or adg_startup() has not been successfully called,
	**   or adg_sz_augment() has not been successfully called,
	**   or adg_augment() HAS been successfully called.
	*/
	if (!sz_srvcb || !Adf_globs || !sz_newadts
		|| !Adf_globs->Adi_fis)
	    break;	    /* Must have initialized the system first	    */

	error->err_code = E_AD9002_AUGMENT_SIZE;
	if (table_size < sz_newadts)
	    break;
	error->err_code = E_AD9003_TABLE_INIT;

	/*
	**  This routine very much parallels adg_startup above.  The various
	**  tables are set up in much the same way -- however, some need not be
	**  reset.
	*/

	/* No need to reset the dbmsinfo tables -- no user extension	    */
	/* Similarly, trace vectors are not reinitialized		    */

	/* Now we calculate the new areas for the various tables.  These    */
	/* will be reasigned into the adf_scb later...			    */
	
	new_operations = (ADI_OPRATION *) ((char *) table_space + sz_srvhd
								+ sz_fxtab);
	new_datatypes = (ADI_DATATYPE *) ((char *) table_space  + sz_srvhd
								+ sz_fxtab
								+ sz_optab);
	new_p_datatypes = (ADI_DATATYPE **) ((char *) table_space + sz_srvhd
	     							  + sz_fxtab
								  + sz_optab
								  + sz_dttab);
	new_coercions = (ADI_COERC_ENT *)  ((char *) table_space + sz_srvhd
								 + sz_fxtab
								 + sz_optab
								 + sz_dttab
								 + sz_dtptab);
	new_fis = (ADI_FI_DESC *)   ((char *) table_space  + sz_srvhd
								 + sz_fxtab
								 + sz_optab
								 + sz_dttab
								 + sz_dtptab
								 + sz_coerctab);
	new_fi_lkup = (ADI_FI_LOOKUP *)   ((char *) table_space  + sz_srvhd
								 + sz_fxtab
								 + sz_optab
								 + sz_dttab
								 + sz_dtptab
								 + sz_coerctab
								 + sz_fitab);
	{
	    i2	    save_i2;
	    PTR	    save_ptr;


	    save_i2 = ((ADF_SERVER_CB *) table_space)->adf_s_reserved;
	    save_ptr = ((ADF_SERVER_CB *) table_space)->adf_l_reserved;
	    
	    /* Zero fill for error checking later... */
	    
	    MEfill(table_size, '\0', (PTR) table_space);
	
	    MEcopy((PTR) scb, (sz_srvhd + sz_fxtab), (PTR) table_space);
	    
	    /* Set up new server	    */
	    /* control block	    */
	    
	    Adf_globs = scb = (ADF_SERVER_CB *) table_space;
	    Adf_globs->adf_s_reserved = save_i2;
	    Adf_globs->adf_l_reserved = save_ptr;
	    Adf_globs->adf_length = table_size;
	    
	    /* Set up new pointer for FEXI table */
	    scb->Adi_fexi = (ADI_FEXI *)((char *)scb + sz_srvhd);
	}
		
	/* Set up the operations table and the fitab constants in the server CB */
	/* -------------------------------------------------------------------- */
	if (db_stat = ad0_opinit(scb, new_objects, new_operations,
						    new_fis, new_fi_lkup, error))
	{
#ifdef xOPTAB_PRINT
	    TRdisplay("adg_augment() returning with ad0_opinit() error: %d\n",
			(int) db_stat);
#endif
	    break;
	}

	/* Set up the datatypes and datatype pointers tables */
	/* ------------------------------------------------- */
	if (db_stat = ad0_dtinit(scb, new_objects, new_datatypes,
				    new_p_datatypes, new_coercions, error))
	{
#ifdef xOPTAB_PRINT
	    TRdisplay("adg_augment() returning with ad0_dtinit() error: %d\n",
			(int) db_stat);
#endif
	    break;
	}

	error->err_code = E_DB_OK;
	break;
    }
    /*
    ** Need to check both db_stat & number of exceptions.  This is because some
    ** optimizing compilers will not reset db_stat at the top of this routine,
    ** because they don't know that we may arrive there ``indirectly.''  When
    ** all compilers and the CL committee understand and approve some sort of
    ** "volatile" handling, we can get around this problem.
    */
    if (db_stat || number_of_exceptions)
	Adf_globs = old_scb;	    /* Restore to operational state */
    EXdelete();
    return(db_stat);
}

/*
** Name: ad0_namesame() - Compare two names for equality, dbmsinfo style.
**
** Description:
**      Looks at two dbmsinfo type names, and returns 0 if they are equal, and
**	non-zero if they are not.
**
** Inputs:
**	dbi_name			dbmsinfo() name ... assumed lower case.
**      kname				ADF query constant name ... assumed
**					lower case and NULL terminated.
**
** Outputs:
**      none
**
**	Returns:
**	    TRUE	if names are same
**	    FALSE	if names are not same
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-sep-87 (thurston)
**          Initial creation.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
*/

static bool
ad0_namesame(
char		    *dbi_name,
char		    *kname)
{
    register char	*c1 = dbi_name;
    register char	*c2 = kname;
    register i4	i;


    for (i = 0; i < ADF_MAXNAME; i++)
    {
	if (*c1++ != *c2)
	    return (FALSE);
	    
	if (*c2++ == EOS)
	    return (TRUE);
    }
    
    return (*c2 == EOS);
}


/*
** Name: ad0_dbiinit() - Initialize table of dbmsinfo() requests.
**
** Description:
**      Initializes the dbmsinfo() requests that ADF will recognize.  It will
**	lowercase all request names in the table.  It is legal for the caller
**	to supply the ptr to the request table as NULL, which means dbmsinfo()
**	will not recognize any requests.
**
** Inputs:
**	dbi_tab				Ptr to ADF_TAB_DBMSIFO structure.
**	    .tdbi_numreqs		Number of dbmsinfo() requests in table.
**	    .tdbi_reqs[]		Array of dbmsinfo() requests.
**      scb				Ptr to ADF's server control block.
**
** Outputs:
**      scb
**	    .Adi_num_dbis		Number of dbmsinfo() requests in table.
**	    .Adi_dbi			Ptr to (array of) dbmsinfo() requests.
**	    .Adk_bintim_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_bintim' constant.
**		.adk_dbi		dbmsinfo() request record for _bintim.
**		.adk_kbit		ADF query constant bit for _bintim.
**	    .Adk_cpu_ms_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_cpu_ms' constant.
**		.adk_dbi		dbmsinfo() request record for _cpu_ms.
**		.adk_kbit		ADF query constant bit for _cpu_ms.
**	    .Adk_et_sec_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_et_sec' constant.
**		.adk_dbi		dbmsinfo() request record for _et_sec.
**		.adk_kbit		ADF query constant bit for _et_sec.
**	    .Adk_dio_cnt_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_dio_cnt' constant.
**		.adk_dbi		dbmsinfo() request record for _dio_cnt.
**		.adk_kbit		ADF query constant bit for _dio_cnt.
**	    .Adk_bio_cnt_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_bio_cnt' constant.
**		.adk_dbi		dbmsinfo() request record for _bio_cnt.
**		.adk_kbit		ADF query constant bit for _bio_cnt.
**	    .Adk_pfault_cnt_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_pfault_cnt' constant.
**		.adk_dbi		dbmsinfo() request record for
**                                      _pfault_cnt. 
**		.adk_kbit		ADF query constant bit for _pfault_cnt.
**	    .Adk_curr_date_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_current_date' constant.
**		.adk_dbi		dbmsinfo() request record for
**                                      _current_date. 
**		.adk_kbit		ADF query constant bit for _current_date.
**	    .Adk_curr_time_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_current_time' constant.
**		.adk_dbi		dbmsinfo() request record for
**                                      _current_time. 
**		.adk_kbit		ADF query constant bit for _current_time.
**	    .Adk_curr_tstmp_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_current_timestamp' constant.
**		.adk_dbi		dbmsinfo() request record for
**                                      _current_timestamp. 
**		.adk_kbit		ADF query constant bit for 
**					_current_timestamp.
**	    .Adk_local_time_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_local_time' constant.
**		.adk_dbi		dbmsinfo() request record for
**                                      _local_time. 
**		.adk_kbit		ADF query constant bit for _local_time.
**	    .Adk_local_tstmp_map		Mapping of dbmsinfo() request to
**					ADF query constant bit for the
**					`_local_timestamp' constant.
**		.adk_dbi		dbmsinfo() request record for
**                                      _local_timestamp. 
**		.adk_kbit		ADF query constant bit for 
**					_local_timestamp.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The memory being used for ADF's server control block will be written
**	    to in order to store the number of, and pointer to, dbmsinfo()
**	    requests in it.  Also, the memory used for the request table will
**	    be written on in order to lowercase all of the request names.
**
** History:
**	10-apr-87 (thurston)
**          Initial creation.
**	28-sep-87 (thurston)
**	    Now sets up the ADK_MAPs in the server CB for the query constants.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	19-aug-2006 (gupsh01)
**	    Added bits to support system constants, current_time, 
**	    current_timestamp, local_time, local_timestamp, current_date.
*/

static DB_STATUS
ad0_dbiinit(
ADF_TAB_DBMSINFO    *dbi_tab,
ADF_SERVER_CB	    *scb)
{
    ADF_DBMSINFO	*dbi;
    i4			i;
    i4			j;
    char		*c;
    ADK_MAP		*kmap;
    DB_STATUS		db_stat;


    if (    dbi_tab == NULL
	||  dbi_tab->tdbi_type != ADTDBI_TYPE
	||  dbi_tab->tdbi_ascii_id != ADTDBI_ASCII_ID
       )
    {
	scb->Adi_num_dbis = 0;
	scb->Adi_dbi      = NULL;

	db_stat = E_DB_OK;
    }
    else
    {
	scb->Adi_num_dbis =  dbi_tab->tdbi_numreqs;
	scb->Adi_dbi      = &dbi_tab->tdbi_reqs[0];
	
	scb->Adk_bintim_map.adk_dbi	    = NULL;
	scb->Adk_bintim_map.adk_kbit	    = ADK_BINTIM;
	scb->Adk_cpu_ms_map.adk_dbi	    = NULL;
	scb->Adk_cpu_ms_map.adk_kbit	    = ADK_CPU_MS;
	scb->Adk_et_sec_map.adk_dbi	    = NULL;
	scb->Adk_et_sec_map.adk_kbit	    = ADK_ET_SEC;
	scb->Adk_dio_cnt_map.adk_dbi	    = NULL;
	scb->Adk_dio_cnt_map.adk_kbit	    = ADK_DIO_CNT;
	scb->Adk_bio_cnt_map.adk_dbi	    = NULL;
	scb->Adk_bio_cnt_map.adk_kbit	    = ADK_BIO_CNT;
	scb->Adk_pfault_cnt_map.adk_dbi	    = NULL;
	scb->Adk_pfault_cnt_map.adk_kbit    = ADK_PFAULT_CNT;
	scb->Adk_curr_date_map.adk_dbi	    = NULL;
	scb->Adk_curr_date_map.adk_kbit     = ADK_CURR_DATE;
	scb->Adk_curr_time_map.adk_dbi	    = NULL;
	scb->Adk_curr_time_map.adk_kbit     = ADK_CURR_TIME;
	scb->Adk_curr_tstmp_map.adk_dbi     = NULL;
	scb->Adk_curr_tstmp_map.adk_kbit    = ADK_CURR_TSTMP;
	scb->Adk_local_time_map.adk_dbi     = NULL;
	scb->Adk_local_time_map.adk_kbit    = ADK_LOCAL_TIME;
	scb->Adk_local_tstmp_map.adk_dbi     = NULL;
	scb->Adk_local_tstmp_map.adk_kbit    = ADK_LOCAL_TSTMP;

	/* Now do validity checks, and lowercase all of the request names */
	/* -------------------------------------------------------------- */
	for (i = 0, dbi = scb->Adi_dbi; i < scb->Adi_num_dbis; i++, dbi++)
	{
	    db_stat = E_DB_FATAL;   /* Assume guilty until proven innocent */

	    /* Make sure function ptr is not NULL */
	    /* ---------------------------------- */
	    if (dbi->dbi_func == NULL)
		break;

	    /* Make sure # inputs is OK */
	    /* ------------------------ */
	    if (    dbi->dbi_num_inputs != 0
		/* {@fix_me@} ... When we allow inputs to dbmsinfo requests:
		&&  dbi->dbi_num_inputs != 1
		*/
	       )
		break;

	    /*******************************************************************
	    ** {@fix_me@} ... Remove the following check when we generalize   **
	    **		      the lenspec stuff; for now, must be fixed len.  **
	    *******************************************************************/
	    if (dbi->dbi_lenspec.adi_lncompute != ADI_FIXED)		    /**/
		break;							    /**/
	    /******************************************************************/

	    /* Result datatype must be intrinsic */
	    /* --------------------------------- */
	    if (    dbi->dbi_dtr != DB_INT_TYPE
		&&  dbi->dbi_dtr != DB_FLT_TYPE
		&&  dbi->dbi_dtr != DB_CHR_TYPE
		&&  dbi->dbi_dtr != DB_CHA_TYPE
		&&  dbi->dbi_dtr != DB_TXT_TYPE
		&&  dbi->dbi_dtr != DB_VCH_TYPE
	       )
		break;

	    /* All OK, so lowercase the request name */
	    /* ------------------------------------- */
	    for (j = 0, c = dbi->dbi_reqname; j < ADF_MAXNAME && *c; j++, c++)
		CMtolower(c, c);


	    /* Check to see if this is one of the ADF query constants and   */
	    /* store address of dbmsinfo() request record in appropriate    */
	    /* appropriate place if it is.				    */
	    /* ------------------------------------------------------------ */
	    if      (ad0_namesame(dbi->dbi_reqname, "_bintim"))
		scb->Adk_bintim_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "_cpu_ms"))
		scb->Adk_cpu_ms_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "_et_sec"))
		scb->Adk_et_sec_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "_dio_cnt"))
		scb->Adk_dio_cnt_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "_bio_cnt"))
		scb->Adk_bio_cnt_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "_pfault_cnt"))
		scb->Adk_pfault_cnt_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "current_date"))
		scb->Adk_curr_date_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "current_time"))
		scb->Adk_curr_time_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "current_timestamp"))
		scb->Adk_curr_tstmp_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "local_time"))
		scb->Adk_local_time_map.adk_dbi = dbi;
	    else if (ad0_namesame(dbi->dbi_reqname, "local_timestamp"))
		scb->Adk_local_tstmp_map.adk_dbi = dbi;

	    db_stat = E_DB_OK;
	}
    }

    return (db_stat);
}

/*
** Name: ad0_opinit() - Init the operations table and sets fitab constants.
**
** Description:
**      Initializes the operations table by copying the READONLY operations
**      structure into the server CB, and then setting up the pointer to the
**      group of function instances and the number of function instances for
**      each operater by looking through the function instance table. 
**
**	This table is all initialized at compile time except for the function
**	instance pointers and counts.
**
**	Also, set the various fitab constants in the server CB while going
**	through the fitab.
**
**	If .Adi_operations has already been filled, then it is expected that
**	this call is to add new operations as a result of user's adding
**	datatypes and operations.  In this case, the old .Adi_operations table
**	is copied, augmented by the information in the new_objects area,
**	and the .Adi_operations field is rewritten.
**
** Inputs:
**      scb				Ptr to ADF's server control block.
**	    .Adi_operations		Ptr to memory to the existing operations
**					table.  Assumed to be aligned on a
**					sizeof(ALIGN_RESTRICT) byte boundary.
**	new_objects			Ptr to ADD_DEFINITION block -- used
**					when new operations are being added to
**					the system.
**	new_operations			Ptr to memory to put the operations
**					table.  Assumed to be aligned on a
**					sizeof(ALIGN_RESTRICT) byte boundary.
**
** Outputs:
**      scb				Ptr to ADF's server control block.
**	    .Adi_operations		Where the operations table is
**          .Adi_comparison_fis		Where the comparisons start.
**          .Adi_operator_fis		Where the operators start.
**          .Adi_agg_func_fis		Where the agg_funcs start.
**          .Adi_norm_func_fis		Where the norm_funcs start.
**          .Adi_copy_coercion_fis	Where the copy coercions start.
**          .Adi_coercion_fis		Where the coercions start.
**          .Adi_num_fis		Total # of elements in fitab.
**          .Adi_fi_biggest		Largest fi ID found in fitab.
**	error->err_code		Filled to indicate error found in user
**					information.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The memory being used for ADF's server control block will be written
**	    to in order to put the operations table there.
**
** History:
**	16-oct-86 (thurston)
**          Initial creation.
**	07-jan-87 (thurston)
**	    Allowed there to be holes in the fitab by having the fi ID set to
**	    ADI_NOFI.  This allows easy removal of fi's from said table.
**	18-feb-87 (thurston)
**	    Added an extra check to make sure fitab has no holes in the
**	    middle of an op-group.
**	25-feb-87 (thurston)
**	    Changed to init operations table in ADF's server control block.
**	10-mar-87 (thurston)
**	    Added code to set the function instance table constants in the
**	    server CB.
**	25-mar-87 (thurston)
**	    Fixed bug where .Adi_num_fis was never initialized to 0.
**	24-jun-87 (thurston)
**	    Added code to handle copy coercions.
**	02-sep-87 (thurston)
**	    Fixed bug in the `do while' loop.  Was looping one two many times,
**	    and was not setting the biggest fi ID properly.
**	26-Jan-89 (fred)
**	    Added code and new parameters to deal with adding new operations
**	    after the server starts up...
**	14-jul-89 (fred)
**	    Added support for QUEL usage of user defined functions.
**	07-aug-89 (fred)
**	    Added code to check that the operation types match the function
**	    instance definition.  UDADT fix.
**      31-aug-1993 (stevet)
**          Added support for class objects which act very much like UDT
**          except it's DT, FI, OP are in 8392 ragne instead of 16784 range.
**	3-jul-96 (inkdo01)
**	    Added support for ADI_PRED_FUNC type (allows infix or function
**	    notation for user-defined predicate functions).
**	23-oct-05 (inkdo01)
**	    Changes to allow definition of aggregate functions for UDTs.
**	11-May-2007 (kschendel) SIR 122890
**	    Always log the most basic add-udf msg to the DBMS log.
*/

static DB_STATUS
ad0_opinit(
ADF_SERVER_CB	    *scb,
ADD_DEFINITION	    *new_objects,
ADI_OPRATION	    *new_operations,
ADI_FI_DESC	    *new_fis,
ADI_FI_LOOKUP	    *new_fi_lkup,
DB_ERROR	    *error)
{
    ADI_FI_DESC		*prev_fptr;
    ADI_FI_DESC		*fptr;
    ADI_FI_ID		prev_fid;
    ADI_FI_ID		fid;
    ADI_FI_ID           add_fistart;
    i4			k = 1;
    ADI_OP_ID           add_opstart;
    ADI_OP_ID		prev_opid;
    ADI_OP_ID		opid;
    ADI_OPRATION        *op;
    bool		found;
    bool		in_hole = FALSE;
    bool		in_coercions = FALSE;
    i4			where = -1;	/* Func type currently processing */
    i4		size;
    DB_STATUS		db_stat;


    /* First order of business is to copy old table into new area */
    /* ---------------------------------------------------------------- */
    if (new_objects)
    {
	op = scb->Adi_operations;
	size = scb->Adi_op_size;
    }
    else
    {
	op = (ADI_OPRATION *)&Adi_3RO_operations[0];
	size = Adi_3RO_ops_size;
    }
	
    MEcopy( (PTR)  op,
	    size,
	    (PTR)  new_operations);

    /* If there are more operations to be added, then add them */
    if (new_objects)
    {
	i4		i;
	i4		j;
	i4		end_of_list;
	ADD_FO_DFN	*no;	    /* New Operation */
	ADD_FO_DFN	*prev_op;
	i4		bad_name;
	char		*cp;

	/* handle both class objects and UDT */
	if( new_objects->add_type == ADD_CLSOBJ_TYPE)
	{
	    add_opstart = ADD_CLSOBJ_OPSTART;
	    add_fistart = ADD_CLSOBJ_FISTART;
	}
	else
	{
	    add_opstart = ADD_USER_OPSTART;
	    add_fistart = ADD_USER_FISTART;
	}

	end_of_list = new_objects->add_fo_cnt;
	op = new_operations;
	while (op->adi_opid != ADI_NOOP)    /* Find end of current table */
	    op++;

	/*
	** Here op is at the current EOL -- we will overwrite it and
	** replace it later...
	*/
	no = new_objects->add_fo_dfn;
	if (!no && new_objects->add_fo_cnt)
	{
	    if (new_objects->add_trace & ADD_T_LOG_MASK)
	    {
		TRdisplay(
"Ad0_opinit:  %d. operations requested, no def'ns found\n",
		    new_objects->add_fo_cnt);
	    }
	    error->err_code = E_AD8000_OP_COUNT_SPEC;
	    return(E_DB_ERROR);
	}
	for ( i = 0;
		i < end_of_list && no;
		i++, no++)
	{
	    bad_name = 0;
	    if (no->fod_object_type != ADD_O_OPERATOR)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_opinit: Invalid fod_object_type (%d./%<%x -- should be %d./%<%x) found\n\
             for operator addition %d. (zero based)\n",
			no->fod_object_type,
			ADD_O_OPERATOR,
			i);
		}
		error->err_code = E_AD8001_OBJECT_TYPE;
		return(E_DB_ERROR);
	    }
	    else if (no->fod_id < add_opstart)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_opinit:  Invalid operator id (%d./%<%x) for operator addition %d.\n",
			no->fod_id,
			i);
		}
		error->err_code = E_AD8002_OP_OPID;
		return(E_DB_ERROR);
	    }
	    else if (/*(no->fod_type < ADI_COMPARISON)
				&& (no->fod_type > ADI_COERCION)
		       Reinsert when we allow non-normal functions -- fred */
		     (no->fod_type != ADI_PRED_FUNC) &&
		     (no->fod_type != ADI_AGG_FUNC) &&
				(no->fod_type != ADI_NORM_FUNC))
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_opinit:  Invalid operator type (%d./%<%x) for operator addition %d.\n",
			no->fod_type,
			i);
		}
		error->err_code = E_AD8003_OP_OPTYPE;
		return(E_DB_ERROR);
	    }
	    else
	    {
		for (prev_op = new_objects->add_fo_dfn;
			prev_op != no;
			prev_op++)
		{
		    if (prev_op->fod_id == no->fod_id)
		    {
			if (new_objects->add_trace & ADD_T_LOG_MASK)
			{
			    TRdisplay(
"Ad0_opinit:  Duplicate operator id (%d./%<%x) found \n\
        for operators %t' and '%t'\n",
			    no->fod_id,
			    STnlength(sizeof(no->fod_name), 
				    (char *)&no->fod_name),
				    &no->fod_name,
			    STnlength(sizeof(prev_op->fod_name),
				      (char *)&prev_op->fod_name),
				    &prev_op->fod_name);
			}
			error->err_code = E_AD8016_OP_DUPLICATE;
			return(E_DB_ERROR);
		    }
		}
	    }

	    cp = no->fod_name.adi_opname;
	    if (!CMnmstart(cp) || (CMalpha(cp) && !CMlower(cp)))
	    {
		bad_name = 1;
	    }
	    else
	    {
		for (j = 0, CMbyteinc(j, cp), CMnext(cp);
			    j < (i4)sizeof(no->fod_name) && *cp;
			CMbyteinc(j, cp), CMnext(cp))
		{
		    if (!CMnmchar(cp) || (CMalpha(cp) && !CMlower(cp)))
		    {
			bad_name = 1;
			break;
		    }
		}
	    }
	    if (bad_name)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_opinit: Invalid name <%t> for operator addition %d. (zero based)\n",
			STnlength(sizeof(no->fod_name), (char *)&no->fod_name),
			&no->fod_name);
		}
		error->err_code = E_AD8015_OP_NAME;
		return(E_DB_ERROR);
	    }
		
	    op->adi_opid = no->fod_id;
	    STRUCT_ASSIGN_MACRO(no->fod_name, op->adi_opname);
	    op->adi_optype = no->fod_type;
	    if ((op->adi_optype == ADI_COMPARISON)
	      || (op->adi_optype == ADI_OPERATOR))
		op->adi_opuse = ADI_INFIX;
	    else
		op->adi_opuse = ADI_PREFIX;
	    op->adi_opqlangs = DB_SQL | DB_QUEL;
	    op->adi_opsystems = ADI_INGRES_6;
	    op->adi_opcntfi = 0;
	    op->adi_optabfidx = (ADI_FI_ID) 0;
	    /* This trdisplay is sufficiently useful that we'll do it
	    ** all the time...
	    */
	    TRdisplay(
"Ad0_opinit: Adding operator %t (%d./%<%x), fod_type %d./%<%x\n",
			STnlength(sizeof(no->fod_name), (char *)&no->fod_name),
			&no->fod_name,
			no->fod_id,
			no->fod_type);
	    size += sizeof(ADI_OPRATION);
	    op++;	/* Move on to next operation */
	}
	op->adi_opid = ADI_NOOP;
	if (i != new_objects->add_fo_cnt)
	{
	    if (new_objects->add_trace & ADD_T_LOG_MASK)
	    {
		TRdisplay(
"Ad0_opinit: Number added (%d.) is not equal to number 	requested (%d.)\n",
		    i, new_objects->add_fo_cnt);
	    }
	    error->err_code = E_AD8004_OP_COUNT_WRONG;
	    return(E_DB_ERROR);
	}
    }

    /*
    ** At this point, the new operations table is complete.
    ** Set the scb to point there
    */
    
    scb->Adi_operations = new_operations;
    scb->Adi_op_size = size;

    if (db_stat = ad0_fiinit(scb, new_objects, new_fis, new_fi_lkup, error))
    {
#ifdef xOPTAB_PRINT
	TRdisplay("Ad0_opinit() returning with ad0_fiinit() error: %d\n",
		    (int) db_stat);
#endif
	return(db_stat);
    }

    prev_fptr = (ADI_FI_DESC *) &scb->Adi_fis[0];
    prev_fid = (ADI_FI_ID) 0;
    prev_opid = prev_fptr->adi_fiopid;
    fptr = (ADI_FI_DESC *) &scb->Adi_fis[1];
    fid = (ADI_FI_ID) 0;
    
    /* Now we can initialize the function instance pointers and counts */
    /* --------------------------------------------------------------- */
    scb->Adi_num_fis = 1;				/* Initial value */
    scb->Adi_fi_biggest = prev_fptr->adi_finstid;	/* Initial value */
    do
    {
	fid++;
	scb->Adi_num_fis++;

	if (	prev_fptr->adi_fitype == ADI_COPY_COERCION
	    ||	prev_fptr->adi_fitype == ADI_COERCION
	   )
	    in_coercions = TRUE;

	if (fptr->adi_finstid == ADI_NOFI)
	{
	    /* Hole in fitab ... skip it and go to next fidesc. */
	    /* ------------------------------------------------ */
	    in_hole = TRUE;
	    continue;
	}

	/* Look for fi ID bigger than biggest one so far. */
	/* ---------------------------------------------- */
	if (fptr->adi_finstid > scb->Adi_fi_biggest)
	    scb->Adi_fi_biggest = fptr->adi_finstid;

	/* Look for start of new func type, and set appropriate index */
	/* ---------------------------------------------------------- */
	if (where != prev_fptr->adi_fitype)
	{
	    where = prev_fptr->adi_fitype;
	    switch (where)
	    {
	      case ADI_COMPARISON:
		scb->Adi_comparison_fis = scb->Adi_num_fis - 2;
		break;

	      case ADI_OPERATOR:
		scb->Adi_operator_fis = scb->Adi_num_fis - 2;
		break;

	      case ADI_AGG_FUNC:
		scb->Adi_agg_func_fis = scb->Adi_num_fis - 2;
		break;

	      case ADI_NORM_FUNC:
		scb->Adi_norm_func_fis = scb->Adi_num_fis - 2;
		break;

	      case ADI_PRED_FUNC:
		scb->Adi_pred_func_fis = scb->Adi_num_fis - 2;
		break;	/* new guy in the block */

	      case ADI_COPY_COERCION:
		scb->Adi_copy_coercion_fis = scb->Adi_num_fis - 2;
		break;

	      case ADI_COERCION:
		scb->Adi_coercion_fis = scb->Adi_num_fis - 2;
		break;

	      default:
		return (E_DB_FATAL);	/* Unknown func type found in fitab */
	    }
	}

	/* Once in COERCIONS, do not do optab processing. */
	/* ---------------------------------------------- */
	if (in_coercions)
	{
	    prev_fptr = fptr;
	    continue;		/* Do not do op-table processing on coercions */
	}

		
	/* Avoids search for operation in the table for functions */
	/* that use new still underfined operators.               */
	/* -------------------------------------------------------*/
	if (prev_opid==ADI_DISABLED){
		
		opid = fptr->adi_fiopid;
		k = 1;
		prev_fptr = fptr;
		prev_opid = opid;
		prev_fid = fid;
		continue;
	}


	if ((opid = fptr->adi_fiopid) == prev_opid)
	{
	    if (in_hole)
		return (E_DB_FATAL);	/* Hole in middle of op-group */
	    k++;
	}
	else
	{

	    in_hole = FALSE;

	    /* prev_fptr points to the starting fidesc for this
	    ** operation, while k is the # of fi's for the operation.
	    */
	    
	    /* First, find the operation in the op table */
	    found = FALSE;
	    for (op = scb->Adi_operations; op->adi_opid != ADI_NOOP; op++)
	    {
		if (op->adi_opid == prev_opid)
		{
		    ADI_FI_DESC	    *local_fptr = prev_fptr;
		    i4		    fcount;
		    
		    found = TRUE;
		    op->adi_opcntfi = k;
		    op->adi_optabfidx = prev_fid;

		    /* Check to see that operation types match */

		    for (fcount = 0; fcount < k; fcount++, local_fptr++)
		    {
			if (op->adi_optype != local_fptr->adi_fitype)
			{
			    if ((prev_opid >= add_opstart) ||
				    (local_fptr->adi_finstid >= add_fistart))
			    { 
				if (new_objects &&
				    (new_objects->add_trace & ADD_T_LOG_MASK))
				{
				    TRdisplay(
"Ad0_opinit:  Function instance number %d./%<%x with opid (%d./%<%x) \n\
has an operation type (%d./%<%x) which does not match that\n\
of the the operator (%d./%<%x).\n",
				    local_fptr->adi_finstid,
				    prev_opid,
				    local_fptr->adi_fitype,
				    op->adi_optype);
				}
				error->err_code = E_AD8019_OPTYPE_MISMATCH;
				return(E_DB_ERROR);
				    /* Not bad when its the user's	    */
				    /* fault			    */
			    }
			    return (E_DB_FATAL);	/* op not found in op table */
			}
		    }
		    /* op->adi_optabfi = prev_fptr; */
		    /*
		    ** Can't "break;" here because there may be synonyms in the
		    ** operations table, in which case more than one entry will
		    ** have the same op ID, and should therefore be filled in
		    ** with the same fi stuff.
		    */
		}
	    }

	    if (found)
	    {
		/* Now reset k and prev_fptr */
		k = 1;
		prev_fptr = fptr;
		prev_opid = opid;
		prev_fid = fid;
	    }
	    else
	    {
		if (prev_opid >= add_opstart)
		{ 
		    if (new_objects &&
			    (new_objects->add_trace & ADD_T_LOG_MASK))
		    {
			TRdisplay(
"Ad0_opinit:  Function instance number %d./%<%x has opid (%d./%<%x) \n\
not found in operator table.\n",
			    prev_fid,
			    prev_opid);
		    }
		    error->err_code = E_AD8002_OP_OPID;
		    return(E_DB_ERROR);	/* Not bad when it's the user's	    */
					/* fault			    */
		}
		return (E_DB_FATAL);	/* op not found in op table */
	    }
	}
    } while ((++fptr)->adi_finstid != ADFI_ENDTAB);
    /*
    ** Note that the above do-while loop is assuming the group of fi's for
    ** COERCIONS is the last group in the fitab, and the group of fi's for
    ** COPY COERCIONS is the next to last group in the fitab, and that none
    ** of the COERCIONS and COPY COERCIONS have op IDs.
    */

#ifdef	xOPTAB_PRINT
    if (new_objects && new_objects->add_trace & ADD_T_LOG_MASK)
    {
	TRdisplay("\n\nOperations table after ad0_opinit():\n");
	TRdisplay(    "====================================\n\n");
	adb_optab_print();
    }
#endif
    
    return (E_DB_OK);
}


/*
** Name: ad0_dtinit() - Initialize the datatypes and datatype pointers tables.
**
** Description:
**      Initializes the datatypes and datatype pointers tables by copying the
**      READONLY datatypes structure into the server CB, doing BTset() calls to
**      set up the legal coercions found in the function instance table, and
**      then setting up the datatype pointers table from the datatypes table. 
**
**	The datatypes table is all initialized at compile time except for the
**	coercion bit masks, which must be done at run time in order to use
**	BTset.
**
** Inputs:
**      scb				Ptr to ADF's server control block.
**	    .Adi_datatypes		Ptr to memory to put the datatypes
**					table.  Assumed to be aligned on a
**					sizeof(ALIGN_RESTRICT) byte boundary.
**	    .Adi_dtptrs			Ptr to memory to put the datatype ptrs
**	new_objects			Ptr to arena of new datatypes,
**					functions, and operators being added.
**	new_datatypes			Ptr to memory to put the datatypes
**					table.  Assumed to be aligned on a
**					sizeof(ALIGN_RESTRICT) byte boundary.
**	new_p_datatypes			Ptr to memory to put the datatype ptrs
**					table.  Assumed to be aligned on a
**					sizeof(ALIGN_RESTRICT) byte boundary.
**	new_coercions			Ptr to memory to put the coercions
**					table.  Assumed to be aligned on a
**					sizeof(ALIGN_RESTRICT) byte boundary.
**
** Outputs:
**	scb
**	    .Adi_datatypes		Datatypes table will be written here.
**	    .Adi_dtptrs			Datatype pointers table will be
**					initialized here.
**	    .Adi_ents_coerc		Number of entries in the sorted coercion
**					table will be placed here.
**	error				Filled with appropriate error for user
**					defined datatypes.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    The memory being used for ADF's server control block will be
**	    written to in order to put the datatypes and datatype pointers
**	    tables there, as well as the sorted coercion table.
**
** History:
**	20-mar-86 (thurston)
**          Initial creation.
**	25-mar-86 (thurston)
**          Added some calls to BTset() for some coercions that were missing.
**	30-jun-86 (thurston)
**	    Made this routine a static routine since it should not be seen
**	    outside of this file.
**	3-jul-86 (ericj)
**	    Add BTset() calls so that coercions from a datatype to itself
**	    will be valid.
**	24-sep-86 (thurston)
**	    Added all of the BTset() calls to handle the 37 new coercions
**	    necessary to support "char", "varchar", and "longtext".
**	25-feb-87 (thurston)
**	    Changed to return DB_STATUS and to init tables in ADF's server
**	    control block.
**	25-mar-87 (thurston)
**	    Modified this routine to look through the coercions in the function
**	    instance table in order to set up the coercion bit masks for each
**	    datatype.  Previously, it was hard-coded as to what coercions to
**	    set, which was (a) a pain in the a-- when adding new coercions to
**	    the system, and (b) ripe for inconsistency.
**	06-oct-88 (thurston)
**	    Added code to initialize the sorted coercion table, and put
**	    appropriate pointers into it from the datatypes table.
**	26-Jan-89 (fred)
**	    Added code to support adding new datatypes, etc. to the system for
**	    full user-defined datatype support.
**	29-mar-90 (jrb)
**	    Added support for internal datatypes being added (this means we
**	    don't do certain consistency checks).
**	09-mar-92 (jrb, merged by stevet)
**	    Added consistency check to ensure no DB_ALL_TYPE's are allowed in
**	    coercion section of function instance table.
**	09-mar-92 (stevet)
**	    Skip datatype checking for DB_ALL_TYPE, this cause problem when
**	    UDTs are loaded.
**       5-May-1993 (fred)
**          Added large OME object support.  This involves moving the
**          datatype attributes into the base structure.  It also
**          requires the propogation of the datatype transformation
**          function for cases where the datatype attributes mark the
**          datatype as peripheral.
**      31-aug-1993 (stevet)
**          Added support for class objects which act very much like UDT
**          except it's DT, FI, OP are in 8392 ragne instead of 16784 range.
**	16-feb-1995 (shero03)
**	    Bug #65565
**	    Copy the xform addr to the adp_xform field
**	    when the datatype is a peripheral type
**	04-Jun-1998 (shero03)
**	    Remove minmaxdv from the HISTOGRAM set of routines.
**      10-aug-1999 (chash01)
**          the original code of testing looks like this 
**             if ( !((...)&&(...)&&(...))
**                 &&
**                   (nd->dtd_object_type == ADD_O_INTERNDT) )
**             {.......}
**
**          When this test is rewritten, it is done in equivalent
**          form ( de Morgan's rule) with some additions. However,
**          the mistake was made and the test became
**             if ( (!(...)||!(...)||!(...))
**                 ||
**                  (nd->dtd_object_type != ADD_O_INTERNDT) )
**             {.......}
**
**          The correct test should look like 
**             if ( (!(...)||!(...)||!(...))
**                 &&
**                  (nd->dtd_object_type != ADD_O_INTERNDT) )
**             {.......}
**
**    The de Morgan Rule should apply only to the scope that
**    is enclosed by the parenthesis immediately after the NOT(!)
**    operator.      
**	29-Mar-02 (gordy)
**	    Renamed unused adi_tpl_num to 'unused'.
**	13-Mar-2006 (kschendel)
**	    Check FI operands properly, don't hardwire to 2.
**	28-apr-06 (dougi)
**	    Add coercions involving data type siblings.
**	11-May-2007 (kschendel) SIR 122890
**	    Always log the most basic add-datatype msg to the dbms log.
**	22-Oct-07 (kiria01) b119354
**	    Split adi_dtcoerce into a quel and SQL version
*/

static DB_STATUS
ad0_dtinit(
ADF_SERVER_CB	    *scb,
ADD_DEFINITION	    *new_objects,
ADI_DATATYPE	    *new_datatypes,
ADI_DATATYPE	    **new_p_datatypes,
ADI_COERC_ENT	    *new_coercions,
DB_ERROR	    *error)
{
    DB_DT_ID		hold_from;
    DB_DT_ID		hold_into;
    ADI_FI_ID		hold_fid;
    ADI_DATATYPE	**dtp;
    ADI_DATATYPE	*dt, *dt1;
    ADI_FI_DESC		*fi;
    ADI_DT_BITMASK	zeromask;
    i4			dt_nat;
    i4			i;
    i4			j;
    i4			n_ent;
    i4			max_ents_coerc;
    ADI_COERC_ENT	*c_ent;
    i4			size;
    i4			bad_name;
    i4                  one_space;
    char		*cp;
    bool		fsib, isib;


    /* First order of business is to copy the old dt table into server CB */
    /* ------------------------------------------------------------------- */
    if (new_objects)
    {
	dt = scb->Adi_datatypes;
	size = scb->Adi_dt_size;
    }
    else
    {
	dt = (ADI_DATATYPE *)&Adi_1RO_datatypes[0];
	size = Adi_dts_size;
    }
	
    MEcopy( (PTR)  dt,
	    size,
	    (PTR)  new_datatypes);
    if (new_objects)
    {
	ADD_DT_DFN		*nd;
	DB_DT_ID     add_high, add_low;
	
	/* handle both class objects and UDT */
	if( new_objects->add_type == ADD_CLSOBJ_TYPE)
	{
	    add_high = ADD_HIGH_CLSOBJ;
	    add_low  = ADD_LOW_CLSOBJ;
	}
	else
	{
	    add_high = ADD_HIGH_USER;
	    add_low  = ADD_LOW_USER;
	}

	dt = new_datatypes;
	while (dt->adi_dtid != DB_NODT)
	    dt++;
	
	nd = new_objects->add_dt_dfn;
	if ((!nd && new_objects->add_dt_cnt)
	    || (nd && !new_objects->add_dt_cnt))
	{
	    if (new_objects->add_trace & ADD_T_LOG_MASK)
		TRdisplay("Ad0_dtinit: No datatype definitions specified\n");
	    error->err_code = E_AD8005_DT_COUNT_SPEC;
	    return(E_DB_ERROR);
	}
	for (i = 0; nd && i < new_objects->add_dt_cnt; i++, nd++, dt++)
	{
	    bad_name = 0;
     
	    /* Make sure block type is correct -- minor check but...	    */

	    if (    nd->dtd_object_type != ADD_O_DATATYPE
		&&  nd->dtd_object_type != ADD_O_INTERNDT)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_dtinit: Invalid dtd_object_type (%d./%<%x -- should be %d./%<%x) found\n\
             for datatype number %d. (zero based)\n",
			nd->dtd_object_type,
			ADD_O_DATATYPE,
			i);
		}
		error->err_code = E_AD8001_OBJECT_TYPE;
		return(E_DB_ERROR);
	    }
	    else if ((nd->dtd_id < add_low) ||
		     (nd->dtd_id > add_high))
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_dtinit:  Invalid datatype id (dtd_id) field (%d./%<%x) \n\
             -- should be between %d./%<%x and %d./%<%x --\n\
             for datatype number %d. (zero based)\n",
			nd->dtd_id, add_low, add_high, i);
		}
		error->err_code = E_AD8006_DT_DATATYPE_ID;
		return(E_DB_ERROR);
	    }
	    else if (nd->dtd_attributes &
		       ~(AD_NOKEY | AD_NOSORT |
			 AD_NOHISTOGRAM | AD_PERIPHERAL | AD_VARIABLE_LEN) )
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_dtinit: Invalid dtd_attributes (%d./%<%x)\n\
             for datatype number %d. (zero based)\n",
			      nd->dtd_attributes, i);
		}
	        error->err_code = E_AD8006_DT_DATATYPE_ID;  /* {@fix_me@} */
		return(E_DB_ERROR);
	    }
	    else if (nd->dtd_attributes &
		       (AD_NOKEY | AD_NOSORT |
			AD_NOHISTOGRAM | AD_PERIPHERAL | AD_VARIABLE_LEN))
	    {
		/*
		** If any bits set, make sure they match up.
		** The rules are as follows:
		**    Peripheral implies (nosort and nokey and
		**                          nohisto), and
		**    No sort implies no keying.
		** Other combinations are legal.
		*/

		if (
		    ( (nd->dtd_attributes & AD_PERIPHERAL)
		        && ((nd->dtd_attributes &
			        (AD_NOSORT | AD_NOKEY | AD_NOHISTOGRAM))
		             != (AD_NOSORT | AD_NOKEY | AD_NOHISTOGRAM)))
		    ||
		        ((nd->dtd_attributes & AD_NOSORT) && 
		          ((nd->dtd_attributes & AD_NOKEY) == 0))
		    )
		{
		    if (new_objects->add_trace & ADD_T_LOG_MASK)
		    {
			TRdisplay(
"Ad0_dtinit: Invalid combination of dtd_attributes (%d./%<%x)\n\
             for datatype number %d. (zero based)\n",
				  nd->dtd_attributes, i);
		    }
		    error->err_code = E_AD8006_DT_DATATYPE_ID;  /* {@fix_me@} */
		    return(E_DB_ERROR);
		}
		if (nd->dtd_attributes & AD_PERIPHERAL)
		{
		    /*
		    ** If this is a peripheral datatype, then check to
		    ** see that it has a valid underlying datatype.
		    ** The only check we will make here is existance
		    ** that the underlying type could exist.
		    */

		    if ((scb->Adi_dtptrs[
			   ADI_DT_MAP_MACRO(nd->dtd_underlying_id)]
			               == 0)
			&& ((nd->dtd_underlying_id < add_low)
			    ||(nd->dtd_underlying_id > add_high)
			    )
			)
		    {
			if (new_objects->add_trace & ADD_T_LOG_MASK)
			{
			    TRdisplay(
"Ad0_dtinit: Invalid underlying type (%d./%<%x) for dtd_id (%d./%<%x)\n\
             for datatype number %d. (zero based)\n",
				      nd->dtd_underlying_id, 
				      nd->dtd_id, i);
			}
			error->err_code = E_AD8006_DT_DATATYPE_ID;
			                                 /* {@fix_me@} */
			return(E_DB_ERROR);
		    }
		}
	    }

	    cp = nd->dtd_name.adi_dtname;
	    if (nd->dtd_object_type != ADD_O_INTERNDT)
	    {
		if (!CMnmstart(cp) || (CMalpha(cp) && !CMlower(cp)))
		{
		    bad_name = 1;
		}
		else
		{
		    one_space = 0; /* Datatype names with one space */
				   /* are cool now... "long varchar" */

		    for (j = 0, CMbyteinc(j, cp), CMnext(cp);
				j < (i4)sizeof(nd->dtd_name) && *cp;
			    CMbyteinc(j, cp), CMnext(cp))
		    {
			if (CMspace(cp))
			{
			    if (one_space++)
			    {
				bad_name = 1;
				break;
			    }
			}
			else if (!CMnmchar(cp)
				    || (CMalpha(cp) && !CMlower(cp)))
			{
			    bad_name = 1;
			    break;
			}
		    }
		}
		if (bad_name)
		{
		    if (new_objects->add_trace & ADD_T_LOG_MASK)
		    {
			TRdisplay(
    "Ad0_dtinit: Invalid name <%t> for datatype number %d. (zero based)\n",
			    STnlength(sizeof(nd->dtd_name), 
				      (char *)&nd->dtd_name),
			    &nd->dtd_name);
		    }
		    error->err_code = E_AD8014_DT_NAME;
		    return(E_DB_ERROR);
		}
	    }

	    /* Memory has been zero'd by caller -- only init nonzero items  */
	    STRUCT_ASSIGN_MACRO(nd->dtd_name, dt->adi_dtname);
	    dt->adi_dtid = nd->dtd_id;
	    dt->adi_dtstat_bits =
		(AD_INDB | AD_USER_DEFINED | AD_CONDEXPORT)
		    | nd->dtd_attributes; 
	    if (dt->adi_dtstat_bits & AD_PERIPHERAL)
	    {
		dt->adi_under_dt = nd->dtd_underlying_id;
	    }
	    else
	    {
		dt->adi_under_dt = DB_NODT;
	    }
	    dt->unused = 0;
	    size += sizeof(ADI_DATATYPE);
	    
	    TRdisplay("Ad0_dtinit: Adding datatype %t (%d./%<%x) into table\n",
			STnlength(sizeof(dt->adi_dtname), 
				  (char *)&dt->adi_dtname),
			&dt->adi_dtname,
			dt->adi_dtid);

	    /*
	    ** Note that although this is ugly, it guarantees that any
	    ** assignment which is added or taken away will be tested for.
	    ** A rather high price to pay, but...
	    */
	    /*
            ** 10-aug-1999 (chash01)  the original code looks like this 
            **    if ( !((...)&&(...)&&(...))
            **         &&
            **         (nd->dtd_object_type != ADD_O_INTERNDT) )
            **    {.......}
            **
            **    When this test is rewritten, it is done in equivalent
            **    form ( de Morgan's rule) with some additions. However,
            **    the mistake was made and the test became
            **    if ( (!(...)||!(...)||!(...))
            **         ||
            **         (nd->dtd_object_type == ADD_O_INTERNDT) )
            **    {.......}
            **
            **    The correct test should look like 
            **    if ( (!(...)||!(...)||!(...))
            **         &&
            **         (nd->dtd_object_type != ADD_O_INTERNDT) )
            **    {.......}
            **
            **    The de Morgan Rule should apply only to the scope that
            **    is enclosed by the parenthesis immediately after the NOT(!)
            **    operator.      
            */
	    if ((
		((dt->adi_dt_com_vect.adp_lenchk_addr =
			nd->dtd_lenchk_addr) == 0) ||
		((dt->adi_dt_com_vect.adp_getempty_addr =
			nd->dtd_getempty_addr) == 0) ||
		((dt->adi_dt_com_vect.adp_tmlen_addr =
			nd->dtd_tmlen_addr) == 0) ||
		((dt->adi_dt_com_vect.adp_tmcvt_addr =
			nd->dtd_tmcvt_addr) == 0) ||
		((dt->adi_dt_com_vect.adp_dbtoev_addr =
			nd->dtd_dbtoev_addr) == 0) ||
		((dt->adi_dt_com_vect.adp_valchk_addr =
			nd->dtd_valchk_addr) == 0) ||

		(((dt->adi_dtstat_bits & AD_NOSORT) == 0) &&
		 ((dt->adi_dt_com_vect.adp_compare_addr = 
		    	nd->dtd_compare_addr) == 0)) ||

        (((dt->adi_dtstat_bits & AD_NOKEY) == 0) &&
	     (((dt->adi_dt_com_vect.adp_keybld_addr = 
		 	nd->dtd_keybld_addr) == 0) ||
		  ((dt->adi_dt_com_vect.adp_minmaxdv_addr =
			nd->dtd_minmaxdv_addr) == 0) ||
	      ((dt->adi_dt_com_vect.adp_hashprep_addr = 
	       		nd->dtd_hashprep_addr) == 0)) ) ||

		(((dt->adi_dtstat_bits & AD_VARIABLE_LEN) != 0) &&
		 ((dt->adi_dt_com_vect.adp_compr_addr = 
			nd->dtd_compr_addr) == 0) ) ||

		(((dt->adi_dtstat_bits & AD_NOHISTOGRAM) == 0) &&
		 (((dt->adi_dt_com_vect.adp_helem_addr =
			nd->dtd_helem_addr) == 0)  ||
		  ((dt->adi_dt_com_vect.adp_hmin_addr =
			nd->dtd_hmin_addr) == 0)  ||
		  ((dt->adi_dt_com_vect.adp_hmax_addr =
			nd->dtd_hmax_addr) == 0)  ||
		  ((dt->adi_dt_com_vect.adp_dhmin_addr =
			nd->dtd_dhmin_addr) == 0) ||
		  ((dt->adi_dt_com_vect.adp_dhmax_addr =
		        nd->dtd_dhmax_addr) == 0) ||
		  ((dt->adi_dt_com_vect.adp_hg_dtln_addr =
		       	nd->dtd_hg_dtln_addr) == 0)) ) ||

         (((dt->adi_dtstat_bits & AD_PERIPHERAL) != 0) &&
		 (((dt->adi_dt_com_vect.adp_seglen_addr =
			nd->dtd_seglen_addr) == 0)  ||
		  ((dt->adi_dt_com_vect.adp_xform_addr =  /* B65565 chk xform */
			nd->dtd_xform_addr) == 0))) )
                &&                                        /* chash01: correct*/
	        (nd->dtd_object_type != ADD_O_INTERNDT))  /* previous errors */
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay("One of the required function addresses is invalid\n\
             for datatype number %d. (zero based)\n", i);
		}
		error->err_code = E_AD8007_DT_FUNCTION_ADDRESS;
		return(E_DB_ERROR);
	    }
	}

	if (i != new_objects->add_dt_cnt)
	{
	    if (new_objects->add_trace & ADD_T_LOG_MASK)
	    {
		TRdisplay(
       "Ad0_dtinit: Number of datatypes(%d.) added != number requested (d.)\n",
			    i, new_objects->add_dt_cnt);
	    }
	    error->err_code = E_AD8008_DT_COUNT_WRONG;
	    return(E_DB_ERROR);
	}
	if (new_objects->add_trace & ADD_T_LOG_MASK)
	{
	    TRdisplay(
		"Ad0_dtinit: Successfully added %d. datatype definitions\n",
			i);
	}
	dt->adi_dtid = DB_NODT;   /* Mark end of list */
    }
    scb->Adi_dt_size = size;
    scb->Adi_datatypes = new_datatypes;

    /* Now set up the datatype ptrs table, zero the coercion bit masks, */
    /* and NULL out the pointer to sorted coercion table                */
    /* ---------------------------------------------------------------- */
    scb->Adi_dtptrs = new_p_datatypes;
    for (i = 0, dtp = scb->Adi_dtptrs; i <= ADI_MXDTS; *dtp++ = NULL, i++)
	;			/* NULL out dtptrs table */
    dt = scb->Adi_datatypes;
    MEfill(sizeof(zeromask), (u_char) 0, (PTR) &zeromask);
    while ((dt_nat = (i4) ADI_DT_MAP_MACRO(dt->adi_dtid)) != DB_NODT)
    {
	STRUCT_ASSIGN_MACRO(zeromask, dt->adi_dtcoerce_quel);
	STRUCT_ASSIGN_MACRO(zeromask, dt->adi_dtcoerce_sql);
	dt->adi_coerc_ents = NULL;

	if (scb->Adi_dtptrs[dt_nat] == NULL)	/* This is to keep synonyms */
	    scb->Adi_dtptrs[dt_nat] = dt;	/* from getting set twice.  */
	dt++;
    }

    /* Now, set coercion bit masks appropriately,        */
    /* and generate the sorted coercion table (unsorted) */
    /* ------------------------------------------------- */
    max_ents_coerc = (sz_coerctab / sizeof(ADI_COERC_ENT)) - 1;
    n_ent = 0;
    c_ent = scb->Adi_tab_coerc = new_coercions;
    for (fi = (ADI_FI_DESC *) &scb->Adi_fis[scb->Adi_coercion_fis];
	 fi->adi_fitype == ADI_COERCION  &&  fi->adi_finstid != ADFI_ENDTAB;
	 fi++
	)
    {
	if (fi->adi_finstid == ADI_NOFI)
	    continue;   /* Hole in fitab ... skip this one. */

	hold_from = fi->adi_dt[0];
	hold_into = fi->adi_dtresult;


	if (hold_from == DB_ALL_TYPE  ||  hold_into == DB_ALL_TYPE)
	{
	    /* This should never happen; DB_ALL_TYPE's are not allowed for
	    ** coercions.
	    */
	    error->err_code = E_AD9999_INTERNAL_ERROR;
	    return(new_objects ? E_DB_ERROR : E_DB_FATAL);
	}
	
	if (!scb->Adi_dtptrs[ADI_DT_MAP_MACRO(hold_into)])
	{
	    if (new_objects && (new_objects->add_trace & ADD_T_LOG_MASK))
	    {
		TRdisplay(
"Ad0_dtinit:  Function instance id %d./%<%x specifies an unknown\n\
 result type %d.(%<%x).\n",
		    fi->adi_finstid,
		    fi->adi_dtresult);
	    }
	    error->err_code = E_AD8017_INVALID_RESULT_TYPE;
	    return(new_objects ? E_DB_ERROR : E_DB_FATAL);
	}
		    
	dt = scb->Adi_dtptrs[ADI_DT_MAP_MACRO(hold_from)];
	if (!dt)	
	{
	    if (new_objects && (new_objects->add_trace & ADD_T_LOG_MASK))
	    {
		TRdisplay(
"Ad0_dtinit:  Function instance id %d./%<%x specifies an unknown\n\
 parameter type id %d.(%<%x).\n",
		    fi->adi_finstid,
		    fi->adi_dt[0]);
	    }
	    error->err_code = E_AD8018_INVALID_PARAM_TYPE;
	    return(new_objects ? E_DB_ERROR : E_DB_FATAL);
	}

	if ((fi->adi_fiflags & ADI_F4096_SQL_CLOAKED) == 0)
	    BTset((i4) ADI_DT_MAP_MACRO(hold_into), (char *) &dt->adi_dtcoerce_sql);
	if ((fi->adi_fiflags & ADI_F8192_QUEL_CLOAKED) == 0)
	    BTset((i4) ADI_DT_MAP_MACRO(hold_into), (char *) &dt->adi_dtcoerce_quel);

	if (++n_ent > max_ents_coerc)
	    return (E_DB_FATAL);

	c_ent->adi_from_dt = hold_from;
	c_ent->adi_into_dt = hold_into;
	c_ent->adi_fid_coerc = fi->adi_finstid;
	c_ent++;

	/* New code to check from, then into types to see if they have 
	** siblings in a data type family. If so, entry must be added for 
	** them, as well. */
	fsib = isib = FALSE;

	/* First check the "from" type. */
	for (i = 0; i < DB_DT_ILLEGAL; i++)
	{
	    if ((dt1 = scb->Adi_dtptrs[i]) == NULL ||
		dt1->adi_dtid == hold_from)
		continue;		/* no type - skip to next */

	    if (dt1->adi_dtfamily == hold_from)
	    {
		/* "from" type has a sibling. Add new entry. */
		fsib = TRUE;
		if ((fi->adi_fiflags & ADI_F4096_SQL_CLOAKED) == 0)
		    BTset((i4) ADI_DT_MAP_MACRO(hold_into),
				(char *) &dt1->adi_dtcoerce_sql);
		if ((fi->adi_fiflags & ADI_F8192_QUEL_CLOAKED) == 0)
		    BTset((i4) ADI_DT_MAP_MACRO(hold_into),
				(char *) &dt1->adi_dtcoerce_quel);
		if (++n_ent > max_ents_coerc)
{
		    TRdisplay(
"Ad0_dtinit:  Ran out of coerc ents %d type %d\n", n_ent, dt1->adi_dtid);
		    return (E_DB_FATAL);
}

		c_ent->adi_from_dt = dt1->adi_dtid;
		c_ent->adi_into_dt = hold_into;
		c_ent->adi_fid_coerc = fi->adi_finstid;
		c_ent++;
	    }
	}

	/* Then check the "into" type. */
	for (i = 0; i < DB_DT_ILLEGAL; i++)
	{
	    if ((dt1 = scb->Adi_dtptrs[i]) == NULL ||
		dt1->adi_dtid == hold_into)
		continue;		/* no type - skip to next */

	    if (dt1->adi_dtfamily == hold_into)
	    {
		/* "into" type has a sibling. Add new entry. */
		isib = TRUE;
		if ((fi->adi_fiflags & ADI_F4096_SQL_CLOAKED) == 0)
		    BTset((i4) ADI_DT_MAP_MACRO(dt1->adi_dtid),
				(char *) &dt->adi_dtcoerce_sql);
		if ((fi->adi_fiflags & ADI_F8192_QUEL_CLOAKED) == 0)
		    BTset((i4) ADI_DT_MAP_MACRO(dt1->adi_dtid),
				(char *) &dt->adi_dtcoerce_quel);
		if (++n_ent > max_ents_coerc)
{
		    TRdisplay(
"Ad0_dtinit:  Ran out of coerc ents %d type %d\n", n_ent, dt1->adi_dtid);
		    return (E_DB_FATAL);
}

		c_ent->adi_from_dt = hold_from;
		c_ent->adi_into_dt = dt1->adi_dtid;
		c_ent->adi_fid_coerc = fi->adi_finstid;
		c_ent++;
	    }
	}

	/* Finally, if there were siblings for both from and into 
	** types, a nested loop is required to combine them. */
	if (fsib && isib)
	 for (i = 0; i < DB_DT_ILLEGAL; i++)
	 {
	    if ((dt = scb->Adi_dtptrs[i]) == NULL ||
		dt->adi_dtid == hold_from)
		continue;		/* no type - skip to next */

	    if (dt->adi_dtfamily == hold_from)
	     for (j = 0; j < DB_DT_ILLEGAL; j++)
	     {
		if ((dt1 = scb->Adi_dtptrs[j]) == NULL ||
		    dt1->adi_dtid == hold_into)
		    continue;		/* no type - skip to next */

		if (dt1->adi_dtfamily == hold_into)
		{
		    /* "into" type has a sibling. Add new entry. */
		    if ((fi->adi_fiflags & ADI_F4096_SQL_CLOAKED) == 0)
			BTset((i4) ADI_DT_MAP_MACRO(dt1->adi_dtid),
				(char *) &dt->adi_dtcoerce_sql);
		    if ((fi->adi_fiflags & ADI_F8192_QUEL_CLOAKED) == 0)
			BTset((i4) ADI_DT_MAP_MACRO(dt1->adi_dtid),
				(char *) &dt->adi_dtcoerce_quel);
		    if (++n_ent > max_ents_coerc)
{
		    TRdisplay(
"Ad0_dtinit:  Ran out of coerc ents %d types %d %d\n", n_ent, dt->adi_dtid,
		dt1->adi_dtid);
			return (E_DB_FATAL);
}

		    c_ent->adi_from_dt = dt->adi_dtid;
		    c_ent->adi_into_dt = dt1->adi_dtid;
		    c_ent->adi_fid_coerc = fi->adi_finstid;
		    c_ent++;
		}
	     }
	 }
    }
    scb->Adi_ents_coerc = n_ent;
    c_ent->adi_from_dt = DB_NODT;	/*   /  set the  \   */
    c_ent->adi_into_dt = DB_NODT;	/*  (  sentinel   )  */
    c_ent->adi_fid_coerc = ADI_NOFI;	/*   \   entry   /   */

    /* Now the sorted coercion table has been filled up, we must sort it */
    /* ----------------------------------------------------------------- */
    /* JRBFIX: This algorithm could be improved.			 */

    c_ent = scb->Adi_tab_coerc;
    for (i = 0; i < n_ent-1; i++)
    {
	for (j = i+1; j < n_ent; j++)
	{
	    if (    (c_ent[j].adi_from_dt <  c_ent[i].adi_from_dt)
		||  (	(c_ent[j].adi_from_dt == c_ent[i].adi_from_dt)
		     &&	(c_ent[j].adi_into_dt <  c_ent[i].adi_into_dt)
		    )
	       )
	    {
		/* swap 'em */
		hold_from = c_ent[j].adi_from_dt;
		hold_into = c_ent[j].adi_into_dt;
		hold_fid  = c_ent[j].adi_fid_coerc;
		c_ent[j].adi_from_dt   = c_ent[i].adi_from_dt;
		c_ent[j].adi_into_dt   = c_ent[i].adi_into_dt;
		c_ent[j].adi_fid_coerc = c_ent[i].adi_fid_coerc;
		c_ent[i].adi_from_dt   = hold_from;
		c_ent[i].adi_into_dt   = hold_into;
		c_ent[i].adi_fid_coerc = hold_fid;
	    }
	}
    }

    /* Finally, set the ptrs for each datatype into the sorted coercion table */
    /* ---------------------------------------------------------------------- */
    hold_from = DB_NODT;
    for (i = 0; i < n_ent; i++)
    {
	if (c_ent[i].adi_from_dt != hold_from)
	{
	    dt = scb->Adi_dtptrs[ADI_DT_MAP_MACRO(c_ent[i].adi_from_dt)];
	    dt->adi_coerc_ents = &c_ent[i];
	    hold_from = c_ent[i].adi_from_dt;
	}
    }

    if (new_objects)
    {
	for (i = 0, fi = scb->Adi_fis;
		i < scb->Adi_coercion_fis;
		i++, fi++)
	{
	    if (fi->adi_finstid == ADI_NOFI)
		continue;   /* Hole in fitab ... skip this one. */

	    if (!scb->Adi_dtptrs[ADI_DT_MAP_MACRO(abs(fi->adi_dtresult))] &&
		fi->adi_dtresult != DB_ALL_TYPE)
	    {
		if (new_objects && (new_objects->add_trace & ADD_T_LOG_MASK))
		{
		    TRdisplay(
"Ad0_dtinit:  Function instance id %d./%<%x specifies an unknown\n\
 result type %d.(%<%x).\n",
			fi->adi_finstid,
			fi->adi_dtresult);
		}
		error->err_code = E_AD8017_INVALID_RESULT_TYPE;
		return(new_objects ? E_DB_ERROR : E_DB_FATAL);
	    }

	    if (fi->adi_numargs > 0)
	    {
		i4 j = fi->adi_numargs;
		DB_DT_ID *dtp = &fi->adi_dt[0];

		while (--j >= 0)
		{
		    if (*dtp != DB_ALL_TYPE
		      && scb->Adi_dtptrs[ADI_DT_MAP_MACRO(*dtp)] == NULL)
		    {
			if (new_objects && (new_objects->add_trace & ADD_T_LOG_MASK))
			{
			    TRdisplay(
"Ad0_dtinit:  Function instance id %d./%<%x specifies an unknown\n\
 parameter %d type id %d.(%<%x).\n",
			    fi->adi_finstid, fi->adi_numargs-j, *dtp);
			}
			error->err_code = E_AD8018_INVALID_PARAM_TYPE;
			return(new_objects ? E_DB_ERROR : E_DB_FATAL);
		    }
		    ++ dtp;
		}
	    }
	}
    }
    return (E_DB_OK);
}

/*{
** Name: ad0_fiinit	- Initialize the function instance tables
**
** Description:
**      This routine initializes the function instance tables.  If there are no
**	user-defined routines to add, then the pointers are simply aimed at the
**	readonly structures, and return controls to the caller.
**
**	If, though, there are user-defined function instances to add, the
**	readonly table is copied to the the new area, and the new data inserted.
**
**	As the function instance table is created, it must be kept in sorted
**	order.  The basic order is that copy_coercions must be next to last, and
**	coercions must be last.  Unfortunately, the #defines values for
**	ADI_COPY_COERCION and ADI_COERCION do not sort precisely this way.
**	Therefore, a comparison table is created below, and the optype values
**	are used to index into here to do the sort.  Thus, all values are
**	correct, except that the two mentioned above are sorted in reverse
**	order.
**
** Inputs:
**      scb                             Adf's server control block
**      new_objects                     Ptr to the control block describing the
**					new objects
**      new_fis                         Ptr to memory to be used for the new
**					function instance table.  Must be zero
**					filled.
**	new_fi_lkup			Ptr to memory to be use for the new
**					function instance lookup table.  Must be
**					zero filled.
**					
**
** Outputs:
**      scb                             Adf's server control block
**	    .Adi_fis			filled with the address of the new
**					control block
**	    .Adi_fi_lkup		Filled with addr of fi lookup table
**	error				Filled with appropriate error when user
**					defined datatypes are specified.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-Jan-89 (fred)
**          Created.
**      11-aug-89 (fred)
**	    Added support for overloading unary operations.
**	27-mar-90 (jrb)
**	    Fixed bug in tracing code for consistency checks on comparison
**	    function instances.
**      08-dec-1992 (stevet)
**          Added support for LEN_EXTERN support for UDT functions.  When
**          defined, user routine will be called to calculate result length
**          of an expression.
**      10-jan-1993 (stevet)
**          Change the order of FI, instead of loading the INGRES FI first
**          now it first load the UDT FI first.  This would avoid problem
**          when DB_ALL_TYPE option is added to function like locate().
**       5-May-1993 (fred)
**          Added large OME object support.  Principally, this
**          involves propogating the fiflags from fid_attributes (to
**          support the [non-aggregate] need for a workspace and to
**          note that a function instance involves indirect usage (so
**          we don't call DMF from deep within DMF).
**	11-jan-1995 (shero03)
**	    Remember the highest used new slot.  When adding
**	    spatial save this number.
**	3-jul-96 (inkdo01)
**	    Added support for ADI_PRED_FUNC type (allows infix or function
**	    notation for user-defined predicate functions) and new
**	    ADE_4CXI_CMP_SET_SKIP null handler required by PRED_FUNC.
**	23-oct-05 (inkdo01)
**	    Changes to allow definition of aggregate functions for UDTs.
**	12-Mar-2006 (kschendel)
**	    Allow multi-operand NORMAL functions;  add NONULLSKIP flag
**	    so that user FI can ask for no null pre-instruction.  Remove
**	    hardwired assumption that user FI has 0, 1, or 2 operands.
**	    Limit operands to max-1 because we need one slot for the result.
**	25-Apr-2006 (kschendel)
**	    Turns out that to make multi-param UDF's useful, we need countvec
**	    style lenspec calls.  Fix here.
**      17-Oct-2006 (hanal04) bug 116809
**          Initialise adi_psfixed to the value in the UDF result fid_rprec.
[@history_template@]...
*/

static DB_STATUS
ad0_fiinit(
ADF_SERVER_CB      *scb,
ADD_DEFINITION     *new_objects,
ADI_FI_DESC        *new_fis,
ADI_FI_LOOKUP      *new_fi_lkup,
DB_ERROR	   *error)
{
    i4		    i;
    i4		    j;
    i4		    max_j = 0;
    i4		    new_fi_limit;
    i4		    start;
    i4		    end;
    i4		    current;
    i4		    old;
    i4		    new;
    ADI_FI_ID       add_fistart;
    ADD_FI_DFN	    *ni;
    ADD_FI_DFN	    *nc;
    struct
    {
	i4		sort_order;	/* Must be sorted in this order */
	i4		l_num_args;	/* With at least this many	    */
					/* arguments, ...		    */
	i4		h_num_args;	/* but not more than this	    */
    }
    		    compare_table[8];

    compare_table[0].sort_order = 0;
    compare_table[0].l_num_args = 0;
    compare_table[0].h_num_args = 0;
    compare_table[ADI_COMPARISON].sort_order = 1;
    compare_table[ADI_COMPARISON].l_num_args = 2;
    compare_table[ADI_COMPARISON].h_num_args = 2;
    compare_table[ADI_OPERATOR].sort_order = 2;
    compare_table[ADI_OPERATOR].l_num_args = 1;
    compare_table[ADI_OPERATOR].h_num_args = 2;
    compare_table[ADI_AGG_FUNC].sort_order = 3;
    compare_table[ADI_AGG_FUNC].l_num_args = 1;
    compare_table[ADI_AGG_FUNC].h_num_args = 2;
    compare_table[ADI_NORM_FUNC].sort_order = 4;
    compare_table[ADI_NORM_FUNC].l_num_args = 0;
    compare_table[ADI_NORM_FUNC].h_num_args = ADI_MAX_OPERANDS-1;
    compare_table[ADI_PRED_FUNC].sort_order = 5;
    compare_table[ADI_PRED_FUNC].l_num_args = 0;
    compare_table[ADI_PRED_FUNC].h_num_args = 2;
    compare_table[ADI_COERCION].sort_order = 7;
    compare_table[ADI_COERCION].l_num_args = 1;
    compare_table[ADI_COERCION].h_num_args = 1;
    compare_table[ADI_COPY_COERCION].sort_order = 6;
    compare_table[ADI_COPY_COERCION].l_num_args = 1;
    compare_table[ADI_COPY_COERCION].h_num_args = 1;

    if (!new_objects)
    {
	scb->Adi_fis = (ADI_FI_DESC *)Adi_fis;
	scb->Adi_fi_size = Adi_fis_size;
	scb->Adi_fi_lkup = (ADI_FI_LOOKUP *)Adi_fi_lkup;
	scb->Adi_fil_size = Adi_fil_size;
	scb->Adi_fi_rt_biggest = (Adi_fil_size/(
			(char *) &scb->Adi_fi_lkup[1] -
			    (char *) &scb->Adi_fi_lkup[0])) - 2;

	scb->Adi_fi_int_biggest = scb->Adi_fi_rt_biggest;
		/*
		** Subtract 2 -- one is the NULL entry at the end (signifying
		** the end of the list), the other is because the use of this
		** field is really as an index offset into the array --
		** therefore we must convert it to be consistent with C's use of
		** zero-based array addressing.
		*/
	return(E_DB_OK);
    }
    else
    {
	/* handle both class objects and UDT */
	if( new_objects->add_type == ADD_CLSOBJ_TYPE)
	    add_fistart = ADD_CLSOBJ_FISTART;
	else
	    add_fistart = ADD_USER_FISTART;
    }
    /* Else... */

    /*
    ** First, sort the newcomers so that we can do a merge join with the
    ** already extant function instances
    */

    start = 0;
    end = new_objects->add_fi_cnt;

    ni = new_objects->add_fi_dfn;
    if ((!ni && new_objects->add_fi_cnt) ||
	    (ni && !new_objects->add_fi_cnt))
    {
	if (new_objects->add_trace & ADD_T_LOG_MASK)
	{
	    if (!ni)
		TRdisplay(
"Ad0_fiinit:  %d. function instances requested -- no definitions found\n",
		    new_objects->add_fi_cnt);
	    else
		TRdisplay(
"Ad0_fiinit:  Definitions found for zero requested function instances\n");
	}
	error->err_code = E_AD800A_FI_COUNT_SPEC;
	return(E_DB_ERROR);
    }
    if (!ni)
    {
	/* Then no work to do...*/
	return(E_DB_OK);
    }
    if (	(ni->fid_optype != ADI_COMPARISON)
	    &&	(ni->fid_optype != ADI_OPERATOR)
	    &&	(ni->fid_optype != ADI_NORM_FUNC)
	    &&	(ni->fid_optype != ADI_AGG_FUNC)
	    &&	(ni->fid_optype != ADI_COERCION)
	    &&	(ni->fid_optype != ADI_PRED_FUNC)
	    )
    {
	if (new_objects->add_trace & ADD_T_LOG_MASK)
	{
	    TRdisplay(
"Ad0_fiinit: Invalid fid_optype (%d./%<%x) for function instance number %d.\n",
		ni->fid_optype, 0);
	}
	error->err_code = E_AD800D_FI_OPTYPE;
	return(E_DB_ERROR);
    }
    
    /* Following loops verify that table is already sorted, rather than
    ** proactively sorting it. 
    **
    ** Actually looks like the pair of loops used to sort. But since we're
    ** only validating the sort, there is no more need to have 2 loops.
    ** Inner loop is superfluous for all but the first iteration. */

    for (i = start; i < end - 1; i++, ni++)
    {
	for (current = i + 1; current < end; current++)
	{
	    nc = &new_objects->add_fi_dfn[current];
	    if (	!current
		    &&  (nc->fid_optype != ADI_COMPARISON)
		    &&	(nc->fid_optype != ADI_OPERATOR)
		    &&	(nc->fid_optype != ADI_NORM_FUNC)
		    &&	(nc->fid_optype != ADI_COERCION)
	    	    &&	(nc->fid_optype != ADI_PRED_FUNC)
		    )
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Invalid fid_optype (%d./%<%x) for function instance number %d.\n",
			nc->fid_optype, current /* was new */);
		}
		error->err_code = E_AD800D_FI_OPTYPE;
		return(E_DB_ERROR);
	    }

	    if ((compare_table[nc->fid_optype].sort_order
				 < compare_table[ni->fid_optype].sort_order)
	      ||
		(   (nc->fid_optype == ni->fid_optype)
		&&
		    (nc->fid_opid < ni->fid_opid)
		))
	    {
		/* Error -- items must be sorted */
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay("Ad0_fiinit:  Function instances out of order\n");
		}
		error->err_code = E_AD8009_FI_SORT_ORDER;
		return(E_DB_ERROR);
	    }
	}
    }

    ni = new_objects->add_fi_dfn;
    
    if ((new_objects->add_trace & ADD_T_LOG_MASK) &&
	    (!ni || (new_objects->add_fi_cnt == 0)))
    {
	TRdisplay("Ad0_fiinit: No new function instances requested\n");
    }

    /* Merge logic - find FI's for operations that exist in both new and
    ** old and merge them (UDTs first). 
    **
    ** Not clear why "start" is used here, since it must be 0. */
    for (old = 0, new = start, i = 0;
	    (scb->Adi_fis[old].adi_finstid != ADFI_ENDTAB) || (new < end);
	    i++)
    {
	
	/* changed to load UDT FI first before INGRES FI */
	if ( (scb->Adi_fis[old].adi_finstid != ADFI_ENDTAB)
	    && (
		(new >= end)   /* Already out of new guys or */
	      || (compare_table[scb->Adi_fis[old].adi_fitype].sort_order
				    < compare_table[ni->fid_optype].sort_order)
	      || ((scb->Adi_fis[old].adi_fitype ==
			    ni->fid_optype)
		 && 
		    (scb->Adi_fis[old].adi_fiopid < ni->fid_opid)
	       )
	     )
	   )
	{
	    /* Got an old entry to copy. */
	    if (new_objects->add_trace & ADD_T_LOG_MASK)
	    {
		TRdisplay(
"Ad0_fiinit: Moving function instance for fid_id %d., fid_opid %d., from %d to %d\n\
		fid_type %d., newfid_id %d., newfid_opid %d., newfid_type %d.\n",
			    scb->Adi_fis[old].adi_finstid,
			    scb->Adi_fis[old].adi_fiopid,
			    old, i,
			    scb->Adi_fis[old].adi_fitype,
			    ni->fid_id, ni->fid_opid, ni->fid_optype);
	    }

	    STRUCT_ASSIGN_MACRO(scb->Adi_fis[old], new_fis[i]);
	    
	    /*
	    ** Now that the function instance is defined, we fill in the special
	    ** information necessary in the rapid lookup table.
	    */

	    j = ADI_LK_MAP_MACRO(new_fis[i].adi_finstid);
	    if (j > max_j)
		max_j = j;		/* Remember the max slot in fi */
    	    STRUCT_ASSIGN_MACRO(scb->Adi_fi_lkup[j], new_fi_lkup[j]);
	    new_fi_lkup[j].adi_fi = &new_fis[i];
	    old++;
	}
	else
	{
	    /* Time to define a new one - first check for errors. */

	    if (ni->fid_object_type != ADD_O_FUNCTION_INSTANCE)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Invalid fid_object_type (%d./%<%x -- should be %d./%<%x) found\n\
             for function instance number %d. (zero based)\n",
			ni->fid_object_type,
			ADD_O_FUNCTION_INSTANCE,
			new);
		}
		error->err_code = E_AD8001_OBJECT_TYPE;
		return(E_DB_ERROR);
	    }
	    else if (	(ni->fid_rltype != ADI_O1)
		    &&	(ni->fid_rltype != ADI_O2)
		    &&	(ni->fid_rltype != ADI_LONGER)
		    &&	(ni->fid_rltype != ADI_SHORTER)
		    &&  (ni->fid_rltype != ADI_FIXED)
		    &&	(ni->fid_rltype != ADI_RESLEN)
		    &&  (ni->fid_rltype != ADI_LEN_EXTERN)
		    &&  (ni->fid_rltype != ADI_LEN_EXTERN_COUNTVEC)
		    )
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Invalid fid_rltype (%d./%<%x) found for function instance number %d. (zero based)\n",
			ni->fid_rltype,
			new);
		}
		error->err_code = E_AD800B_FI_RLTYPE;
		return(E_DB_ERROR);
	    }
	    else if (ni->fid_id < add_fistart)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Invalid function instance id (%d.) for instance number %d.\n",
			ni->fid_id, new);
		}
		error->err_code = E_AD800C_FI_INSTANCE_ID;
		return(E_DB_ERROR);
	    }
	    else if ((ni->fid_cmplmnt != ADI_NOFI) &&
			(ni->fid_cmplmnt < add_fistart))
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Invalid function instance complement id (%d.) for instance number %d.\n",
			ni->fid_cmplmnt, new);
		}
		error->err_code = E_AD800C_FI_INSTANCE_ID;
		return(E_DB_ERROR);
		
	    }
	    else if (   (ni->fid_numargs
				< compare_table[ni->fid_optype].l_num_args)
		      ||
		      	(ni->fid_numargs
				> compare_table[ni->fid_optype].h_num_args)
		    )
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Wrong number of arguments (%d.) found for instance %d.\n\
                value should be between %d. and %d.\n",
			ni->fid_numargs,
			new,
			compare_table[ni->fid_optype].l_num_args,
			compare_table[ni->fid_optype].h_num_args);
		}
		error->err_code = E_AD800E_FI_NUMARGS;
		return(E_DB_ERROR);
	    }
	    else if ((ni->fid_optype == ADI_COMPARISON) &&
			(ni->fid_cmplmnt == ADI_NOFI))
	    {
		/* Note that further testing for this is done after the	    */
		/* complete table is filled.  This test verifies initial    */
		/* compliance, the other test verifies that in fact the	    */
		/* function instance id specified is valid		    */

		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Comparison found without complement instance id -- instance %d\n",
			    new);
		}
		error->err_code = E_AD800F_FI_NO_COMPLEMENT;
		return(E_DB_ERROR);
	    }
	    else if (ni->fid_numargs && !ni->fid_args)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: %d. arguments specified, none provided for instance %d.\n",
			ni->fid_numargs, new);
		}
		error->err_code = E_AD8010_FI_ARG_SPEC;
		return(E_DB_ERROR);
	    }
	    else if (new_fi_lkup[ADI_LK_MAP_MACRO(ni->fid_id)].adi_fi)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit: Duplicate function instance definition found\n\
             for fid_id (%d./%<%x), instance number %d.\n",
			ni->fid_id,
			new);
		}
		error->err_code = E_AD8011_FI_DUPLICATE;
		return(E_DB_ERROR);
	    }
	    else if (!ni->fid_routine)
	    {
		if (new_objects->add_trace & ADD_T_LOG_MASK)
		{
		    TRdisplay(
"Ad0_fiinit:  No fid_routine for instance number %d.\n",
				new);
		}
		error->err_code = E_AD8013_FI_ROUTINE;
		return(E_DB_ERROR);
	    }
	    /* Else.... */
	    
	    if (new_objects->add_trace & ADD_T_LOG_MASK)
	    {
		TRdisplay(
"Ad0_fiinit: Adding (@ %d.) function instance for \n\
	fid_id %d.(%<%x),\n\
	fid_optype %d. (%<%x),\n\
	fid_opid %d.(%<%x),\n\
	fid_cmplmnt %d.(%<%x),\n\
	fid_result %d. (%<%x),\n\
	fid_numargs %d.,\n\
	fid_rltype %d.(%<%x),\n\
        fid_rlength %d. (%<%x)\n",
			    i,
			    ni->fid_id,
			    ni->fid_optype,
			    ni->fid_opid,
			    ni->fid_cmplmnt,
			    ni->fid_result,
			    ni->fid_numargs,
			    ni->fid_rltype,
			    ni->fid_rlength);
		if (ni->fid_numargs)
		{
		    TRdisplay(
"       with argument type(s) ( %#.#{%.#d.(%2<%.#x) %})\n",
			ni->fid_numargs, sizeof(DB_DT_ID),
			ni->fid_args,
			sizeof(DB_DT_ID),
			0); /* Offset into the `structure' for the %x */
	 	}
	    }
	    new_fis[i].adi_finstid = ni->fid_id;
	    new_fis[i].adi_fitype = ni->fid_optype;
	    new_fis[i].adi_fiopid = ni->fid_opid;
	    new_fis[i].adi_cmplmnt = ni->fid_cmplmnt;
	    new_fis[i].adi_fiflags = ni->fid_attributes;
	    if ((new_fis[i].adi_fiflags & ADI_F4_WORKSPACE)
	     && (ni->fid_wslength > 0))
		new_fis[i].adi_agwsdv_len = ni->fid_wslength;
	    else
		new_fis[i].adi_agwsdv_len = 0;
	    new_fis[i].adi_dtresult = ni->fid_result;
	    /* Force +datatype if sql is to calculate nullability */
	    if ((ni->fid_attributes & ADI_F256_NONULLSKIP) == 0)
		new_fis[i].adi_dtresult = abs(new_fis[i].adi_dtresult);
	    if (ni->fid_numargs)
	    {
		i4 j = ni->fid_numargs;
		DB_DT_ID *from = ni->fid_args;
		DB_DT_ID *to = &new_fis[i].adi_dt[0];

		while (--j >= 0)
		{
		    *to++ = *from++;
		}
	    }
	    new_fis[i].adi_numargs = ni->fid_numargs;
	    if (ni->fid_optype == ADI_COMPARISON)
		new_fis[i].adi_npre_instr = ADE_3CXI_CMP_SET_SKIP;
	    else if (ni->fid_optype == ADI_PRED_FUNC)
		new_fis[i].adi_npre_instr = ADE_4CXI_CMP_SET_SKIP;
	    else if (ni->fid_attributes & ADI_F256_NONULLSKIP)
		new_fis[i].adi_npre_instr = ADE_0CXI_IGNORE;
	    else if (ni->fid_optype == ADI_AGG_FUNC)
	    {
		/* This is a hack because ADD_FI_DFN doesn't define
		** the pre-instructions. */
		if (ni->fid_opid == ADI_SUM_OP || ni->fid_opid == ADI_MAX_OP
			|| ni->fid_opid == ADI_MIN_OP)
		    new_fis[i].adi_npre_instr = ADE_5CXI_CLR_SKIP;
		else new_fis[i].adi_npre_instr = ADE_2CXI_SKIP;
	    }
	    else
		new_fis[i].adi_npre_instr = ADE_1CXI_SET_SKIP;

	    new_fis[i].adi_lenspec.adi_lncompute = ni->fid_rltype;
	    new_fis[i].adi_lenspec.adi_fixedsize = ni->fid_rlength;
            new_fis[i].adi_lenspec.adi_psfixed = ni->fid_rprec;
	    j = ADI_LK_MAP_MACRO(new_fis[i].adi_finstid);
	    if (j > max_j)
		max_j = j;		/* Remember the max slot in fi */
	    /*
	    ** Now that the function instance is defined, we fill in the special
	    ** information necessary in the rapid lookup table.
	    */
	    new_fi_lkup[j].adi_fi = &new_fis[i];
	    new_fi_lkup[j].adi_func = ni->fid_routine;
	    new_fi_lkup[j].adi_agend = 0;

	    if (ni->fid_rltype == ADI_LEN_EXTERN
	      || ni->fid_rltype == ADI_LEN_EXTERN_COUNTVEC)
	    {
		new_fis[i].adi_lenspec.adi_fixedsize = (i4)ni->fid_id;
		new_fi_lkup[j].adi_agbgn = ni->lenspec_routine;
	    }
	    else
	    {
		new_fi_lkup[j].adi_agbgn = 0;
	    }

	    new++;
	    ni++;
	}
    }
    new_fi_limit = i;
    for (i = 0; new_fis[i].adi_fitype == ADI_COMPARISON; i++)
    {
	j = ADI_LK_MAP_MACRO(new_fis[i].adi_cmplmnt);

	if ((new_fi_lkup[j].adi_fi == 0)
	  || (new_fi_lkup[j].adi_fi->adi_cmplmnt != new_fis[i].adi_finstid))
	{
	    /* if complement is not a valid id or			    */
	    /*	complement(complement) != original then error		    */

	    if (new_objects->add_trace & ADD_T_LOG_MASK)
	    {
		if (new_fi_lkup[j].adi_fi == 0)
		{
		    TRdisplay(
"Ad0_fiinit:  Comparison function instance id's (%d/%<%x) complement id \n\
(%d./%<%x) does not exist\n",
			new_fis[i].adi_finstid,
			new_fis[i].adi_cmplmnt);
		}
		else
		{
		    TRdisplay(
"Ad0_fiinit:  Comparison function instance id's (%d./%<%x) \n\
complement (%d./%<%x) has complement (%d./%<%x).  These should be equal.\n",
			new_fis[i].adi_finstid,
			new_fis[i].adi_cmplmnt,
			new_fi_lkup[j].adi_fi->adi_cmplmnt);
		}
	    }
	    error->err_code = E_AD800F_FI_NO_COMPLEMENT;
	    return(E_DB_ERROR);
	}
    }
    if (new_objects->add_trace & ADD_T_LOG_MASK)
    {
	TRdisplay("Ad0_fiinit: %d. function instances successfully added\n",
			new);
    }

    /*
    ** If spatial types, save the highest used func instance slot 
    */
    if (new_objects->add_type == ADD_CLSOBJ_TYPE)
         scb->Adi_fi_int_biggest = max_j;

    new_fis[new_fi_limit].adi_finstid =
		new_fis[new_fi_limit].adi_cmplmnt = ADFI_ENDTAB;
    scb->Adi_fis = new_fis;
    scb->Adi_fi_lkup = new_fi_lkup;

    return(E_DB_OK);
}

static i4
ad0_handler(
EX_ARGS		*args)
{
    return(EXDECLARE);
}

/*{
** Name: ad0_hfinit - initialize floating-point constants for adc_hdec().
**
** Description:
**      This routine initializes certain floating-point values that are needed
**	for processing of floating-point numbers in adc_hdec(). These values 
**	are operating system dependent.
**
** Inputs:
**      adc_hdec		- ptr to the ADC_HDEC.
**
** Outputs:
**      adc_hdec		- ADC_HDEC struct with following fields
**				  filled in:
**	    .fp_f4neg		- multiplication factor for negative f4's.
**	    .fp_f4nil		- a number to be used when f4's are equal to 0.
**	    .fp_f4pos		- multiplication factor for positive f4's.
**	    .fp_f8neg		- multiplication factor for negative f8's.
**	    .fp_f8nil		- a number to be used when f8's are equal to 0.
**	    .fp_f8pos		- multiplication factor for positive f8's.
**
**	Returns:
**	    0	- if succeeded.
**	    4	- if f4 processing failed.
**	    8	- if f8 processing failed.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Initializes fp_f4neg, fp_f4nil, fp_f4pos, fp_f8neg, fp_f8nil, 
**	fp_f8pos in Adc_hdec in the ADF server control block.
**
** History:
**      05-feb-91 (stec)
**          Based on code removed from opqoptdb.sc.
*/
static i4
ad0_hfinit(
ADC_HDEC    *adc_hdec)
{
    i4		i;
    f8		f8val;
    char	fstr[DBL_DIG + 4];
    STATUS	stat;
    i4		ret_stat;

    ret_stat = 0;
    
    for (;;)
    {
	/*
	** In both cases below the value of the define is 
	** decremented by 2 to account for the leading zero,
	** i.e., "0.", and for the last meaningful digit.
	** appended to the string outside the for loop.  
	*/

	for (i = FLT_DIG - 2, STcopy("0.", fstr); i--;)
	{
	    STcat(fstr, "0");
	}
	STcat(fstr, "1");

	stat = CVaf(fstr, '.', &f8val);
	if (stat != OK)
	{
	    ret_stat = 4;
	    break;
	}

	adc_hdec->fp_f4nil = (f4)f8val;
	adc_hdec->fp_f4pos = 1.0 - adc_hdec->fp_f4nil;
	adc_hdec->fp_f4neg = 1.0 + adc_hdec->fp_f4nil;
	
	for (i = DBL_DIG - 2, STcopy("0.", fstr); i--;)
	{
	    STcat(fstr, "0");
	}
	STcat(fstr, "1");

	stat = CVaf(fstr, '.', &f8val);
	if (stat != OK)
	{
	    ret_stat = 8;
	    break;
	}

	adc_hdec->fp_f8nil = f8val;
	adc_hdec->fp_f8pos = 1.0 - adc_hdec->fp_f8nil;
	adc_hdec->fp_f8neg = 1.0 + adc_hdec->fp_f8nil;

	break;

    } /* end of the for statement. */

    return (ret_stat);
}

/*{
** Name: adg_init_unimap()      - Init memory in ADF
**                                for unicode mapping files.
**
** Description:
**      This function is the external ADF call "adg_init_unimap()"
**      This is used by libq presently to init the mapping files
**      if unicode coercion was required.
**
** Inputs:
**      none
**
** Outputs:
**      none    Memory is allocated to Adf_globs->Adu_maptbl.
**
**      Returns:
**          E_DB_OK                     Operation succeeded.
**
** History:
**      03-Feb-2005 (gupsh01)
**          Added.
**	14-Sep-2007 (gupsh01)
**	    Return the status as promised.
*/
DB_STATUS
adg_init_unimap(void)
{
  STATUS db_stat = OK;
  char converter[MAX_LOC];

  if (Adf_globs != NULL && Adf_globs->Adu_maptbl == NULL)
  {
    if ((db_stat = adu_getconverter(converter)) == E_DB_OK)
      db_stat = adu_readmap (converter);
    else
      db_stat = adu_readmap ("default");
  }
  return db_stat;
}

/*{
** Name: adg_setnfc - Routine to set the database type NFC
**
** Description:
**      This is used by libq which initializes the adf control
**      structure with adf_uninorm_flag set to ADU_UNINORM_NFC
** History:
**      17-May-2005 (gupsh01)
**          Added
**
*/
DB_STATUS
adg_setnfc(ADF_CB *adf_scb)
{
    STATUS stat = OK;

    if (adf_scb != NULL)
    {
        adf_scb->adf_uninorm_flag =  AD_UNINORM_NFC;
    }
    else
        return (E_DB_ERROR);

    return(E_DB_OK);
}


/*{
** Name: ad0_fivalidate	- validate sequence of entries in default FI table
**
** Description:
**      This routine loops over the entries in the default function instance
**	table to verify that its entries follow the appropriate rules of
**	ordering. That is, they must be sorted on function category (comparison,
**	aggregate, normal, ...) and within that, on operator ID. Errors are
**	displayed when sequence errors are encountered.
**
** Inputs:
**      scb                             Adf's server control block
**
** Outputs:
**	error				Filled with appropriate error when user
**					defined datatypes are specified.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-oct-2006 (dougi)
**          Written to validate FI table order.
*/

static DB_STATUS
ad0_fivalidate(
ADF_SERVER_CB      *scb,
DB_ERROR	   *error)
{
    i4		    i;
    i4		    j;
    i4		    max_j = 0;
    i4		    new_fi_limit;
    i4		    start;
    i4		    end;
    i4		    current;
    i4		    old;
    i4		    new;
    ADI_FI_DESC     *prevfi, *currfi;
    struct
    {
	i4		sort_order;	/* Must be sorted in this order */
	i4		l_num_args;	/* With at least this many	    */
					/* arguments, ...		    */
	i4		h_num_args;	/* but not more than this	    */
    }
    		    compare_table[8];


    /* Set up "compare_table[]" same as in ad0_fiinit(). */
    compare_table[0].sort_order = 0;
    compare_table[0].l_num_args = 0;
    compare_table[0].h_num_args = 0;
    compare_table[ADI_COMPARISON].sort_order = 1;
    compare_table[ADI_COMPARISON].l_num_args = 2;
    compare_table[ADI_COMPARISON].h_num_args = 2;
    compare_table[ADI_OPERATOR].sort_order = 2;
    compare_table[ADI_OPERATOR].l_num_args = 1;
    compare_table[ADI_OPERATOR].h_num_args = 2;
    compare_table[ADI_AGG_FUNC].sort_order = 3;
    compare_table[ADI_AGG_FUNC].l_num_args = 1;
    compare_table[ADI_AGG_FUNC].h_num_args = 2;
    compare_table[ADI_NORM_FUNC].sort_order = 4;
    compare_table[ADI_NORM_FUNC].l_num_args = 0;
    compare_table[ADI_NORM_FUNC].h_num_args = 4;
    compare_table[ADI_PRED_FUNC].sort_order = 5;
    compare_table[ADI_PRED_FUNC].l_num_args = 0;
    compare_table[ADI_PRED_FUNC].h_num_args = 2;
    compare_table[ADI_COERCION].sort_order = 7;
    compare_table[ADI_COERCION].l_num_args = 1;
    compare_table[ADI_COERCION].h_num_args = 1;
    compare_table[ADI_COPY_COERCION].sort_order = 6;
    compare_table[ADI_COPY_COERCION].l_num_args = 1;
    compare_table[ADI_COPY_COERCION].h_num_args = 1;

    /* Following loop verifies that table is already sorted. Simply compares
    ** successive entries until "next" entry has fiid -1. */
    for (prevfi = &scb->Adi_fis[0], currfi = &prevfi[1];
	    currfi->adi_finstid != ADFI_ENDTAB; prevfi = currfi++)
    {
	if ((prevfi->adi_fitype != ADI_COMPARISON)
		    &&	(prevfi->adi_fitype != ADI_AGG_FUNC)
		    &&	(prevfi->adi_fitype != ADI_OPERATOR)
		    &&	(prevfi->adi_fitype != ADI_NORM_FUNC)
		    &&	(prevfi->adi_fitype != ADI_COERCION)
		    &&	(prevfi->adi_fitype != ADI_COPY_COERCION)
	    	    &&	(prevfi->adi_fitype != ADI_PRED_FUNC)
		    )
	{
#ifdef xOPTAB_PRINT
	    TRdisplay(
"Ad0_fivalidate: Invalid adi_fitype (%d./%<%x) for function instance number %d.\n",
			prevfi->adi_fitype, prevfi->adi_finstid);
#endif
	    error->err_code = E_AD800D_FI_OPTYPE;
	    return(E_DB_ERROR);
	}

	if ((compare_table[currfi->adi_fitype].sort_order
				 < compare_table[prevfi->adi_fitype].sort_order)
	      ||
		(currfi->adi_fitype == prevfi->adi_fitype &&
		    currfi->adi_fitype != ADI_COERCION &&
		    currfi->adi_fiopid < prevfi->adi_fiopid
		))
	{
	    /* Error -- items must be sorted */
#ifdef xOPTAB_PRINT
	    TRdisplay("Ad0_fivalidate:  Function instances out of order\n\
		adi_fiopid (%d./%<%x), function instance number %d.\n",
			prevfi->adi_fiopid, prevfi->adi_finstid);
#endif
	    error->err_code = E_AD8009_FI_SORT_ORDER;
	    return(E_DB_ERROR);
	}
    }

    return(E_DB_OK);
}
