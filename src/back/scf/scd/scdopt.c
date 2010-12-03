/*
**Copyright (c) 2005, 2010 Ingres Corporation
**
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <cm.h>
#include    <cs.h>
#include    <cv.h>
#include    <tm.h>
#include    <ci.h>
#include    <er.h>
#include    <ex.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <st.h>
#include    <lk.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <qsf.h>
#include    <ddb.h>
#include    <opfcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <scf.h>
#include    <sxf.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <gca.h>
#include    <gwf.h>
#include    <lg.h>
#include    <uld.h>

/* added for scs.h prototypes, ugh! */
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>
#include    <scdopt.h>

/*
** Name: scdopt.c - fetch options during server startup
**
** History:
**	1-Sep-93 (seiwald)
**	    Written.
**	12-nov-93 (stephenb)
**	    Changed !c2.mode to ii.*.c2.security_auditing, firstly to be 
**	    compatible with CBF. and secondly because we cannot allow 
**	    users to have separate C2 states for different server 
**	    types/flavours, auditing must be on or off for all servers on all
**	    nodes of the installation.
**	17-dec-93 (rblumer)
**	    removed SCO_FIPS startup option.
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).
**	6-jan-94 (robf)
**          Move new Secure parameters in.
**	8-feb-94 (robf)
**          Allow secure level to be specified by node
**	14-feb-94 (rblumer)
**	    changed SCO_CSRDEFER/SCO_CSRDIRECT on/off options to
**	    SCO_CSRUPDATE deferred/direct option (see 09-feb-94 LRC, B59119).
**	    replaced FLATOPT and FLATNONE options with SCO_FLATTEN option;
**	    got rid of sc_noflatten field  (see 09-feb-94 LRC, B59120).
**	12-may-94 (rog)
**	    Added undocumented SCO_ULM_CHUNK_SIZE parameter to allow ULM to
**	    be tuned.
**	16-sep-1994 (mikem)
**	    bug #56647
**	    Added dmf_int_sort_size parameter which allows users to set the
**	    threshold of when dmf will go to disk for doing a sort.
**	24-apr-1995 (cohmi01)
**	    Added options for IOMASTER  server.
**	04-may-1995 (cohmi01)
**	    Add batchmode parm - sets SC_BATCH_SERVER bit in Sc_main_cb.
**      24-jul-1995 (lawst01)
**          Add gca single user parm - sets SC_GCA_SINGLE_MODE bit in   
**          SC_main_cb.
**	01-mar-1996 (nick)
**	    Private dmf cache values were never being found and hence
**	    defaults were being used. #75017
**      06-mar-1996 (stial01)
**          Added dmf buffer cache parameters for 4k,8k,16k,32k,64k pools
**          Added dmf_maxtuplen
**      06-mar-1996 (nanpr01)
**          Private cache parameter was not being read. The reason being,
**	    shared cachename is always defined. Consequently, a wrong parmeter
**	    was generated. It should check for the sharedcache, rather than
**	    cachename.
**	    Also changed the name of dmf_maxtuplen to just max_tuple_length 
**	    since star also uses this parameter and does not have dmf. The 
**	    sco_special field for this parameter is set to blank instead of
**	    3.
**	4-jun-96 (stephenb)
**	    Add SCO_REP_QMAN and SCO_REP_QSIZE options for DBMS replication.
**	06-jun-1996 (thaju02)
**	    Support of Variable Page Size Project:
**	    Modified cache buffer parameters due to changes made to front
**	    end (cbf).  Added cache status flags for each of the caches
**	    (...cache.p<#>k_status) in the scd_opttab[]. Note that
**	    the p<#>k_status parameter MUST PRECEDE the <#>k cache parameters,
**	    in the scd_opttab[], where <#> can be 4, 8, 16, 32, 64.  Thus
**	    if cache status is "OFF", we do not put cache parameters in control 
**	    blocks.
**	16-jul-96 (inkdo01)
**	    Added parameter to force pre-2.0 OPF join cardinality
**	    computation (opf_old_jcard).
**	24-jul-96 (nick)
**	    Default table journaling PM is 'default_journaling' and not
**	    'default_table-journaling'. #77945
**	09-apr-97 (cohmi01)
**	    Allow readlock, timeout & maxlocks system defaults to be 
**	    configured. #74045, #8616.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced support for Distributed Multi-Cache Management (DMCM).
**          Defined new constant SCO_DMCM and PM string "!.dmcm".
**      25-nov-1996 (nanpr01)
**          Sir 79075 : allow setting the default page size through CBF.
**	12-dec-96 (inkdo01)
**	    Added opf_maxmemf parameter to define proportion of OPF memory
**	    pool available to any single session.
**      14-mar-97 (inkdo01)
**          Added opf_spat_key to assign default selectivity to spatial preds.
**	08-apr-1997 (nanpr01)
**	    Added back the dropped dmf_hash_size parameter. This is really
**	    the hash size for TCB cache and has noting to do with dmf
**	    cache parameter. Also subtract 1 from the value to dmc_start
**	    since CBF sets this value to POWER2 not (POWER2-1).
**	16-may-97 (inkdo01)
**	    Added parameter to force pre-2.0 OPF logic in the handling of 
**	    conjunctive normal form transformation of Boolean expressions. 
**	18-jul-97 (inkdo01)
**	    Change opf_maxmemf CBF value from float (between 0 and 1) to
**	    int (percent value), and convert it to float proportion here.
**      14-oct-97 (stial01)
**          Changes to distinguish readlock value/isolation level
**          Isolation levels are not valid readlock values.
**          Added support for system isolation level SCO_SYS_ISOLATION
**          Added SCO_LOG_READNOLOCK: log message if readlock=nolock reduces
**          isolation level, default = OFF.
**	27-aug-1997 (bobmart)
**	    Added log_esc_lpr_sc, log_esc_lpr_ut, log_esc_lpt_sc
**	    and log_esc_lpt_ut, to control logging of escalation
**	    messages on locks per transaction and locks per resource
**	    for both system catalogs and user tables.
**	17-nov-97 (inkdo01)
**	    Added opf_cnffact as parm which is multiplied against
**	    fax resulting from CNF transform, then compared against MAXBF
**	    to determine whether to avoid CNF transform.
**	25-nov-97 (stephenb)
**	    add new parameters to define replicator locking strategy.
**	31-aug-98 (sarjo01)
**	    Added new SCO_JOINOP_TIMEOUT param.
**	13-May-1998 (jenjo02)
**	    Replaced SCO_WRITEBEHIND with per-cache boolean parameters
**	    for cache-specific WriteBehind threads.
**	26-may-1998 (nanpr01)
**	    Added 2 new paramter to configure the exchange buffer for
**	    creating parallel index.
**	15-sep-1998 (hanch04)
**	    Fix bad integration.  Renumber SCO_JOINOP_TIMEOUT.
**	15-sep-98 (sarjo01)
**	    Added new SCO_JOINOP_TIMEOUTABORT param.
**	14-dec-1998 (thaju02)
**	    disallow btree extension tables. This check has been moved
**	    from dmpe.c to scd_opt().
**	27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      31-aug-1999 (stial01)
**          Added blob_etab_page_size
**      20-sep-1999 (stial01)
**          Btree extension tables allowed again.
**	29-jun-99 (hayke02)
**	    Added opf_stats_nostats_max to limit tuple estimates from
**	    joins between table with stats and table without. This change
**	    fixes bug 97450.
**	28-jan-00 (inkdo01)
**	    Added qef_hash_mem for hash join implementation.
**      10-Mar-2000 (wanfr01)
**          Bug 100847, INGSRV 1127:
**          Added sc_assoc_request field to allow timing out sessions who
**          connect directly to the server without communicating.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	21-jul-00 (inkdo01)
**	    Add support for psf_maxmemf option, like opf_maxmemf.
**	30-aug-00 (inkdo01)
**	    Add opf_rangep_max to control QEF key strats of host var range preds.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	3-nov-00 (inkdo01)
**	    Added opf_cstimeout to allow user control over OPF time slicing.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Added rule_upd_prefetch as a boolean server config parameter
**          with a default of ON. If set to OFF, sc_rule_upd_prefetch_off
**          must be set to TRUE.
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          Allow priority of Event Thread to be configurable.
**	09-mar-2001 (stial01)
**          Added SCO_PAGETYPE_V*
**      06-mar-2001 (stial01)
**          Fixed SCO_PAGETYPE_V* code to increment dca
**      04-dec-2001 (stial01)
**          Moved SCO_PAGETYPE from cache parameter section to random parameter
**      06-feb-2002 (stial01)
**          Added ulm_pad_bytes
**	16-jul-2002 (toumi01)
**	    Add config parameter for hash join (default is off); this startup
**	    setting is toggled by OP162.
**	22-jul-02 (hayke02)
**	    Added opf_stats_nostats_factor to scale the opf_stats_nostats_max
**	    calculation. This change fixes bug 108327.
**	14-jan-03 (inkdo01)
**	    Added CBF parm opf_new_enum to enable new OPF enumeration algorithm.
**	21-mar-03 (hayke02)
**	    Added opf_old_subsel to switch off the 'not in'/'not exists'
**	    subselect to outer join transform. This change fixes bug 109670.
**	14-apr-03 (toumi01)
**	    Fix opf_hash_join processing. Since it is now ON by default (in
**	    scdinit.c) the option type must be 'z' (ON/OFF defaults to ON)
**	    instead of 't' (ON/OFF defaults to OFF), else the semantic routine
**	    is only called when config.dat turns the parameter ON. Hence it
**	    was impossible to turn it OFF. In the semantics, turn it OFF.
**	16-jul-03 (hayke02)
**	    Added psf_vch_prec to allow varchar precedence rather than the
**	    default char precedence. This change fixes bug 109134.
**      16-sep-03 (stial01)
**          Added SCO_ETAB_TYPE (SIR 110923)
**	10-nov-03 (inkdo01)
**	    Added SCO_OPF_PQDOP, SCO_OPF_PQTHRESH for parallel query 
**	    processing.
**	3-Dec-2003 (schka24)
**	    Remove some unused parameters, mostly RDF;  pass a QEF cb.
**	8-Mar-2004 (schka24)
**	    Add dmf_tcb_limit.
**	25-mar-04 (inkdo01)
**	    Change "opf_new_enum" (a.k.a. "greedy") to default to "on".
**      11-may-04 (stial01)  
**          Removed unnecessary max_btree_key
**      26-jul-04 (hayke02)
**	    Add opf_old_idxorder to use reverse var order when adding useful
**	    secondary indices. This change fixes problem INGSRV 2919, bug
**	    112737.
**	17-aug-2004 (thaju02)
**	    For now, if we encounter integer overflow, cap it at 2Gig so
**	    we can get the server up.
**      31-aug-2004 (sheco02)
**          X-integrate change 466442 to main and bump up SCO_PSF_VCH_PREC
**	    from 185 to 195.
**      20-sep-2004 (devjo01)
**          Add support for SCO_CLASS_NODE_AFFINITY option.
**      06-oct-2004 (stial01)
**          Added SCO_PAGETYPE_V5
**	18-oct-2004 (shaha03)
**	    SIR #112918, Added configurable default cursor open mode support.
**	    Added SCO_DEFCRS_MODE.
**	03-nov-2004 (thaju02)
**	    Define opf_memory, psf_memory, qef_sorthash_memory, qsf_memory,
**	    rdf_memory and sxf_memory as SIZE_TYPE.
**      08-nov-2004 (stial01)
**          Added degree_of_parallelism
**      16-dec-2004 (stial01)
**          Changes for config param system_lock_level
**      30-mar-2005 (stial01)
**          Added dmf_pad_bytes
**	24-Oct-2005 (schka24)
**	    Added qef_max_mem_sleep
**	23-Nov-2005 (kschendel)
**	    Added result_structure
**	27-july-06 (dougi)
**	    Added sort_cpu_factor, sort_io_factor.
**	24-aug-06 (toumi01)
**	    Add PSQ_GTT_SYNTAX_SHORTCUT to control GTT "session." optional
**	    feature.
**      30-Aug-2006 (kschendel)
**          Added dmf_build_pages.
**	30-aug-2006 (thaju02)
**	    Add server config parameter rule_del_prefetch, boolean with
**	    default value of ON. If rule_del_prefetch is set to OFF, 
**	    sc_rule_del_prefetch_off is to be set to TRUE. (B116355)
**	31-aug-06 (toumi01)
**	    Delete PSQ_GTT_SYNTAX_SHORTCUT.
**	03-oct-2006 (gupsh01)
**	    Added SCO_DATE_TYPE_ALIAS.
**	6-Mar-2007 (kschendel) SIR 122512
**	    Added new qef hash parameters.
**	13-may-2007 (dougi)
**	    Added SCO_AUTOSTRUCT.
**      06-jun-2007 (huazh01)
**          Add SCO_QEF_NO_DEPENDENCY_CHK for the config parameter which 
**          switches ON/OFF the fix to b112978. 
**	26-Jun-2007 (kschendel) SIR 122890
**	    Strangely enough, the scan_factor params for other than 2K
**	    were never implemented!
**	    (jgramling) Make sure that qef_dsh_memory can accept a full
**	    SIZE_TYPE parameter, ie > 2 Gb.
**	10-Jul-2007 (kibro01) b118702
**	    Add new 'ii.*.config.date_alias' parameter and deprecate
**	    old 'ii.$.dbms.*.date_type_alias'
**      05-Nov-2007 (wanfr01)
**          SIR 119419
**          Added core_enabled parameter
**	3-march-2008 (dougi)
**	    Added cache_dynamic for cached dynamic query plans.
**	30-oct-08 (hayke02)
**	    Added opf_greedy_factor to adjust ops_cost/jcost comparison in
**	    opn_corput() for greedy enum. This change fixes bug 121159.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added SCO_MLLIST to hold the mustlog_db_list for a given DBMS.
**	26-Aug-2009 (kschendel) 121804
**	    Need uld.h to satisfy gcc 4.3.
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added SCO_FLATNCARD for
**	    .cardinality_check, defaulting to on.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Also pick up on cardinality errors if requested in OPF.
**	03-Dec-2009 (kiria01) b122952
**	    Add .opf_inlist_thresh for control of eq keys on large inlists.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add SCO_PAGETYPE_V6, SCO_PAGETYPE_V7,
**          system_lock_level "mvcc"
**      11-Jan-2010 (hanal04) bug 120482
**          Pick up opf_pq_dop and change the default to 0 (OFF). This
**          allows those that want to use parallel query to use it as
**          documented.
**	13-Apr-2010 (toumi01) SIR 122403
**	    Add CRYPT_MAXKEYS for data encryption a rest.
**	08-Mar-2010 (thaju02)
**	    Remove max_tuple_length.
**	29-apr-2010 (stephenb)
**	    Add option "!.batch_copy_optim"
**	20-Jul-2010 (kschendel) SIR 124104
**	    Add create-compression.
**	29-Jul-2010 (miket) BUG 124154
**	    Improve dmf_crypt_maxkeys handling.
**	    Supply missing break in CRYPT_MAXKEYS.
**      11-Aug-2010 (hanal04) Bug 124180
**          Added money_compat for backwards compatibility of money
**          string constants.
**	14-Oct-2010 (kschendel) SIR 124544
**	    Result-structure is handled by PSF now, not OPF.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Delete sca.h include. Function prototypes moved to
**	    scf.h for exposure to DMF.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for setting installation wide collation defaults
**	    .default_collation & .default_unicode_collation.
*/

