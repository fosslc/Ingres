/*
**Copyright (c) 2004 Ingres Corporation
**
**  24-jul-89 (stec)
**	Added OPQ_PRMLEN define.
**  22-jan-90 (stec)
**	Added a field to OPQ_TBLINFO.
**	Renamed to OPQ_DBCAPS.
**  21-mar-90 (stec)
**	Create typedef OPQ_GLOBAL.
**  10-apr-90 (stec)
**	Removed MAXATTS. MAXRELS.
**  16-may-90 (stec)
**	Moved some FUNC_EXTERNs from source files.
**  11-jan-91 (stec)
**	Changed return type for opq_exhandler() to STATUS.
**  28-feb-91 (stec)
**	Changed defines TEXT7O. Removed define TEXT6 and added
**	defines TEXT6I, TEXT6O instead.
**  20-mar-91 (stec)
**	Added OPQ_INIPREC define. Modified TEXT7O to specify 
**	left justification for floating-point numbers.
**	Added OPQ_MARGIN.
**  12-feb-92 (rog)
**	Removed OPQ_ESTATUS because FAIL is a much better error exit code.
**  09-nov-92 (rganski)
**	Added prototypes, except for FEadfcb() and opq_error().
**	Moved FUNC_EXTERNs to bottom of file to fix problem of undefined types
**	in prototypes, and changed their ordering to correspond to the ordering
**	in the source modules.
**	Added FUNC_EXTERNs for new functions opq_adferror and
**	opq_translatetype, and old function r_rel_list_from_rel_rel, which is
**	now extern rather than static.
**  14-dec-92 (rganski)
**	Added new bound_length parameter to badarglist() and setattinfo()
**	prototypes.
**	Added defines for MINLENGTH and MAXLENGTH, for new length flag.
**  18-jan-93 (rganski)
**	Added prototype for opq_print_stats(). Moved typedef of OPQ_DBUF here
**	from opqoptdb.sc.
**	Added new versions of TEXT1 which contain new version field.
**	Added new versions of TEXT6 which contain new value length field.
**	Added #defines of OPQ_IO_VALUE_LENGTH and OPQ_LINE_SIZE.
**	Added typedef OPQ_IO_VALUE.
**  26-apr-93 (rganski)
**  	Character Histogram Enhancements project:
**  	Added TEXT9O through TEXT12I for printing and scanning character set
**  	statistics.
**	Added typedef OPQ_BMCHARSET, which is a bitmap used for recording
**	characters present in a column's character positions,
**	OPQ_BMCHARSETBITS, which is the length of such a bitmap in bits, and
**	OPQ_BMCHARSETBYTES, which is the length of such a bitmap in bytes.
**  21-jun-93 (rganski)
**  	Changed OPQ_MERROR to 12, since E_OP0917_HTYPE requires 12 params.
**  20-sep-93 (rganski)
**  	Delimited ID support. Added prototypes for opq_idxlate() and
**  	opq_idunorm(), among other things.
**  18-oct-93 (rganski)
**  	Added prototype for opq_collation().
**  8-jan-96 (inkdo01)
**	Added another I/O pair of TEXT7 formats to support per-cell repfacts
**	and added parm to opq_print_stats.
**  25-march-1997(angusm)
**	Add new type OPQ_DELIM (bug 76860)
**  08-jun-1999 (chash01)
**      add relwid (row width) at the end of opq_rlist, RMS GW need row width
**      to calculate (approximate) page number.	
**  26-apr-01 (inkdo01)
**	Changed -zh displayed col name from COMPOSITE to IICOMPOSITE for 
**	composite histos (to be consistent with statdump).
**	17-july-01 (inkdo01)
**	    Changed isrelation, i_rel_list_... and r_rel_list_... to support
**	    "-xr" command line parms.
**  31-aug-01 (toumi01)
**	fix discrepancy found during i64_aix port: for getspace change size
**	from u_i4 to SIZE_TYPE
**  29-apr-2002 (wanfr01)
**      Bug 106456, INGSRV 1615
**      Changed opq_error so that it correctly uses variable length arguments
**  22-mar-2005 (thaju02)
**	Added waitopt to badarglist() prototype.
**  06-apr-2005 (toumi01)
**	Remove TEXTNULLX, obsoleted by changes to opqopt.sc.
**  21-Apr-2005 (thaju02)
**	For OPQ_BUFTYPE increased maxtup from DB_MAXTUP(2008) to 
**	DB_MAXSTRING(32000). (B114369)
**  29-sep-05 (inkdo01)
**	Diddled a couple of constants to permit up to 32000 cells.
**	17-oct-05 (inkdo01)
**	    Add precision to repetition factor display to get rid of 
**	    those annoying OP0991 warnings.
**  01-nov-2005 (thaju02)
**	Add statdump flag to i_rel_list_from_input() prototype.
**	22-dec-06 (hayke02)
**	    Add nosetstats boolean to badarglist() prototype for new "-zy"
**	    optimizedb flag which switches off 'set statistics' if it has
**	    already been run. This change implements SIR 117405.
**	24-Aug-2009 (kschendel) 121804
**	    Add missing prototypes.
*/

