/*
**Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: bsosi.h - common definitions for BS interface to OSI
**
** History:
**	14-Mar-92 (sweeney)
**	    Cloned from bstliio.h 
**	29-Mar-92 (sweeney)
**	    added a struct t_bind and i4 tsdu
**	    to allow non-use of t_getinfo.
**	9-Sep-92 (gautam)
**	    Isolated OSI related information only into this file.
**	    bstliosi.c now includes both this file and bstliio.h.
**	13-Sep-92 (gautam)
**	    Added in HP-UX specific information
**	18-Mar-94 (rcs)
**	    Bug #59572
**	    MAX_TSAP_SIZE increased to 1024, since we know this is
**	    required on some platforms (eg. ICL).
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**      29-Mar-99 (hweho01)
**          Added support for AXI 64-bit (ris_u64) platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-jul-2001 (stephenb)
**	    Add support fir i64_aix
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

#if defined(any_aix)
/*
** This is the structure of an OSIMF address buf on the RS/6000
** passed to bind()
*/
struct rs_addr_buf
  {
     short     tsel_len;
     unsigned  char      tsel[32];
     unsigned  char      nsel_len;
     unsigned  char      nsel[20];
  } ;

/* These are fixed on the RS/6000 */
#define  TSAP_SIZE       55     
#define  MAX_TSAP_SIZE   55

#endif /* aix */


#if defined(any_hpux)     /* HP */
/*
** On HP, t_bind() can operate just on a tsel with and the nsel is not 
** required.
** When t_connect(), however, the complete tsap is required.
** 
** There is no fixed structure  in HP unlike IBM.  The TSAP needs to
** be constructed as a series of concatenated bytes.
*/

#endif /* any_hpux */

/*
** this has to be big enough for all tsaps
** n.b there is no way to determine this
** and in principle it can be unbounded
** if it's too small we fail on t_bind	  
** this value is known to be large enough 
** for all ICL platforms and for the IBM RS6000
*/

#ifndef MAX_TSAP_SIZE
#define MAX_TSAP_SIZE 1024 
#endif

#define MAX_NSAP_SIZE 20
#define MAX_TSEL_LEN  32

#define LISTEN_Q        10


