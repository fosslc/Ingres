/*
** Copyright (c) 2004 Ingres Corporation 
*/

#ifndef FSTM_HDR_INCLUDED
#define FSTM_HDR_INCLUDED

# include	<si.h>
# include	<lo.h>
# include	<ft.h>

/*
**                       Row Control Block
**
**	History:
**	14-aug-89 (sylviap)
**		Changed mffsmore_proc() to be a bool function.
**		Added PRTKEY (print key).
** 	16-dec-91 (pearl)
**		Fix for bug 40202.  Add field to BCB structure to indicate
**		whether to print time string or not.  At present, checked
**		in _GetFile().  Report Writer shouldn't print time stamp
**		across the top of a report that the user has chosen to
**		put in a file.
**	30-oct-92 (leighb) DeskTop Porting Change:
**		Increased MSDOS defined BB_SIZE to almost 64KB.
**	5-feb-93 (rogerl) add onpopup - TRUE if input-popup is being displayed.
**		caller needs to know in case of popup interrupt.
**	8-mar-93 (rogerl) delete onpopup.
**      10-sep-93 (dkhor)
**              On axp_osf, pointers are 8 bytes long.  The hardcoded
**              RCBLEN is disastrous, need to bump up RCBLEN or else
**              isql/iquel and reportwriter will fail in
**              front/utils/uf/ufbrowse.qsc.
**  6-oct-1999 (rodjo04) bug 98949 INGCBT239
**       Doubled defined BB_SIZE. 
**	20-mar-2000 (kitch01)
**		Bug 99136. Created BFPOS structure to hold an index record for
**		the spill file. Amended BCB to hold index records for the spill
**		file so that we can correctly use the spill file.
**      20-jun-2001 (bespi01)
**              In order to track the length of lines in the spill file, a 
**              length field was added to the bfpos record.
**              Bug 104175, Problem INGCBT 335
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (somsa01)
**	    Changed type of recpos in BFPOS structure to OFFSET_TYPE.
**      08-mar-2002 (horda03)
**              Improve performacne of ISQL when large outputs generated.
**              Use as much memory as we can. Now allow as many Browse
**              Buffers (BBLIST) as the process memory allows. To ensure
**              that we have memory available for use if we exhaust memory
**              will creating hte "complete" browse buffer list, reserve a
**              memory buffer, which wil be released to allow creation of
**              file I/O stream. The amount of memory required for the
**              file I/O stream is dependant on the size of the records
**              written to disk (i.e RACC_BLKSIZE).
**              Bug 107271.
**	28-Mar-2005 (lakvi01)
**	    Prevented multiple inclusion of fstm.h.
**	24-Aug-2009 (kschendel) 121804
**	    Add some function prototypes for gcc 4.3.
*/
typedef struct _rcb
   {
   struct _rcb  *nrcb;  /* addr of next row control block             */
   struct _rcb  *prcb;  /* addr of previous row control block         */
   i2    len;           /* length of data row in this rcb             */
   char  data[2];
   }  RCB;
#define RCBLEN sizeof(struct _rcb *)+sizeof(struct _rcb *)+sizeof(i2)


typedef struct _bfpos
{
	struct _bfpos *nbfpos;  /* addr of next spill file index record	    */
	struct _bfpos *pbfpos;  /* addr of previous spill file index record */
	OFFSET_TYPE   recpos;	/* start position of this record in spill file */
	i4	      recnum;	/* record number in the spill file */
	i4 	reclen; /* length of line in spill file including \n */
	i4	num_recs; /* Number of consequative records in spill file of this length */
} BFPOS;
#define BFPOSLEN sizeof(struct _bfpos)

typedef struct _bblist
{
   struct _bblist *nxtbb;
   struct _bblist *prvbb;
   char   *bb;
   char   *bbend;
} BBLIST;


/* Reserve 3 * <Num pages required to store RACC_BLKSIZE>. This memory
** will be released when process has exhausted its memory quota during
** the retrieval of a query. Thus allowing the spill file to be created,
** and hence dumping the Browse Buffer records to disk; allowing the
** entire query to be retieved.
*/
#define BBMEMBUFPAGES ( (3 * (RACC_BLKSIZE / ME_MPAGESIZE) + 1) )
     