# define	NAMELEN		DB_GW1_MAXNAME
/* max length of database name - should max value in DBMS.H
*/
#  define	MAXARGLEN	1000
/* maximum number of arguments that can be handled by OPTIMIZEDB from an
** input file specification or command line spec
*/
# define	MAXCELLS	32000
/* maximum number of cells than can be defined for a histogram
*/
# define	MINCELLS	1
/* minimum number of cells than can be defined for a histogram, this does not
** include the "lower bound" cell, i.e. 1 implies one upper bound and a default
** lower bound
*/
# define	MINLENGTH	1
/* minimum length for boundary values, as specified in "-length" flag.
*/
# define	MAXLENGTH	DB_MAX_HIST_LENGTH
/* maximum length for boundary values, as specified in "-length" flag.
*/
# define        MAXIFLAGS       10
/* maximum number of ingres flags
*/
# define        MAXBUFLEN       256
/* maximum buffer size for length of one argument from the input specification
** file
*/
# define        OPQ_MERROR      12
/* maximum number of parameters to the error formatting routine
*/
# define	OPQ_PRMLEN	20
/* Maximum length of command line argument to be saved in a local variable
** to be converted to lower case for parsing.
*/
# define	OPQ_IO_VALUE_LENGTH	(4 * DB_MAX_HIST_LENGTH)
/* Maximum number of characters in the printout of a boundary value
*/
# define	OPQ_LINE_SIZE		(70 + OPQ_IO_VALUE_LENGTH)
/* Maximum number of characters in a statdump line
*/
# define	OPQ_BMCHARSETBITS	256
/* Length in bits of a character set bitmap.
*/
# define	OPQ_BMCHARSETBYTES	(OPQ_BMCHARSETBITS / BITSPERBYTE)
/* Length in bytes of a character set bitmap.
*/

# define TEXT1 	"\n\n*** statistics for database %s"
# define TEXT1O "\n\n*** statistics for database %s version: %s"
# define TEXT1I "\n\n*** statistics for database %s version: %s"
# define TEXT2 	"\n*** table %s rows:%d pages:%d overflow pages:%d"
# define TEXT2A	"*** table "
# define TEXT2B	"rows:%d pages:%d overflow pages:%d"
# define TEXT2C "rows"
# define TEXT3 	"\n*** column %s of type %s (length:%d, scale:%d, %s)"
# define TEXT3A	"*** column "
# define TEXT3B	"of type %s (length:%d, scale:%d, %s)"
# define TEXT3C	"of t"
# define TEXT3D	"\n*** column IICOMPOSITE of type character (length:%d, scale:%d, %s)"
# define TEXT4O "\ndate:%24.24s unique values:%f"
# define TEXT4I1 "\ndate: %*s unique values:%f"
# define TEXT4I2 "\ndate: %*s %*s unique values:%f"
# define TEXT4I3 "\ndate: %*s %*s %*s unique values:%f"
# define TEXT5 	"\nrepetition factor:%f unique flag:%c complete flag:%d"
# define TEXT5O	"\nrepetition factor:%%-%d.%df unique flag:%%c complete flag:%%d"
# define TEXT6O "\ndomain:%%d histogram cells:%%d null count:%%-%d.%df"
# define TEXT6O1 "\ndomain:%%d histogram cells:%%d null count:%%-%d.%df value length:%%d"
# define TEXT6I "\ndomain:%d histogram cells:%d null count:%f value length:%d"
# define TEXT7O "\ncell:%%5d    count:%%-%d.%df    value:"
# define TEXT7I "\ncell:%d    count:%f    value:"
# define TEXT7O1 "\ncell:%%5d    count:%%-%d.%df    repf:%%-%d.%df    value:"
# define TEXT7I1 "\ncell:%d    count:%f    repf:%f    value:"
# define TEXT8 	"%s"
# define TEXT9O	"\nunique chars: "
# define TEXT9I	"\nunique chars: %d"
# define TEXT10O "\nchar set densities: "
# define TEXT10I "\nchar set densities: %f"
# define TEXT11O "%%-%d.%df "
# define TEXT11I "%f"
# define TEXT12O "%d "
# define TEXT12I "%d"

