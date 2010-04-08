/*
** Copyright (c) 1989, 2000 Ingres Corporation
*/

#ifndef		SD_INCLUDED
#define		SD_INCLUDED	1

/**CL_SPEC
** Name: SD.H - This file contains the external structs for SD.
**
** Description:
**      The SD.H file is used to contain all structures and 
**      definitions required to call the compatability library
**      SD routines.  These routines perform all Smart Disc
**      operations for INGRES, such as compiling a query and doing
**      the file access.
**
** Specification:
**  
** 
**  Description:
**  This is a definition of all new CL routines to be invoked, in 
**  conjunction with existing CL, to access a VME CAFS unit.  
**  
**  What Smart Disc requires is that the conditions for search be surfaced 
**  for use at the DI level. It will require optimiser changes to check 
**  whether SD can handle a particular search and if so 'compile' the 
**  query into a Task Specification Block (TSB) to be passed via 
**  DMF to what are essentially DI extensions.  
**  
**  The DI extensions then interrupt the normal open-read-close flow to 
**  make it open-setcondition-read-close. The benefit is that the read 
**  activity will read in only those tuples that match the condition with 
**  no mainframe cost (because it is done in the CAFS disc controller). 
**  The down side and complication factor is that CAFS cannot handle 
**  every condition that SQL might care to throw at a table, is read 
**  only, and needs to do locking at the table and not page/record 
**  level.
**
**  The CL interface requires CL to tell mainline what CAFS is capable 
**  of and mainline makes the CAFS/no-CAFS decision. A lower level interface 
**  is then provided which takes a form of the condition/table info which 
**  is suitable for generating the TSB.  The decision on whether CAFS is 
**  usable in a particular call is based on the type of query in terms of 
**  datatypes, comparisons, and conjunctions forming the compound condition. 
**  The decision also involves determining whether the container for the 
**  table (ie the physical file or files where a multi-location table is 
**  concerned) is actually on a CAFS unit.
**
**  In order to find out whether particular table's file(s) is/are 
**  located on a Smart Disc, a DI-like routine is provided so that this 
**  can be determined along with other table info, on a per file basis.  
**  In order to give the optimiser the info it needs to make the 
**  CAFS/no-CAFS decision, a CL routine for calling by the optimiser 
**  will return a structure containing, among other things, which INGRES 
**  datatypes are supported in elementary comparisons and what CAFS 
**  data-types they should be mapped onto, the maximum complexity that 
**  can be coped with, and other information pertaining to the structures 
**  to be passed to the SDcompile() CL routine (like maximum TSB area size).  
**  If the optimiser goes for CAFS then the second function in this section 
**  is invoked to compile its input to a CAFS Search Task Specification 
**  Block.
**  
**  The SD routines which do the actual input operations are much 
**  simplified by being cast as an extension to DI.  This means that SD 
**  does not handle file open/close type activities but runs on the back 
**  of normal IO.  One implication of this is that DIopen needs to always 
**  request CAFS capabilities on all connections. If such capability is 
**  refused this should be recorded in the DIio struct in order to 
**  implement SDinformation(). Other than that the open can proceed as 
**  normal, as long as all tables for which CAFS is available have it 
**  on their connections.  There needs to be a trace option on DIopen to 
**  log those occasions  when CAFS access was refused, for diagnostic 
**  purposes.
**
**  There are a number of ways in which 'SD' input operations differ 
**  from standard 'DI' input. First, notice it is input and not input/output, 
**  SD only handles input operations (searches).  The SD routines take over 
**  after the table has been opened via the normal DI route. Two levels of 
**  interface could be provided: 
**  1 A CL interface which returns a 'pseudo page' of tuples that satisfy 
**  the  condition.  
**  
**  2 A CL interface which returns a single tuple at each call.  
**  The 'pseudo page' solution works best for DMF. 
**
**  Using the Pseudo page interface, a page of tuples is retrieved using 
**  SDpageread(). SDpageread() will scan a number of INGRES pages/VME 
**  blocks and deliver a pseudo-page of 'hit' records, being rows which 
**  satisfy the programmed condition. The caller of SDpageread() limits 
**  the search to a start page/number of pages,  SDpageread() responds with: 
**  
**  1. The number of pages scanned, which could be less than the request 
**  if the pseudo page of 'hit' records is filled. In this case a further 
**  call of SDpageread() will be needed, specifying a new start page and 
**  number of pages to complete the scan, and so on.  
**  
**  2. The 'pseudo page' of rows. These rows are in CAFS format and, 
**  importantly, their TIDs are at the front of each row because they come 
**  from different pages. This return is only available if the status 
**  return is OK or DI_LASTPAGE.  
**  
**  3. A STATUS which can be set to one of: 
**      OK - Here is a full page and there may be more 
**      SD_LASTPAGE - Here is a partly filled page and all the requested 
**      pages have been scanned 
**      SD_NOHIT - No records were found in the requested scan that matched 
**      the condition.
**        + a number of error messages based on validation of inputs.
**
**  This means that in order to effect a scan of a table you call 
**  SDpageread(), process the page and recall SDpageread() with an updated 
**  start page/number of pages until the status returned tells you 
**  there are no more rows to find.
**
**  At any point a search can be abandoned simply by not calling any more 
**  routines, without incurring penalties. DIclose() will close the 
**  connection or a further call of SDpageread() for the same file with a 
**  different TSB will reset the condition for this connection and start 
**  a new scan.  It is a function of the CL to spot that a connection is 
**  being used to continue the same scan without an intervening IO and 
**  so optimise out CAFS initialisation each time.
**
**  The condition has already been checked as appropriate for CAFS and 
**  the data required by the CAFS unit, the Task Specification Block (TSB), 
**  has already been produced (compiled) and stored along with the Query 
**  Execution Plan (QEP) to be available on a per-thread basis. 
**
**  Note that it is a facility of the system to handle the switching of 
**  programs on CAFS to cope with multiple connections (to the same or 
**  multiple tables) but a function of these interfaces to cope with 
**  switching of condition for multiple threads using the same connection 
**  within a single multi-threaded server. 
**
**  History:
**      19-sep-89 (kwatts)
**          Created new for 6.2.
**	15-feb-91 (kwatts)
**	    Modified for VMS so that if xDEV_TEST is not set the SD
**	    procedures are declared as cpp macros.
**	22-nov-91 (kwatts)
**	    Removed define for E_SD_MASK (it goes in compat.h)
**	04_dec_92 (kwatts)
**	    Added SD_DATA_VALUE structure for new Smart Disk binding
**	    information
**          Also changes to SD_CAPABILITIES for binding info.
**	26-jan-93 (kwatts)
**	    Missed a change for new Smart Disk regarding decimal - the
**	    kcormask field in a GDD is now a precision field.
**	03-mar-96 (albany)
**	    Cross integrated Unix 423246:
**		06-feb-96 (johnst)
**		Bug 72205.
**		Adding true binary compression (TCB_C_HICOMPRESS) to ingres 
**		broke ICL SCAFS which only knew the previous TCB_C_STANDARD 
**		compression.  Changed SD_ATTR_INFO.compressed element from a 
**		bool to a char and defined SD_C_NONE, SD_C_STANDARD, 
**		SD_C_HICOMPRESS ingres storage compression types to pass 
**		no/low/hi compression info through this interface. The 
**		compression type values are copied from the TCB_C_* values 
**		defined in dmp.h.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

/*
**  Forward and/or External typedef/struct references.
*/

