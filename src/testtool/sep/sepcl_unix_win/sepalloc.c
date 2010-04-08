/*
**  include files
*/
#include <compat.h>
#include <cs.h>
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
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**	 7-may-1993 (donj)
**	    Change args in SEP_MEalloc to u_i4 to match MEreqmem().
**	19-may-1993 (donj)
**	    Add tracing of CL ME tables.
**	20-may-1993 (donj)
**	    Added some extra tracing statements to look for alignment bug.
**	21-may-1993 (donj)
**	    Refined tracing.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**	16-aug-1993 (donj)
**	    Take out most tracing, found memory bug.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**       9-feb-1994 (donj)
**	    ifdeff'd out the tracing statements. We might need them at a latter
**	    time.
**      05-Feb-96 (fanra01)
**          Added conditional for using semaphore when linking with DLLs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

#if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF    CS_SEMAPHORE  ME_stream_sem;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF    CS_SEMAPHORE  ME_stream_sem;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */
char
*SEP_MEalloc(u_i4 tag,u_i4 size,bool clear,STATUS *ME_status)
{
    STATUS        MEsts ;
    char         *p = NULL ;
#if 0
    if (tracing&(TRACE_ME_TAG|TRACE_ME_HASH))
    {
	SIfprintf(traceptr,"SEP_ME> Inside of SEP_MEalloc()\n");
	SIfprintf(traceptr,"SEP_ME> tag=");
	SEP_MEprint_tagname(tag);
	SIfprintf(traceptr,"\nSEP_ME> size= %d clear= %d MEstatus= %d\n"
                          ,size,clear,ME_status);
	SIfprintf(traceptr,"SEP_ME>\n");
    }
#endif
    /*
    ** This next paragraph is modelled after STtalloc().
    */

    gen_Psem( &ME_stream_sem );
    p = (char *)MEreqmem(tag, (size+8), clear, &MEsts);
    gen_Vsem( &ME_stream_sem );

    if (ME_status != NULL)
	*ME_status = MEsts;
#if 0
    if (tracing&(TRACE_ME_TAG|TRACE_ME_HASH))
    {
        SIfprintf(traceptr,"SEP_ME> Leaving SEP_MEalloc()\n");
        SIfprintf(traceptr,"SEP_ME> return pointer = %d\n",p );
	SIfprintf(traceptr,"SEP_ME> MEreqmem status = %d\n",MEsts);
        SIfprintf(traceptr,"SEP_ME>\n");
	SIflush(traceptr);
    }
#endif
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