# define COMPOSITE "composite                       "
# define DATE_SIZE   25
# define END_OF_DATA	100

/* defines shared by statdump, optimizedb, utils */

# define	OPQ_MTAG	(u_i4)2351	/* memory tag */

# define	OPQ_NO_RETRIES	0		/* No retries in case of
						** serialization error.
						*/
# define	OPQ_SER_RETRIES	3		/* no. of retries in case of
						** serialization error.
						*/
# define	OPQ_SERIAL  0			/* indicates serialization
						** error.
						*/
# define	OPQ_INTR    1			/* interrupt */
# define	OPQ_DEFAULT 3			/* all other interrupts */

# define	OPQ_MARGIN  100			/* extra table slots */


/*}
** Name: OPQ_NAME - define a db name, with room for a null character
**
** Description:
**      Defines memory for a name, with room for a null terminating character
**
** History:
**      9-dec-86 (seputis)
**          initial creation
[@history_template@]...
*/

typedef struct _OPQ_NAME
{
    union 
    {
	    DB_ATT_NAME	   attname;         /* room for attribute name */
	    DB_OWN_NAME    ownname;         /* room for owner name */
            DB_DB_NAME	   dbname;          /* room for data base name */
            DB_TAB_NAME	   tabname;	    /* room for table name */
            char           maxname[DB_GW1_MAXNAME];
    }   nametype;
    char	    nullchar;		    /* room for a null character */
} OPQ_NAME;

/*}
** Name: OPQ_DELIM - define a tab name, with room for a null character
**
** Description:
**      Defines memory for a table name, with room for a null 
**	terminating character
**
** History:
**	25-march-1997 (angusm)
**	derived from OPQ_NAME
[@history_template@]...
*/

typedef struct _OPQ_DELIM
{
    union 
    {
            DB_TAB_NAME	   tabname;	    /* room for table name */
            char           maxname[DB_MAX_DELIMID];
    }   nametype;
    char	    nullchar;		    /* room for a null character */
} OPQ_DELIM;