/*
The following structures are defined below:
*/
typedef struct _SD_ATTR_INFO    SD_ATTR_INFO;
typedef struct _SD_CAPABILITIES SD_CAPABILITIES;
typedef struct _SD_GDD          SD_GDD;
typedef struct _SD_NODE         SD_NODE;
typedef i2 			SD_DT_ID;

/*
**  Forward and/or External function references or macro definitions.
*/
#ifdef xDEV_TEST
FUNC_EXTERN STATUS SDcompile();   /*Compiles a condition */
FUNC_EXTERN STATUS SDfree();   /*Frees SD buffers after a scan */
FUNC_EXTERN STATUS SDgencapabs();/*Gives Smart Disc capabilities.*/
FUNC_EXTERN void   SDgivecapabs();/*Give capabilities of one item.*/
FUNC_EXTERN STATUS SDinformation(); /*Check on Smart Disc suitability */
FUNC_EXTERN f4     SDrpi();   /*Give relative perf. indicator for Smart Disc */
FUNC_EXTERN STATUS SDpageread();  /*Reads a pseudo page from the device */
FUNC_EXTERN STATUS SDunpack();  /*Unpacks a pseudo page */
#else
#define SDcompile(sdid, ingst, nds, nco, fls, fco, lit, lco, TSBb, TSBl,\
		bco, binf, err)  (SD_NOSD)
