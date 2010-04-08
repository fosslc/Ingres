/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**	Name: gwftrace.h, GWF trace descriptions
**	
**	Description:
**		This file describes the constants needed to set
**		and test trace flags in GWF
**
**		These trace flags are set by calls to gwf_trace. The
**		trace point number determines the type of trace operation.
**
**	History:
**		14-sep-1992 (robf)
**			Created .
**		17-nov-1992 (robf)
**			Added definition for GWF_DISP_PARAM_MAX for
**			maximum number of parameters passed to gwf_display()
**		11-oct-93 (swm)
**	    		Bug #56448
**			Made gwf_display() portable. The old gwf_display()
**			took an array of variable-sized arguments. It could
**			not be re-written as a varargs function as this
**			practice is outlawed in main line code.
**			The problem was solved by invoking a varargs function,
**			TRformat, directly.
**			The gwf_display() function has now been deleted, but
**			for flexibilty gwf_display has been #defined to
**			TRformat to accomodate future change.
**			All calls to gwf_display() changed to pass additional
**			arguments required by TRformat:
**			    gwf_display(gwf_scctalk,0,trbuf,sizeof(trbuf) - 1,
**			this emulates the behaviour of the old gwf_display()
**			except that the trbuf must be supplied.
**			Note that the size of the buffer passed to TRformat
**			has 1 subtracted from it to fool TRformat into NOT
**			discarding a trailing '\n'.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#define GWF_MACRO(v, b)         ((v)[(b) >> 5] & (1 << ((b) & (BITS_IN(i4) - 1))))
#define GWF_SET_MACRO(v,b)      ((v)[(b) >> 5] |= (1 << ((b) & (BITS_IN(i4)-1))))
#define GWF_CLR_MACRO(v,b)      ((v)[(b) >> 5] &= ~(1 << ((b) & (BITS_IN(i4)-1))))
#if 0
#define GWF_MAIN_MACRO(t)	GWF_MACRO(Gwf_facility->gwf_trace,t)
#define GWF_SXA_MACRO(t)	GWF_MACRO(Gwf_facility->gwf_trace,t+600)
#define GWF_MAIN_SESS_MACRO(sess,t)     GWF_MACRO(sess->gws_trace,t)
#define GWF_SXA_SESS_MACRO(sess,t)      GWF_MACRO(sess->gws_trace,t+600)
#define GWF_MAIN_SESS_MACRO(sess,t)     GWF_MACRO(Gwf_facility->gwf_trace,t)
#define GWF_SXA_SESS_MACRO(sess,t)      GWF_MACRO(Gwf_facility->gwf_trace,t+600)
#endif
#define	gwf_display	TRformat
/*
**	Global macros
*/
#define GWF_MAIN_MACRO(t)	GWF_MACRO(Gwf_facility->gwf_trace,t)
#define GWF_SXA_MACRO(t)	GWF_MACRO(Gwf_facility->gwf_trace,t+600)
/*
**	Session-level macros
*/
#define GWF_MAIN_SESS_MACRO(t)    gwf_chk_sess_trace(t)
#define GWF_SXA_SESS_MACRO(t)     gwf_chk_sess_trace(t+600)

/*
**	Maximum size of trace message 
*/
#define GWF_MAX_MSGLEN	 128	
/*
**	GWF_MAIN_MACRO - Main GWF routines
**	start at 0
**	T              Description
** ----------  ---------------------------------------------------------
**    10	Enable dumping of Gateway TCBs
**    11	Trace REGISTER statement
**    12	Trace REGISTER-INDEX statement
**    13	Trace REMOVE statement
*/

/*
**	GWF_SXA_MACRO - SXA (C2/Security Audit Gateway) macros
**	start at 600
**
**	T              Description
** ----------  ---------------------------------------------------------
**	1	Print info on REGISTER statement
**	2	Print info on record GET operation
**	3	Print info on table CLOSE operation
**	4	Print info on table OPEN operation
**	5	Print memory allocation in SXA 
**	6	Enable "dummy" SXF calls.
**	7	Print each SXF record as it is read on GET operations
**	8	Print gateway message id mappings
**	9	Print various data conversion info.
**     10	Trace session-level info.
**     25	Dump C2-SXA statistics to session.
**     50       Print bad/unknown record access/objects
*/
/*
**	Function declarations
*/
FUNC_EXTERN 	bool 	gwf_chk_sess_trace( i4 );
FUNC_EXTERN	i4	gwf_scctalk(
				PTR	    arg1,
				i4	    msg_length,
				char        *msg_buffer);
