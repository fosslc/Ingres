/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: rdftrace.H - define RDF trace points
**
** Description:
**
**	RDF currently has a trace vector that will hold up to 128 trace
**	points.  The following constants (defined in RDF.H) describe the
**	RDF trace vector:
**		RDF_NB	-  Number of Bits in map (this translates to the 
**			    number of unique RDF trace points)
**		RDF_NVP -  Number of flags with associated Value Pairs.
**			    This is the number of trace points that accept
**			    values.  NOTE: THE WAY THAT ULT IS SET UP, THESE
**			    MUST BE THE LAST TRACE FLAGS IN THE BIT ARRAY.
**			    IE, IF NVP IS SET TO 20 AND NB IS SET TO 128, THEN
**			    ONLY TRACE POINTS RD109 THRU RD128 WILL ACCEPT
**			    VALUES.
**		RDF_NVAO - Number of Values At Once -- Ie the parameter that
**			    says how large to make the buffer to hold trace 
**			    point values.
**
**	If you want to increase the RDF trace vector, you need to increase
**	these constants in RDF.H, and make corresponding documentation
**	changes here.
**		
**      This file defines RDF trace points.  Currently, the trace points are:
**	RD0001 TO RD0019 -- Action traces -- require an immediate action, do
**			    not accept parameters.
**	    001 -- Invalidate RDF relation cache.
**	    002 -- Invalidate RDF relation cache and RDF Query Tree cache.
**	    003 -- Invalidate LDBDESC cache.
**	    004 -- Invalidate RELATION, QTREE and LDBDESC caches.
**	    005 -- Invalidate DEFEAULTS cache.
**	    0010 -- Invalidate All RDF caches.
**	    0011 -- Dump Memory Data
**	    0012 -- Dump RDF Statistics
**	RD0020 TO RD0100 -- RDF processing traces -- will cause RDF to change 
**			    how it does its processing, depending on how 
**			    traces are set.
**	    020 -- SKIP checking iisynonym catalog when attempting to build a
**		    cache entry for an object.
**	    021 -- Skip updating iidd_tables when in distributed thread.
**	    023 -- Print RDF queries to LDB
**	    030 -- Let every other call to RDU_XLOCK create a private object
**		    (to simulate concurrency)
**	RD0101 TO RD0110 -- Not used at this time, will accept parameters.
**	RD0111 TO RD0128 -- RDR INFO DUMPS:
**	  NOTE:  RDR INFO DUMPS take 1 parameter.  If that parameter is 
**		 specified (any value except zero), then only objects owned
**		 by users are dumped.  If the parameter is not specified or
**		 is specified to zero, then INGRES owned objects are dumped
**		 as well.
**	    111 -- Dump all RDR_INFO information (ie,do dumps 112 thru 123)
**	    112 -- Dump the relation information.
**	    113 -- Dump the attributes information.
**	    114 -- Dump the indexes information.
**	    115 -- Dump integrity tuples.
**	    116 -- Dump protection tuples.
**	    117 -- Dump the statistics information.
**	    118 -- Dump attribute hash table information.
**	    119 -- Dump primary key information.
**	    120 -- Dump iirule tuples.
**	    121 -- Dump view information.
**	    122 -- Dump RDR_INFO strcuture (high level RDF info)
**	    123 -- Dump LDBdesc cache object
**	    124 -- Dump cache object when unfixing it (like 111, but 111 is 
**		   done at creation time and this is done at unfix time)
**	    125 -- Dump cache object when invalidating it (like 111, but 111 is 
**		   done at creation time and this is done at unfix time)
**	    126-128 -- not used at this time.
**
**
** History:
**      08-dec-1989 (teg)
**	    initial creation.
**	16-sep-92 (teresa)
**	    define trace points to invalidate LDBdesc cache (rd3 and rd4).
**	17-sep-92 (teresa)
**	    defined trace points rd0021, rd0022, rd0023, rd0123, rd0124 & rd0125
**	25-feb-93 (teresa)
**	    defined trace points rd0005, rd0010 for FIPS constraints.
**	22-apr-94 (teresa)
**	    Defined trace points rd0011, rd0012
**	28-may-1997 (shero03)
**	    Reverse the meaning for RDF_CHECK_SUM
[@history_template@]...
**/

/* define action traces */
#define RD0001  1     /* trace value - invalidate rdf relation cache. */
#define RD0002  2     /* trace value - invalidate rdf relation and qtree cache*/
#define RD0003  3     /* trace value - invalidate ldbdesc cache */
#define RD0004  4     /* trace value - inval. relation, qtree & ldbdesc cache */
#define RD0005  5     /* trace value - invalidate defaults cache */

#define RD0010  10    /* trace value - invalidate ALL RDF caches */

#define RD0011  11     /* trace value - dump memory data */
#define RD0012  12     /* trace value - dump memory data */
#define RD0013  13     /* trace value - dump PSF RDF allocated memory */

#define RD_ACT_MAX  19

/* define RDF processing traces */
#define RD0020	20    /* skip querying iisynonym catalog while attempting to
		      ** build an entry on the relation cache. */
#define	RD0021  21    /* Skip updating iidd_tables when in distributed thread.*/
#define RD0022  22    /* Skip calculating checksum */
#define			RDU_CHECKSUM	22  /* Calculate checksum */
#define	RD0023  23    /* print rdf queries to LDB */
#define			RDU_QRYTXT	23  /* print RDF Queries */
#define RD0030  30    /* cause every other call to rdu_xlock to make a private
		      ** object (to simulate concurrency) */

/* trace points 111 to 49 are for RDF INFO dumps */
#define RD0111	111   /* trace value - requests dump of all RDF info on obj */
#define RD0112	112   /* trace value - requests dump of RDF relation info */
#define RD0113	113   /* trace value - requests dump of RDF attribute info */
#define RD0114	114   /* trace value - requests dump of RDF index info */
#define RD0115	115   /* trace value - requests dump of RDF integrity info */
#define RD0116	116   /* trace value - requests dump of RDF protection info */
#define RD0117	117   /* trace value - requests dump of RDF statistic info */
#define	RD0118  118   /* trace value - requests dump attribute hash info */
#define	RD0119  119   /* trace value - requests dump primary key info */
#define	RD0120  120   /* trace value - requests dump of RDF rule info */
#define	RD0121  121   /* trace value - requests dump of RDF view info */
#define RD0122	122   /* trace value - requests dump of RDF_INFO ptrs/values */
#define RD0123  123   /* trace value - requests dump of LDB descriptor */
#define RD0124  124   /* trace value - dump infoblk when unfixing */
#define RD0125  125   /* trace value - dump infoblk when invalidating */