/*}
** Name: OPQ_RLIST - relation desciptor on which to find statistics
**
** Description:
**      Contains info on relation upon which statistics will be gathered
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	16-sep-88 (stec)
**	    Modified (removed/added fields).
**	13-sep-89 (stec)
**	    Added samplecrtd flag and tidcol field to hold
**	    the name of column with tid values when sampling.
**	20-sep-93 (rganski)
**	    Delimited ID support: added delimname field, which holds the
**	    delimited table name. If relation name is not delimited, this field
**	    is same as relname.
**	25-march-1997(angusm)
**	    Change type of samplename (bug 76860)
**      08-jun-1999 (chash01)
**         add relwid (row width) at the end of opq_rlist, RMS GW need row width
**         to calculate (approximate) page number.	
**	5-apr-00 (inkdo01)
**	    Changes for composite histograms.
**	2-nov-00 (inkdo01)
**	    Added flag to indidcate presence of comp histogram for statdump.
**	29-may-01 (inkdo01)
**	    Remove superfluous tidcol field.
**	21-Mar-2005 (schka24)
**	    Try to resolve the ongoing struggle with delimited table names
**	    by insisting that delimname always be the translated table
**	    name with double-quotes added if necessary.  Add argname which
**	    is the exact command line parameter.
*/
typedef struct _OPQ_RLIST
{
	OPQ_NAME	relname;	/* relation name with room for
                                        ** null terminator character */
	OPQ_DELIM	delimname;	/* Delimited relation name (dquotes
					** added if needed); null-terminated */
	OPQ_DELIM	argname;	/* Relation name exactly as given on
					** the command line */
	i4		ntups;		/* number of tuples in relation */
	OPQ_DELIM	samplename;	/* relation name with sample data */
	i4		nsample;	/* number of sample tuples */
	OPQ_NAME	ownname;	/* owner name with room for
                                        ** null terminator character */
	i4		reltid;		/* 1st part of Ingres relation id */
	i4		reltidx;	/* 2nd part of Ingres relation id */
	i4		pages;		/* estimated no. of physical pages*/
	i4		overflow;	/* estimated no. of overflow pages*/
	i4		attcount;	/* count of interesting attributes*/
	bool		statset;	/* indicates if 'set statistics' has
					** been executed. */
	bool		sampleok;	/* TRUE if sampling allowed
					** (an ingres table) */
	bool		physinfo;	/* TRUE if present, else FALSE */
	bool		withstats;	/* TRUE if stats exist, else FALSE */
	bool		samplecrtd;	/* TRUE if sample table created */
	bool		index;		/* TRUE if stats on index */
	bool		comphist;	/* TRUE if composite hist on keys */
        i4              relwid;         /* row width for rms gateway use */
} OPQ_RLIST;


/*}
** Name: OPQ_ALIST - descriptor of an attribute
**
** Description:
**      Contains info about an attribute upon which statistics will be gathered
**
** History:
**      29-nov-86 (seputis)
**          initial creation.
**	05-jan-89 (stec)
**	    Modify to reflect requirements for new OPQ code.
**	10-jul-89 (stec)
**	    Added userlen to attribute info.
**	20-sep-93 (rganski)
**	    Delimited ID support: added field delimname.
**	25-march-1997(angusm)
**	    Change type of delimname (bug 76860)
**	28-dec-04 (inkdo01)
**	    Add collID.
*/
typedef struct _OPQ_ALIST
{
	DB_DATA_VALUE	attr_dt;	/* attribute datatype */
	DB_DATA_VALUE	attr1_dt;	/* copy of attr_dt - used in min/max
                                        ** calculation */
	DB_DATA_VALUE   hist_dt;	/* histogram datatype */
	DB_ATT_ID	attrno;         /* attribute id */
	OPQ_NAME	attname;        /* attribute name with room for
                                        ** null terminating character */
	OPQ_DELIM	delimname;      /* Delimited attribute name with 
					** room for null terminating 
					** character */
	OPQ_NAME	typename;	/* name of column data type */
	char		nullable;	/* Y/N */
	i2		scale;		/* column scale */
	i2		collID;		/* column collation ID */
	i2		userlen;	/* column len (user specified) */
	bool		withstats;	/* TRUE if stats exist, else FALSE */
	bool		comphist;	/* TRUE if descr represents a 
					** composite histogram */
} OPQ_ALIST;

/*}
** Name: OPQ_MAXELEMENT - histogram element
**
** Description:
**      This structure should define the alignment and size of the largest
**      histogram element.
**
** History:
**      8-dec-86 (seputis)
**          initial creation
**	18-jan-93 (rganski)
**	    Increased maxsize to DB_MAX_HIST_LENGTH.
[@history_template@]...
*/
typedef struct _OPQ_MAXELEMENT
{
    union
    {
        i4		   int_alignment;      /* align for integer */
	f8                 float_alignment;    /* align for float */
	char               maxsize[DB_MAX_HIST_LENGTH];
	                                       /* at least DB_MAX_HIST_LENGTH
					       ** bytes for one element */
    }   histodata; 
} OPQ_MAXELEMENT;