#define SDfree(f, TSBb, TSBl, err_code)		(OK)
#define SDgencapabs(capabs)			(SD_BAD_ID)
#define SDgivecapabs(sd_id, attrinfo)		\
		((attrinfo)->SD_type=0, \
		 (attrinfo)->possible=SD_NOT_THIS, \
		 (attrinfo)->nullaction=0)
#define SDinformation(f, ppage, sd_id)		(*sd_id=SD_NONE, OK)
#define SDpageread(f, TSBb, TSBl, n, page, buf, bufl, err) \
		(SD_NOCOND)
#define SDrpi(sd_id, tupsz, tabsz, tuinta, esthit)	(2.0)
#define SDunpack(TSBb, TSBl, tupl, tupa, tida, err)	(DI_BADPARAM)
#endif

/* 
** Defined Constants
*/

/* SD return status codes. */
/* The first one should really live in compat.h */
# define                SD_OK           0
# define                SD_EOT       (E_CL_MASK + E_SD_MASK + 0x01)
# define                SD_LASTPAGE  (E_CL_MASK + E_SD_MASK + 0x02)
# define                SD_BADCOND   (E_CL_MASK + E_SD_MASK + 0x03)
# define                SD_BADPARAM  (E_CL_MASK + E_SD_MASK + 0x04)
# define                SD_NOCOND    (E_CL_MASK + E_SD_MASK + 0x05)
# define                SD_SHORT_TSB (E_CL_MASK + E_SD_MASK + 0x06)
# define                SD_ERROR     (E_CL_MASK + E_SD_MASK + 0x07)
# define                SD_NOSD      (E_CL_MASK + E_SD_MASK + 0x08)
# define                SD_SHORT_BUF (E_CL_MASK + E_SD_MASK + 0x09)
# define                SD_VME_ERROR (E_CL_MASK + E_SD_MASK + 0x0A)
# define                SD_BAD_ID    (E_CL_MASK + E_SD_MASK + 0x0B)
# define                SD_SPARE_1   (E_CL_MASK + E_SD_MASK + 0x0C)
# define                SD_SPARE_2   (E_CL_MASK + E_SD_MASK + 0x0D)
# define                SD_SPARE_3   (E_CL_MASK + E_SD_MASK + 0x0E)
# define                SD_SPARE_4   (E_CL_MASK + E_SD_MASK + 0x0F)
# define                SD_SPARE_5   (E_CL_MASK + E_SD_MASK + 0x10)
# define                SD_SPARE_6   (E_CL_MASK + E_SD_MASK + 0x11)

/*}
** Name: SD_DT_ID - Datatype id to match mainline's DB_DT_ID.
**
** Description:
**      All datatype ids to be used in INGRES must be declared as DB_DT_IDs.
**      Legal values for a DB_DT_ID are in the range 1-127, with certain numbers
**      excluded.  Furthermore, the negation of these IDs
**	are also legal, and represent the nullable versions of the corresponding
**	datatypes.  The limited range of 1-127 has to do with the method the
**      DBMS uses for determining which datatypes a given datatype can be
**      automatically converted (coerced) into.  That method uses a 128 bit long
**      bit-mask to represent some set of datatypes.
**
**	All DB_DATA_VALUEs and DB_EMBEDDED_DATAs will be "of" one of the
**	datatypes whose IDs are defined here.  Note that the semantics of the
**	data may differ depending on whether it is a DB_DATA_VALUE or a
**	DB_EMBEDDED_DATA.  For instance, take the datatype ID DB_CHR_TYPE:
**	When used as a DB_DATA_VALUE type, it limits the number of characters
**	to DB_CHAR_MAX, and limits the set of legal characters to the printable
**	ones.  However, when used as a DB_EMBEDDED_DATA type, these limitations
**	do not apply.
**
**      The SD version is created to conform to the architectural requirements
**      of INGRES. The SD CL will refer to the SD versions and mainline will
**      be responsible for the (null) mapping onto DB types. Note that not
**      all the DB_XXX_TYPE defines are declared as SD's, only those that
**      are of interest.
**
** History:
**      26-sep-89 (kwatts)
**          Initial creation, by copy from dbms.h and edit.
*/