/*
** Name: SCD_OPT - SCF startup option table
**
** Description:
**	This entire structure is solely for the use of scd_options().
**
**	    sco_index contains a flag used to distribute the option value
**	    to the proper location in the right SCF control block.  
**	
**	    sco_argtype specifies the type of flag:
**	    	't'	value "on" or "off" - default "off"
**	    	'z'	value "on" or "off" - default "on"
**	    	'o'	nonnegative integer value
**		's'	nonnegative SIZE_TYPE value
**	    	'g'	string value
**	    	'f'	nonnegative floating point value
**	    	'F'	positive floating point value
**	    	'P'	percentage floating point value
**	
**	    sco_special accomodates the oddball uses of the parameters:
**	    	'1' switches the defaults around for DMF parameters
**		'2' switches the defaults back afterwards
**		'3' (as well as '1' and '2') are to be skipped by STAR.
**		
**	    sco_name is the name of the parameter, as handed to PMget().
*/

typedef struct _SCD_OPT SCD_OPT;

struct _SCD_OPT {
	short	sco_index;
	char	sco_argtype;
	char	sco_special;
	char	*sco_name;
} ;

# define	SCO_ACTIVE             	  1
# define	SCO_C2_MODE            	  2
# define	SCO_CACHENAME          	  3
# define	SCO_CHECK_DEAD         	  4
# define	SCO_COMPLETE           	  5
# define	SCO_CP_TIMER           	  6
# define	SCO_QEF_DSHMEM		  7	/* QEF DSH pool limit */
# define	SCO_CSRUPDATE          	  8
# define	SCO_CURS_LIMIT         	  9
# define	SCO_DBCACHE_SIZE       	 10
# define	SCO_DBCNT              	 11
# define	SCO_DBLIST             	 12
# define	SCO_DBUFFERS           	 13
# define	SCO_DGBUFFERS          	 14
# define	SCO_DGCOUNT            	 15
# define	SCO_DMSCANFACTOR       	 16
# define	SCO_DTCB_HASH          	 17
# define	SCO_DX_NORECOVR        	 18
# define	SCO_ECHO               	 19
# define	SCO_EVENTS             	 20
# define	SCO_EXACTKEY           	 21
# define	SCO_FAST_COMMIT        	 22
# define        SCO_DMCM                 23
# define	SCO_FLATTEN           	 24
# define	SCO_FLATNAGG           	 25
# define	SCO_FLATNSING          	 26
# define	SCO_OPOLDJCARD		 27
# define	SCO_FLIMIT             	 28
# define	SCO_GC_INTERVAL        	 29
# define	SCO_GC_NUMTICKS        	 30
# define	SCO_GC_THRESHOLD       	 31
# define	SCO_LOCK_CACHE         	 32
# define	SCO_LOGWRITER          	 33
# define	SCO_MLIMIT             	 34
# define	SCO_NAMES_REG          	 35
# define	SCO_NONKEY             	 36
# define	SCO_NOSTARCLUSTER      	 37
# define	SCO_OPCPUFACTOR        	 38
# define	SCO_OPF_MEM            	 39
# define	SCO_OPREPEAT           	 40
# define	SCO_OPSORTMAX          	 41
# define	SCO_OPTIMEOUT          	 42
# define	SCO_PSF_MEM            	 43
# define	SCO_QESORTSIZE         	 44
# define	SCO_QP_SIZE            	 45
# define	SCO_QS_POOL            	 46
# define	SCO_RANGEKEY           	 47
# define	SCO_RDCOLAVG           	 48
# define	SCO_QEF_SHMEM        	 49	/* QEF sort+hash memory */
# define	SCO_RDFDDBS            	 50
# define	SCO_DTCB_LIMIT		 51	/* dmf tcb_limit */
# define	SCO_DOP			 52	/* degree of parallelism */
# define	SCO_RDF_AVGLDBS        	 53
# define	SCO_DMF_PAD_BYTES	 54	/* dmf pad bytes */
# define	SCO_RDMEMORY           	 55
# define	SCO_RDMULTICACHE       	 56
# define	SCO_RDSYNONYMS         	 57
# define	SCO_RDTBLMAX           	 58
# define	SCO_ROWCOUNT_ESTIMATE  	 59
# define	SCO_RULEDEPTH          	 60
# define	SCO_SERVER_CLASS       	 61
# define	SCO_SHAREDCACHE        	 62
# define	SCO_SOLE_CACHE           63
# define	SCO_SOLE_SERVER        	 64
# define	SCO_SXF_MEM            	 65
# define	SCO_TBLCACHE_SIZE      	 66
# define	SCO_WBEND              	 67
# define	SCO_WBSTART            	 68
# define	SCO_QEF_MAX_MEM_SLEEP	 69
# define	SCO_LOCAL_VNODE		 70
# define	SCO_TABLE_JNL_DEFAULT	 71
# define	SCO_ULM_CHUNK_SIZE	 72
# define	SCO_INT_SORT_SIZE	 73
# define	SCO_OPF_MEMF		 74
# define	SCO_SPATKEY		 75
# define	SCO_OPOLDCNF 		 76
# define        SCO_AMBREP_64COMPAT      77
# define	SCO_OPF_CNFF		 78
# define	SCO_PSORT_BSIZE		 79
# define	SCO_PSORT_ROWS		 80
# define	SCO_PSORT_NTHREADS	 81
# define	SCO_PIND_BSIZE		 82
# define	SCO_PIND_NBUFFERS	 83
# define	SCO_STATS_NOSTATS_MAX	 84
# define        SCO_ASSOC_TIMEOUT        85
# define	SCO_PSF_MEMF		 86
# define	SCO_RANGEP_MAX		 87
# define	SCO_QEHASHSIZE         	 88
# define	SCO_CSTIMEOUT		 89
# define        SCO_EVENT_PRIORITY       90
# define	SCO_STATS_NOSTATS_FACTOR 91
# define	SCO_OPOLDSUBSEL		 92
# define	SCO_OPOLDIDXORDER	 93
# define	SCO_RESULTSTRUCT	 94	/* result structure */
# define	SCO_SORT_CPUFACTOR	 95
# define	SCO_SORT_IOFACTOR	 96
# define	SCO_AUTOSTRUCT		 97
# define	SCO_CACHE_DYNAMIC	 98
# define	SCO_CREATE_COMPRESSION	 99	/* create-table compression */

# define	SCO_SESSION_CHK_INTERVAL 100	/* Session check interval */
# define	SCO_SECURE_LEVEL	 101
# define	SCO_BATCHMODE     	 102    /* batch server */
# define	SCO_READAHEAD		 103    /* # of read ahead  threads */
# define	SCO_IOMAST_WALONG	 104    /* # of write along threads */
# define	SCO_IOMAST_SLAVES	 105    /* # of slaves for iomaster */
# define        SCO_GCA_SINGLE_MODE      106    /* gca single mode */

# define        SCO_4K_DBUFFERS             107
# define        SCO_4K_DGBUFFERS            108
# define        SCO_4K_DGCOUNT              109
# define        SCO_4K_WBEND                110
# define        SCO_4K_WBSTART              111
# define        SCO_4K_FLIMIT               112
# define        SCO_4K_MLIMIT               113
# define        SCO_4K_NEWALLOC             114

# define        SCO_8K_DBUFFERS             115
# define        SCO_8K_DGBUFFERS            116
# define        SCO_8K_DGCOUNT              117
# define        SCO_8K_WBEND                118
# define        SCO_8K_WBSTART              119
# define        SCO_8K_FLIMIT               120
# define        SCO_8K_MLIMIT               121
# define        SCO_8K_NEWALLOC             122

# define        SCO_16K_DBUFFERS            123
# define        SCO_16K_DGBUFFERS           124
# define        SCO_16K_DGCOUNT             125
# define        SCO_16K_WBEND               126
# define        SCO_16K_WBSTART             127
# define        SCO_16K_FLIMIT              128
# define        SCO_16K_MLIMIT              129
# define        SCO_16K_NEWALLOC            130

# define        SCO_32K_DBUFFERS            131
# define        SCO_32K_DGBUFFERS           132
# define        SCO_32K_DGCOUNT             133
# define        SCO_32K_WBEND               134
# define        SCO_32K_WBSTART             135
# define        SCO_32K_FLIMIT              136
# define        SCO_32K_MLIMIT              137
# define        SCO_32K_NEWALLOC            138

# define        SCO_64K_DBUFFERS            139
# define        SCO_64K_DGBUFFERS           140
# define        SCO_64K_DGCOUNT             141
# define        SCO_64K_WBEND               142
# define        SCO_64K_WBSTART             143
# define        SCO_64K_FLIMIT              144
# define        SCO_64K_MLIMIT              145
# define        SCO_64K_NEWALLOC            146