/*}
** Name: OPQ_BUFTYPE - defines max memory area required for tuple buffer
**
** Description:
**      Max memory area required for a tuple buffer
**
** History:
**      8-dec-86 (seputis)
**	21-Apr-2005 (thaju02)
**	    Increased maxtup from DB_MAXTUP(2008) to DB_MAXSTRING(32000).
**	    (B114369)
[@history_template@]...
*/
typedef struct _OPQ_BUFTYPE
{
    union 
    {
	    ALIGN_RESTRICT  align_restrict;	/* align this structure */
	    char           maxtup[DB_MAXSTRING+1]; 
    }   buf;
} OPQ_BUFTYPE;

/*}
** Name: OPQ_IO_VALUE - defines max memory area required for value buffer for
** 			printing or reading statdump boundary values
**
** Description:
**      Max memory area required for a value buffer. Align restricted and
**      contains room for text length field (DB_CNT_SIZE).
**
** History:
**	18-jan-93 (rganski)
**	    Initial creation as variant of OPQ_BUFTYPE.
**
**[@history_template@]...
*/
typedef struct _OPQ_IO_VALUE
{
    union 
    {
	    ALIGN_RESTRICT  	align_restrict;
	    char		value[DB_CNTSIZE + OPQ_IO_VALUE_LENGTH + 1];
    }   buf;
} OPQ_IO_VALUE;

/*}
** Name: OPQ_ERRCB - error control block for optimizer utilities
**
** Description:
**      Contains information used for error reporting in the optimizer 
[@comment_line@]...
**
** History:
**      26-nov-86 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPQ_ERRCB
{
    char            errbuf[ER_MAX_LEN + 1]; /* buffer used to contain error
					    ** message
					    */
    char            adfbuf[ER_MAX_LEN + 1]; /* buffer for adf error message */
    DB_STATUS       status;		    /* operation status */
    DB_ERROR        error;		    /* utility error code */
}   OPQ_ERRCB;

/*}
** Name: OPQ_ENVIRONMENT - environment state of utility
**
** Description:
**      Structure used to keep information about the current environment
**      in which the utility is operating
**
** History:
**      26-nov-86 (seputis)
**          initial creation
**	9-june-99 (inkdo01)
**	    Change fid to ifid and add ofid, so optdb can have both input and
**	    output files.
[@history_template@]...
*/
typedef struct _OPQ_ENV
{
    bool            dbopen;             /* TRUE is database is open */
    FILE	    *ifid;		/* input file id if open, else NULL */
    FILE	    *ofid;		/* output file id if open, else NULL */
} OPQ_ENV;

/*}
** Name: OPQ_DBMS - dbms type
**
** Description:
**      Data type describing DBMS in use.
**
** History:
**      01-feb-89 (stec)
**          Written.
**	24-jul-90 (stec)
**	    Added a define for the RMS gateway.
*/
typedef struct _OPQ_DBMS
{
    i2		dbms_type;	    /* DBMS type */
# define    OPQ_DBMSUNKNOWN 0x0
# define    OPQ_GENDBMS	    0x01    /* bit 0 */
# define    OPQ_INGRES	    0x02    /* bit 1 */
# define    OPQ_STAR	    0x04    /* bit 2 */
# define    OPQ_RMSGW	    0x08    /* bit 3 */
} OPQ_DBMS;

/*
** Name: OPQ_LANG - language info.
**
** Description:
**      Data type describing languages.
**
** History:
**      01-mar-89 (stec)
**          Written.
*/
typedef struct _OPQ_LANG
{
    i4	    ing_sqlvsn;		/* ingres sql version */
    i4	    com_sqlvsn;		/* common sql version */
# define    OPQ_NOTSUPPORTED	0
# define    OPQ_SQL600		600
# define    OPQ_SQL601		601
# define    OPQ_SQL602		602
# define    OPQ_SQL603		603
				/* version
				** 00600 - 6.0
				** 00601 - 6.1
				** 00000 - not supported
				** value from iidbcapabilities
				*/
} OPQ_LANG;