#define                 SD_NODT         (SD_DT_ID)  0   /* Used to represent */
							/* NO datatype.      */
/*
**  Names of datatype ID codes that can exist in relations.  The first six of
**  these are based on 4.0 definitions in ingsym.h.
*/
#define                 SD_INT_TYPE     (SD_DT_ID) 30  /* "i" */
#define                 SD_FLT_TYPE     (SD_DT_ID) 31  /* "f" */
#define                 SD_CHR_TYPE     (SD_DT_ID) 32  /* "c" */
#define                 SD_TXT_TYPE     (SD_DT_ID) 37  /* "text" */
#define                 SD_DTE_TYPE     (SD_DT_ID)  3  /* "date" */
#define                 SD_MNY_TYPE     (SD_DT_ID)  5  /* "money" */
#define			SD_CHA_TYPE	(SD_DT_ID) 20  /* "char" */
#define			SD_VCH_TYPE	(SD_DT_ID) 21  /* "varchar" */
#define			SD_DEC_TYPE	(SD_DT_ID) 10  /* "decimal" */


/*}
** Name: SD_DATA_VALUE - internal representation of a data value.
**
** Description:
**      This structure matches how the INGRES DBMS represents any data value.
**      It contains the datatype and length of the data value. Unlike the
**	DBMS version, the Smart Disk one carries an offset instead of
**	an address. The address is only provided at fix-up time, and the
**	offset will be relative to that address.
**	It  is based on DB_DATA_VALUE in the DBMS.
**
** History:
**	04-dec-1992 (kwatts)
**	    Initial creation from a copy of DB_DATA_VALUE in dbms.h
*/
typedef struct 
{
    i4		    sd_data_offset;	/* this will be an offset
                                        ** into a data area.
                                        */
    i4              sd_length;          /* Number of bytes of storage
                                        ** reserved for the actual data.
                                        */
    SD_DT_ID        sd_datatype;        /* The datatype of the data
                                        ** value represented here.
                                        */
    i2              sd_prec;            /* Precision of the data value.
                                        ** Most datatypes will not need
                                        ** this, however, there are some
                                        ** that will; such as DECIMAL.
                                        ** In the DECIMAL case, the high
					** order byte will represent the
					** value's precision (total # of
					** significant digits), and the
					** low order byte will hold the
					** value's scale (the # of these
					** digits that are to the right
					** of an implied decimal point.
					*/
}   SD_DATA_VALUE;

/*}
** Name: SD_ATTR_INFO - The rules for one datatype derived from a call 
**                      to SDgivecapabs()
**
** Description:
**      This structure is passed on a call of SDgivecapabs(). Mainline must 
**	complete the first 5 fields (ingres_type, operator, compressed, 
**	attno, and length) before calling to get the other parts of the
**	structure completed.
**
** History:
**     20-sep-89 (kwatts)
**          Created new for 6.2.
**     26-sep-89 (kwatts)
**          Change from DB_DT_ID to SD_DT_ID.
**     30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**     7-dec-1992 (kwatts)
**          Changes for 6.5  
**          possible - now a bit field,  value changes and added 
**          SD_SOME_FIELDS,   SD_LATE_VALUES
**          Added   *SDintercol()  *SDthisbind()
**	03-mar-96 (albany)
**	    Cross integrated Unix 423246:
**		06-feb-96 (johnst)
**		Bug 72205.
**		Adding true binary compression (TCB_C_HICOMPRESS) to ingres 
**		broke ICL SCAFS which only knew the previous TCB_C_STANDARD 
**		compression.  Changed SD_ATTR_INFO.compressed element from a 
**		bool to a char and defined SD_C_NONE, SD_C_STANDARD, 
**		SD_C_HICOMPRESS ingres storage compression types to pass 
**		no/low/hi compression info through this interface. The 
**		compression type values are copied from the TCB_C_* values 
**		defined in dmp.h.
*/
struct _SD_ATTR_INFO
	{
	i4		ingres_store;
#define                 SD_HEAP_STORE   3L   /* heap storage structure id */
#define                 SD_ISAM_STORE   5L   /* isam storage structure id */
#define                 SD_HASH_STORE   7L   /* hash storage structure id */
#define                 SD_BTRE_STORE   11L  /* btree storage structure id */
	SD_DT_ID	ingres_type;
	i4		operator;
#define			SD_EQ		0
#define			SD_NEQ		1
#define			SD_GT 		2
#define			SD_GE 		3
#define			SD_LT 		4
#define			SD_LE 		5
#define			SD_LIKE		6
#define			SD_NLIKE	7
	char		compressed;
#define                 SD_C_NONE       0
#define                 SD_C_STANDARD   1
#define                 SD_C_HICOMPRESS 7
	i4		attno;
	i2		precision;
	i2		length;
	/* Rest of fields provided by  SD routines */
	i1 		SD_type; 	
	i4		possible;
#define			SD_NOT_THIS	0x00
#define			SD_NULL_ONLY	0x01
#define			SD_ALL_VALUES	0x02
#define			SD_SOME_VALUES	0x04
#define			SD_LATE_VALUES  0x08
#define			SD_SOME_FIELDS  0x10
	i4		nullaction;
#define			SD_NEVER_NULL	0
#define			SD_NULL_INC  	1
#define			SD_NULL_SEP	2
	i4		factor;
	bool		(* SDthisvalue)(/* void *, */ 		/*value */
					/* i4, */		/*length */
					/* SD_ATTR_INFO * */);	/*attrinfo*/
	STATUS		(* SDconvert)	(/* void *, */		/*value*/
					/* i4, */		/*length*/
					/* SD_ATTR_INFO *, */	/*attrinfo*/
					/* char *, */		/*buffer*/
					/* i4 *, */		/*buflength*/
					/* i4 * */);		/*maskstart*/
        bool            (*SDintercol) ( /* SD_ATTR_INFO *left */
                                        /* SD_ATTR_INFO *right */
                                      );
        bool            (*SDthisbind) ( /* SD_ATTR_INFO *left */
                                        /* SD_DT_ID  ingrestype*/
                                        /* i4 length */
                                        /* i4 precision */
                                      );
	};


