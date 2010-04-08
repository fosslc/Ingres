/******************************************************************************
**
** Copyright (c) 1987, 2001 Ingres Corporation
**
** History:
**      12-Sep-95 (fanra01)
**          Added preprocessor statements to remove GLOBALDEF from __declspec
**          when referenced in a DLL.
**	19-mar-1996 (canor01)
**	    remove __declspec completely.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	10-dec-2001 (somsa01)
**	    The Intel compiler caught the fact that gcx_te was GLOBALREF'd
**	    in gcatrace.h, but not GLOBALDEF'd here.
**
******************************************************************************/

#include <compat.h>
#include <gcatrace.h>
#include <ex.h>
#include <tm.h>
#include <pc.h>
#include <st.h>
#include <tr.h>

/* declare storage for tracepoints */

GLOBALDEF   GCX_TE_BLOCK gcx_te[GCX_TE_COUNT];
FACILITYDEF GCX_TP_BLOCK gcx_tp[GCX_TP_COUNT];
FACILITYDEF i4           gcx_curtrace = 0;
FACILITYDEF i4           gcx_curentry = 1;

FUNC_EXTERN i4  GCX_exit_handler();

#define hdr1\
 "Trace taken @ %s by process %x ----------------------------------------\n",\
 timestr
#define hdr2 "TE# TP#    ptr   name               args\n"
#define line1 "%35s %s \n", head_buf, body_buf

/******************************************************************************
** routine which allocates a tracepoint.
******************************************************************************/
i4  
GCX1_trace_alloc(name, fmtstr, gcx_curtrace_ptr, gcx_te_ptr)
	char           *name;
	char           *fmtstr;
	i4            **gcx_curtrace_ptr;
	GCX_TE_BLOCK  **gcx_te_ptr;
{
	i4              tmp_entry;

	/*
	 * if we've run out of entries, then return the zeroth
	 */
	if (gcx_curentry == GCX_TP_COUNT)
		tmp_entry = 0;
	else {
		tmp_entry = gcx_curentry;	/* save the current entry */
		gcx_curentry++;	/* bump for next time around */

		/* stash away tracepoint info. */
		gcx_tp[tmp_entry].name = name;
		gcx_tp[tmp_entry].trdsp_str = fmtstr;
	}

	/* hand back addresses of trace entry info. */
	*gcx_curtrace_ptr = &gcx_curtrace;
	*gcx_te_ptr = gcx_te;
	return (tmp_entry);
}


/******************************************************************************
**
******************************************************************************/
VOID
GCX_print_a_trace(_entry)
	i4              _entry;
{
	char            head_buf[256];	/* hope this is big enough */
	char            body_buf[256];	/* hope this is big enough */
	int             tn = gcx_te[_entry].trace_number;

	if (tn == 0)
		return;

	STprintf(head_buf, "%3d %3d %8x %s, ", _entry, tn,
		 gcx_te[_entry].ptr, gcx_tp[tn].name);

	/*
	 * if there are items, then format and print them out, else dont.
	 */

	if (gcx_te[_entry].items > 0)
		STprintf(body_buf, gcx_tp[tn].trdsp_str,
			 gcx_te[_entry].item1,
			 gcx_te[_entry].item2,
			 gcx_te[_entry].item3,
			 gcx_te[_entry].item4,
			 gcx_te[_entry].item5);

	else
		body_buf[0] = 0;

	/*
	 * display the tracepoint
	 */
	TRdisplay(line1);
}


/******************************************************************************
**
******************************************************************************/
GCX2_print_trace()
{
	i4              i;
	i4              curtrace;
	SYSTIME         timeval;
	char            timestr[256];	/* me too! */
	PID             pid;

	/*
	 * get header info.
	 */
	TMnow(&timeval);
	TMstr(&timeval, &timestr[0]);
	PCpid(&pid);

	/*
	 * store header information.
	 */

	TRdisplay(hdr1);
	TRdisplay(hdr2);

	/*
	 * go through all the entries in the trace log and print them out *
	 * using TRdisplay
	 */

	curtrace = gcx_curtrace;/* save this away */

	/*
	 * print out each entry in the right order.
	 */

	for (i = curtrace; i < GCX_TE_COUNT; i++)
		GCX_print_a_trace(i);
	for (i = 0; i < curtrace; i++)
		GCX_print_a_trace(i);

	return (OK);
}


/******************************************************************************
**
******************************************************************************/
i4
GCX_exit_handler(exception_args)
	EX_ARGS        *exception_args;
{
	i4              index;

	/*
	 * See whether exception is one we bother about...
	 */
	if (index = EXmatch(exception_args->exarg_num, 4,
			    (EX) EXSEGVIO,
			    (EX) EXBUSERR,
			    (EX) EXINTDIV,
			    (EX) EXINTOVF)) {
		/*
		 * add an entry which says why we exited...
		 */

		GCX_TRACER_1_MACRO("GCX_exit_handler>", NULL,
			   "exit reason = %.4x", (i4)exception_args->exarg_num);
		GCX2_print_trace();
	}
	return EX_RESIGNAL;
}