/*}
** Name: OPQ_TBLINFO - database capabilities.
**
** Description:
**      Describes the database capabilities.
**
** History:
**      01-mar-89 (stec)
**          Written.
**	21-dec-89 (stec)
**	    Added info about table name case
**	    to be used in queries.
**	22-jan-90 (stec)
**	    Added a field to OPQ_TBLINFO to indicate
**	    availability of UDTs.
**	    Renamed to OPQ_DBCAPS.
**	13-apr-90 (stec)
**	    Added max_cols.
**	14-dec-92 (rganski)
**	    Added field std_cat_level, which is derived from
**	    STANDARD_CATALOG_LEVEL in iidbcapabilities.
**	20-sep-93 (rganski)
**	    Delimited ID support. Added field delim_case, which corresponds to
**	    DB_DELIMITED_CASE. Added field cui_flags, which is used for case
**	    translation and normalization of identifiers.
*/
typedef struct _OPQ_DBCAPS
{
    i4		phys_tbl_info_src;	/* physical table info source flag */
# define    OPQ_U_SRC	0   /* source unknown */
# define    OPQ_P_SRC	1   /* iiphysical_tables only */
# define    OPQ_T_SRC   2   /* iitables and iiphysical_tables */
    i4		tblname_case;
    i4		delim_case;
# define    OPQ_NOCASE	0   /* no case */
# define    OPQ_UPPER	1   /* upper case */
# define    OPQ_LOWER	2   /* lower case */
# define    OPQ_MIXED	3   /* mixed case */
    u_i4	cui_flags;	    /* Used for identifier translation */
    i4		cat_updateable;	    /* Indicates what catalogs
				    ** are updateable
				    */
# define    OPQ_CATNOTUPD   0x0	    /* No catalogs are updateable */
# define    OPQ_STATS_UPD   0x1	    /* iistats updateable   */
# define    OPQ_HISTS_UPD   0x2	    /* iihistograms updateable */
# define    OPQ_TABLS_UPD   0x4	    /* iitables updateable */
    i4		udts_avail;	    /* Indicates whether UDTs
				    ** are available.
				    */
# define    OPQ_UDTS	    0x01    /* UDTs allowed. */
# define    OPQ_NOUDTS	    0x02    /* UDTs not allowed. */
    i4		max_cols;	    /* value of MAX_COLUMNS */
    i4		std_cat_level;	    /* value of STANDARD_CATALOG_LEVEL */
} OPQ_DBCAPS;

/*}
** Name: OPQ_GLOBAL - global data structure.
**
** Description:
**      Consolidates info used in OPF utility routines.
**
** History:
**      11-mar-90 (stec)
**          Written.
**	10-apr-90 (stec)
**	    Added opq_maxrel.
**  20-mar-91 (stec)
**	Added OPQ_INIPREC define.
**   2-nov-93 (robf)
**      Add opq_no_tblstats.
**	29-apr-98 (inkdo01)
**	    Add opq_lvrepf to leave repf unmodified.
**	10-jul-00 (inkdo01)
**	    Add opq_newrepfest to request enhanced distinct value 
**	    estimation ("-zdn")
**	30-oct-00 (inkdo01)
**	    Add opq_comppkey to request composite histogram on
**	    primary key structure.
**	28-may-01 (inkdo01)
**	    Add opq_newquery, opq_nott to support new performance options.
**	21-oct-05 (inkdo01)
**	    Add opq_noAFVs to suppress new exact cell insertion into
**	    inexact histogram.
**	26-oct-05 (inkdo01)
**	    Add opq_sample flag to distinguish real samples from 
**	    uses of -zfq option that generate temp tables.
*/
typedef struct _OPQ_GLOBAL
{
    char	*opq_utilid;	/* Name of the utility */
    char	*opq_dbname;	/* Name of the database */
    OPQ_NAME	opq_owner;	/* Owner of the database. */
    OPQ_NAME	opq_dba;	/* DBA of the database */
    OPQ_DBCAPS	opq_dbcaps;	/* capabilities info */
    OPQ_DBMS	opq_dbms;	/* dbms info */
    OPQ_LANG	opq_lang;	/* language info */
    OPQ_ENV	opq_env;	/* environment */
    ADF_CB	opq_adfscb;	/* ADF control block */    
    ADF_CB	*opq_adfcb;	/* ptr to the ADF control block */
    bool	opq_cats;	/* Flag indicating if catalogs are
				** of interest */
    bool	opq_lvrepf;	/* Flag indicating rep factor to be left
				** as in current iistats row */
    bool	opq_newrepfest;	/* Flag requesting enhanced distinct 
				** value estimation ("-zdn") */
    bool	opq_comppkey;	/* Flag requesting composite histogram
				** on pkey structure ("-zcpk") */
    i4		opq_prec;	/* precision for floating pt nos */    
#define		    OPQ_INIPREC	    7
    i4		opq_maxrel;	/* maximum number of relations
				** that will be considered
				** for any database.
				*/
    bool	opq_newquery;	/* Flag requesting new aggregate query ("-zfq") */
    bool	opq_nott;	/* Flag requesting new query temp table logic
				** to be disabled ("-znt") */
    bool	opq_no_tblstats;/* Flag indicating session has table_statistics
			        ** priv. This is TRUE when session does not
				** have table stats.
				*/
    bool	opq_noAFVs;	/* Flag indicating that hcarray[] not be 
				** used to create additional exact cells in 
				** an inexact histogram */
    bool	opq_sample;	/* Flag indicating whether "-zs" parm 
				** was specified */
    bool	opq_hexout;	/* Flag indicating whether "-zhex" pam
				** was specified */
} OPQ_GLOBAL;

