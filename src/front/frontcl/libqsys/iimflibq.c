/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <iisqlda.h>

#if defined(UNIX) || defined(NT_GENERIC)

/**
**  Name: iimflibq.c - Provide interface to MF COBOL processor.
**
**  Description:
**	The routines defined in this file provide the interface to the MF
**	COBOL processor to LIBQ and some small utility functions for 
**	string and numeric data processing.
**
**  Defines:
**      IIMFlibq		- Load up all LIBQ calls for MF COBOL.
**
**  Function Interfaces (see notes):
**	IIXerrtest
**	IIXnextget
**	IIXnexec
**	IIXcsRetrieve
**	IIXcsRetScroll
**      IIXsqInit
**      IIXcsGetio
**      IIXcsDaGet
**      IIXcsERetrieve
**      IIXgetdomio
**      IIXsqFlush
**      IIXflush
**
**  Long-name Interfaces (see notes):
**	IIXLQprsProcSt[atus    ]
**	IILQpriProcIni[t       ]
**	IILQprvProcVal[io      ]
**	IILQshSetHandl[er      ]
**
**  Notes:
**	0. Files in MF COBOL run-time layer:
**		iimfdata.c	- Data processing interface
**		iimflibq.c	- Interface to LIBQ
**		iimffrs.c	- Interface to FRS (runsys)
**
**	1. When adding new entries:
**		In LIBQ make sure to update iimflibq.c/IIMFlibq.
**		In FRS make sure to update iimffrs.c/IIMFfrs.
**
**	2. If the new entry IS NOT a function and the name is less than 14
**	   characters then just enter it into the main "loading" routine.
**
**	3. If the new entry IS a function create an interface entry in the
**	   appropriatre file and follow II with 'X' and do not enter it into
**	   the main "loading" routine.  The code generator automatically
**	   inserts an 'X' if a function for MF.  See next point if this is
**	   a function with name length > 14.
**
**	4. If the new entry has a name longer than 14 characters then
**	   you must create a special entry for it (truncated to 14 characters)
**	   that calls the real entry.  If this is a function, leave the 'X'
**	   but still truncate.  The number 14 is what is the current maxmimum
**	   on Sun.
**	   *  If your system allows names longer than 14 leave the code
**	      generator (gen__II which truncates names to 14) alone and this
**	      interface needs not be changed.
**	   *  If your system needs names shorter than 14 then do the following:
**	      1. Modify the maximum length in the code generator and truncate
**		 to this shorter value (if less than 7) we're in trouble.
**	      2. Add to this file all the newly truncated names in the
**		 long-name section.
**	      3. Leave the old truncated-to-14 names alone so that users on
**		 existing running systems need not re-preprocess.
**
**	5. Parameter passing mechanisms are available to pass BY VALUE and
**	   BY REFERENCE.  Most work is done in the code generator.  Some 
**	   work is required for decimal conversions and, in some cases,
**	   string conversions too.  See the file iimfdata.c for the
**	   few processing routines.
**		
** 	6. MF COBOL does not support the GIVING clause.  For all function
**	   calls, the layer sets ret_val to the return function value.
**	   Ret_val (IIRES) is passed back as a parameter.
**
**  History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
**	20-dec-89 (barbara)
**	   Updated for Phoenix/Alerters.
**	09-feb-90 (kwatts, ken)
**	   Added additional routines to support run-time if for unstructured
**   	   code generation. New routines are only called if the program is
**          preprocessed with the -u (unstructured) flag.
**         	IIXsqInit
**         	IIXcsGetio
**         	IIXcsDaGet
**         	IIXcsERetrieve
**         	IIXgetdomio
**         	IIXsqFlush
**         	IIXflush
**         plus amendments to the following, which are called whether or not
**         -u is used:
**	   	IIXnextget
**	   	IIXcsRetrieve
**	   Amendments do not change logic when proper compiler IF... END-IF is
**         being used because the two calls that remain set, but never test,
**         the run-time IF booleans.
**	5-july-1990 (gurdeep rahi)
**	    Handle variable number argument passing from COBOL to C.  Write
**	    wrapper routines for the following -:
**	    IIsqConnect() - Taken out from IIMFlibq() for MF_COBOL
**	    IIingopen()   - Taken out from IIMFlibq()
**	    The COBOL pre-precessor code generator (see cobgen.c) now
**	    generates call to the wrapper routines with extra argument,
**	    ie.
**	    IIXsqConnect() instead of IIsqConnect() etc.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	04-oct-95 (sarjo01)
**		Added NT_GENERIC to ifdef.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-oct-2002 (toumi01)
**	    add long name IIXLQprsProcStatus to resolve link of MF COBOL
**	    with names no longer truncated to 14 characters
**	19-nov-2002 (toumi01)
**	    add resultCount parameter to IIXLQprsProcSt[atus] to match
**	    the new 2.6 version of IILQprsProcStatus
*/

