
/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <ddb.h>
# include    <dudbms.h>
# include    <ulm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <adulcol.h>
# include    <dmf.h>
# include    <dmtcb.h>
# include    <scf.h>
# include    <qsf.h>
# include    <qefrcb.h>
# include    <rdf.h>
# include    <cui.h>
# include    <psfparse.h>
# include    <qefnode.h>
# include    <qefact.h>
# include    <qefqp.h>
# include    <intl.h>
# include    <cm.h>
# include    <si.h>
# include    <pc.h>
# include    <lo.h>
# include    <ex.h>
# include    <er.h>
# include    <cv.h>
# include    <st.h>
# include    <me.h>
# include    <generr.h>
/* beginning of optimizer header files */
# include    <opglobal.h>
# include    <opdstrib.h>
# include    <opfcb.h>
# include    <ophisto.h>
# include    <opq.h>

exec sql include sqlca;
exec sql include sqlda;
exec sql declare s statement;
exec sql declare c1 cursor for s;

/*
** Name:	opqdata.c
**
** Description:	Global data for opq facility.
**
** History:
**
**      19-Oct-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPOPFLIBDATA
**	    in the Jamfile.
**	15-july-05 (inkdo01)
**	    Update default histogram sizes to 100 cells for inexact and 
**	    300 for exact.
**	22-dec-06 (hayke02)
**	    Add nosetstats bool for -zy optimizedb flag to switch off 'set
**	    statistics'. This change implements SIR 117405.
*/

/*
**
LIBRARY	=	IMPOPFLIBDATA 
**
*/

/*
**  Data from opqutils.sc
*/

GLOBALDEF OPQ_GLOBAL	    opq_global;		/* global data struct */
GLOBALDEF ER_ARGUMENT	    er_args[OPQ_MERROR];
						/* array of arg structs for
						** ERslookup.
						*/ 
GLOBALDEF i4		    opq_retry	= OPQ_NO_RETRIES;	
						/* serialization error
						** retries count.
						*/
GLOBALDEF i4		    opq_excstat = OPQ_SERIAL;
						/* additional info about 
						** exception processed.
						*/
GLOBALDEF IISQLDA	    _sqlda;
						/* SQLDA area for all
						** dynamically built queries.
						*/
GLOBALDEF OPQ_DBUF	    opq_date	= {0, 0};
                                                /* buffer for date 
						*/
# ifdef xDEBUG
GLOBALDEF i4                opq_xdebug  = (bool)FALSE;  /* debugging only */
# endif /* xDEBUG */

/*
**  Data from opqoptdb.sc
*/

GLOBALDEF       i4         uniquecells     = 300;
/* -zu# flag - If there are not too many unique values for a column, then it is
** worth allowing more cells in order to produce an exact histogram.  This
** number, therefore, specifies the number of unique values that the histogram
** is automatically extended to accommodate.
** - seems like a good default, will handle up to 150 unique values (even more
** if integer domain and some of the values are adjacent)
*/
GLOBALDEF       i4         regcells        = 100;
/* - the histogram can contain no more than this number of cells.  Larger
** numbers require more processing time by the query optimizer but, because
** they are more accurate, generally result in more efficient query execution
** plans
** - if there are too many unique values for a histo with uniquecells then do  
** a regular histo with 15 cells
*/
GLOBALDEF       OPQ_RLIST       **rellist       = NULL;
/* pointer to an array of pointers to relation descriptors - one for each
** relation to be processed by OPTIMIZEDB
*/
GLOBALDEF       OPQ_ALIST       **attlist       = NULL;
/* pointer to an array of pointers to attribute descriptors - used within
** the context of one relation, i.e. one attribute descriptor for each
** attribute of the relation currently being processed
*/
GLOBALDEF       bool            verbose         = FALSE;
/* -zv Verbose. Print information about each column as it is being processed
*/
GLOBALDEF       bool            histoprint      = FALSE;    
/* -zh Histograms. Print the histogram that was generated for each column
** This flag also implies the -zv flag
*/
GLOBALDEF       bool            keyatts         = FALSE;
/* Not used
*/
GLOBALDEF       bool            key_attsplus    = FALSE;   
/* -zk In addition to any columns specified for the table, statistics for
** columns that are keys on the table or are indexed are also generated
*/
GLOBALDEF       bool            minmaxonly      = FALSE;
 
GLOBALDEF       bool            opq_cflag;
/* complete flag.
*/
GLOBALDEF       bool            nosetstats = FALSE;
/* -zy switch off 'set statistics'
*/
GLOBALDEF       OPS_DTLENGTH    bound_length    = 0;
/* The -length command line flag. If not supplied by the user, remains 0; this
** tells optimizedb to figure out the best length for the boundary values. 
** Range is 1 to 1950.
*/
GLOBALDEF       bool            sample          = FALSE;
/* boolean indicating if sampling requested.
*/
GLOBALDEF bool      pgupdate = FALSE;   /* TRUE if page and row count
                            ** in iirelation (or equiv.)
                            ** has been requested.
                            */

GLOBALDEF OPQ_DBUF      opq_sbuf              = {0, 0};
                                              /* buffer for dynamic SQL queries
                                              ** (statements)
                                              */

/*
** data from opqstatd.sc
*/

GLOBALDEF       bool            quiet           = FALSE;
/* -zq Quiet mode.  Print out only the information contained in the
** iistatistics table and not the histogram information contained in the
** iihistogram table
*/
GLOBALDEF       bool            supress_nostats = FALSE;
/* -zqq Supress mode.  Do not print out warning messages for tables with
** no statistics.
*/
GLOBALDEF       bool            deleteflag      = FALSE;
/* -zdl Delete statistics from the system catalogs.  When this flag is
** included, the statistics for the specified tables and columns (if any
** are specified) are deleted rather than displayed
*/


GLOBALDEF	VOID	(*opq_exit)(VOID) = NULL;
GLOBALDEF	VOID	(*usage)(VOID) = NULL;
