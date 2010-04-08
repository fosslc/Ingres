/******************************************************************************
**
** Copyright (c) 1998 Ingres Corporation
**
******************************************************************************/

/****************************************************************************
**
** Name: classsamp.h - The monitor sampler thread's managed object (MO) classes.
**
** Description:
**      This file contains the managed objects used exclusively within the 
** 	CSsampler, the monitor sampler thread.
**
** History:
**	13-Nov-1998 (wonst02)
**	    Original.
**
******************************************************************************/

GLOBALDEF char CSsamp_index_name[] = "exp.clf.nt.cs.samp_index";

GLOBALDEF MO_CLASS_DEF CS_samp_classes[] =
{
    /* this is really attached, and uses the default index method */

    { MO_INDEX_CLASSID|MO_CDATA_INDEX, CSsamp_index_name,
	0, MO_READ, 0,
	0, MOstrget, MOnoset, 0, MOname_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numsamples", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numsamples),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numsamples),
        MOuintget, MOnoset, 0, MOidata_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numtlssamples", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numtlssamples),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numtlssamples),
        MOuintget, MOnoset, 0, MOidata_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numtlsslots", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numtlsslots),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numtlsslots),
        MOuintget, MOnoset, 0, MOidata_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numtlsprobes", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numtlsprobes),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numtlsprobes),
        MOuintget, MOnoset, 0, MOidata_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numtlsreads", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numtlsreads),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numtlsreads),
        MOintget, MOnoset, 0, MOidata_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numtlsdirty", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numtlsdirty),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numtlsdirty),
        MOintget, MOnoset, 0, MOidata_index },

    { MO_CDATA_INDEX, "exp.clf.nt.cs.samp_numtlswrites", 
     	MO_SIZEOF_MEMBER(CSSAMPLERBLK,numtlswrites),
        MO_READ, CSsamp_index_name, 
	CL_OFFSETOF(CSSAMPLERBLK,numtlswrites),
        MOintget, MOnoset, 0, MOidata_index },

    { 0 }
};

/*
** Sampler Thread information
** 
** 	One object per thread 
*/

GLOBALDEF char thread_index[] = "exp.clf.nt.cs.thread_index";

GLOBALDEF MO_CLASS_DEF CS_samp_threads[] =
{
    { 0, "exp.clf.nt.cs.thread_numsamples", 
     	MO_SIZEOF_MEMBER(THREADS,numthreadsamples), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,numthreadsamples),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },
				   	
/* Thread states[0..7]: FREE, COMPUTABLE, EVENT_WAIT, MUTEX,
** STACK_WAIT, UWAIT, CNDWAIT, <invalid> */
    { 0, "exp.clf.nt.cs.thread_stateFREE", 
     	MO_SIZEOF_MEMBER(THREADS,state[0]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[0]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_stateCOMPUTABLE", 
     	MO_SIZEOF_MEMBER(THREADS,state[1]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[1]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_stateEVENT_WAIT", 
     	MO_SIZEOF_MEMBER(THREADS,state[2]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[2]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_stateMUTEX", 
     	MO_SIZEOF_MEMBER(THREADS,state[3]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[3]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_stateSTACK_WAIT", 
     	MO_SIZEOF_MEMBER(THREADS,state[4]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[4]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_stateUWAIT", 
     	MO_SIZEOF_MEMBER(THREADS,state[5]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[5]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_stateCNDWAIT", 
     	MO_SIZEOF_MEMBER(THREADS,state[6]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,state[6]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

/* Compute facilities [0..17] <None>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,
**			         RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF,SXF */
    { 0, "exp.clf.nt.cs.thread_facilityCLF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[1]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[1]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityADF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[2]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[2]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityDMF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[3]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[3]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityOPF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[4]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[4]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityPSF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[5]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[5]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityQEF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[6]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[6]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityQSF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[7]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[7]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityRDF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[8]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[8]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilitySCF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[9]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[9]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityULF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[10]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[10]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityDUF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[11]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[11]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityGCF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[12]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[12]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityRQF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[13]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[13]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityTPF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[14]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[14]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilityGWF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[15]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[15]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_facilitySXF", 
     	MO_SIZEOF_MEMBER(THREADS,facility[16]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,facility[16]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

/* Event types[0..11]:	CS_DIO_MASK, CS_LIO_MASK, CS_BIO_MASK,
**			CS_LOG_MASK, CS_LOCK_MASK, CS_LGEVENT_MASK,
**			CS_LKEVENT_MASK, <unknown> */

    { 0, "exp.clf.nt.cs.thread_eventDISKIO", 
     	MO_SIZEOF_MEMBER(THREADS,event[0]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[0]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_eventLOGIO", 
     	MO_SIZEOF_MEMBER(THREADS,event[2]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[2]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_eventMESSAGEIO", 
     	MO_SIZEOF_MEMBER(THREADS,event[4]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[4]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_eventLOG", 
     	MO_SIZEOF_MEMBER(THREADS,event[6]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[6]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_eventLOCK", 
     	MO_SIZEOF_MEMBER(THREADS,event[7]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[7]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_eventLOGEVENT", 
     	MO_SIZEOF_MEMBER(THREADS,event[8]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[8]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0, "exp.clf.nt.cs.thread_eventLOCKEVENT", 
     	MO_SIZEOF_MEMBER(THREADS,event[9]), MO_READ, thread_index, 
	CL_OFFSETOF(THREADS,event[9]),
        MOintget, MOnoset, (PTR)-1, CS_samp_thread_index },

    { 0 }
};

