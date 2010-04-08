/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include        <cs.h>
#include	<st.h>
#include	<tr.h>
#include	<scf.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include        <qsf.h>
#include	<qefrcb.h>
#include	<qefcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>

/**
**
**  Name: RDFSHUT.C - Shut down RDF for the server.
**
**  Description:
**	This file contains the routine for shutting down RDF for the server. 
**
**	rdf_shutdown - Shut down RDF for the server.
**
**
**  History:    
**      15-apr-86 (ac)
**          written
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**      30-aug-88 (mings)
**	    modified for Titan.
**	17-oct-89 (teg)
**	    lifted from titan for local as part of fix to bug 5446.
**	    (Removed the includes of distributed header files.)
**	31-aug-90 (teresa)
**	    picked up 6.3 bugfix 4390.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of st.h and tr.h
**	15-Aug-1997 (jenjo02)
**	    Call CL directly for semaphores instead of going thru SCF.
**	    Removed rdf_scfcb, rdf_fscfcb.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions. Finally fixed all those embarassing 
**	    misspellings in shutdown stats!
**	3-Dec-2003 (schka24)
**	    Correct lies about parameters to tune in the shutdown report.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**/

/*{
** Name: rdf_shutdown - Shut down RDF for the server.
**
**	External call format:	status = rdf_call(RDF_SHUTDOWN, &rdf_ccb)
**
** Description:
**	This routine shut down RDF for the server. It is used when the whole
**	server is being shut down.
**
**	The shut down procedure is based on rdv_state in server control block.
**	    - RDV_1_RELCACHE_INIT   call ulm to shutdown relation descriptor pool     
**	    - RDV_2_LDBCACHE_INIT   call ulh to close ldb descriptor pool
**	    - RDV_3_RELHASH_INIT    call ulh to close relation hash table
**	    - RDV_4_QTREEHASH_INIT  call ulh to close qtree hash table
**	    - RDV_5_DEFAULTS_INIT   call ulh to close defaults hash table
**
** Inputs:
**
**      rdf_ccb                          
** Outputs:
**      rdf_ccb                          
**					
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_ccb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_ccb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	15-apr-86 (ac)
**          written
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**      13-oct-87 (seputis)
**          rewrote to call SCF directly
**	27-jan_89 (mings)
**	    modified for titan
**	15-dec-89 (teg)
**	    add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**	    routines to use.
**	18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	23-jan-92 (teresa)
**	    SYBIL CHANGES: rdv_c1_poolid -> rdv_poolid; remove logic to 
**	    shutdown LDB cache (since there's now just 1 pool)
**	16-jul-92 (teresa)
**	    prototypes
**	04-feb-93 (teresa)
**	    add support to shut down DEFAULTS cache
*/
DB_STATUS
rdf_shutdown(	RDF_GLOBAL	*global,
		RDF_CCB		*rdf_ccb)
{
    DB_STATUS           status,
			temp_status;    
    ULH_RCB	        *ulhrcb = &global->rdf_ulhcb;
    ULM_RCB             *ulmrcb = &global->rdf_ulmcb;

    status = temp_status = E_DB_OK;
    global->rdfccb = rdf_ccb;
    CLRDBERR(&rdf_ccb->rdf_error);

    /* first report all rdf statistics */
    rdf_report( &(Rdi_svcb->rdvstat) );


    /* 
    ** 1. Close any initialized hash tables 
    */

	/* ldbdesc hash table */
    if (Rdi_svcb->rdv_state & RDV_2_LDBCACHE_INIT)
    {
	ulhrcb->ulh_hashid = Rdi_svcb->rdv_dist_hashid;
	temp_status = ulh_close(ulhrcb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    rdu_ferror(global, temp_status, &ulhrcb->ulh_error,
		       E_RD012A_ULH_CLOSE, 0);
	    if (DB_SUCCESS_MACRO(status))
		status = temp_status;
	}
    }

	/* relation hash table */
    if (Rdi_svcb->rdv_state & RDV_3_RELHASH_INIT)
    {
	ulhrcb->ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;
	temp_status = ulh_close(ulhrcb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    rdu_ferror(global, temp_status, &ulhrcb->ulh_error,
		       E_RD012A_ULH_CLOSE, 0);
	    if (DB_SUCCESS_MACRO(status))
		status = temp_status;
	}
    }

	/* qtree hash table */
    if (Rdi_svcb->rdv_state & RDV_4_QTREEHASH_INIT)
    {
	ulhrcb->ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	temp_status = ulh_close(ulhrcb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    rdu_ferror(global, temp_status, &ulhrcb->ulh_error,
		       E_RD012A_ULH_CLOSE, 0);
	    if (DB_SUCCESS_MACRO(status))
		status = temp_status;
	}
    }

	/* defaults hash table */
    if (Rdi_svcb->rdv_state & RDV_5_DEFAULTS_INIT)
    {
	ulhrcb->ulh_hashid = Rdi_svcb->rdv_def_hashid;
	temp_status = ulh_close(ulhrcb);

	if (DB_FAILURE_MACRO(temp_status))
	{
	    rdu_ferror(global, temp_status, &ulhrcb->ulh_error,
		       E_RD012A_ULH_CLOSE, 0);
	    if (DB_SUCCESS_MACRO(status))
		status = temp_status;
	}
    }


    /* 2. Shut down the memory from the RDF cache */

    if (Rdi_svcb->rdv_state & RDV_1_RELCACHE_INIT)
    {
	ulmrcb->ulm_facility = DB_RDF_ID;
	ulmrcb->ulm_poolid = Rdi_svcb->rdv_poolid;

	temp_status = ulm_shutdown(ulmrcb);  /* ulm shutdown */

	if (DB_FAILURE_MACRO(temp_status))
	{
	    rdu_ferror(global, temp_status, &ulmrcb->ulm_error,
		E_RD025B_ULMSHUT, 0);
	    if (DB_SUCCESS_MACRO(status))
		status = temp_status;
	}
    }

    return (status);
}