/* Booleans to control run-time IF.
** When generating unstructured COBOL these booleans ensure that calls of 
** routines which, in a structured object, would not be made, act as
** null when the IF is flattened.
**
** next_if is TRUE if the procedures in the IF would be executed. It itself
**         is set TRUE by IIXsqInit, IIxsqFlush and IIXflush, set FALSE by
**         a failing call of IIXnextget and controls entry to 
**         IIgetdomio and IIcsDaGet, only calling-on if TRUE.
**
** csr_if  is TRUE if the procedures in the IF would be executed. It itself
**         is set TRUE by IIXsqInit and IIXcsERetrieve, set false by a 
**         failing call on IIXcsRetrieve and controls entry to
**         IIcsGetio, IIcsERetrieve, and IIcsDaGet, only calling on
**         if TRUE.
*/

static bool csr_if = TRUE;
static bool next_if = TRUE;


/* Dummy SQLCA typedef to pull in the extern SQLCA */
typedef struct _sqlca_dummy {
	char	sqlca_buffer[200];
} SQLCA_DUMMY;

extern	SQLCA_DUMMY	sqlca;	/* Bogus external for the real sqlca */

/*{
** Name: IIMFlibq 	- Cause loader to see all of LIBQ entry points.
**
** Description:
**	This routine must never actually be called.  All the external LIBQ
**	routines are "pretended to be called" by this routine so that the
**	MF COBOL interpreter/loader will pull them in.  This routine can be
**	considered as a load vector.
**
**	An equivalent file exists for the FRS calls.
**
** Notes:
**	1. When a new routine is added to LIBQ make sure to enter a reference
**	   to that new call from this routine.  Confirm that the external
**	   reference is in the correct case.  For example:
**		IIcsERplace
**	2. If adding a function do not enter it here, but make a new function
**	   entry procedure later on (beginning with IIX).
**	3. If the name is too long (see above) then this requires a separate
**	   interface too.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
** 	16-nov-89 (neil)
**	   Written for MF COBOL interface.
**	15-dec-89 (neil)
**	   Added IIsqTPC for 6.3 path (already in 6.4 path).
**      26-feb-1991 (kathryn) Added:
**              IILQssSetSqlio, IILQisInqSqlio and IILQshSetHandler
**	25-apr-1991 (teresal)
**	   Remove IILQegEvGetio.	
**	08-mar-2008 (toumi01) BUG 119457
**	   Add IIXcsRetScroll MF interface.
*/

