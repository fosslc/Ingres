/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/
 
 
#include <compat.h>
#include <gl.h>
#include <gcatrace.h>
#include <er.h>
#include <st.h>
#include <ex.h>
#include <tm.h>
#include <pc.h>
#include <tr.h>

/*
**
**									    
**  History:
**	07-jun-89 (pasker)						    
**	    New module. Implements tracing support			    
**	07-jun-89 (pasker)						    
**	    Only print items if there are any.  Print starting at the proper
**	    place in the array...
**	15-jun-89 (pasker)
**	    Make the exit handler check the exit reason.  It only makes
**	    a dump file if the reason is even and its not CLI forced exit.
**	15-jun-89 (pasker)
**	    Only send the stuff to a file if the II_GCAxx_TRACE_FILE
**	    logical exists.
**	22-jun-89 (pasker)
**	    Added code which will print tracepoints as they're encountered.
**	    This is useful for non-time sensitive problems.
**	01-sep-89 (ham)
**	    Fix access violation in GCX3_print_trace.
**      18-Jan-90 (cmorris)
**          Initial SUN implementation:- ripped off from Pasker's VMS one!!
**      21-Feb-90 (cmorris)
**          Only output trace file for a specific list of exceptions.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**      10-nov-97 (loera01)
**	    In GCX_exit_handler(), removed conditional which examines
**          the exception argument, so that all exceptions are traced.
**          Added call to EXsys_report() to get a good log of the error
**	    in the error log--not just in tracing.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**          
*/
 
/*
** declare storage for tracepoints
*/
 
GLOBALDEF GCX_TE_BLOCK	gcx_te[GCX_TE_COUNT];
GLOBALDEF GCX_TP_BLOCK	gcx_tp[GCX_TP_COUNT];
GLOBALDEF i4           gcx_curtrace = 0;
GLOBALDEF i4           gcx_curentry = 1;
 
FUNC_EXTERN i4 GCX_exit_handler();
 
#define hdr1\
 "Trace taken @ %s by process %x ----------------------------------------\n",\
 timestr
#define hdr2 "TE# TP#    ptr   name               args\n"
#define line1 "%35s %s \n", head_buf, body_buf
 

/*									    */
/* routine which allocates a tracepoint.				    */
/*									    */
i4 GCX1_trace_alloc(name, fmtstr, gcx_curtrace_ptr, gcx_te_ptr)
char *name;
char *fmtstr;
i4  **gcx_curtrace_ptr;
GCX_TE_BLOCK **gcx_te_ptr;
{
    i4 tmp_entry;
 
    /*
    ** if we've run out of entries, then return the zeroth
    */
    if (gcx_curentry == GCX_TP_COUNT)
        tmp_entry = 0;
    else
    {
	tmp_entry = gcx_curentry;	/* save the current entry */
    	gcx_curentry++;		/* bump for next time around */
 
	/* stash away tracepoint info. */
        gcx_tp[tmp_entry].name = name;
	gcx_tp[tmp_entry].trdsp_str = fmtstr;
    }
 
    /* hand back addresses of trace entry info. */
    *gcx_curtrace_ptr = &gcx_curtrace; 
    *gcx_te_ptr = gcx_te;
    return (tmp_entry);
}

VOID
GCX_print_a_trace(_entry)
i4 _entry;
{
    char head_buf[256]; /* hope this is big enough */
    char body_buf[256]; /* hope this is big enough */
    int tn = gcx_te[_entry].trace_number;
 
    if (tn == 0)
	return;
    
    STprintf (head_buf, "%3d %3d %8x %s, ", _entry, tn,
		    gcx_te[_entry].ptr, gcx_tp[tn].name);
 
    /*
    ** if theres items, then format and print them out, else dont.
    */
    if (gcx_te[_entry].items > 0)
	STprintf (body_buf, gcx_tp[tn].trdsp_str,
		    gcx_te[_entry].item1,
		    gcx_te[_entry].item2,
		    gcx_te[_entry].item3,
		    gcx_te[_entry].item4,
		    gcx_te[_entry].item5);
 
    else
	body_buf[0] = 0;
 
    /* display the tracepoint */
    TRdisplay (line1);
}

GCX2_print_trace()
{
    i4 i;
    i4 curtrace;
    SYSTIME timeval;
    char timestr[256]; /* me too! */
    PID pid;
 
    /*
    ** get header info.
    */
    TMnow(&timeval);
    TMstr(&timeval, &timestr[0]);
    PCpid(&pid);
 
    /*
    ** store header information.
    */
 
    TRdisplay (hdr1);
    TRdisplay (hdr2);
 
    /*
    ** go through all the entries in the trace log and print them out
    ** using TRdisplay
    */
 
    curtrace = gcx_curtrace;	/* save this away */
 
    /*
    ** print out each entry in the right order.
    */
    
    for (i = curtrace;  i < GCX_TE_COUNT; i++) GCX_print_a_trace(i);
    for (i = 0;         i < curtrace;     i++) GCX_print_a_trace(i);
 
    return (OK);
}

i4
GCX_exit_handler(exception_args)
EX_ARGS *exception_args;
{
    i4	index;
    char errmsg[256];
    CL_ERR_DESC local_sys_err;
     
    /*
    ** add an entry which says why we exited...
    */
    if ( !EXsys_report( exception_args, errmsg ))
    {
        STprintf( errmsg, "GC exit; reason is Exception is %.4x (%d.)",
            exception_args->exarg_num, exception_args->exarg_num );
    }
    ERsend( ER_ERROR_MSG, errmsg, STlength(errmsg), &local_sys_err );
    GCX_TRACER_1_MACRO ("GCX_exit_handler>", NULL,
        "exit reason = %.4x",exception_args->exarg_num);
    GCX2_print_trace();
    return EX_RESIGNAL;
}