/*}
** Name: OPQ_HDEC - global data structure for histdec().
**
** Description:
**      Consolidates floating point values used by
**	histdec() proc.
**
** History:
**      11-mar-90 (stec)
**          Written.
*/
typedef struct _OPQ_HDEC
{
	f4	opq_f4neg;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f4 negative
			    */
	f4	opq_f4nil;  /* value
			    ** to be used in histdec()
			    ** when f4 is zero
			    */
	f4	opq_f4pos;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f4 positive
			    */
	f8	opq_f8neg;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f8 negative
			    */
	f8	opq_f8nil;  /* value
			    ** to be used in histdec()
			    ** when f8 is zero
			    */
	f8	opq_f8pos;  /* multiplication factor
			    ** to be used in histdec()
			    ** when f8 positive
			    */
} OPQ_HDEC;

/*}
** Name: OPQ_DBUF - data structure for buffer with length
**
** Description:
**	Used for buffers in which the length of the buffer is referenced also.
**	length must be kept up to date by the allocator.
**
** History:
**      18-jan-93 (rganski)
**          Moved here from opqoptdb.sc.
*/
typedef struct _OPQ_DBUF {
    i4		length;
    PTR		bufptr;
} OPQ_DBUF;

/*}
** Name: OPQ_BMCHARSET - bitmap for characters present in a character column
**
** Description:
**	A pointer to a bit map representing the ASCII characters present in a
**	single character position of a character column. E.g. if a column
**	contains only the values 'F' and 'M', bits 70  (ASCII 'F') and 77
**	(ASCII 'M') will be set; all other bits will be 0. Used for generating
**	character set statistics.
**	OPQ_BMCHARSETBITS defines length of these bit maps.
**
**	See Character Histogram Enhancements spec for details.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
*/
typedef char *OPQ_BMCHARSET;


/*
**  External function references.
*/
FUNC_EXTERN ADF_CB	*FEadfcb();		/* routine to fetch front end
                                                ** ADF control block
						*/
FUNC_EXTERN i4		opq_scanf(
				char		*buf,
				char		*fmt,
				char		decimal,
				PTR		arg1,
				PTR		arg2,
				PTR		arg3,
				PTR		arg4,
				PTR		arg5);
FUNC_EXTERN VOID	opx_error(
			        i4		errcode);
FUNC_EXTERN VOID	opq_error(DB_STATUS dbstatus, i4 errnum, i4 argcount, ...);
FUNC_EXTERN VOID	opq_sqlerror(VOID);
FUNC_EXTERN VOID	opq_adferror(
			        ADF_CB	 	*adfcb, 
			        i4 	errnum,
			        i4 	p1, 
				PTR 		p2,
				i4 	p3, 
				PTR 		p4);
FUNC_EXTERN STATUS	opq_exhandler(
				EX_ARGS		*ex_args);
FUNC_EXTERN VOID	adfinit(
				OPQ_GLOBAL  	*g);
FUNC_EXTERN VOID	adfreset(
				OPQ_GLOBAL  	*g);
FUNC_EXTERN VOID	nostackspace(
				char		*utilid,
				STATUS		status,
				u_i4	size);