VOID
IIMFlibq()
{
    static	i4	never_ever_call = 0;

    if (never_ever_call == 0 || never_ever_call != 0)
    {
	IIbreak();
	IIexit();
	IIeqiqio();
	IIeqstio();
	IIretinit();
	IIputdomio();
	IIsexec();
	IIsyncup();
	IIutsys();
	IIwritio();
	IIcsClose();
	IIcsDelete();
	IIcsERplace();
	IIcsReplace();
	IIcsOpen();
	IIcsQuery();
	IIexDefine();
	IIexExec();
	IIputctrl();
	IIxact();
	IIvarstrio();
	IIcsRdO();
	IIseterr();
	IIsqDisconnect();
	IIsqStop();
	IIsqPrint();
	IIsqUser();
	IIsqExImmed();
	IIsqExStmt();
	IIsqDaIn();
	IIsqPrepare();
	IIsqDescribe();
	IIsqMods();
	IIsqParms();
	IILQsidSessID();
	IIsqTPC();
	IILQesEvStat();
	IILQssSetSqlio();
	IILQisInqSqlio();
    } /* If never ever */
    never_ever_call = 0;
} /* IIMFlibq */

/*
** Function Interfaces
*/
VOID
IIXerrtest(ret_val)		/* Map ret_val by reference */
i4	*ret_val;
{
    *ret_val = IIerrtest();
}

VOID
IIXnexec(ret_val)		/* Map ret_val by reference */
i4	*ret_val;
{
    *ret_val = IInexec();
}

VOID
IIXnextget(ret_val)		/* Map ret_val by reference and also 	*/
i4	*ret_val;		/* set next_if FALSE before returning 	*/
				/* a value of false (0).		*/
{
    next_if = ( (*ret_val = IInextget()) != 0);
}

VOID
IIXcsRetrieve(ret_val,  cursor_name, cid1, cid2)
i4	*ret_val;		/* Map ret_val by reference and also 	*/
char	*cursor_name;		/* set csr_if FALSE before returning 	*/
i4	cid1, cid2;		/* a value of false (0).		*/
{
    csr_if = ( (*ret_val = IIcsRetrieve(cursor_name, cid1, cid2)) != 0);
}

VOID
IIXcsRetScroll(ret_val,  cursor_name, cid1, cid2, fetcho, fetchn)
i4	*ret_val;		/* Map ret_val by reference and also 	*/
char	*cursor_name;		/* set csr_if FALSE before returning 	*/
i4	cid1, cid2;		/* a value of false (0).		*/
i4	fetcho, fetchn;
{
    csr_if = ( (*ret_val = IIcsRetScroll(cursor_name, cid1, cid2, fetcho, fetchn)) != 0);
}

VOID
IIXsqFlush(filename, linenum)		/* Set next_if TRUE after call	*/
char	*filename;
i4	linenum;
{
	IIsqFlush(filename, linenum);	
	next_if = TRUE;
}

VOID
IIXcsGetio( indaddr, isvar, type, length, address )
i2	*indaddr;			/* Only call if csr_if is TRUE	*/
i4	isvar, type, length;
char	*address;
{
	if (csr_if)
	    IIcsGetio( indaddr, isvar, type, length, address );
}

VOID
IIXcsERetrieve()			/* Only call if csr_if is TRUE	*/
{					/* then set csr_if to TRUE	*/
	if (csr_if)
	    IIcsERetrieve();
	csr_if = TRUE;
}

i4
IIXgetdomio(indaddr, isvar, hosttype, hostlen, addr)
i2		*indaddr;		/* Only call if next_if is TRUE	*/
i4		isvar;
i4		hosttype;
i4		hostlen;
char		*addr;
{
	if (next_if)
	    IIgetdomio(indaddr, isvar, hosttype, hostlen, addr);
}

VOID
IIXsqInit(sqc)				/* Set next_if and csr_if TRUE 	*/
register i4		*sqc;		/* after call.			*/
{
	IIsqInit(sqc);
	next_if = TRUE;
	csr_if = TRUE;
}

VOID
IIXflush(file_nm, line_no)		/* Set next_if TRUE after call	*/
char	*file_nm;
i4	line_no;
{
	IIflush(file_nm, line_no);
	next_if = TRUE;
}

VOID
IIXcsDaGet(lang, sqd)			/* Only call if next_if and 	*/
i4	lang;				/* csr_if are both TRUE		*/
i4	*sqd;
{
	if (next_if && csr_if)
	    IIcsDaGet(lang, sqd);
}

