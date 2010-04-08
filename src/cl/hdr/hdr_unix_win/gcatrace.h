/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: GCATRACE.H 
**
** Description:
**	This file contains datastructures and macros which are used by CL and
**	mainline code to store trace information in GCA. Since GCA is linked in
**	to most pieces of INGRES (dbms_server, com_server, front-ends,
**	gateways, star, nameserver, etc) its a nice and handy way to store a
**	common communications trace.
**
**	The reason this module sits in CLHDR is because of following dependency
**	shows that the only point in common between GCF and CLF is CLHDR.
**	
**		    GCF can depend on GCF .H files
**		    GCF can depend on COMMONHDR .H files
**		    GCF can depend on CLHDR .H files
**		    
**		    CLF can depend on CL .H files
**		    CLF can depend on CLHDR .H files
**
** History: $Log-for RCS$
**      07-jun-89 (pasker)
**	    Initial module creation.
**	13-jun-89 (pasker)
**	    Sun CL version which expands tracing stuff to nothing.
**	22-jun-89 (pasker)
**	    Added code which will print tracepoints as they're encountered.
**	    This is useful for non-time sensitive problems.
**	10-Oct-89 (seiwald)
**	    Merged UNIX and VMS versions.
**      19-Jan-90 (cmorris)
**          Enabled non-null version always.
**	25-May-90 (ham)
**	    Change the code back to indexining into the array
**	    rather than using pointers.  The pointers get trashed
**	    if there is a collision when we get an AST delivered.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT and
**          using data from a DLL in code built for static libraries.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#define	GCX_TP_COUNT 500
#define	GCX_TE_COUNT 500

typedef struct _GCX_TP_BLOCK
    {
    char	*name;
    char	*trdsp_str;
    } GCX_TP_BLOCK;

typedef struct _GCX_TE_BLOCK
    {
    i2		trace_number;
    PTR		ptr;
    i2		items;
    i4		item1;
    i4		item2;
    i4		item3;
    i4		item4;
    i4		item5;
    } GCX_TE_BLOCK;


#define GCX_PRINT_TRACE_MACRO()\
    GCX2_print_trace()
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF GCX_TE_BLOCK	gcx_te[];
GLOBALDLLREF i4		gcx_curtrace;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF GCX_TE_BLOCK	gcx_te[];
GLOBALREF i4		gcx_curtrace;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

#ifdef xDEBUG
#define GCX_TRACER_MACRO(xname, fmtstr, xptr, xitems, xitem1, xitem2, xitem3, xitem4, xitem5)\
    {\
    static i4  trace_number = -1;\
    static i4  gcx_curtrace_ptr;\
    static GCX_TE_BLOCK *gcx_te_ptr;\
    if (trace_number == -1)\
	trace_number = GCX1_trace_alloc(xname,\
					fmtstr,\
					&gcx_curtrace_ptr,\
					&gcx_te_ptr);\
    gcx_curtrace_ptr = gcx_curtrace;\
    gcx_te[gcx_curtrace_ptr].trace_number = trace_number;\
    gcx_te[gcx_curtrace_ptr].items = xitems;\
    gcx_te[gcx_curtrace_ptr].item1 = xitem1;\
    gcx_te[gcx_curtrace_ptr].item2 = xitem2;\
    gcx_te[gcx_curtrace_ptr].item3 = xitem3;\
    gcx_te[gcx_curtrace_ptr].item4 = xitem4;\
    gcx_te[gcx_curtrace_ptr].item5 = xitem5;\
    gcx_te[gcx_curtrace_ptr].ptr   = (PTR) xptr;\
    (gcx_curtrace)++;\
    if (gcx_curtrace >= GCX_TE_COUNT) gcx_curtrace = 0;\
    }
#else
#define GCX_TRACER_MACRO(xname, fmtstr, xptr, xitems, xitem1, xitem2, xitem3, xitem4, xitem5)
#endif /* xDEBUG */

#define GCX_TRACER_0_MACRO(name, ptr)\
    GCX_TRACER_MACRO (name,NULL,ptr,0, 0,0,0,0,0); 

#define GCX_TRACER_1_MACRO(name, ptr, fmtstr, item1)\
    GCX_TRACER_MACRO (name,fmtstr,ptr,1, item1,0,0,0,0); 

#define GCX_TRACER_2_MACRO(name, ptr, fmtstr, item1,item2)\
    GCX_TRACER_MACRO (name,fmtstr,ptr,2, item1,item2,0,0,0);
 
#define GCX_TRACER_3_MACRO(name, ptr, fmtstr, item1,item2,item3)\
    GCX_TRACER_MACRO (name,fmtstr,ptr,3, item1,item2,item3,0,0);

#define GCX_TRACER_4_MACRO(name, ptr, fmtstr, item1,item2,item3,item4)\
    GCX_TRACER_MACRO (name,fmtstr,ptr,4, item1,item2,item3,item4,0); 

#define GCX_TRACER_5_MACRO(name, ptr, fmtstr, item1,item2,item3,item4,item5)\
    GCX_TRACER_MACRO (name,fmtstr,ptr,5, item1,item2,item3,item4,item5);