/*}
** Name: SD_CAPABILITIES - Structure for general information regarding  
**                      Smart Disc, obtained from SDgencapabs().
**
** Description:
**      This structure is passed on a call of SDgencapabs() and is completely
**	initialised by the CL. It is then used to determine various
**	parameters for the Smart Disc decision and compilation.
**
** History:
**     03-nov-89 (kwatts)
**          Created new for 6.2.
**     07-dec-1992 (kwatts)
**          Addition of SDbind() and inter column compares for 6.5
*/
struct _SD_CAPABILITIES
	{
	i4 				SD_identifier;
#define					SD_NONE		0
#define					SD_CAFS		1
	i4				SD_max_factor;
	i4				SD_TSB_max_size;
	i4				SD_max_literals;
	i4				SD_attr_semantics;
#define					SD_NORMAL	0
#define					SD_CAFS_SEMS	1
	STATUS	(*SDbind)		(i4 ,		/*num_binds*/
					 char **, 	/*bind_addresses*/
					 char *,	/*TSBbuffer*/
					 i4 ,		/*TSBlength*/
					 char *,	/*genTSB*/
					 i4 *,		/*genTSBlength*/
					 CL_ERR_DESC *);	/*err_code*/
        i4                               SDoptions;
#define                 SD_BIND         0x01
#define                 SD_FCOMPARE     0x02  
        } ;

/*}
** Name: SD_GDD - Smart Disc attribute info structure
**
** Description: 
**  The description of the attributes in the Tuple is defined by a pointer 
**  to a row of General Data Descriptors (GDD). These describe the 
**  attributes in their physical order on disc. There is a GDD for each 
**  attribute in the fixed part of the tuple, with separate GDDs for 
**  any null indicator bytes, plus a single GDD for all self identifying 
**  field attributes in a compressed table.
**
**  SD_type is set to the value extracted from the SD_ATTR_INFO structure 
**  for the particular attribute, or 0 for a null indicator byte. The 
**  exception to this is where the INGRES data structure ends with some 
**  CAFS self-identifying format items, which is the case when the table 
**  is compressed. For this case, all self identifying items and their 
**  null indicators are subsumed into a single GDD, which will be the 
**  last GDD and have type set to 4.
**
**  flagsandqual is used to define which items we wish to actually retrieve.  
**  Since we are retrieving the whole tuple the value to set in this field 
**  is X30 in the general case and for null indicator bytes, X20 for a 
**  field which is followed by its null indicator, and X10 for the 
**  single GDD used for self identifying format items.
**
**  length defines the length, in bytes, of the attribute. Set to 1 for 
**  the self identifying format GDD. Length here must be exclusive of 
**  the null indicator byte, as it has its own GDD of length 1.
**
**  precision defines the precision for a decimal data-type, otherwise it
**  is set to zero.
**
**  The row of GDDs should include an entry for every attribute and every 
**  null indicator in the fixed part of the tuple, in the order that 
**  they appear on the disc (ie physical order). If there are any fillers 
**  to enforce alignment, these too must be represented by 
**  GDDs (of SD_type 0).
**
** History:
**     06-nov-89 (kwatts)
**          Created new for 6.2.
**     26-jan-93 (kwatts)
**	    kcormask has been changed to precision field.
*/