VOID
IIXLQprsProcStatus(ret_val, resultCount)
i4	*ret_val;
i4	resultCount;
{
    *ret_val = IILQprsProcStatus(resultCount);
}

/*
** Long-name Interfaces
12345678901234|
*/
VOID
IIXLQprsProcSt(ret_val, resultCount)
i4	*ret_val;
i4	resultCount;
{
    *ret_val = IILQprsProcStatus(resultCount);
}

VOID
IILQpriProcIni(type, name)
i4	type;
char	*name;
{
    IILQpriProcInit(type, name);
}

VOID
IILQprvProcVal(pname, pref, ind, isref, type, len, addr)
char	*pname;
i4	pref;
i2	*ind;
i4	isref, type, len;
char	*addr;
{
    IILQprvProcValio(pname, pref, ind, isref, type, len, addr);
}
VOID
IILQshSetHandl(hdlr,funcptr)
i4	hdlr;	
i4	(*funcptr)();
{
    IILQshSetHandler(hdlr,funcptr);
}

/*
 * Function Interfaces for MF-COBOL wrapper routines
 */
VOID
IIXsqConnect(count,lan,a1,a2,a3,a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)i4  count, lan;
char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9, *a10, *a11, *a12, *a13, *a14;
{
    char        *v_args[14];
    i4          i;

    for (i = 0; i < 14; i++)            /* Initialize varying arg array */
    v_args[i] = (char *)0;
      
    /* Portable fall-through switch to pick up correct # of args */
    switch(count)
    {
      case 14:  v_args[13] = a14;
      case 13:  v_args[12] = a13;
      case 12:  v_args[11] = a12;
      case 11:  v_args[10] = a11;
      case 10:  v_args[ 9] = a10;
      case  9:  v_args[ 8] = a9;
      case  8:  v_args[ 7] = a8;
      case  7:  v_args[ 6] = a7;
      case  6:  v_args[ 5] = a6;
      case  5:  v_args[ 4] = a5;
      case  4:  v_args[ 3] = a4;
      case  3:  v_args[ 2] = a3;
      case  2:  v_args[ 1] = a2;
      case  1:  v_args[ 0] = a1;

    }
       
    IIsqConnect(lan, v_args[0], v_args[1], v_args[2], v_args[3],
	  v_args[4], v_args[5], v_args[6], v_args[7], v_args[8], v_args[9],               v_args[10], v_args[11], v_args[12], v_args[13]);
}

VOID
IIXingopen(count,lan,a1,a2,a3,a4, a5, a6, a7, a8, a9, a10, a11, a12,
	   a13, a14, a15)
i4  count, lan;
char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9, *a10, *a11,
    *a12, *a13, *a14, *a15;
{
    char        *v_args[15];
    i4          i;
     
    for (i = 0; i < 15; i++)            /* Initialize varying arg array */
    v_args[i] = (char *)0;
  
    /* Portable fall-through switch to pick up correct # of args */
    switch(count)

    {
      case 15:  v_args[14] = a15;
      case 14:  v_args[13] = a14;
      case 13:  v_args[12] = a13;
      case 12:  v_args[11] = a12;
      case 11:  v_args[10] = a11;
      case 10:  v_args[ 9] = a10;
      case  9:  v_args[ 8] = a9;
      case  8:  v_args[ 7] = a8;
      case  7:  v_args[ 6] = a7;
      case  6:  v_args[ 5] = a6;
      case  5:  v_args[ 4] = a5;
      case  4:  v_args[ 3] = a4;
      case  3:  v_args[ 2] = a3;
      case  2:  v_args[ 1] = a2;
      case  1:  v_args[ 0] = a1;
    }
       
    IIingopen(lan, v_args[0], v_args[1], v_args[2], v_args[3],
	  v_args[4], v_args[5], v_args[6], v_args[7], v_args[8], v_args[9],               v_args[10], v_args[11], v_args[12], v_args[13], v_args[14]);
}
			   
#endif /* UNIX */