FUNC_EXTERN PTR		getspace(
				char		*utilid,
				PTR		*spaceptr,
				SIZE_TYPE	size);
FUNC_EXTERN VOID	fileinput(
				char            *utilid,
				char            ***argv,
				i4             *argc);
FUNC_EXTERN bool	isrelation(
				char		*relptr,
				bool		*excl);
FUNC_EXTERN bool	isattribute(
				char		*attptr);
FUNC_EXTERN OPS_DTLENGTH align(
			        OPS_DTLENGTH    realsize);
FUNC_EXTERN VOID	setattinfo(
				OPQ_GLOBAL	*g,
				OPQ_ALIST       **attlist,
				i4             *index,
				OPS_DTLENGTH	bound_length);
FUNC_EXTERN bool	badarglist(
				OPQ_GLOBAL	*g,
				i4		argc,
				char            **argv,
				i4             *dbindex,
				char            *ifs[],
				char            ***dbnameptr,
				bool	        *verbose,
				bool	        *histoprint,
				bool  	        *key_attsplus,
				bool	        *minmaxonly,
				i4         *uniquecells,
				i4         *regcells,
				bool            *quiet,
				bool            *deleteflag,
				char		**userptr,
				char            **ifilenm,
				char            **ofilenm,
				bool		*pgupdate,
				f8		*samplepct,
				bool		*supress_nostats,
				bool		*comp_flag,
				bool		*nosetstats,
				OPS_DTLENGTH	*bound_length,
				char            **waitopt,
				bool		*nosample);
FUNC_EXTERN VOID	prelem(
			        OPQ_GLOBAL	*g,
				PTR		dataptr,
				DB_DATA_VALUE   *datatype,
				FILE		*outf);
FUNC_EXTERN VOID	opq_print_stats(
				OPQ_GLOBAL	*g,
				OPQ_RLIST	*relp,
				OPQ_ALIST       *attrp,
				char		*version,
				i4		row_count,
				i4		pages,
				i4		overflow,
				char        	*date,
				f8		nunique,
				f8          	reptfact,
				char           	*unique_flag,
				i1          	iscomplete,
				i2          	domain,
				i2		cellnm,
				OPO_TUPLES    	nullcount,
				OPH_COUNTS	cell_count,
				OPH_COUNTS	cell_repf, 
				OPH_CELLS	intervals,
				FILE		*outf,
				bool		verbose,
				bool		quiet);
FUNC_EXTERN VOID	opq_cntrows(
				OPQ_RLIST   	*rp);
FUNC_EXTERN VOID	opq_phys(
				OPQ_RLIST   	*rp);
FUNC_EXTERN VOID	opq_idxlate(
				OPQ_GLOBAL	*g,
				char		*src,
				char		*xlatename,
				char		*delimname);
FUNC_EXTERN VOID	opq_idunorm(
				char		*src,
				char		*dst);
FUNC_EXTERN bool	i_rel_list_from_input(
				OPQ_GLOBAL	*g,
				OPQ_RLIST	**rellist,
				char		**argv,
				bool		statdump);
FUNC_EXTERN VOID	opq_mxrel(
				OPQ_GLOBAL	*g,
				int	    	argc,
				char	    	*argv[]);
FUNC_EXTERN VOID	opq_collation(
				OPQ_GLOBAL 	*g);
FUNC_EXTERN VOID	opq_ucollation(
				OPQ_GLOBAL	*g);
FUNC_EXTERN VOID	opq_owner(
				OPQ_GLOBAL 	*g);
FUNC_EXTERN VOID	opq_upd(
				OPQ_GLOBAL 	*g);
FUNC_EXTERN VOID	opq_dbmsinfo(
				OPQ_GLOBAL  	*g);
FUNC_EXTERN VOID	opq_translatetype(
				ADF_CB		*adfcb, 
				char		*es_type,
				char 		*es_nulls, 
				OPQ_ALIST 	*attrp);
FUNC_EXTERN bool	r_rel_list_from_rel_rel(
				OPQ_GLOBAL	*g,
				OPQ_RLIST       *rellst[],
				OPQ_RLIST       *ex_rellst[],
				bool		statdump);


