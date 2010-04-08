/*
**  include files
*/

#include <compat.h>
#include <lo.h>
#include <me.h>
#include <si.h>

#define sepalloc_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	16-jul-1992 (donj)
**	    Created.
**       2-jun-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	    Added some extra tracing statements to look for alignment bug.
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      17-aug-1993 (donj)
**	    Took out most tracing, found the memory bug.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
*/


GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

char
*SEP_MEalloc( u_i4 tag, u_i4 size, i4 clear, STATUS *ME_status )
{
    STATUS        MEsts ;
    char         *p;

    if (tracing&(TRACE_ME_TAG|TRACE_ME_HASH))
    {
	SIfprintf(traceptr,"SEP_ME> Inside of SEP_MEalloc()\n");
	SIfprintf(traceptr,"SEP_ME>tag=");
	SEP_MEprint_tagname(tag);
	SIfprintf(traceptr," size= %d clear= %d MEstatus= %d\n"
                          ,size,clear,ME_status);
	SIfprintf(traceptr,"SEP_ME>\n");
    }

    p = MEreqmem(tag, size, clear, &MEsts);
    if (ME_status != NULL)
	*ME_status = MEsts;

    if (tracing&(TRACE_ME_TAG|TRACE_ME_HASH))
    {
        SIfprintf(traceptr,"SEP_ME> Leaving SEP_MEalloc()\n");
        SIfprintf(traceptr,"SEP_ME> return pointer = %d\n",p );
        SIfprintf(traceptr,"SEP_ME> MEreqmem status= %d\n",MEsts);
        SIfprintf(traceptr,"SEP_ME>\n");
    }
    return (p);
}

VOID
SEP_MEprint_tagname(i2 tag)
{
    switch (tag)
    {
       case SEP_ME_TAG_EVAL:     SIfprintf(traceptr,"SEP_ME_TAG_EVAL");     break;
       case SEP_ME_TAG_NODEL:    SIfprintf(traceptr,"SEP_ME_TAG_NODEL");    break;
       case SEP_ME_TAG_CANONS:   SIfprintf(traceptr,"SEP_ME_TAG_CANONS");   break;
       case SEP_ME_TAG_PAGES:    SIfprintf(traceptr,"SEP_ME_TAG_PAGES");    break;
       case SEP_ME_TAG_PARAM:    SIfprintf(traceptr,"SEP_ME_TAG_PARAM");    break;
       case SEP_ME_TAG_SEPFILES: SIfprintf(traceptr,"SEP_ME_TAG_SEPFILES"); break;
       case SEP_ME_TAG_GETLOC:   SIfprintf(traceptr,"SEP_ME_TAG_GETLOC");   break;
       case SEP_ME_TAG_TOKEN:    SIfprintf(traceptr,"SEP_ME_TAG_TOKEN");    break;
       case T_BLOCK_TAG:         SIfprintf(traceptr,"T_BLOCK_TAG");         break;
       case T_BUNCH_TAG:         SIfprintf(traceptr,"T_BUNCH_TAG");         break;
       case E_BLOCK_TAG:         SIfprintf(traceptr,"E_BLOCK_TAG");         break;
       case E_BUNCH_TAG:         SIfprintf(traceptr,"E_BUNCH_TAG");         break;
       case SEP_ME_TAG_ENVCHAIN: SIfprintf(traceptr,"SEP_ME_TAG_ENVCHAIN"); break;
       case SEP_ME_TAG_SEPLO:    SIfprintf(traceptr,"SEP_ME_TAG_SEPLO");    break;
       case SEP_ME_TAG_SED:      SIfprintf(traceptr,"SEP_ME_TAG_SED");      break;
       case SEP_ME_TAG_FILL:     SIfprintf(traceptr,"SEP_ME_TAG_FILL");     break;
       case SEP_ME_TAG_CMMDS:    SIfprintf(traceptr,"SEP_ME_TAG_CMMDS");    break;
       default:                  SIfprintf(traceptr,"***(%d)***",tag);      break;
    }
}