/*{
** Name: rdf_report - report RDF statistics
**
**
** Description:
**
**	This routine outputs the statistics that RDF gathered and kept in
**	Rdi_svcb->rdvstat. These statistics will be output at server shutdown
**	if the user defines II_DBMS_LOG, or otherwise defines TRdisplay
**	output.
**
**	FORMAT:
**
**		          CACHE ALLOCATION/TUNING INFORMATON
** :----------------------------  HARD LIMITS  --------------------------------:
** :cache size in bytes  : xxxxxxxxxx  [Tune w/ RDF_MEMORY parameter]	       :
** :max tables on cache  : xxxxxxxxxx  [Tune w/ RDF_MAX_TABLES parameter]      :
** :max QRY Tree obects  : xxxxxxxxxx  [Tune w/ RDF_MAX_TABLES parameter]      :
** :max LDB descriptors  : xxxxxxxxxx  [Tune as	RDF_AVG_LDBS & RDF_CACHE_DDBS] :
** :max defaults on cache: xxxxxxxxxx  [Tune w/ RDF_COL_DEFAULTS parameter]    :
** :------------------------- STARTUP PARAMETERS-------------------------------:
** :rdf_memory	       : xxxxxxxxxx DEFLT rdf_tbl_synonyms  : xxxxxxxxxx DEFLT :
** :rdf_max_tbls       : xxxxxxxxxx DEFLT rdf_cache_ddbs    : xxxxxxxxxx DEFLT :
** :rdf_col_defaults   : xxxxxxxxxx DEFLT rdf_avg_ldbs      : xxxxxxxxxx DEFLT :
**  (where DEFLT is only specified if the use did not specify the parameter 
**  and this is an RDF default value)
** :---------------------------------------------------------------------------:
**
**		          CATALOG OPERATION STATISTICS
**
** +-------------+-------------------------------------------------------------+
** :  OPERATION  :  REQUEST  ON CACHE     BUILT NOT FOUND CAT APPEND CAT DELETE:
** +-------------:--------- --------- --------- --------- ---------- ----------+
** :Existance Chk:xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx        N/A        N/A:
** :Core Catalog :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx        N/A        N/A:
** :Synonym	 :xxxxxxxxx       N/A xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Statistics   :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx        N/A        N/A:
** :Integrity    :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :View         :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Procedure    :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Rule         :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Event Permits:xxxxxxxxx       N/A xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Permit Tree  :xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :All To All   :      N/A       N/A       N/A	xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Retr to All  :      N/A       N/A       N/A	xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Instal Cats  :      N/A       N/A       N/A	xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
** :Comments Cat :      N/A       N/A       N/A	xxxxxxxxx xxxxxxxxxx xxxxxxxxxx:
**
**		        QRYMOD OPERATION STATISTICS
**
** +-----------------+---------------------------------------------------------+
** :                 :						       MULTIPLE:
** :    OPERATION    :   REQUEST   ON CACHE    BUILT     NOT FOUND   READ REQ'D:
** +-----------------: ---------- ---------- ----------  ----------  ----------:
** :Protection Tuples: xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx  xxxxxxxxxx  xxxxxxxxxx:
** :Integrity Tuples : xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx  xxxxxxxxxx  xxxxxxxxxx:
** :Rule Tuples      : xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx  xxxxxxxxxx  xxxxxxxxxx:
** :Proc Param Tups  : xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx  xxxxxxxxxx  xxxxxxxxxx:
** :Key Tuples       : xxxxxxxxxx xxxxxxxxxx xxxxxxxxxx  xxxxxxxxxx  xxxxxxxxxx:
** :Event Permit Tups: xxxxxxxxxx        N/A xxxxxxxxxx  xxxxxxxxxx  xxxxxxxxxx:
**
** 			    GENERAL STATISTICS
**
** +---------------------------------------------------------------------------+
** :   RESOURCE REQUESTS     :		   ERROR CONDITIONS		       :
** +-------------------------+-------------------------------------------------:
** :Avg Col/Table xxxxxxxxxx : Exception Cnt xxxxxxxxx: No Memory    xxxxxxxxxx:
** :Avg Idx/Table xxxxxxxxxx : Bad Unfix Req xxxxxxxxx: Unfix Errors xxxxxxxxxx:
** :Tables Cached xxxxxxxxxx :						       :
**
**			MEMORY MANAGEMENT STATISTICS
**
** +------------------------------+-------------------------------------------+
** :	         FIX		  :	  UNFIX		  SHARED      PRIVATE :
** +------------------------------+ ----------------   ----------  -----------:
** :Rel Cache Object   xxxxxxxxxx : Rel Cache Object   xxxxxxxxxx  xxxxxxxxxxx:
** :Integrity Tree Obj xxxxxxxxxx : Integrity Tree Obj xxxxxxxxxx  xxxxxxxxxxx:
** :Procedure Object   xxxxxxxxxx : Procedure Object   xxxxxxxxxx  xxxxxxxxxxx:
** :Permit Tree Object xxxxxxxxxx : Permit Tree Object xxxxxxxxxx  xxxxxxxxxxx:
** :Rule Tree Object   xxxxxxxxxx : Rule Tree Object   xxxxxxxxxx  xxxxxxxxxxx:
**
** +--------------------------------------------------------------------------+
** :				LOCK FOR UPDATE				      :
** +--------------------------------------------------------------------------+
** : Shared Cache Object   xxxxxxxxxx : Shared Cache Overflow Ctr  xxxxxxxxxx :
** : Private Cache Object  xxxxxxxxxx : Private Cache Overflow Ctr xxxxxxxxxx :
**
** +--------------------------------------------------------------------------+
** :			 CACHE INVALIDATION REQUESTS			      :
** +------------------------------------+-------------------------------------+
** :  Single Object	xxxxxxxxxxx   :  Tree Class		xxxxxxxxxxx   :
** :  Procedure Object	xxxxxxxxxxx   :  Tree Object		xxxxxxxxxxx   :
** :  RELATION Cache	xxxxxxxxxxx   :                                       :
** :  QTREE Cache	xxxxxxxxxxx   :  Obj not on cache	xxxxxxxxxxx   :
**
** +------------------------------------+-------------------------------------+
** :         MEMORY RECLAIM	        :	     MEMORY REFRESH	      :
** +------------------------------------+-------------------------------------+
** :  Rel Cache           xxxxxxxxxx    :  Rel cache Object       xxxxxxxxxx  :
** :  QT Cache		  xxxxxxxxxx    :  QT Cache Object        xxxxxxxxxx  :
** :  LDB Cache		  xxxxxxxxxx    :				      :
**
**==============================================================================
**
**  MAPPING:
**
** CACHE ALLOCATION/TUNING INFORMATON
** ----------------------------------
** :----------------------------  HARD LIMITS  --------------------------------:
** :cache size in bytes  : Rdi_svcb->RDV_POOL_SIZE
** :max tables on cache  : Rdi_svcb->RDV_CNT0_RDESC
** :max QRY Tree obects  : Rdi_svcb->RDV_CNT1_QTREE
** :max LDB descriptors  : Rdi_svcb->RDV_CNT2_DIST
** :max defaults on cache: Rdi_svcb->rdv_cnt3_DEFAULT
** : RDF_MEMORY: RDV_IN0_RDF_MEMORY	rdf_table_synonyms: RDV_IN3_TABLE_SYNONYMS
** : rdf_max_tables: RDV_IN1_MAX_TABLES  rdf_cache_ddbs:  RDV_IN4_CACHE_DDBS
** : rdf_max_col_default: RDV_IN2_TABLE_COLUMNS  rdf_average_ldbs: RDV_IN5_AVERAGE_LDBS
** :---------------------------------------------------------------------------:
**
** CATALOG OPERATION STATISTICS
** ---------------------------
** Existance Check  : RDS_R0_EXISTS RDS_C0_EXISTS RDS_B0_EXISTS N/A  N/A
** Core Catalog     : RDS_R1_CORE   RDS_C1_CORE   RDS_B1_CORE  N/A  N/A
** Synonym	    : RDS_R16_SYN   N/A  RDS_R16_SYN   RDS_B16_SYN  RDS_A16_SYN RDS_D16_SYN
** Statistics       : RDS_R11_STAT  RDS_C11_STAT  RDS_B11_STAT  N/A  N/A
** Integrity        : RDS_R3_INTEG  RDS_C3_INTEG  RDS_B3_INTEG  RDS_A3_INTEG RDS_D3_INTEG
** View             : RDS_R4_VIEW   RDS_C4_VIEW   RDS_B4_VIEW   RDS_A4_VIEW  RDS_D4_VIEW
** Procedure        : RDS_R5_PROC   RDS_C5_PROC   RDS_B5_PROC   RDS_A5_PROC  RDS_D5_PROC
** Rule             : RDS_R6_RULE   RDS_C6_RULE   RDS_B6_RULE   RDS_A6_RULE  RDS_D6_RULE
** Event Permits    : RDS_R7_EVENT  N/A	          RDS_B7_EVENT  RDS_A7_EVENT RDS_D7_EVENT
** Permit/Protection: RDS_R2_PERMIT RDS_C2_PERMIT RDS_B2_PERMIT RDS_A2_PERMIT RDS_D2_PERMIT
** Sequence         : RDS_R21_SEQS  N/A	          RDS_B21_SEQS  RDS_A21_SEQS RDS_D21_SEQS
** All To All       : N/A  N/A  N/A  RDS_A8_ALLTOALL RDS_D8_ALLTOALL
** Retrieve to All  : N/A  N/A  N/A  RDS_A9_RTOALL   RDS_D9_RTOALL
** Installation Cats: N/A  N/A  N/A  RDS_A10_DBDB    RDS_D10_DBDB
** Comments Catalog : N/A  N/A	N/A  RDS_A17_COMMENT  RDS_D17_COMMENT
**
** QRYMOD OPERATION STATISTICS
** --------------------------
** Protection Tuples: RDS_R14_PTUPS RDS_C14_PTUPS RDS_B14_PTUPS RDS_Z14_PTUPS RDS_M14_PTUPS
** Integrity Tuples : RDS_R12_ITUPS RDS_C12_ITUPS RDS_B12_ITUPS RDS_Z12_ITUPS RDS_M12_ITUPS
** Rule Tuples      : RDS_R15_RTUPS RDS_C15_RTUPS RDS_B15_RTUPS RDS_Z15_RTUPS RDS_M15_RTUPS
** Proc Param Tups  : RDS_R18_PPTUPS RDS_C19_PPTUPS RDS_B18_PPTUPS RDS_Z18_PPTUPS RDS_M18_PPTUPS
** Key Tuples	    : RDS_R19_KTUPS RDS_C19_KTUPS RDS_B19_KTUPS RDS_Z19_KTUPS RDS_M19_KTUPS
** Event Permit Tups: RDS_R13_ETUPS N/A 	  RDS_B13_ETUPS RDS_Z13_ETUPS RDS_M13_ETUPS
**
** GENERAL STATISTICS
** -------------------
** Avg Col/Table RDS_N1_AVG_COL : Exception Cnt RDS_N7_EXCEPTIONS : No Memory    RDS_N10_NOMEM
** Avg Idx/Table RDS_N2_AVG_IDX : Bad Unfix Req RDS_U8_INVALID    : Unfix Errors RDS_U9_FAIL
** Tables Cached RDS_N0_TABLES
**
** MEMORY MANAGEMENT STATISTICS
** ----------------------------
** Fix:				        Unfix:
** Rel Cache Object   RDS_F0_RELATION : Rel Cache Object   RDS_U11_SHARED   RDS_U10_PRIVATE
** Integrity Tree Obj RDS_F1_INTEGRITY: Integrity Tree Obj RDS_U1_INTEGRITY RDS_U5P_INTEGRITY
** Procedure Object   RDS_F2_PROCEDURE: Procedure Object   RDS_U2_PROCEDURE RDS_U6P_PROCEDURE
** Permit Tree Object RDS_F3_PROTECT  : Permit Tree Object RDS_U0_PROTECT   RDS_U4P_PROTECT
** Rule Tree Object   RDS_F4_RULE     : Rule Tree Object   RDS_U3_RULE      RDS_U7P_RULE
**
** Lock for update:
** Shared Cache Object	RDS_F2_SUPDATE : Shared Cache Overflow Ctr  RDS_F3_SUPDATE
** Private Cache Object RDS_F0_PRIVATE : Private Cache Overflow Ctr RDS_F1_PRIVATE
**
** cache invalidation requests:
**   Single Object     RDS_I1_OBJ_INVALID     : Tree Class	  RDS_I4_CLASS_INVALID
**   Procedure Object  RDS_I2_P_INVALID       : Tree Object	  RDS_I5_TREE_INVALID
**   Relation Cache    RDS_I7_CACHE_INVALID   : LDBdesc Object	  RDS_I8_CACHE_INVALID
**   Qtree Cache       RDS_I6_QTCACHE_INVALID : Obj not on cache  RDS_I3_FLUSHED
**
** memory reclaim	      		 memory refresh
**   Rel Cache	RDS_N9_RELMEM_CLAIM  :  Rel cache Object  RDS_X1_RELATION
**   QT Cache   RDS_N8_qtMEM_CLAIM   :  QT Cache Object   RDS_X2_QTREE
**   LDB Cache  RDS_N11_ldbMEM_CLAIM :
**   Defaults	RDF_N12_DEFAULT_CLAIM:
**
** Inputs:
**	stat				pointer to RDF_STATISTICS
** Outputs:
**	none
**
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		Statistical information is output IFF TRdisplay output
**		is assigned.
** History:
**	01-feb-91 (teresa)
**          initial creation.
**	07-jul-92 (teresa)
**	    update statistics reporting for sybil.
**	16-jul-92 (teresa)
**	    prototypes
**	16-sep-92 (teresa)
**	    report rds_i8_cache_invalid statistic.
**	29-jan-93 (teresa)
**	    added support for procedure parameter and key statistics reporting.
**	 1-dec-93 (robf)
**          added support for security alarm stats gathering
**	19-mar-02 (inkdo01)
**	    Added sequence statistics.
**	3-Dec-2003 (schka24)
**	    Correct parameter name lies, fix char var length.
*/