# define        SCO_4K_SCANFACTOR           147 
# define        SCO_8K_SCANFACTOR           148
# define        SCO_16K_SCANFACTOR          149
# define        SCO_32K_SCANFACTOR          150
# define        SCO_64K_SCANFACTOR          151

#define		SCO_4K_STATUS		    153
#define		SCO_8K_STATUS		    154
#define		SCO_16K_STATUS		    155
#define		SCO_32K_STATUS		    156
#define		SCO_64K_STATUS		    157

#define		SCO_REP_QMAN		    158
#define		SCO_REP_QSIZE		    159
#define		SCO_DEF_PAGE_SIZE	    160

#define  	SCO_LKMODE_SYS_READ	 161 /* system default for readlock */
#define  	SCO_LKMODE_SYS_MAXLK	 162 /* system default for maxlocks */
#define  	SCO_LKMODE_SYS_TIMEOUT	 163 /* system default for lk timeout */
#define         SCO_SYS_ISOLATION        164 /* system default for isolation */
#define         SCO_LOG_READNOLOCK       165 /* log readlock=nolock lowers iso*/
#define		SCO_LOG_ESC_LPR_SC       166 /* log esc on locks/resource sys cat */
#define		SCO_LOG_ESC_LPR_UT       167 /* log esc on locks/resource user table */
#define		SCO_LOG_ESC_LPT_SC       168 /* log esc on locks/trans sys cat */
#define		SCO_LOG_ESC_LPT_UT       169 /* log esc on locks/trans user table */
#define		SCO_REP_SALOCK		 170 /* replicator shad/arch lockmode*/
#define		SCO_REP_IQLOCK		 171 /* replicator input q lockmode */
#define		SCO_REP_DQLOCK		 172 /* replicator dist q lockmode */
#define		SCO_REP_DTMAXLOCK	 173 /* rep dist thread maxlocks */
#define 	SCO_BLOB_ETAB_STRUCT     174 /* default blob etab structure */

/* WriteBehind support per cache */
#define		SCO_2K_WRITEBEHIND	    175
#define		SCO_4K_WRITEBEHIND	    176
#define		SCO_8K_WRITEBEHIND	    177
#define		SCO_16K_WRITEBEHIND	    178
#define		SCO_32K_WRITEBEHIND	    179
#define		SCO_64K_WRITEBEHIND	    180

#define 	SCO_JOINOP_TIMEOUT       181 /* default JOINOP TIMEOUT value*/
#define 	SCO_JOINOP_TIMEOUTABORT  182 /* default JOINOP 
						TIMEOUTABORT value */
#define         SCO_ETAB_PAGE_SIZE          183 /* etab page size */
#define		SCO_ETAB_TYPE		 184    /* default,new,newheap  */

#define		SCO_PAGETYPE_V1		    185
#define		SCO_PAGETYPE_V2		    186
#define		SCO_PAGETYPE_V3		    187
#define		SCO_PAGETYPE_V4		    188
#define		SCO_PAGETYPE_V5		    189

#define		SCO_ULM_PAD_BYTES	    190

#define		SCO_HASH_JOIN		    191
#define		SCO_OPF_NEWENUM		    192

#define        	SCO_RULE_UPD_PREFETCH       193

