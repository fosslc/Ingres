/*
** Copyright (c) 1983, 2005 Ingres Corporation
**
*/
#ifndef SILOCAL_H
#define SILOCAL_H
#include <rms.h>
/*
** File: silocal.h
**
** History:
**      24-jul-1998 (rigka01)
**          Cross integrate bug 86989 from 1.2 to 2.0 codeline
**          bug 86989: When passing a large CHAR parameter with report,
**          the generation of the report fails due to parm too big
**          Add SI_MAXOUT
**	27-jul-2005 (abbjo03)
**	    Add rec_length to FILEX to store the true logical record length.
**      30-Dec-2009 (horda03) Bug 123091/123092
**          Allow FILEX structures to be linked together for delaying a
**          sys$flush.
*/

# define	SI_BAD_SYNTAX	(E_CL_MASK | E_SI_MASK | 1)
# define	SI_CANT_OPEN	(E_CL_MASK | E_SI_MASK | 4)

#define SI_MAXOUT 4096

#define MAX_RMS_SEQ_RECLEN	32767

/*
** RACC files start out with a leading magic string and a 4 byte
** character size value.  The magic string assures that we created
** the file as a RACC file in the first place.
*/
#define RACCMAGIC "]?"
#define RACCMLEN 2			/* length of RACCMAGIC */
#define RACCLEADING (RACCMLEN+4)	/* leading bytes on RACC file */

typedef struct _FILEX FILEX;

typedef struct {

      FILEX *next;
      FILEX *prev;
   }
   FILEX_LINK;

struct _FILEX
{
    FILEX_LINK  link;          /* Chain together FILEX's waiting a flush
                               ** Must be first entry of structure.
                               */
    i4		rec_length;
    FABDEF	fx_fab;
    RABDEF	fx_rab;
    XABRDTDEF	xab_rdt;
    i4          flush_pending;
    i4          delay_flush;
    i4          busy_buf;      /* Buffer being written to, don't flush */
};

#endif