struct _SD_GDD 
	{
	i1  		SD_type;
	i1  		flagsandqual;
#define			SD_FLAG_GENERAL		0X30
#define			SD_FLAG_NULLABLE	0X20
#define			SD_FLAG_SIF		0X10
	i2		length;
	i2		precision;
	} ;

/*}
** Name: SD_NODE - A single entry in the Smart Disc Condition structure 
**
** Description:
**
**  This structure is used to create a table of SD_NODEs to represent a
**  condition for the Smart Disc interface SDcompile(). The first entry 
**  in any condition table is of type HEAD, and then its parameters 
**  (op1/op2) point at other entries in the table, which can be and/or/not, 
**  a comp (comparison), or TRUE/FALSE. 
**
**  Each of these has a breakdown of parameters as shown in the 
**  structure. Note that:
**
**      cp	Is a condition pointer which is an index down a table of
**              SD_NODE entries.
**
**      fdp	Is a field pointer, which indexes a table of SD_GDD entries
**              describing the attributes of the tuple.
**
**      mcp	Is a mask/constant pointer. It is the index down a 
**              mask/constants string of a particular mask or constant. These
**              indices are returned from calls of the SDconvert procedure
**              in an SD_ATTR_INFO structure.
**
**      and the estimated hit rate is only advisory.
**
** History:
**     06-nov-89 (kwatts)
**          Created new for 6.2.
**     07-dec-1992 (kwatts)
**          addition of inter-column compare and binding for 6.5
*/
struct _SD_NODE
	{
	i1	type;
#define	SD_NODE_T_HEAD		0
#define	SD_NODE_T_AND 		1
#define	SD_NODE_T_OR  		2

/* Some values not present, CAFS facilities not planned to be */
/* supported */
#define	SD_NODE_T_NOT 		7
#define	SD_NODE_T_COMP		8
#define	SD_NODE_T_TRUE		12
#define	SD_NODE_T_FALSE		13
#define SD_NODE_T_FCOMP         21      /* compare fields */
#define SD_NODE_T_LCOMP         22      /* compare bind param */
	i1	qual;	/* 0 unless C_COMPARE type */
#define	SD_QUAL_U_LT		1	/* unsigned operations */
#define	SD_QUAL_EQ		2
#define	SD_QUAL_U_GT		3
#define	SD_QUAL_U_LE		4
#define	SD_QUAL_NE		5
#define	SD_QUAL_U_GE		6
#define	SD_QUAL_S_LT		7	/* signed operations */
#define	SD_QUAL_S_GT		9
#define	SD_QUAL_S_LE		10
#define	SD_QUAL_S_GE		12

	i2	function;	/* Always 0, unused CAFS facility	*/

	i2	op1;		/* For HEAD, AND, OR, NOT	cp	*/
 				/* For C_COMPARE, FCOMP, LCOMP	fdp	*/
				/* For TRUE, FALSE		0	*/

	i2	op2;		/* For AND, OR			cp	*/
				/* For HEAD, NOT, TRUE, FALSE	0	*/
				/* For C_COMPARE		mcp	*/
				/*  where mcp points to constant	*/
                                /* For LCOMP                    bno     */
                                /*  a unique bind number for this lit   */
                                /*  in this condition                   */
                                /* FOR FCOMP                    fdp     */
	i2	triop;		/* For C_COMPARE, mcp pointing at	*/
				/* the mask if present, else 0		*/

	i2	estimate;	/* For C_COMPARE, estimated hit rate	*/
				/* (%) for this condition.		*/
				/* 0=> No information on hit rate	*/

	char   	spare[4];	/* Set to x00's*/

	} ;
#endif		/* SD_INCLUDED */