#define		SCO_OPF_PQDOP		    194
#define		SCO_OPF_PQTHRESH	    195
#define		SCO_PSF_VCH_PREC	    196
#define		SCO_DEFCRS_MODE		    197
#define		SCO_SYS_LOCK_LEVEL	    198
#define		SCO_OPF_PQPTHREADS	    199
#define         SCO_CLASS_NODE_AFFINITY     200
#define		SCO_RULE_DEL_PREFETCH	    202
#define		SCO_DATE_TYPE_ALIAS	    203
#define         SCO_QEF_NO_DEPENDENCY_CHK   204
#define         SCO_CORE_ENABLED	    205
#define         SCO_DMF_BUILD_PAGES         206    /* dmf_build_pages */
#define		SCO_OPGREEDYFACTOR	    207
#define         SCO_MLLIST                  208
# define	SCO_QEF_HASH_RBSIZE	    209	/* Hash read buffer size */
# define	SCO_QEF_HASH_WBSIZE	    210	/* Hash write buffer size */
# define	SCO_QEF_HASH_CMP_THRESHOLD  211	/* Hash compress threshold */
# define	SCO_QEF_HASHJOIN_MIN	    212 /* Hashjoin allocation min */
# define	SCO_QEF_HASHJOIN_MAX	    213 /* Hashjoin allocation max */
#define		SCO_FLATNCARD		    214 /* Disable sing-select card check */
#define		SCO_INLIST_THRESH	    215 /* IN LIST key threshold */
#define		SCO_PAGETYPE_V6		    216
#define		SCO_PAGETYPE_V7		    217
#define		SCO_CRYPT_MAXKEYS	    218 /* max crypt shmem keys */
#define		SCO_BATCH_COPY_OPTIM	    219
#define		SCO_MONEY_COMPAT            220
#define		SCO_DEFAULT_COLL	    221 /* Default collation */
#define		SCO_DEFAULT_UNI_COLL        222 /* Default Unicode collation */
static SCD_OPT scd_opttab[] =
{
    /* echoing first so the rest get echoed */

    SCO_ECHO,			'z',	' ',	"!.echo",		

    /* needed for cache dependent parameters */
    
    SCO_SHAREDCACHE,		't',	'3',	"!.cache_sharing",
    SCO_CACHENAME,		'g',	'3',	"!.cache_name",

    /* cache dependent parameters */

    SCO_DBUFFERS,		'o',	'1',	"!.dmf_cache_size",
    SCO_FLIMIT,			'o',	'3',	"!.dmf_free_limit",
    SCO_DGBUFFERS,		'o',	'3',	"!.dmf_group_count",
    SCO_DGCOUNT,		'o',	'3',	"!.dmf_group_size",
    SCO_MLIMIT,			'o',	'3',	"!.dmf_modify_limit",
    SCO_DMSCANFACTOR,		'f',	'3',	"!.dmf_scan_factor",
    SCO_WBEND,			'o',	'3',	"!.dmf_wb_end",
    SCO_WBSTART,		'o',	'3',	"!.dmf_wb_start",
    SCO_2K_WRITEBEHIND,		'z',	'3',	"!.dmf_write_behind",

    SCO_4K_STATUS,		't',	'3',	"!.cache.p4k_status",
    SCO_4K_DBUFFERS,		'o',	'3',	"!.p4k.dmf_cache_size",
    SCO_4K_FLIMIT,		'o',	'3',	"!.p4k.dmf_free_limit",
    SCO_4K_DGBUFFERS,		'o',	'3',	"!.p4k.dmf_group_count",
    SCO_4K_DGCOUNT,		'o',	'3',	"!.p4k.dmf_group_size",
    SCO_4K_MLIMIT,		'o',	'3',	"!.p4k.dmf_modify_limit",
    SCO_4K_WBEND,		'o',	'3',	"!.p4k.dmf_wb_end",
    SCO_4K_WBSTART,		'o',	'3',	"!.p4k.dmf_wb_start",
    SCO_4K_NEWALLOC,    	't',    '3',    "!.p4k.dmf_separate",
    SCO_4K_SCANFACTOR,   	'f',    '3',    "!.p4k.dmf_scan_factor",
    SCO_4K_WRITEBEHIND,   	'z',    '3',    "!.p4k.dmf_write_behind",

    SCO_8K_STATUS,		't',	'3',	"!.cache.p8k_status",
    SCO_8K_DBUFFERS,		'o',	'3',	"!.p8k.dmf_cache_size",
    SCO_8K_FLIMIT,		'o',	'3',	"!.p8k.dmf_free_limit",
    SCO_8K_DGBUFFERS,		'o',	'3',	"!.p8k.dmf_group_count",
    SCO_8K_DGCOUNT,		'o',	'3',	"!.p8k.dmf_group_size",
    SCO_8K_MLIMIT,		'o',	'3',	"!.p8k.dmf_modify_limit",
    SCO_8K_WBEND,		'o',	'3',	"!.p8k.dmf_wb_end",
    SCO_8K_WBSTART,		'o',	'3',	"!.p8k.dmf_wb_start",
    SCO_8K_NEWALLOC,    	't',    '3',    "!.p8k.dmf_separate",
    SCO_8K_SCANFACTOR,    	'f',    '3',    "!.p8k.dmf_scan_factor",
    SCO_8K_WRITEBEHIND,   	'z',    '3',    "!.p8k.dmf_write_behind",

    SCO_16K_STATUS,		't',	'3',	"!.cache.p16k_status",
    SCO_16K_DBUFFERS,		'o',	'3',	"!.p16k.dmf_cache_size",
    SCO_16K_FLIMIT,		'o',	'3',	"!.p16k.dmf_free_limit",
    SCO_16K_DGBUFFERS,    	'o',	'3',	"!.p16k.dmf_group_count",
    SCO_16K_DGCOUNT,		'o',	'3',	"!.p16k.dmf_group_size",
    SCO_16K_MLIMIT,		'o',	'3',	"!.p16k.dmf_modify_limit",
    SCO_16K_WBEND,		'o',	'3',	"!.p16k.dmf_wb_end",
    SCO_16K_WBSTART,		'o',	'3',	"!.p16k.dmf_wb_start",
    SCO_16K_NEWALLOC,    	't',    '3',    "!.p16k.dmf_separate",
    SCO_16K_SCANFACTOR,    	'f',    '3',    "!.p16k.dmf_scan_factor",
    SCO_16K_WRITEBEHIND,   	'z',    '3',    "!.p16k.dmf_write_behind",

    SCO_32K_STATUS,		't',	'3',	"!.cache.p32k_status",
    SCO_32K_DBUFFERS,		'o',	'3',	"!.p32k.dmf_cache_size",
    SCO_32K_FLIMIT,		'o',	'3',	"!.p32k.dmf_free_limit",
    SCO_32K_DGBUFFERS,    	'o',	'3',	"!.p32k.dmf_group_count",
    SCO_32K_DGCOUNT,		'o',	'3',	"!.p32k.dmf_group_size",
    SCO_32K_MLIMIT,		'o',	'3',	"!.p32k.dmf_modify_limit",
    SCO_32K_WBEND,		'o',	'3',	"!.p32k.dmf_wb_end",
    SCO_32K_WBSTART,		'o',	'3',	"!.p32k.dmf_wb_start",
    SCO_32K_NEWALLOC,    	't',    '3',    "!.p32k.dmf_separate",
    SCO_32K_SCANFACTOR,    	'f',    '3',    "!.p32k.dmf_scan_factor",
    SCO_32K_WRITEBEHIND,   	'z',    '3',    "!.p32k.dmf_write_behind",

    SCO_64K_STATUS,		't',	'3',	"!.cache.p64k_status",
    SCO_64K_DBUFFERS,		'o',	'3',	"!.p64k.dmf_cache_size",
    SCO_64K_FLIMIT,		'o',	'3',	"!.p64k.dmf_free_limit",
    SCO_64K_DGBUFFERS,    	'o',	'3',	"!.p64k.dmf_group_count",
    SCO_64K_DGCOUNT,		'o',	'3',	"!.p64k.dmf_group_size",
    SCO_64K_MLIMIT,		'o',	'3',	"!.p64k.dmf_modify_limit",
    SCO_64K_WBEND,		'o',	'3',	"!.p64k.dmf_wb_end",
    SCO_64K_WBSTART,		'o',	'3',	"!.p64k.dmf_wb_start",
    SCO_64K_NEWALLOC,    	't',    '3',    "!.p64k.dmf_separate",
    SCO_64K_SCANFACTOR,    	'f',    '3',    "!.p64k.dmf_scan_factor",
    SCO_64K_WRITEBEHIND,   	'z',    '3',    "!.p64k.dmf_write_behind",

    /* Switch out of screwy-dmf-cache mode */
    SCO_C2_MODE,		't',	'2',	"ii.*.c2.security_auditing",

    /* remaining random parameters */
    SCO_SECURE_LEVEL,		'g',	' ',	"ii.$.secure.level",
    SCO_ASSOC_TIMEOUT,          'o',    ' ',    "!.association_timeout",
    SCO_LOCK_CACHE,		't',	'3',	"!.cache_lock",
    SCO_CHECK_DEAD,		'o',	'3',	"!.check_dead",
    SCO_CORE_ENABLED,		't',	' ',	"!.core_enabled",
    SCO_CP_TIMER,		'o',	'3',	"!.cp_timer",
    SCO_CREATE_COMPRESSION,	'g',	'3',	"!.create_compression",
    SCO_CURS_LIMIT,		'o',	' ',	"!.cursor_limit",
    SCO_DBCNT,			'o',	' ',	"!.database_limit",
    SCO_DBLIST,			'g',	' ',	"!.database_list",
    SCO_MLLIST,			'g',	' ',	"!.mustlog_db_list",
    SCO_CSRUPDATE,		'g',	' ',	"!.cursor_update_mode",
    SCO_DEFCRS_MODE,            'g',    ' ',    "!.cursor_default_open",
    SCO_DMF_BUILD_PAGES,        'o',    '3',    "!.dmf_build_pages",
    SCO_DTCB_HASH,		'o',    '3',    "!.dmf_hash_size",
    SCO_DBCACHE_SIZE,		'o',	'3',	"!.dmf_db_cache_size",
    SCO_TBLCACHE_SIZE,		'o',	'3',	"!.dmf_tbl_cache_size",
    SCO_INT_SORT_SIZE,		'o',	'3',	"!.dmf_int_sort_size",
    SCO_DTCB_LIMIT,		'o',    '3',    "!.dmf_tcb_limit",
    SCO_DOP,			'o',	' ',	"!.degree_of_parallelism",
    SCO_DMF_PAD_BYTES,		'o',	' ',	"!.dmf_pad_bytes",
    SCO_CRYPT_MAXKEYS,		'o',	'3',	"!.dmf_crypt_maxkeys",
    SCO_EVENTS,			'o',	'3',	"!.event_limit",
    SCO_EVENT_PRIORITY,         'o',    '3',    "!.event_priority",
    SCO_FAST_COMMIT,		't',	'3',	"!.fast_commit",
    SCO_GC_INTERVAL,		'o',	'3',	"!.gc_interval",
    SCO_GC_NUMTICKS,		'o',	'3',	"!.gc_num_ticks",
    SCO_GC_THRESHOLD,		'o',	'3',	"!.gc_threshold",
    SCO_LOCAL_VNODE,		'g',	' ',	"!.local_vnode",
    SCO_LOGWRITER,		'o',	'3',	"!.log_writer",
    SCO_NAMES_REG,		'z',	' ',	"!.name_service",
    SCO_NOSTARCLUSTER,		'z',	' ',	"!.cluster_mode",
    SCO_TABLE_JNL_DEFAULT,	'z',	' ',	"!.default_journaling",
    SCO_DX_NORECOVR,		'z',	' ',	"!.distributed_recovery",
    SCO_ACTIVE,			'o',	' ',	"!.opf_active_limit",
    SCO_OPOLDJCARD,		't',	' ',	"!.opf_old_jcard",
    SCO_OPOLDCNF,		't',	' ',	"!.opf_old_cnf",
    SCO_OPOLDIDXORDER,		't',	' ',	"!.opf_old_idxorder",
    SCO_OPOLDSUBSEL,		't',	' ',	"!.opf_old_subsel",
    SCO_OPF_CNFF,		'o',	' ',	"!.opf_cnffact",
    SCO_COMPLETE,		't',	' ',	"!.opf_complete",
    SCO_OPCPUFACTOR,		'F',	' ',	"!.opf_cpu_factor",
    SCO_EXACTKEY,		'P',	' ',	"!.opf_exact_key",
    SCO_OPF_MEM,		's',	' ',	"!.opf_memory",
    SCO_OPF_MEMF,		'o',	' ',	"!.opf_maxmemf",
    SCO_NONKEY,			'P',	' ',	"!.opf_non_key",
    SCO_SPATKEY,		'P',	' ',	"!.opf_spat_key",
    SCO_RANGEKEY,		'P',	' ',	"!.opf_range_key",
    SCO_RANGEP_MAX,		'o',	' ',	"!.opf_rangep_max",
    SCO_CSTIMEOUT,		'o',	' ',	"!.opf_cstimeout",
    SCO_OPREPEAT,		'F',	' ',	"!.opf_repeat_factor",
    SCO_OPSORTMAX,		'o',	' ',	"!.opf_sort_max",
    SCO_OPTIMEOUT,		'F',	' ',	"!.opf_timeout_factor",
    SCO_STATS_NOSTATS_MAX,	't',	' ',	"!.opf_stats_nostats_max",
    SCO_HASH_JOIN,		'z',	' ',	"!.opf_hash_join",
    SCO_OPF_NEWENUM,		'z',	' ',	"!.opf_new_enum",
    SCO_OPF_PQDOP,		'o',	' ',	"!.opf_pq_dop",
    SCO_OPF_PQTHRESH,		'f',	' ',	"!.opf_pq_threshold",
    SCO_OPF_PQPTHREADS,		'o',	' ',	"!.opf_pq_partthreads",
    SCO_STATS_NOSTATS_FACTOR,	'F',	' ',	"!.opf_stats_nostats_factor",
    SCO_OPGREEDYFACTOR,		'F', 	' ',	"!.opf_greedy_factor",
    SCO_INLIST_THRESH,		'o',	' ',	"!.opf_inlist_thresh",
    SCO_PSF_MEM,		's',	' ',	"!.psf_memory",
    SCO_PSF_VCH_PREC,		't',	' ',	"!.psf_vch_prec",
    SCO_PSF_MEMF,		'o',	' ',	"!.psf_maxmemf",
    SCO_QEF_DSHMEM,		's',	' ',	"!.qef_dsh_memory",
    SCO_QP_SIZE,		'o',	' ',	"!.qef_qep_mem",
    SCO_QESORTSIZE,		'o',	' ',	"!.qef_sort_mem",
    SCO_QEHASHSIZE,		'o',	' ',	"!.qef_hash_mem",
    SCO_QEF_SHMEM,		's',	' ',	"!.qef_sorthash_memory",
    SCO_QEF_MAX_MEM_SLEEP,	'o',    ' ',	"!.qef_max_mem_sleep",
    SCO_QEF_HASH_RBSIZE,	'o',	' ',	"!.qef_hash_rbsize",
    SCO_QEF_HASH_WBSIZE,	'o',	' ',	"!.qef_hash_wbsize",
    SCO_QEF_HASH_CMP_THRESHOLD,	'o',	' ',	"!.qef_hash_cmp_threshold",
    SCO_QEF_HASHJOIN_MIN,	'o',	' ',	"!.qef_hashjoin_min",
    SCO_QEF_HASHJOIN_MAX,	'o',	' ',	"!.qef_hashjoin_max",
    SCO_FLATNAGG,		'z',	' ',	"!.qflatten_aggregate",
    SCO_FLATNSING,		'z',	' ',	"!.qflatten_singleton",
    SCO_FLATTEN,		'z',	' ',	"!.query_flattening",
    SCO_FLATNCARD,		'z',	' ',	"!.cardinality_check",
    SCO_QS_POOL,		's',	' ',	"!.qsf_memory",
    SCO_RDF_AVGLDBS,		'o',	' ',	"!.rdf_avg_ldbs",
    SCO_RDFDDBS,		'o',	' ',	"!.rdf_cache_ddbs",
    SCO_RDTBLMAX,		'o',	' ',	"!.rdf_max_tbls",
    SCO_RDMEMORY,		's',	' ',	"!.rdf_memory",
    SCO_RDMULTICACHE,		't',	' ',	"!.rdf_multi_cache",
    /* !.rdf_tbl_cols is deprecated but still accepted */
    SCO_RDCOLAVG,		'o',	' ',	"!.rdf_tbl_cols",
    /* !.rdf_col_defaults is what rdf_tbl_cols really means.
    ** Put rdf_col_defaults second, since we process these things in
    ** order and we want rdf_col_defaults to prevail if both are given.
    */
    SCO_RDCOLAVG,		'o',	' ',	"!.rdf_col_defaults",
    SCO_RDSYNONYMS,		'o',	'3',	"!.rdf_tbl_synonyms",
    SCO_RESULTSTRUCT,		'g',	' ',	"!.result_structure",
    SCO_RULEDEPTH,		'o',	'3',	"!.rule_depth",
    SCO_RULE_UPD_PREFETCH,      'z',    ' ',    "!.rule_upd_prefetch",
    SCO_ROWCOUNT_ESTIMATE,	'o',	' ',	"!.scf_rows",
    SCO_SERVER_CLASS,		'g',	' ',	"!.server_class",
    SCO_SOLE_CACHE,		't',	'3',	"!.sole_cache",
    SCO_SOLE_SERVER,		't',	' ',	"!.sole_server",
    SCO_SXF_MEM,		's',	' ',	"!.sxf_memory",
    SCO_ULM_CHUNK_SIZE,		'o',	' ',	"!.ulm_chunk_size",
    SCO_ULM_PAD_BYTES,		'o',	' ',	"!.ulm_pad_bytes",
    SCO_SESSION_CHK_INTERVAL,	'o',	' ',	"!.session_check_interval",
    SCO_BATCHMODE,           	't',	' ',	"!.batchmode",
    SCO_READAHEAD,    		'o',	' ',	"!.read_ahead",
    SCO_IOMAST_WALONG,		'o',	' ',	"!.iom_write_along",
    SCO_IOMAST_SLAVES,		'o',	' ',	"!.iom_num_slaves",
    SCO_GCA_SINGLE_MODE,        't',    ' ',    "!.gca_single_mode",
    SCO_LKMODE_SYS_READ,        'g',    ' ',    "!.system_readlock",
    SCO_LKMODE_SYS_MAXLK,       'o',    ' ',    "!.system_maxlocks",
    SCO_LKMODE_SYS_TIMEOUT,     'o',    ' ',    "!.system_timeout",
    SCO_SYS_ISOLATION,          'g',    ' ',    "!.system_isolation",
    SCO_SYS_LOCK_LEVEL,		'g',	' ',	"!.system_lock_level",
    SCO_LOG_READNOLOCK,         't',    ' ',    "!.log_readnolock",
    SCO_LOG_ESC_LPR_SC,		't',    ' ',    "!.log_esc_lpr_sc",
    SCO_LOG_ESC_LPR_UT,		't',    ' ',    "!.log_esc_lpr_ut",
    SCO_LOG_ESC_LPT_SC,		't',    ' ',    "!.log_esc_lpt_sc",
    SCO_LOG_ESC_LPT_UT,		't',    ' ',    "!.log_esc_lpt_ut",
    SCO_AMBREP_64COMPAT,	't',	' ',	"!.ambig_replace_64compat",
    SCO_REP_QMAN,		'o',	' ',	"!.rep_qman_threads",
    SCO_REP_QSIZE,		'o',	'3',	"!.rep_txq_size",
    SCO_REP_SALOCK,		'g',	'3',	"!.rep_sa_lockmode",
    SCO_REP_IQLOCK,		'g',	'3',	"!.rep_iq_lockmode",
    SCO_REP_DQLOCK,		'g',	'3',	"!.rep_dq_lockmode",
    SCO_REP_DTMAXLOCK,		'o',	' ',	"!.rep_dt_maxlocks",
    SCO_DMCM,                   't',    '3',    "!.dmcm",
    SCO_DEF_PAGE_SIZE,          'o',    ' ',    "!.default_page_size",
    SCO_BLOB_ETAB_STRUCT,       'g',    ' ',    "!.blob_etab_structure",
    SCO_PSORT_BSIZE,		'o',	'3',	"!.psort_bsize",
    SCO_PSORT_ROWS,		'o',	'3',	"!.psort_rows",
    SCO_PSORT_NTHREADS,		'o',	'3',	"!.psort_nthreads",
    SCO_PIND_BSIZE,		'o',	'3',	"!.pindex_bsize",
    SCO_PIND_NBUFFERS,		'o',	'3',	"!.pindex_nbuffers",
    SCO_SORT_CPUFACTOR,		'F',	' ',	"!.sort_cpu_factor",
    SCO_SORT_IOFACTOR,		'F',	' ',	"!.sort_io_factor",
    SCO_JOINOP_TIMEOUT,		'f',	' ',	"!.opf_joinop_timeout",
    SCO_JOINOP_TIMEOUTABORT,	'f',	' ',	"!.opf_timeout_abort",
    SCO_ETAB_PAGE_SIZE,         'o',    ' ',    "!.blob_etab_page_size",
    SCO_ETAB_TYPE,		'g',	' ',	"!.blob_etab_type",
    SCO_PAGETYPE_V1,		'z',	'3',	"!.pagetype_v1", /*default on*/
    SCO_PAGETYPE_V2,		'z',	'3',	"!.pagetype_v2", /*default on*/
    SCO_PAGETYPE_V3,		'z',	'3',	"!.pagetype_v3", /*default on*/
    SCO_PAGETYPE_V4,		'z',	'3',	"!.pagetype_v4", /*default on*/
    SCO_PAGETYPE_V5,		'z',	'3',	"!.pagetype_v5", /*default on*/
    SCO_CLASS_NODE_AFFINITY,    't',    '3',    "!.class_node_affinity",
    SCO_RULE_DEL_PREFETCH,      'z',    ' ',    "!.rule_del_prefetch",
    SCO_DATE_TYPE_ALIAS,	'g',	' ',	"ii.$.config.date_alias",
    SCO_AUTOSTRUCT,		't',	' ',	"!.table_auto_structure",
    SCO_CACHE_DYNAMIC,		't',	' ',	"!.cache_dynamic",
    SCO_QEF_NO_DEPENDENCY_CHK,  't',    ' ',    "!.qef_no_dependency_chk",
    SCO_PAGETYPE_V6,		'z',	'3',	"!.pagetype_v6", /*default on*/
    SCO_PAGETYPE_V7,		'z',	'3',	"!.pagetype_v7", /*default on*/
    SCO_BATCH_COPY_OPTIM,	'z',	' ',	"!.batch_copy_optim",
    SCO_MONEY_COMPAT,   	't',	' ',	"!.money_compat",
    SCO_DEFAULT_COLL,		'g',	' ',	"!.default_collation",
    SCO_DEFAULT_UNI_COLL,	'g',	' ',	"!.default_unicode_collation",
    0, 0, 0, 0
} ;