VOID
rdf_report(RDF_STATISTICS *stat)
{
#define		BLANKS	"     "
#define		DEFLT   "DEFLT"

    i4	cols;
    i4	idxs;
    float	round;
    char	default1[sizeof(DEFLT)];	/* Yes this includes the null */
    char	default2[sizeof(DEFLT)];
    char	*def1= &default1[0];
    char	*def2= &default2[0];
    i4	int1, int2;

    round = stat->rds_n1_avg_col + 0.5;
    cols = round;
    round = stat->rds_n2_avg_idx + 0.5;
    idxs = round;

    TRdisplay(  "\n\n--------------------------  RDF Statistics Report: -----------------------------\n\n\n");
    /* section 1: report cache tuning information */
    TRdisplay(  "		     CACHE ALLOCATION/TUNING INFORMATON\n\n");
    TRdisplay(  " :----------------------------  HARD LIMITS  --------------------------------:\n");

    TRdisplay(  " :cache size in bytes  : %10d  [Tune w/ RDF_MEMORY parameter]          :\n",
		Rdi_svcb->rdv_pool_size);
    TRdisplay(  " :max tables on cache  : %10d  [Tune w/ RDF_MAX_TABLES parameter]      :\n",
		Rdi_svcb->rdv_cnt0_rdesc);
    TRdisplay(  " :max QRY Tree obects  : %10d  [Tune w/ RDF_MAX_TABLES parameter]      :\n",
		Rdi_svcb->rdv_cnt1_qtree);
    TRdisplay(  " :max LDB descriptors  : %10d  [Tune as RDF_AVG_LDBS * RDF_CACHE_DDBS] :\n",
		Rdi_svcb->rdv_cnt2_dist);
    TRdisplay(  " :max defaults on cache: %10d  [Tune w/ RDF_COL_DEFAULTS parameter]    :\n",
		Rdi_svcb->rdv_cnt3_default);
    TRdisplay(  " :------------------------- STARTUP PARAMETERS-------------------------------:\n");
    
    if (Rdi_svcb->rdv_in0_memory == RD_NOTSPECIFIED)
    {
	int1 = Rdi_svcb->rdv_pool_size;
	STcopy(DEFLT,def1);
    }
    else
    {
	int1 = Rdi_svcb->rdv_in0_memory;
	STcopy(BLANKS,def1);
    }
    if (Rdi_svcb->rdv_in3_table_synonyms == RD_NOTSPECIFIED)
    {
	i4	objcnt;

	objcnt = (Rdi_svcb->rdv_in1_max_tables == RD_NOTSPECIFIED) ? 
		    RDF_MTRDF : Rdi_svcb->rdv_in1_max_tables;
	int2 =  RDF_SYNCNT * objcnt;
	STcopy(DEFLT,def2);
    }
    else
    {
	int2 = Rdi_svcb->rdv_in3_table_synonyms;
	STcopy(BLANKS,def2);
    }
    TRdisplay(" :rdf_memory         : %10d %5s rdf_tbl_synonyms  : %10d %5s :\n",
	    int1, def1, int2, def2);

    if (Rdi_svcb->rdv_in1_max_tables == RD_NOTSPECIFIED)
    {
	int1 = RDF_MTRDF;
	STcopy(DEFLT,def1);
    }
    else
    {
	int1 = Rdi_svcb->rdv_in1_max_tables;
	STcopy(BLANKS,def1);
    }
    if (Rdi_svcb->rdv_in4_cache_ddbs == RD_NOTSPECIFIED)
    {
	int2 = Rdi_svcb->rdv_max_sessions;
	STcopy(DEFLT,def2);
    }
    else
    {
	int2 = Rdi_svcb->rdv_in4_cache_ddbs;
	STcopy(BLANKS,def2);
    }
    TRdisplay(" :rdf_max_tbls       : %10d %5s rdf_cache_ddbs    : %10d %5s :\n",
	    int1, def1, int2, def2);

    if (Rdi_svcb->rdv_in2_table_columns == RD_NOTSPECIFIED)
    {
	int1 = RDF_COLDEFS;
	STcopy(DEFLT,def1);
    }
    else
    {
	int1 = Rdi_svcb->rdv_in2_table_columns;
	STcopy(BLANKS,def1);
    }
    if (Rdi_svcb->rdv_in5_average_ldbs == RD_NOTSPECIFIED)
    {
	int2 = RDF_AVGLDBS;
	STcopy(DEFLT,def2);
    }
    else
    {
	int2 = Rdi_svcb->rdv_in5_average_ldbs;
	STcopy(BLANKS,def2);
    }
    TRdisplay(" :rdf_col_defaults   : %10d %5s rdf_avg_ldbs      : %10d %5s :\n",
	    int1, def1, int2, def2);




    TRdisplay(  " :---------------------------------------------------------------------------:\n");

    /* section 2: report statistics on catalog operations */
    TRdisplay(  "		        CATALOG OPERATION STATISTICS\n\n");
    TRdisplay(  " +-------------+-------------------------------------------------------------+\n");
    TRdisplay(  " :  OPERATION  :  REQUEST  ON CACHE     BUILT NOT FOUND CAT APPEND CAT DELETE:\n");
    TRdisplay(  " +-------------:--------- --------- --------- --------- ---------- ----------+\n");
    TRdisplay(  " :Existance Chk:%9d %9d %9d %9d        N/A        N/A:\n",
	stat->rds_r0_exists, 
	stat->rds_c0_exists, 
	stat->rds_b0_exists,
	stat->rds_z0_exists);
    TRdisplay(  " :Core Catalog :%9d %9d %9d %9d        N/A        N/A:\n",
	stat->rds_r1_core,
	stat->rds_c1_core,
	stat->rds_b1_core,
	stat->rds_z1_core);
    TRdisplay(  " :Synonym	:%9d       N/A %9d %9d %10d %10d:\n",
	stat->rds_r16_syn,
	stat->rds_b16_syn,
	stat->rds_z16_syn,
	stat->rds_a16_syn,
	stat->rds_d16_syn);
    TRdisplay(  " :Statistics   :%9d %9d %9d %9d        N/A        N/A:\n",
	stat->rds_r11_stat,
	stat->rds_c11_stat,
	stat->rds_b11_stat,
	stat->rds_r11_stat - stat->rds_c11_stat - stat->rds_b11_stat);
    TRdisplay(  " :Integrity    :%9d %9d %9d %9d %10d %10d:\n",
	stat->rds_r3_integ,
	stat->rds_c3_integ,
	stat->rds_b3_integ,
	stat->rds_r3_integ - stat->rds_c3_integ - stat->rds_b3_integ,
	stat->rds_a3_integ,
	stat->rds_d3_integ);
    TRdisplay(  " :View         :%9d %9d %9d %9d %10d %10d:\n",
	stat->rds_r4_view,
	stat->rds_c4_view,
	stat->rds_b4_view,
	stat->rds_r4_view - stat->rds_c4_view - stat->rds_b4_view,
	stat->rds_a4_view,
	stat->rds_d4_view);
    TRdisplay(  " :Procedure    :%9d %9d %9d %9d %10d %10d:\n",
	stat->rds_r5_proc,
	stat->rds_c5_proc,
	stat->rds_b5_proc,
	stat->rds_r5_proc - stat->rds_c5_proc -	stat->rds_b5_proc,
	stat->rds_a5_proc,
	stat->rds_d5_proc);
    TRdisplay(  " :Rule         :%9d %9d %9d %9d %10d %10d:\n",
	stat->rds_r6_rule,
	stat->rds_c6_rule,
	stat->rds_b6_rule,
	stat->rds_r6_rule - stat->rds_c6_rule - stat->rds_b6_rule,
	stat->rds_a6_rule,
	stat->rds_d6_rule);
    TRdisplay(  " :Event Permits:%9d       N/A %9d %9d %10d %10d:\n",
	stat->rds_r7_event,
	stat->rds_b7_event,
	stat->rds_r7_event - stat->rds_b7_event,
	stat->rds_a7_event,
	stat->rds_d7_event);
    TRdisplay(  " :Permit Tree  :%9d %9d %9d %9d %10d %10d:\n",
	stat->rds_r2_permit,
	stat->rds_c2_permit,
	stat->rds_b2_permit,
	stat->rds_r2_permit - stat->rds_c2_permit - stat->rds_b2_permit,
	stat->rds_a2_permit,
	stat->rds_d2_permit);
    TRdisplay(  " :Sequence     :%9d       N/A %9d %9d %10d %10d:\n",
	stat->rds_r21_seqs,
	stat->rds_b21_seqs,
	stat->rds_r21_seqs - stat->rds_b21_seqs,
	stat->rds_a21_seqs,
	stat->rds_d21_seqs);
    TRdisplay(  " :Security Alrm:%9d %9d %9d %9d %10d %10d:\n",
	stat->rds_r20_alarm,
	stat->rds_c20_alarm,
	stat->rds_b20_alarm,
	stat->rds_r20_alarm - stat->rds_c20_alarm - stat->rds_b20_alarm,
	stat->rds_a20_alarm,
	stat->rds_d20_alarm);
    TRdisplay(  " :All To All   :      N/A       N/A       N/A       N/A %10d %10d:\n",
	stat->rds_a8_alltoall,
	stat->rds_d8_alltoall);
    TRdisplay(  " :Retr to All  :      N/A       N/A       N/A       N/A %10d %10d:\n",
	stat->rds_a9_rtoall,
	stat->rds_d9_rtoall);
    TRdisplay(  " :Instal Cats  :      N/A       N/A       N/A       N/A %10d %10d:\n",
	stat->rds_a10_dbdb,    
	stat->rds_d10_dbdb);
    TRdisplay(  " :Comments Cat :      N/A       N/A       N/A       N/A %10d %10d:\n\n",
	stat->rds_a17_comment,    
	stat->rds_d17_comment);

    /* section 3: report statistics on QTUPLE_ONLY operations 
    **		  (ie, obtaining tuple from catalogs for QRYMOD */
    TRdisplay(  "		        QRYMOD OPERATION STATISTICS\n\n");
    TRdisplay(  " +-----------------+---------------------------------------------------------+\n");
    TRdisplay(  " :                 :						      MULTIPLE:\n");
    TRdisplay(  " :    OPERATION    :   REQUEST   ON CACHE    BUILT     NOT FOUND   READ REQ'D:\n");
    TRdisplay(  " +-----------------: ---------- ---------- ----------  ----------  ----------:\n");
    TRdisplay(  " :Protection Tuples: %10d %10d %10d  %10d  %10d:\n",
	stat->rds_r14_ptups,
	stat->rds_c14_ptups,
	stat->rds_b14_ptups,
	stat->rds_z14_ptups,
	stat->rds_m14_ptups);
    TRdisplay(  " :Integrity Tuples : %10d %10d %10d  %10d  %10d:\n",
	stat->rds_r12_itups,
	stat->rds_c12_itups,
	stat->rds_b12_itups,
	stat->rds_z12_itups,
	stat->rds_m12_itups);
    TRdisplay(  " :Rule Tuples      : %10d %10d %10d  %10d  %10d:\n",
	stat->rds_r15_rtups,
	stat->rds_c15_rtups,
	stat->rds_b15_rtups,
	stat->rds_z15_rtups,
	stat->rds_m15_rtups);
    TRdisplay(  " :Proc Param Tups  : %10d %10d %10d  %10d  %10d:\n",
	stat->rds_r18_pptups,
	stat->rds_c18_pptups,
	stat->rds_b18_pptups,
	stat->rds_z18_pptups,
	stat->rds_m18_pptups);
    TRdisplay(  " :Key Tuples       : %10d %10d %10d  %10d  %10d:\n",
	stat->rds_r19_ktups,
	stat->rds_c19_ktups,
	stat->rds_b19_ktups,
	stat->rds_z19_ktups,
	stat->rds_m19_ktups);
    TRdisplay(  " :Event Permit Tups: %10d        N/A %10d  %10d  %10d:\n",
	stat->rds_r13_etups,
	stat->rds_b13_etups,
	stat->rds_z13_etups,
	stat->rds_m13_etups);
    TRdisplay(  " :Alarm Tuples     : %10d %10d %10d  %10d  %10d:\n\n",
	stat->rds_r20_atups,
	stat->rds_c20_atups,
	stat->rds_b20_atups,
	stat->rds_z20_atups,
	stat->rds_m20_atups);
	/* event permit tuples are not cached */

    /* section 4: General Statistics */
    TRdisplay(  " 			    GENERAL STATISTICS\n\n");
    TRdisplay(  " +---------------------------------------------------------------------------+\n");
    TRdisplay(  " :   RESOURCE REQUESTS     :		   ERROR CONDITIONS		      :\n");
    TRdisplay(  " +-------------------------+-------------------------------------------------:\n");
    TRdisplay(  " :Avg Col/Table %10d : Exception Cnt %9d: No Memory    %10d:\n",
	cols,
	stat->rds_n7_exceptions,
	stat->rds_n10_nomem);
    TRdisplay(  " :Avg Idx/Table %10d : Bad Unfix Req %9d: Unfix Errors %10d:\n",
	idxs,
	stat->rds_u8_invalid,
	stat->rds_u9_fail);
    TRdisplay(  " :Tables Cached %10d :						      :\n\n",
	stat->rds_n0_tables);


    /* section 5: Memory Management Statistics */
    TRdisplay(  "			MEMORY MANAGEMENT STATISTICS\n\n");
    TRdisplay(  " +------------------------------+-------------------------------------------+\n");
    TRdisplay(  " :	         FIX		 :	 UNFIX		SHARED      PRIVATE  :\n");
    TRdisplay(  " +------------------------------+ ----------------   ----------  -----------:\n");
    TRdisplay(  " :Rel Cache Object   %10d : Rel Cache Object   %10d  %11d:\n",
	stat->rds_f0_relation,
	stat->rds_u11_shared,
	stat->rds_u10_private);
    TRdisplay(  " :Integrity Tree Obj %10d : Integrity Tree Obj %10d  %11d:\n",
	stat->rds_f1_integrity,
	stat->rds_u1_integrity,
	stat->rds_u5p_integrity);
    TRdisplay(  " :Procedure Object   %10d : Procedure Object   %10d  %11d:\n",
	stat->rds_f2_procedure,
	stat->rds_u2_procedure,
	stat->rds_u6p_procedure);
    TRdisplay(  " :Permit Tree Object %10d : Permit Tree Object %10d  %11d:\n",
	stat->rds_f3_protect,
	stat->rds_u0_protect,
	stat->rds_u4p_protect);
    TRdisplay(  " :Rule Tree Object   %10d : Rule Tree Object   %10d  %11d:\n\n",
	stat->rds_f4_rule,
	stat->rds_u3_rule,
	stat->rds_u7p_rule);

    /* section 6: Update/lock Statistics */
    TRdisplay(  " +--------------------------------------------------------------------------+\n");
    TRdisplay(  " :				LOCK FOR UPDATE				     :\n");
    TRdisplay(  " +--------------------------------------------------------------------------+\n");
    TRdisplay(  " : Shared Cache Object   %10d : Shared Cache Overflow Ctr  %10d :\n",
	stat->rds_l2_supdate,
	stat->rds_l3_supdate);
    TRdisplay(  " : Private Cache Object  %10d : Private Cache Overflow Ctr %10d :\n\n",
	stat->rds_l0_private,
	stat->rds_l1_private);

    /* section 7: Cache Invalidation Statistics */
    TRdisplay(  " +--------------------------------------------------------------------------+\n");
    TRdisplay(  " :			 CACHE INVALIDATION REQUESTS			     :\n");
    TRdisplay(  " +------------------------------------+-------------------------------------+\n");
    TRdisplay(  " :  Single Object	%11d   :  Tree Class		%11d  :\n",
	stat->rds_i1_obj_invalid,
	stat->rds_i4_class_invalid);
    TRdisplay(  " :  Procedure Object	%11d   :  Tree Object		%11d  :\n",
	stat->rds_i2_p_invalid,
	stat->rds_i5_tree_invalid);
    TRdisplay(  " :  RELATION Cache	%11d   :  LDBDESC Cache         %11d  :\n",
	stat->rds_i7_cache_invalid,
	stat->rds_i8_cache_invalid);
    TRdisplay(  " :  QTREE Cache	%11d   :  Obj not on cache	%11d  :\n\n",
	stat->rds_i6_qtcache_invalid,
	stat->rds_i3_flushed);

    /* section 8: Memory reclaim, memory refresh Statistics */
    TRdisplay(  " +------------------------------------+-------------------------------------+\n");
    TRdisplay(  " :         MEMORY RECLAIM	       :	    MEMORY REFRESH	     :\n");
    TRdisplay(  " +------------------------------------+-------------------------------------+\n");
    TRdisplay(  " :  Rel Cache           %10d    :  Rel cache Object       %10d  :\n",
	stat->rds_n9_relmem_claim,
	stat->rds_x1_relation);
    TRdisplay(  " :  QT Cache		 %10d    :  QT Cache Object        %10d  :\n",
	stat->rds_n8_QTmem_claim,
	stat->rds_x2_qtree);
    TRdisplay(  " :  LDB Cache		 %10d    :				     :\n\n",
	stat->rds_n11_LDBmem_claim);
    TRdisplay(  " :  Defaults Cache	 %10d    :				     :\n\n",
	stat->rdf_n12_default_claim);
    TRdisplay(  "note: a single table may show up several times in the Tables Cached count if it is\n");
    TRdisplay(  "      flushed from cache several times.\n\n");
    TRdisplay(  "\n----------------------------  End RDF Statistics   ------------------------------\n\n\n");
}