/*
**                       Browse Control Block
*/
typedef struct _bcb
   {
   FTINTRN *bcb_eyeball; /* control block eyeball field              */

   i4    n_bb;          /* Number of browse buffers allocated         */
   BBLIST *firstbb;     /* addr of First Browse buffer info. */
   BBLIST *lastbb;      /* addr of Last Browse Buffer info. the "active one" */
   char  *membuffer;    /* A block of memory that can be release if we need to
                        ** use a spill file - thus ensuring we don't run out
                        ** of memory building spill record indecies.
                        */
   i4    sz_bb;         /* size of browse buffer (bytes)              */

   i4    nrows;         /* number of rows in the browse buffer        */

   RCB  *frcb;          /* addr of rcb for first row in memory        */
   i4    frec;          /* record number of 1st browse buffer row     */

   RCB  *lrcb;          /* addr of rcb for last row in memory         */
   i4    lrec;          /* record number of last browse buffer row    */

   RCB  *vrcb;          /* addr of rcb for first visible row          */
   i4    vrec;          /* record number of first visible row         */
   i4    vcol;          /* first visible column                       */

   LOCATION bfloc;      /* browsefile location                        */
   char  bfname[MAX_LOC];/* browsefile file name                      */
   FILE  *bffd;         /* browsefile file descriptor                 */
   BFPOS *fbfidx;		/* addr of first browse file index record	  */
   BFPOS *vbfidx;		/* addr of first visible browse file record   */
   BFPOS *lbfidx;		/* addr of last visible browse file record    */
   bool	 bf_in_use;		/* TRUE if browsefile in use for MTdraw()     */
   i4    vbf_rec;               /* Record number of first visible browse
                                ** file record.
                                */

   i4    nrecs;         /* number of records in the browse file       */
   i4    mxcol;         /* widest column in file                      */

   i4    mx_rows;       /* number of rows on the screen               */
   i4    mx_cols;       /* number of columns on the screen            */

   char *rd_ahead;      /* ptr to extra record read by FStop          */

   bool eor_warning;    /* TRUE after 'end of results' warning issued */

   char *Afldptr;       /* pointers to memory gotten by FSbldinp      */
   char *Afld;          /* pointers to memory gotten by FSbldinp      */
   char *Aregfld;       /* pointers to memory gotten by FSbldinp      */

   bool req_begin;      /* TRUE after FSmore begins a request         */
   bool req_complete;   /* TRUE after FSmore finds request is complete*/
   bool backend;        /* TRUE if FSmore should get data from backend*/

   LOCATION ifloc;      /* inputfile location                         */
   char ifname[32];     /* inputfile file name                        */
   FILE *iffd;          /* inputfile file descriptor                  */
   i4    ifrc;          /* inputfile open return code                 */

   bool	firstscr;	/* TRUE if still on the FIRST (enter stmt) screen */
   bool editqry;	/* TRUE if user wants to edit the query on error  */
   bool print_time;	/* TRUE if time stamp is to be printed	      */
   i4	rows_added;	/* number of rows added to the 'in-memory buffer' */

   i4   (*nxrec)();     /* Address of 'next record' routine           */

   }  BCB;

typedef struct fstm_mtcb
{
	i4	(*mfputmu_proc)();	/* Routine ptr to FSputmenu() */
	VOID	(*mfdmpmsg_proc)();	/* Routine ptr to FDdmpmsg() */
	bool	(*mffsmore_proc)();	/* Routine ptr to FSmore() */
	VOID	(*mfdmpcur_proc)();	/* Routine ptr to FDdmpcur() */
	VOID	(*mffsdiag_proc)();	/* Routine ptr to FSdiag() */
} FSMTCB;

#ifndef MSDOS
#define BB_SIZE 0x10000
#else
#define BB_SIZE 0x1FFC0
#endif

#define HELPKEY 101
#define ENDKEY  102
#define TOPKEY  103
#define BOTKEY  104
#define SVEKEY  105
#define MENUKEY 106
#define PRTKEY  107

/* Function prototypes that depend on fstm data structures */

FUNC_EXTERN bool IIUFadd_AddRec(BCB *, char *, bool);


#endif /* FSTM_HDR_INCLUDED */