GLOBALREF SC_MAIN_CB     *Sc_main_cb;    /* Core structure for SCF */

/*
** Name: scd_options() - fetch a pile of options using PMget()
**
** Description:
**	Calls PMget() for each option in the option table, converts the
**	value appropriately, and distributes the value into the right
**	SCF control block.
**
** Inputs:
**	DMC_CHAR_ENTRY	*dca
**	i4		*dca_index
**	OPF_CB		*opf_cb
**	RDF_CCB		*rdf_ccb
**	DMC_CB          *dmc
**	PSQ_CB		*psq_cb
**	QEF_SRC		*qef_cb
**	QSF_RCB		*qsf_rcb
**	SXF_RCB		*sxf_rcb
**	SCD_CB		*scd_cb
**
** Side Effects:
**	SC_MAIN_CB	*Sc_main_cb	Poked at mercilessly
**
*/

STATUS
scd_options( 
	DMC_CHAR_ENTRY  *dca,
	i4		*dca_index,
	OPF_CB		*opf_cb,
	RDF_CCB		*rdf_ccb,
	DMC_CB          *dmc,
	PSQ_CB		*psq_cb,
	QEF_SRCB	*qef_cb,
	QSF_RCB		*qsf_rcb,
	SXF_RCB		*sxf_rcb,
	SCD_CB		*scd_cb )
{
	SCD_OPT		*scdopt;
	i4 		echo = 1;
	i4		failures = 0;
	DMC_CHAR_ENTRY	*dca_save = dca;
	char		*save_def3;
	char		*save_def4;
	i4		cache_4k_on = 0;
	i4		cache_8k_on = 0;
	i4		cache_16k_on = 0;
	i4		cache_32k_on = 0;
	i4		cache_64k_on = 0;
	i4		cval_stat = 0;


	/* XXX - the config tools use "." as a decimal, so must we! */

	char		*decimal_point = ".";	

	/*
	 * Step through the option table:
	 *	Fiddle with defaults, to accomodate insane DMF paramters.
	 *	Get the option's value using PMget()
	 *	Convert & check the option's value, according to argtype
	 *	Distribute option value, according to index
	 *	Log an error if any of the above failed
	 */

	for( scdopt = scd_opttab; scdopt->sco_name; scdopt++ )
	{
	    char	*scd_svalue;
	    f8		scd_dvalue;
	    i4		scd_value;
	    SIZE_TYPE	scd_stvalue;
	    STATUS	status = OK;

	    /*
	     * Fiddle with defaults, to accomodate insane DMF paramters.
	     */

	    switch( scdopt->sco_special )
	    {
	    case '1':
		save_def3 = PMgetDefault( 3 );
		save_def4 = PMgetDefault( 4 );

		if (scd_cb->sharedcache)
		{
		    /*
		    ** shared cache values are stored as 
		    ** ii.<host>.dbms.shared.<cache_name>.<value>
		    */
		    PMsetDefault(3, "shared");
		    PMsetDefault(4, 
			scd_cb->cache_name ? scd_cb->cache_name : save_def3 );
		}
		else
		{
		    /*
		    ** private cache values are stored as
		    ** ii.<host>.dbms.private.<class>.<value>
		    */
		    PMsetDefault(3, "private");
		    PMsetDefault(4, save_def3);
		}
		break;

	    case '2':
		PMsetDefault( 3, save_def3 );
		PMsetDefault( 4, save_def4 );
		break;
	    }

	    /*
	     * Skip the option if it's not for us.
	     */

	    switch( scdopt->sco_special )
	    {
	    case '1':
	    case '2':
	    case '3':	/* Not star */
	        if( Sc_main_cb->sc_capabilities & SC_STAR_SERVER )
		    continue;
		break;
	    }

	    /* 
	     * Get option's value using PMget() 
	     */

	    if( PMget( scdopt->sco_name, &scd_svalue ) != OK )
		continue;

	    /*
	    ** if the p4k_status parameter is OFF, and the option name
	    ** contains !.p4k.xxxxxx, then ignore
	    */
	    if ( (!cache_4k_on && 
		    STncasecmp(scdopt->sco_name, "!.p4k.", 6 ) == 0) ||
	    	 (!cache_8k_on &&
		    STncasecmp(scdopt->sco_name, "!.p8k.", 6 ) == 0) ||
	    	 (!cache_16k_on &&
		    STncasecmp(scdopt->sco_name, "!.p16k.", 7 ) == 0) ||
	    	 (!cache_32k_on &&
		    STncasecmp(scdopt->sco_name, "!.p32k.", 7 ) == 0) ||
	    	 (!cache_64k_on &&
		    STncasecmp(scdopt->sco_name, "!.p64k.", 7 ) == 0) )
		continue;

	    if( echo )
		ERoptlog( scdopt->sco_name, scd_svalue );

	    /* 
	     * Convert & check the option's value, according to argtype 
	     */

	    switch( scdopt->sco_argtype )
	    {
	    case 'f':
		/* set a non-negative floating-point option */

		if( CVaf( scd_svalue, *decimal_point, &scd_dvalue ) != OK &&
		    CVaf( scd_svalue, '.', &scd_dvalue ) != OK ) 
		{
		    status = E_CS002A_FLOAT_ARG;
		}
		else if( scd_dvalue < 0.0 )
		{
		    status = E_CS002B_NONNEGATIVE_VALUE;
		}
		break;

	    case 'F': 
		/* set a positive floating-point option */

		if( CVaf( scd_svalue, *decimal_point, &scd_dvalue ) != OK &&
		    CVaf( scd_svalue, '.', &scd_dvalue ) != OK ) 
		{
		    status = E_CS002A_FLOAT_ARG;
		}
		else if( scd_dvalue <= 0.0 )
		{
		    status = E_CS0026_POSITIVE_VALUE;
		}
		break;

	    case 'g':
		/* set a string option */

		if( *scd_svalue && STchr( "-0123456789", *scd_svalue ) )
		{
		    status = E_CS0024_NON_NUMERIC_ARG;
		}
		break;

	    case 's':
		/* SIZE_TYPE */
		/* drop thru to 'o' case for 32-bit platforms */
#if defined(LP64)
		if ( (cval_stat = CVal8( scd_svalue, (i8 *)&scd_stvalue)) != OK )
		{
		    status = E_CS0025_NUMERIC_ARG;
		}
		break;
#endif

	    case 'o':
		/* set a numeric option */

		if( (cval_stat = CVal( scd_svalue,
                        (scdopt->sco_argtype == 'o' ? &scd_value : (i4 *)&scd_stvalue))) != OK )
		{
		    if (cval_stat == CV_OVERFLOW)
		    {
			TRdisplay("scd_options: %s overflow, capping to MAXI4\n", scdopt->sco_name);
			scd_value = MAXI4;
		    }
		    else
			status = E_CS0025_NUMERIC_ARG;
		}
		else if( scd_value < 0 )
		{
		    status = E_CS0026_POSITIVE_VALUE;
		}
		break;

	    case 'P':
		/* set a percentage floating-point option */

		if( CVaf( scd_svalue, *decimal_point, &scd_dvalue ) != OK &&
		    CVaf( scd_svalue, '.', &scd_dvalue ) != OK ) 
		{
		    status = E_CS002A_FLOAT_ARG;
		}
		else if( scd_dvalue <= 0.0 || scd_dvalue >= 1.0 )
		{
		    status = E_CS002C_FRACTION_VALUE;
		}
		break;

	    case 't':
		/* set a switch option: on = true */

		if( STcasecmp( scd_svalue, "on" ) )
		    continue;
		break;
		    
	    case 'z':
		/* clear a switch option */

		if( !STcasecmp( scd_svalue, "on" ) )
		    continue;
		break;
	    }

	    if( status )
	    {
		sc0e_put( status, (CL_ERR_DESC *)0, 1,
		    STlength( scdopt->sco_name ), scdopt->sco_name,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
		failures++;
		break;
	    }

	    /*
	     * Distribute option value, according to index
	     */

	    switch( scdopt->sco_index )
	    {
	    case SCO_ECHO:
		/* echo: off */
		echo = FALSE;
		break;

	    case SCO_DBUFFERS:
		dca->char_id = DMC_C_BUFFERS;
		dca++->char_value = scd_value;
		break;
		
	    case SCO_DGBUFFERS:
		dca->char_id = DMC_C_GBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_DGCOUNT:
		dca->char_id = DMC_C_GCOUNT;
		dca++->char_value = scd_value;
		break;

            case SCO_DMF_BUILD_PAGES:
                dca->char_id = DMC_C_BUILD_PAGES;
                dca->char_value = scd_value;
                ++dca;
                break;

	    case SCO_DTCB_HASH:
		dca->char_id = DMC_C_TCB_HASH;
		dca++->char_value = (scd_value -1);
		break;

	    case SCO_DTCB_LIMIT:
		dca->char_id = DMC_C_TCB_LIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_DOP:
		/* Degree of Parallelism: Pass to DMF and OPF */
		dca->char_id = DMC_C_DOP;
		dca++->char_value = scd_value;
		opf_cb->opf_pq_dop = scd_value;
		break;

	    case SCO_DMF_PAD_BYTES:
		/* Set the default DMF pad bytes. */
		{
		    dca->char_id = DMC_C_PAD_BYTES;
		    dca++->char_value = scd_value;
		}
		break;

	    case SCO_CRYPT_MAXKEYS:
		/* Set the maximum shmem active encryption keys */
		dca->char_id = DMC_C_CRYPT_MAXKEYS;
		dca++->char_value = scd_value;
		break;

	    case SCO_DBCNT:
		scd_cb->max_dbcnt = scd_value;
		break;

	    case SCO_QP_SIZE:
		scd_cb->qef_data_size = scd_value;
		break;

	    case SCO_QS_POOL:
		qsf_rcb->qsf_pool_sz = scd_stvalue;
		break;
	    
	    case SCO_ROWCOUNT_ESTIMATE:
		Sc_main_cb->sc_irowcount = scd_value;
		break;

	    case SCO_RULEDEPTH:
		Sc_main_cb->sc_rule_depth = scd_value;
		break;

            case SCO_RULE_UPD_PREFETCH:
                Sc_main_cb->sc_rule_upd_prefetch_off = TRUE;
                break;

	    case SCO_EVENTS:
		Sc_main_cb->sc_events = scd_value;
		break;

            case SCO_EVENT_PRIORITY:
                Sc_main_cb->sc_event_priority = scd_value;
                break;

	    case SCO_OPF_CNFF:
		opf_cb->opf_cnffact = scd_value;
		break;

	    case SCO_OPF_MEM:
		opf_cb->opf_mserver = scd_stvalue;
		break;

	    case SCO_OPF_MEMF:
		opf_cb->opf_maxmemf = (float)scd_value/100.0;
		if (opf_cb->opf_maxmemf <= 0.0 ||
			opf_cb->opf_maxmemf >= 1.0)
		    opf_cb->opf_maxmemf = 0.5;
		break;

	    case SCO_OPF_PQDOP:
		opf_cb->opf_pq_dop = scd_value;
		break;

	    case SCO_OPF_PQTHRESH:
		opf_cb->opf_pq_threshold = (float)scd_value;
		break;

	    case SCO_OPF_PQPTHREADS:
		opf_cb->opf_pq_partthreads = scd_value;
		break;

	    case SCO_PSF_MEM:
		Sc_main_cb->sc_psf_mem = scd_stvalue;
		break;

	    case SCO_PSF_VCH_PREC:
		Sc_main_cb->sc_psf_vch_prec = TRUE;
		break;

	    case SCO_PSF_MEMF:
		psq_cb->psq_maxmemf = (float)scd_value/100.0;
		if (psq_cb->psq_maxmemf <= 0.0 ||
			psq_cb->psq_maxmemf >= 1.0)
		    psq_cb->psq_maxmemf = 0.5;
		break;

	    case SCO_OPCPUFACTOR:
		opf_cb->opf_cpufactor = scd_dvalue;
		break;

	    case SCO_OPTIMEOUT:
		opf_cb->opf_timeout = scd_dvalue;
		break;

	    case SCO_JOINOP_TIMEOUT:
		opf_cb->opf_joinop_timeout = scd_dvalue;
		break;

	    case SCO_JOINOP_TIMEOUTABORT:
		opf_cb->opf_joinop_timeoutabort = scd_dvalue;
		break;

	    case SCO_OPREPEAT:
		opf_cb->opf_rfactor = scd_dvalue;
		break;

	    case SCO_OPSORTMAX:
		opf_cb->opf_sortmax = scd_value;
		break;

	    case SCO_ACTIVE:
		opf_cb->opf_mxsess = scd_value;
		break;

	    case SCO_EXACTKEY:
		opf_cb->opf_exact = scd_dvalue;
		break;

	    case SCO_RANGEKEY:
		opf_cb->opf_range = scd_dvalue;
		break;

	    case SCO_RANGEP_MAX:
		opf_cb->opf_rangep_max = scd_value;
		break;

	    case SCO_CSTIMEOUT:
		opf_cb->opf_cstimeout = scd_value;
		break;

	    case SCO_NONKEY:
		opf_cb->opf_nonkey = scd_dvalue;
		break;

	    case SCO_SPATKEY:
		opf_cb->opf_spatkey = scd_dvalue;
		break;

	    case SCO_OPOLDJCARD:
		/* opf_old_jcard on */
		opf_cb->opf_smask |= OPF_OLDJCARD;
		break;

	    case SCO_OPOLDCNF:
		opf_cb->opf_smask |= OPF_OLDCNF;
		break;

	    case SCO_OPOLDIDXORDER:
		opf_cb->opf_smask |= OPF_OLDIDXORDER;
		break;

	    case SCO_OPOLDSUBSEL:
		opf_cb->opf_smask |= OPF_OLDSUBSEL;
		break;

	    case SCO_STATS_NOSTATS_MAX:
		opf_cb->opf_smask |= OPF_STATS_NOSTATS_MAX;
		break;

	    case SCO_HASH_JOIN:
		opf_cb->opf_smask &= ~OPF_HASH_JOIN;
		break;

	    case SCO_OPF_NEWENUM:
		opf_cb->opf_smask &= ~OPF_NEW_ENUM;
		break;

	    case SCO_STATS_NOSTATS_FACTOR:
		opf_cb->opf_stats_nostats_factor = scd_dvalue;
		break;

	    case SCO_OPGREEDYFACTOR:
		opf_cb->opf_greedy_factor = scd_dvalue;
		break;

	    case SCO_INLIST_THRESH:
		opf_cb->opf_inlist_thresh = scd_value;
		break;

	    case SCO_COMPLETE:
		/* opf_complete: on */
		opf_cb->opf_smask |= OPF_COMPLETE;
		break;

	    case SCO_RESULTSTRUCT:
		{
		    char *ch = scd_svalue;
		    bool compressed;
		    i4 value;

		    compressed = FALSE;
		    if (*ch == 'c' || *ch == 'C')
		    {
			compressed = TRUE;
			++ ch;
		    }
		    value = uld_struct_xlate(ch);
		    if (value == 0)
		    {
			TRdisplay("scd_options: invalid result_struct %s, using CHEAP\n",
				scd_svalue);
			value = DB_HEAP_STORE;
			compressed = TRUE;
		    }
		    psq_cb->psq_result_struct = value;
		    psq_cb->psq_result_compression = compressed;
		    break;
		}

	    case SCO_DEFAULT_COLL:
	    case SCO_DEFAULT_UNI_COLL:
		{
		    static const char *collname_array[] = {
#			define _DEFINE(n,v,Ch,Un,t) t,
#			define _DEFINEEND
			DB_COLL_MACRO
#			undef _DEFINEEND
#			undef _DEFINE
		    };
		    DB_COLL_ID i = DB_NOCOLLATION+1;
		    while (i < DB_COLL_LIMIT &&
			    STbcompare(scd_svalue, 0, collname_array[i+1], 0, TRUE) != 0)
			i++;
		    if (i == DB_COLL_LIMIT)
		    {
			if (*scd_svalue && *scd_svalue != ' ')
			    TRdisplay("scd_options: invalid default collation %s\n",
					scd_svalue);
			i = DB_UNSET_COLL;
		    }
		    if (scdopt->sco_index == SCO_DEFAULT_UNI_COLL)
		    {
			dca->char_id = DMC_C_DEF_UNI_COLL;
			dca++->char_value = i;
			psq_cb->psq_def_unicode_coll = i;
		    }
		    else
		    {
			dca->char_id = DMC_C_DEF_COLL;
			dca++->char_value = i;
			psq_cb->psq_def_coll = i;
		    }
		    break;
		}

	    case SCO_CACHE_DYNAMIC:
		/* cache_dynamic: on */
		psq_cb->psq_flag |= PSQ_CACHEDYN;
		Sc_main_cb->sc_csrflags |= SC_CACHEDYN;
		break;

	    case SCO_AUTOSTRUCT:
		/* table_auto_structure: on */
		opf_cb->opf_autostruct = TRUE;
		break;

	    case SCO_FAST_COMMIT:
		/* fast_commit: on */
		Sc_main_cb->sc_fastcommit = TRUE;
		dmc->dmc_flags_mask |= DMC_FSTCOMMIT;
		break;

            case SCO_DMCM:
                /* dmcm: on */
                dca->char_id = DMC_C_DMCM;
                dmc->dmc_flags_mask |= DMC_DMCM;
                Sc_main_cb->sc_dmcm = TRUE;
                dca++->char_value = scd_value;
                break;

	    case SCO_PSORT_BSIZE:
		dca->char_id = DMC_C_PSORT_BSIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_PIND_BSIZE:
		dca->char_id = DMC_C_PIND_BSIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_SORT_CPUFACTOR:
		dca->char_id = DMC_C_SORT_CPUFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	    case SCO_SORT_IOFACTOR:
		dca->char_id = DMC_C_SORT_IOFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	    case SCO_PIND_NBUFFERS:
		dca->char_id = DMC_C_PIND_NBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_REP_QMAN:
		Sc_main_cb->sc_rep_qman = scd_value;
		break;

	    case SCO_SESSION_CHK_INTERVAL:
		Sc_main_cb->sc_session_chk_interval = scd_value;
		break;

	    case SCO_2K_WRITEBEHIND:
		/* 2k WriteBehind OFF */
		dca++->char_id = DMC_C_2K_WRITEBEHIND;
		break;

	    case SCO_WBSTART:
		dca->char_id = DMC_C_WSTART;
		dca++->char_value = scd_value;
		break;

	    case SCO_WBEND:
		dca->char_id = DMC_C_WEND;
		dca++->char_value = scd_value;
		break;

	    case SCO_MLIMIT:
		dca->char_id = DMC_C_MLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_FLIMIT:
		dca->char_id = DMC_C_FLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_DBCACHE_SIZE:
		dca->char_id = DMC_C_DBCACHE_SIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_PSORT_ROWS:
		dca->char_id = DMC_C_PSORT_ROWS;
		dca++->char_value = scd_value;
		break;

	    case SCO_PSORT_NTHREADS:
		dca->char_id = DMC_C_PSORT_NTHREADS;
		dca++->char_value = scd_value;
		break;

	    case SCO_INT_SORT_SIZE:
		dca->char_id = DMC_C_INT_SORT_SIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_BLOB_ETAB_STRUCT:
	    {
	        i4 dca_val;

		dca->char_id = DMC_C_BLOB_ETAB_STRUCT;
		dca_val = uld_struct_xlate(scd_svalue);
		if (dca_val != DB_HASH_STORE && dca_val != DB_BTRE_STORE
		  && dca_val != DB_ISAM_STORE)
		{
		    /* Someone edited it by hand? */
		    TRdisplay("scd_options: invalid blob etab struct %s, using hash\n",
				scd_svalue);
		    dca_val = DB_HASH_STORE;
		}

		dca++->char_value = dca_val;
		break;
	    }
	    case SCO_REP_QSIZE:
		dca->char_id = DMC_C_REP_QSIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_REP_DTMAXLOCK:
		dca->char_id = DMC_C_REP_DTMAXLOCK;
		dca++->char_value = scd_value;
		break;


	    case SCO_REP_SALOCK:
		dca->char_id = DMC_C_REP_SALOCK;
		if (STcasecmp(scd_svalue, ERx("row") ) == 0)
		    dca++->char_value = DMC_C_ROW;
		else if (STcasecmp(scd_svalue, ERx("page") ) == 0)
		    dca++->char_value = DMC_C_PAGE;
		else if (STcasecmp(scd_svalue, ERx("table") ) == 0)
		    dca++->char_value = DMC_C_TABLE;
		else /* default, should be set to USER */
		    dca++->char_value = 0;
		break;

	    case SCO_REP_IQLOCK:
		dca->char_id = DMC_C_REP_IQLOCK;
		if (STcasecmp(scd_svalue, ERx("row") ) == 0)
		    dca++->char_value = DMC_C_ROW;
		else if (STcasecmp(scd_svalue, ERx("page") ) == 0)
		    dca++->char_value = DMC_C_PAGE;
		else if (STcasecmp(scd_svalue, ERx("table") ) == 0)
		    dca++->char_value = DMC_C_TABLE;
		else /* default, should be set to USER */
		    dca++->char_value = 0;
		break;

	    case SCO_REP_DQLOCK:
		dca->char_id = DMC_C_REP_DQLOCK;
		if (STcasecmp(scd_svalue, ERx("row") ) == 0)
		    dca++->char_value = DMC_C_ROW;
		else if (STcasecmp(scd_svalue, ERx("table") ) == 0)
		    dca++->char_value = DMC_C_TABLE;
		else /* default is page */
		    dca++->char_value = DMC_C_PAGE;
		break;

	    case SCO_DEF_PAGE_SIZE:
		dca->char_id = DMC_C_DEF_PAGE_SIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_TBLCACHE_SIZE:
		dca->char_id = DMC_C_TBLCACHE_SIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_SOLE_CACHE:
		/* sole_cache: on */
		Sc_main_cb->sc_solecache = DMC_S_SINGLE;
		break;

	    case SCO_SOLE_SERVER:
		/* sole_server: on */
		Sc_main_cb->sc_soleserver = DMC_S_SINGLE;
		break;

	    case SCO_SHAREDCACHE:
		/* cache_sharing: on */
		scd_cb->sharedcache = TRUE;
		dca->char_id = DMC_C_SHARED_BUFMGR;
		dca++->char_value = TRUE;
		break;

	    case SCO_CACHENAME:
		scd_cb->cache_name = scd_svalue;
		break;
		
	    case SCO_LOCK_CACHE:
		/* cache_lock: on */
		dca->char_id = DMC_C_LOCK_CACHE;
		dca++->char_value = scd_value;
		break;
		
	    case SCO_NAMES_REG:
		/* name_service: off */
		Sc_main_cb->sc_names_reg = FALSE;
		break;

	    case SCO_SERVER_CLASS:
		scd_cb->server_class = scd_svalue;
		break;

	    case SCO_RDMEMORY:
		rdf_ccb->rdf_cachesiz = scd_stvalue;
		break;

	    case SCO_RDCOLAVG:
		rdf_ccb->rdf_colavg = scd_value;
		break;

	    case SCO_RDTBLMAX:
		rdf_ccb->rdf_max_tbl = scd_value;
		break;

	    case SCO_RDMULTICACHE:
		/* rdf_multi_cache: on */
		rdf_ccb->rdf_multicache = TRUE;
		break;

	    case SCO_RDSYNONYMS:
		rdf_ccb->rdf_num_synonym = scd_value;
		break;

	    case SCO_RDFDDBS:
		rdf_ccb->rdf_maxddb = scd_value;
		break;

	    case SCO_RDF_AVGLDBS:
		rdf_ccb->rdf_avgldb = scd_value;
		break;

	    case SCO_QEF_DSHMEM:
		qef_cb->qef_dsh_maxmem = scd_stvalue;
		break;

	    case SCO_QESORTSIZE:
		qef_cb->qef_sort_maxmem = scd_value;
		/* OPF wants to know too */
		opf_cb->opf_qefsort = scd_value;
		break;

	    case SCO_QEHASHSIZE:
		qef_cb->qef_hash_maxmem = scd_value;
		/* OPF doesn't really care, but just in case it starts to */
		opf_cb->opf_qefhash = scd_value;
		break;

	    case SCO_QEF_SHMEM:
		qef_cb->qef_sorthash_memory = scd_stvalue;
		break;

	    case SCO_QEF_MAX_MEM_SLEEP:
		qef_cb->qef_max_mem_sleep = scd_value;
		break;

	    case SCO_QEF_HASH_RBSIZE:
		qef_cb->qef_hash_rbsize = scd_value;
		break;

	    case SCO_QEF_HASH_WBSIZE:
		qef_cb->qef_hash_wbsize = scd_value;
		break;

	    case SCO_QEF_HASH_CMP_THRESHOLD:
		qef_cb->qef_hash_cmp_threshold = scd_value;
		break;

	    case SCO_QEF_HASHJOIN_MIN:
		qef_cb->qef_hashjoin_min = scd_value;
		break;

	    case SCO_QEF_HASHJOIN_MAX:
		qef_cb->qef_hashjoin_max = scd_value;
		break;

	    case SCO_DMSCANFACTOR:
		dca->char_id = DMC_C_SCANFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	    case SCO_DBLIST:
		scd_cb->dblist = scd_svalue;
		break;

	    case SCO_MLLIST:
		scd_cb->mllist = scd_svalue;
		break;

	    case SCO_DX_NORECOVR:
		/* distributed_recovery: off */
		Sc_main_cb->sc_nostar_recovr = TRUE;
		break;

	    case SCO_NOSTARCLUSTER:
		/* cluster_mode: off */
		Sc_main_cb->sc_no_star_cluster = TRUE;
		break;

	    case SCO_LOGWRITER:
		scd_cb->num_logwriters = scd_value;
		break;

	    case SCO_CHECK_DEAD:
		dca->char_id = DMC_C_CHECK_DEAD_INTERVAL;
		dca++->char_value = scd_value;
		break;

	    case SCO_CORE_ENABLED:
		dca->char_id = DMC_C_CORE_ENABLED;
		dca++->char_value = scd_value;
		break;

	    case SCO_GC_INTERVAL:
		dca->char_id = DMC_C_GC_INTERVAL;
		dca++->char_value = scd_value;
		if( !scd_value )
		    scd_cb->start_gc_thread = 0;
		break;

	    case SCO_GC_NUMTICKS:
		dca->char_id = DMC_C_GC_NUMTICKS;
		dca++->char_value = scd_value;
		break;

	    case SCO_GC_THRESHOLD:
		dca->char_id = DMC_C_GC_THRESHOLD;
		dca++->char_value = scd_value;
		break;

	    case SCO_CP_TIMER:
		scd_cb->cp_timeout = scd_value;
		break;

	    case SCO_CREATE_COMPRESSION:
		/* Values should be validated by cbf */
		if (STcasecmp(scd_svalue, "none") == 0)
		    psq_cb->psq_create_compression = DMU_COMP_OFF;
		else if (STcasecmp(scd_svalue, "data") == 0)
		    psq_cb->psq_create_compression = DMU_COMP_ON;
		else if (STcasecmp(scd_svalue, "hidata") == 0)
		    psq_cb->psq_create_compression = DMU_COMP_HI;
		else
		    TRdisplay("%@ Invalid create_compression value %s ignored\n",
			scd_svalue);
		break;


	    case SCO_CSRUPDATE:
		if (STcasecmp(scd_svalue, ERx("deferred") ) == 0)
		{
		    Sc_main_cb->sc_csrflags |= SC_CSRDEFER;
		    /* We don't tell PSF about this since it's the default */
		}
		else if (STcasecmp(scd_svalue, ERx("direct") ) ==
			 0)
		{
		    Sc_main_cb->sc_csrflags |= SC_CSRDIRECT;
		    psq_cb->psq_flag |= PSQ_DFLT_DIRECT_CRSR;
		}
		else
		{
		    sc0e_put(E_SC026B_INVALID_CURSOR_MODE, (CL_ERR_DESC *)0, 1,
			     STlength(scd_svalue), scd_svalue,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0 );
		    failures++;
		    break;
		}
		break;
	   case SCO_DEFCRS_MODE:
		if (STcasecmp(scd_svalue, ERx("readonly") ) == 0)
		{
			Sc_main_cb->sc_defcsrflag |= SC_CSRRDONLY;
			psq_cb->psq_flag2 |= PSQ_DFLT_READONLY_CRSR;
		}
		else if (STcasecmp(scd_svalue, ERx("update") ) == 0)
		{
			Sc_main_cb->sc_defcsrflag |= SC_CSRUPD;
			/*We don't tell PSF about this since it's the default */
		}
		else
		{
		    sc0e_put(E_SC026B_INVALID_CURSOR_MODE,(CL_ERR_DESC *)0, 1,
			 	STlength(scd_svalue), scd_svalue,
				0, (PTR)0,
				0, (PTR)0,
				0, (PTR)0,
				0, (PTR)0,
				0, (PTR)0 );
			failures++;
			break;
		}
		break;

	    case SCO_FLATTEN:
		/* query_flattening: off */
		Sc_main_cb->sc_flatflags |= 
			(SC_FLATNONE | SC_FLATNAGG | SC_FLATNSING);
		opf_cb->opf_smask |= OPF_GSUBOPTIMIZE;
		break;

	    case SCO_FLATNAGG:
		/* qflatten_aggregate: off */
		Sc_main_cb->sc_flatflags |= SC_FLATNAGG;
		opf_cb->opf_smask |= OPF_NOAGGFLAT;
		break;

	    case SCO_FLATNSING:
		/* qflatten_singleton: off */
		Sc_main_cb->sc_flatflags |= SC_FLATNSING;
		psq_cb->psq_flag |= PSQ_NOFLATTEN_SINGLETON_SUBSEL;
		break;

	    case SCO_FLATNCARD:
		/* qflatten_chk_card: off */
		Sc_main_cb->sc_flatflags |= SC_FLATNCARD;
		psq_cb->psq_flag |= PSQ_NOCHK_SINGLETON_CARD;
		opf_cb->opf_smask |= OPF_DISABLECARDCHK;
		break;

	    case SCO_SXF_MEM:
		sxf_rcb->sxf_pool_sz = scd_stvalue;
		break;

	    case SCO_CURS_LIMIT:
		Sc_main_cb->sc_acc = scd_value;	
		break;

	    case SCO_SECURE_LEVEL:
		scd_cb->secure_level = scd_svalue;
		break;

	    case SCO_C2_MODE:
		/* c2.mode: on */
		scd_cb->c2_mode = TRUE;
		break;

	    case SCO_TABLE_JNL_DEFAULT:
		/* default_journaling: off */
		psq_cb->psq_flag |= PSQ_NO_JNL_DEFAULT;
		break;

	    case SCO_ULM_CHUNK_SIZE:
		/* Set the default ULM chunk size. */
		{
		    DB_STATUS	stat;

		    stat = ulm_chunk_init(scd_value);
		    if (DB_FAILURE_MACRO(stat))
			failures++;
		}
		break;

	    case SCO_ULM_PAD_BYTES:
		/* Set the default ULM pad bytes. */
		{
		    DB_STATUS	stat;
		    stat = ulm_pad_bytes_init(scd_value);
		    if (DB_FAILURE_MACRO(stat))
			failures++;
		}
		break;

	    case SCO_BATCHMODE:
		Sc_main_cb->sc_capabilities |= SC_BATCH_SERVER;
		break;

            case SCO_GCA_SINGLE_MODE:
                Sc_main_cb->sc_capabilities |= SC_GCA_SINGLE_MODE;
                break;
 
	    case SCO_IOMAST_WALONG:
		/* number of write along threads for IOMASTER server */
		scd_cb->num_writealong = scd_value;
		break;

	    case SCO_READAHEAD:    
		/* number of read ahead  threads */
		scd_cb->num_readahead  = scd_value;
		break;

	    case SCO_IOMAST_SLAVES:
		/* number of IO slaves specifically for IOMASTER server */
		scd_cb->num_ioslaves  = scd_value;
		break;

	    case SCO_LKMODE_SYS_READ:
		{
		    i4 dca_val;
		    if (STcasecmp(scd_svalue, ERx("nolock") ) == 0)
		    {
			dca_val = DMC_C_READ_NOLOCK;
		    }
		    else if (STcasecmp(scd_svalue, ERx("shared") ) == 0)
		    {
			dca_val = DMC_C_READ_SHARE;
		    }
		    else if (STcasecmp(scd_svalue, ERx("exclusive") ) == 0)
		    {
			dca_val = DMC_C_READ_EXCLUSIVE;
		    }
		    else
			break;   /* Allow DMF to set 'system' defaults */

		    dca->char_id = DMC_C_LREADLOCK;
		    dca++->char_value = dca_val;

		    break;
		}

	    case SCO_LKMODE_SYS_MAXLK:
		dca->char_id = DMC_C_LMAXLOCKS;
		dca++->char_value = scd_value;
		break;

	    case SCO_LKMODE_SYS_TIMEOUT:
		dca->char_id = DMC_C_LTIMEOUT;
		dca++->char_value = scd_value;
		break;

	    case SCO_SYS_ISOLATION:
		{
		    i4 dca_val;
		    if (STcasecmp(scd_svalue, ERx("serializable") ) == 0)
		    {
			dca_val = DMC_C_SERIALIZABLE;
		    }
		    else if (STcasecmp(scd_svalue, ERx("repeatable_read") ) == 0)
		    {
			dca_val = DMC_C_REPEATABLE_READ;
		    }
		    else if (STcasecmp(scd_svalue, ERx("read_committed") ) == 0)
		    {
			dca_val = DMC_C_READ_COMMITTED;
		    }
		    else if (STcasecmp(scd_svalue, ERx("read_uncommitted")
			) == 0)
		    {
			dca_val = DMC_C_READ_UNCOMMITTED;
		    }
		    else
			break;   /* Allow DMF to set 'system' defaults */

		    dca->char_id = DMC_C_ISOLATION_LEVEL;
		    dca++->char_value = dca_val;

		    break;
		}

	    case SCO_LOG_READNOLOCK:
	    {
                /* on */
                dca->char_id = DMC_C_LOG_READNOLOCK;
                dca++->char_value = scd_value;
                break;
	    }

	    case SCO_LOG_ESC_LPR_SC:
	    {
                /* on */
                dca->char_id = DMC_C_LOG_ESC_LPR_SC;
                dca++->char_value = scd_value;
                break;
	    }

	    case SCO_LOG_ESC_LPR_UT:
	    {
                /* on */
                dca->char_id = DMC_C_LOG_ESC_LPR_UT;
                dca++->char_value = scd_value;
                break;
	    }

	    case SCO_LOG_ESC_LPT_SC:
	    {
                /* on */
                dca->char_id = DMC_C_LOG_ESC_LPT_SC;
                dca++->char_value = scd_value;
                break;
	    }

	    case SCO_LOG_ESC_LPT_UT:
	    {
                /* on */
                dca->char_id = DMC_C_LOG_ESC_LPT_UT;
                dca++->char_value = scd_value;
                break;
	    }

	    case SCO_4K_STATUS:
		   cache_4k_on = TRUE;
		break;
		    
	    case SCO_4K_DBUFFERS:
		dca->char_id = DMC_C_4K_BUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_DGBUFFERS:
		dca->char_id = DMC_C_4K_GBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_DGCOUNT:
		dca->char_id = DMC_C_4K_GCOUNT;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_WBSTART:
		dca->char_id = DMC_C_4K_WSTART;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_WBEND:
		dca->char_id = DMC_C_4K_WEND;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_MLIMIT:
		dca->char_id = DMC_C_4K_MLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_FLIMIT:
		dca->char_id = DMC_C_4K_FLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_NEWALLOC:
		dca->char_id = DMC_C_4K_NEWALLOC;
		dca++->char_value = scd_value;
		break;

	    case SCO_4K_WRITEBEHIND:
		/* 4k WriteBehind OFF */
		dca++->char_id = DMC_C_4K_WRITEBEHIND;
		break;

	    case SCO_4K_SCANFACTOR:
		dca->char_id = DMC_C_4K_SCANFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	    case SCO_8K_STATUS:
		   cache_8k_on = TRUE;
		break;
		    
	    case SCO_8K_DBUFFERS:
		dca->char_id = DMC_C_8K_BUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_DGBUFFERS:
		dca->char_id = DMC_C_8K_GBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_DGCOUNT:
		dca->char_id = DMC_C_8K_GCOUNT;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_WBSTART:
		dca->char_id = DMC_C_8K_WSTART;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_WBEND:
		dca->char_id = DMC_C_8K_WEND;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_MLIMIT:
		dca->char_id = DMC_C_8K_MLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_FLIMIT:
		dca->char_id = DMC_C_8K_FLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_NEWALLOC:
		dca->char_id = DMC_C_8K_NEWALLOC;
		dca++->char_value = scd_value;
		break;

	    case SCO_8K_WRITEBEHIND:
		/* 8k WriteBehind OFF */
		dca++->char_id = DMC_C_8K_WRITEBEHIND;
		break;

	    case SCO_8K_SCANFACTOR:
		dca->char_id = DMC_C_8K_SCANFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	   case SCO_AMBREP_64COMPAT:
		psq_cb->psq_flag |= PSQ_AMBREP_64COMPAT;
		break;

           case SCO_MONEY_COMPAT:
                Sc_main_cb->sc_money_compat = TRUE;
                break;

	    case SCO_16K_STATUS:
		   cache_16k_on = TRUE;
		break;
		    
	    case SCO_16K_DBUFFERS:
		dca->char_id = DMC_C_16K_BUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_DGBUFFERS:
		dca->char_id = DMC_C_16K_GBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_DGCOUNT:
		dca->char_id = DMC_C_16K_GCOUNT;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_WBSTART:
		dca->char_id = DMC_C_16K_WSTART;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_WBEND:
		dca->char_id = DMC_C_16K_WEND;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_MLIMIT:
		dca->char_id = DMC_C_16K_MLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_FLIMIT:
		dca->char_id = DMC_C_16K_FLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_NEWALLOC:
		dca->char_id = DMC_C_16K_NEWALLOC;
		dca++->char_value = scd_value;
		break;

	    case SCO_16K_WRITEBEHIND:
		/* 16k WriteBehind OFF */
		dca++->char_id = DMC_C_16K_WRITEBEHIND;
		break;

	    case SCO_16K_SCANFACTOR:
		dca->char_id = DMC_C_16K_SCANFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	    case SCO_32K_STATUS:
		   cache_32k_on = TRUE;
		break;
		    
	    case SCO_32K_DBUFFERS:
		dca->char_id = DMC_C_32K_BUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_DGBUFFERS:
		dca->char_id = DMC_C_32K_GBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_DGCOUNT:
		dca->char_id = DMC_C_32K_GCOUNT;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_WBSTART:
		dca->char_id = DMC_C_32K_WSTART;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_WBEND:
		dca->char_id = DMC_C_32K_WEND;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_MLIMIT:
		dca->char_id = DMC_C_32K_MLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_FLIMIT:
		dca->char_id = DMC_C_32K_FLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_NEWALLOC:
		dca->char_id = DMC_C_32K_NEWALLOC;
		dca++->char_value = scd_value;
		break;

	    case SCO_32K_WRITEBEHIND:
		/* 32k WriteBehind OFF */
		dca++->char_id = DMC_C_32K_WRITEBEHIND;
		break;

	    case SCO_32K_SCANFACTOR:
		dca->char_id = DMC_C_32K_SCANFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

	    case SCO_64K_STATUS:
		   cache_64k_on = TRUE;
		break;
		    
	    case SCO_64K_DBUFFERS:
		dca->char_id = DMC_C_64K_BUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_DGBUFFERS:
		dca->char_id = DMC_C_64K_GBUFFERS;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_DGCOUNT:
		dca->char_id = DMC_C_64K_GCOUNT;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_WBSTART:
		dca->char_id = DMC_C_64K_WSTART;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_WBEND:
		dca->char_id = DMC_C_64K_WEND;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_MLIMIT:
		dca->char_id = DMC_C_64K_MLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_FLIMIT:
		dca->char_id = DMC_C_64K_FLIMIT;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_NEWALLOC:
		dca->char_id = DMC_C_64K_NEWALLOC;
		dca++->char_value = scd_value;
		break;

	    case SCO_64K_WRITEBEHIND:
		/* 64k WriteBehind OFF */
		dca++->char_id = DMC_C_64K_WRITEBEHIND;
		break;

	    case SCO_64K_SCANFACTOR:
		dca->char_id = DMC_C_64K_SCANFACTOR;
		dca++->char_fvalue = scd_dvalue;
		break;

            case SCO_ASSOC_TIMEOUT:
                if (scd_value > 0) 
                   Sc_main_cb->sc_assoc_timeout = scd_value;
                break;

	    case SCO_ETAB_PAGE_SIZE:
		dca->char_id = DMC_C_ETAB_PAGE_SIZE;
		dca++->char_value = scd_value;
		break;

	    case SCO_ETAB_TYPE:
		if (STcasecmp(scd_svalue, ERx("default") ) == 0)
		{
		    /* Nothing to do if default */
		}
		else if (STcasecmp(scd_svalue, ERx("new") ) == 0)
		{
		    dca->char_id = DMC_C_ETAB_TYPE;
		    dca++->char_value = DMC_C_ETAB_NEW;
		}
		else if (STcasecmp(scd_svalue, ERx("newheap") ) == 0)
		{
		    dca->char_id = DMC_C_ETAB_TYPE;
		    dca++->char_value = DMC_C_ETAB_NEWHEAP;
		}
		else
		{
		    /*
		    ** This should never happen if the configuration rule
		    ** for this parameter is defined properly in dbms.crs
		    */
		    TRdisplay("scd_options: Invalid etab type %s\n", scd_svalue);
		}
		break;

	    case SCO_PAGETYPE_V1:
		/* page type v1 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V1;
		break;

	    case SCO_PAGETYPE_V2:
		/* page type v2 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V2;
		break;

	    case SCO_PAGETYPE_V3:
		/* page type v3 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V3;
		break;

	    case SCO_PAGETYPE_V4:
		/* page type v4 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V4;
		break;

	    case SCO_PAGETYPE_V5:
		/* page type v5 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V5;
		break;

	    case SCO_PAGETYPE_V6:
		/* page type v6 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V6;
		break;

	    case SCO_PAGETYPE_V7:
		/* page type v7 OFF */
		dca++->char_id = DMC_C_PAGETYPE_V7;
		break;

	    case SCO_SYS_LOCK_LEVEL:
		{
		    i4 dca_val;
		    if (STcasecmp(scd_svalue, ERx("default") ) == 0)
		    {
			dca_val = DMC_C_SYSTEM;
		    }
		    else if (STcasecmp(scd_svalue, ERx("row") ) == 0)
		    {
			dca_val = DMC_C_ROW;
		    }
		    else if (STcasecmp(scd_svalue, ERx("page") ) == 0)
		    {
			dca_val = DMC_C_PAGE;
		    }
		    else if (STcasecmp(scd_svalue, ERx("table") ) == 0)
		    {
			dca_val = DMC_C_TABLE;
		    }
		    else if (STcasecmp(scd_svalue, ERx("mvcc") ) == 0)
		    {
			dca_val = DMC_C_MVCC;
		    }
		    else
			break;   /* Allow DMF to set 'system' defaults */

		    dca->char_id = DMC_C_LLEVEL;
		    dca++->char_value = dca_val;

		    break;
		}

	    case SCO_RULE_DEL_PREFETCH:
		Sc_main_cb->sc_rule_del_prefetch_off = TRUE;
		break;
		
            case SCO_CLASS_NODE_AFFINITY:
                /* Restrict server class to current node */
		Sc_main_cb->sc_class_node_affinity = 1;
                dmc->dmc_flags_mask |= DMC_CLASS_NODE_AFFINITY;
                break;

	    case SCO_DATE_TYPE_ALIAS:
		if (STcasecmp(scd_svalue, ERx("ansidate") ) == 0)
		    Sc_main_cb->sc_date_type_alias = SC_DATE_ANSIDATE;
		else if (STcasecmp(scd_svalue, ERx("ingresdate") ) == 0)
		    Sc_main_cb->sc_date_type_alias = SC_DATE_INGRESDATE;
		else if (STcasecmp(scd_svalue, ERx("none") ) == 0)
		    Sc_main_cb->sc_date_type_alias = SC_DATE_NONE;
		break;

            case SCO_QEF_NO_DEPENDENCY_CHK:
                qef_cb->qef_nodep_chk = TRUE;
                break; 
                
	    case SCO_BATCH_COPY_OPTIM:
		/* batch copy optimization off */
		Sc_main_cb->sc_batch_copy_optim = FALSE;
		break;
 
	    default:
		sc0e_put( E_SC0232_INVALID_STARTUP_CODE, (CL_ERR_DESC *)0, 2,
		    sizeof( scdopt->sco_index ), (PTR)&scdopt->sco_index,
		    sizeof( scdopt->sco_argtype ), (PTR)&scdopt->sco_argtype,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
		failures++;
		break;
	    }
	}

	/* Patch up dca_index count */

	*dca_index += dca - dca_save;

	/* Don't start up on errors. */

	if( failures )
	    return E_SC0124_SERVER_INITIATE;
	
	return OK;
}
