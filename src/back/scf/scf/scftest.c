#ifdef IIDEV_TEST
/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cs.h>
#include    <st.h>
#include    <tr.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <cut.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <dmccb.h>
#include    <dudbms.h>
#include    <adf.h>
#include    <ddb.h>
#include    <copy.h>
#include    <qefrcb.h>
#include    <dmrcb.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <sc.h>
#include    <scserver.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
/*
** Name: scftest.c - SCF Test code
**
** Description:
**	This file contains SCF testing modules, currently only the main CUT
**	test module.
**
** History:
**	14-jun-1999 (somsa01)
**	    Include st.h .
**	18-jan-2001 (somsa01)
**	    scs_atfactot() now takes two parameters.
**	25-Aug-2004 (drivi01)
**	    Due to change 469648 CUT_BUFFER has been replaced with
**	    CUT_RCB.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Add sc0e.h include.
*/

/*
** forward and internal definitions
*/

/*
** command buffer definition (this block is passed through the COMMAND_BUF
** CUT buffer
*/
typedef struct
{
    CS_SID		thread;
    i4		command;
#define		COMM_TEST		1 /* test */
#define		READ_NOWAIT		2 /* read from buffer */
#define		WRITE_NOWAIT		3 /* write to buffer */
#define		COMM_SLEEP		4 /* sleep for given time */
#define		COMM_SHUTDOWN		5 /* terminate thread */
#define		COMM_ATTACH		6 /* attach to buffer */
#define		COMM_DETACH		7 /* detach from buffer */
    i4		value;
} COMM_BUF;

/*
** list of buffers passed to created threads
*/
#define		READ_BUF		0
#define		WRITE_BUF		1
#define		COMMAND_BUF		2
#define		NUM_BUFS		3

typedef struct
{
    i4		buf_use;
    PTR		buf_handle;
} BUF_LIST;

/*
** internal functions
*/
static		DB_STATUS		thread_main(SCF_FTX	*ftx);
static		DB_STATUS		thread_child(SCF_FTX	*ftx);

/*
** Name: scf_cut_test - test CUT
**
** Description:
**	Test the functionality of the CUT modules
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	2-jul-98 (stephenb)
**	    Created
*/
VOID
scf_cut_test()
{
    SCF_CB	scf;
    SCD_SCB	*scb;
    SCF_FTC	ftc;
    STATUS	status;
    char	thread_name[] = "<CUT Test Master>";

    CSget_scb((CS_SCB**)&scb);
    
    /*
    ** Create the base thread (which will in turn create other threads)
    */
    scf.scf_type = SCF_CB_TYPE;
    scf.scf_length = sizeof(SCF_CB);
    scf.scf_session = DB_NOSESSION;
    scf.scf_facility = DB_SCF_ID;
    scf.scf_ptr_union.scf_ftc = &ftc;
    
    ftc.ftc_facilities = (1 << DB_ADF_ID) | (1 << DB_SCF_ID) | 
	(1 << DB_CUF_ID);
    ftc.ftc_data = NULL;
    ftc.ftc_data_length = 0;
    ftc.ftc_thread_name = thread_name;
    ftc.ftc_priority = SCF_CURPRIORITY;
    ftc.ftc_thread_entry = thread_main;
    ftc.ftc_thread_exit = NULL;
    
    (VOID)scs_atfactot(&scf, scb);

    return;
}

/*
** Name: thread_main
**
** Description:
**	controling function for main CUT test thread. This function creates the
**	other test threads and also creates the communications buffers which
**	are used to coordinate with the other threads.
**
** Inputs:
**	ftx	Thread execution control block
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	2-jul-98 (stephenb)
**	    Created
*/
static DB_STATUS
thread_main(SCF_FTX	*ftx)
{
    CUT_RCB		cut_buf[NUM_BUFS];
    CUT_RCB		*buf_ptr[NUM_BUFS];
    CUT_BUF_INFO	cut_buf_info;
    DB_STATUS		status;
    PTR			thread;
    PTR			buf_handle;
    DB_ERROR		error;
    SCF_CB		scf;
    SCD_SCB		*scb;
    SCF_FTC		ftc;
    BUF_LIST		*buf_list;
    i4			num_threads = 3;
    COMM_BUF		comm_buf;
    CS_SID		thread_id[2];
    i4			num_cells;
    STATUS		cl_stat;
    i4			i;
    PTR			buf_dat;
    char		child_name1[] = "<CUT Test Child 1>";
    char		child_name2[] = "<CUT Test Child 2>";

    CSget_scb((CS_SCB**)&scb);
    
    /*
    ** Initialize
    */
    STcopy("CUT test, read buffer", cut_buf[READ_BUF].cut_buf_name);
    cut_buf[READ_BUF].cut_cell_size = 10;
    cut_buf[READ_BUF].cut_num_cells = 10;
    cut_buf[READ_BUF].cut_buf_use = CUT_BUF_READ;
    
    STcopy("CUT test, write buffer", cut_buf[WRITE_BUF].cut_buf_name);
    cut_buf[WRITE_BUF].cut_cell_size = 10;
    cut_buf[WRITE_BUF].cut_num_cells = 5;
    cut_buf[WRITE_BUF].cut_buf_use = CUT_BUF_WRITE;

    STcopy("CUT test, command buffer", cut_buf[COMMAND_BUF].cut_buf_name);
    cut_buf[COMMAND_BUF].cut_cell_size = sizeof(comm_buf);
    cut_buf[COMMAND_BUF].cut_num_cells = 2;
    cut_buf[COMMAND_BUF].cut_buf_use = CUT_BUF_WRITE;

    TRdisplay("%@ CUT test: Master: Initializing...\n");
    status = cut_thread_init(&thread, &error);
    if (status != E_DB_OK)
    {
	if (error.err_code > E_CUF_INTERNAL)
	    sc0e_uput(error.err_code, 0, 0, 0, (PTR)0, 0, (PTR)0, 0, 
		(PTR)0, 0, (PTR)0, 0, (PTR)0, 0, (PTR)0 );
	TRdisplay("%@ CUT test: Master: Error initializing thread in CUT, \
test terminated\n");
	return(E_DB_ERROR);
    }

    /*
    ** Create buffers
    */
    for (i = 0; i < NUM_BUFS; i++)
    {
	buf_ptr[i] = &cut_buf[i];
    }
    status = cut_create_buf(NUM_BUFS, buf_ptr, &error);
    if (status != E_DB_OK)
    {
	if (error.err_code > E_CUF_INTERNAL)
	    sc0e_uput(error.err_code, 0, 0, 0, (PTR)0, 0, (PTR)0, 0, 
		(PTR)0, 0, (PTR)0, 0, (PTR)0, 0, (PTR)0 );
	TRdisplay("%@ CUT test: Master: Error creating CUT buffers, \
test terminated\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** get data area
    */
    buf_dat = MEreqmem(0, 100, TRUE, &cl_stat);
    if (cl_stat != OK || buf_dat == NULL)
    {
	sc0e_0_put(E_SC0023_INTERNAL_MEMORY_ERROR, 0);
	TRdisplay("%@ CUT Test: Master: Can't get memory, test terminated\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** Load up buffer list
    */
    buf_list = (BUF_LIST *)MEreqmem(0, (NUM_BUFS + 1) * sizeof(BUF_LIST),
	TRUE, &cl_stat);
    if (cl_stat != OK || buf_list == NULL)
    {
	sc0e_0_put(E_SC0023_INTERNAL_MEMORY_ERROR, 0);
	TRdisplay("%@ CUT Test: Master: Can't get memory, test terminated\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    buf_list[READ_BUF].buf_use = CUT_BUF_WRITE;
    buf_list[READ_BUF].buf_handle = cut_buf[READ_BUF].cut_buf_handle;
    buf_list[WRITE_BUF].buf_use = CUT_BUF_READ;
    buf_list[WRITE_BUF].buf_handle = cut_buf[WRITE_BUF].cut_buf_handle;
    buf_list[COMMAND_BUF].buf_use = CUT_BUF_READ;
    buf_list[COMMAND_BUF].buf_handle = cut_buf[COMMAND_BUF].cut_buf_handle;
    /* null terminated list */
    buf_list[NUM_BUFS].buf_use = 0;
    buf_list[NUM_BUFS].buf_handle = NULL;

    /*
    ** create threads and pass buffer address
    */
    scf.scf_type = SCF_CB_TYPE;
    scf.scf_length = sizeof(SCF_CB);
    scf.scf_session = DB_NOSESSION;
    scf.scf_facility = DB_SCF_ID;
    scf.scf_ptr_union.scf_ftc = &ftc;
    
    ftc.ftc_facilities = SCF_CURFACILITIES;
    ftc.ftc_data = (PTR)buf_list;
    ftc.ftc_data_length = (NUM_BUFS + 1) * sizeof(BUF_LIST);
    ftc.ftc_thread_name = child_name1;
    ftc.ftc_priority = SCF_CURPRIORITY;
    ftc.ftc_thread_entry = thread_child;
    ftc.ftc_thread_exit = NULL;

    status = scs_atfactot(&scf, scb);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT Test: Master: Can't create child threads, \
exiting tests...\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    thread_id[0] = ftc.ftc_thread_id;
    TRdisplay("%@ CUT test: Master: created thread %x\n", thread_id[0]);

    ftc.ftc_thread_name = child_name2;
    status = scs_atfactot(&scf, scb);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT Test: Master: Can't create child threads, \
exiting tests...\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    thread_id[1] = ftc.ftc_thread_id;
    TRdisplay("%@ CUT test: Master: created thread %x\n", thread_id[1]);

    /*
    ** wait for threads to connect
    */
    TRdisplay("%@ CUT Test: Master: Waiting for Children to connect to \
buffer...\n");
    status = cut_event_wait(&cut_buf[READ_BUF], CUT_ATTACH, (PTR)&num_threads,
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: errror occurred waiting on buffer \
event for buffer 1\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    status = cut_event_wait(&cut_buf[WRITE_BUF], CUT_ATTACH, 
	(PTR)&num_threads, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: errror occurred waiting on buffer \
event for buffer 2\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    status = cut_event_wait(&cut_buf[COMMAND_BUF], CUT_ATTACH, 
	(PTR)&num_threads, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: errror occurred waiting on buffer \
event for buffer 3\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: all threads connected\n");

    /*
    ** Send test command on command buffer
    */
    comm_buf.thread = (CS_SID)0; /* all threads */
    comm_buf.command = COMM_TEST;
    comm_buf.value = 25;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Sending test command to all threads, \
vaue = %d...\n", comm_buf.value);

    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }


    /*
    ** write data to write buffer (3 cells)
    */
    
    MEfill(9, '1', buf_dat);
    buf_dat[9] = 0;
    MEfill(9, '2', buf_dat+10);
    buf_dat[19] = 0;
    MEfill(9, '3', buf_dat+20);
    buf_dat[29] = 0;
    num_cells = 3;
    TRdisplay("%@ CUT test: Master: writing %d cells to write buffer...\n", 
	num_cells);
    status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: buffer head at %d, writing the following:\n"
	, cut_buf_info.cut_buf_head);
    for (i = 0; i < num_cells; i++)
    {
	TRdisplay("%@ CUT test: Master: Cell %d = %s\n", 
	    (cut_buf_info.cut_buf_head + i)%cut_buf[WRITE_BUF].cut_num_cells,
	    buf_dat+(cut_buf[WRITE_BUF].cut_cell_size*i));
    }
    status = cut_write_buf(&num_cells, &cut_buf[WRITE_BUF], buf_dat, TRUE, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to write buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell thread 1 to read 2 cells
    */
    comm_buf.thread = thread_id[0];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 2;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read %d cells \
from my write buffer without waiting (should work)...\n", 
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    
    /*
    ** tell thread 2 to read 1 cell
    */
    comm_buf.thread = thread_id[1];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read %d cell \
from my write buffer without waiting (should work)...\n", 
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell thread 1 to read 1 cell
    */
    comm_buf.thread = thread_id[0];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read %d cell \
from my write buffer without waiting (should work)...\n", 
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell thread 2 to read 2 cells
    */
    comm_buf.thread = thread_id[1];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 2;    
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read %d cells \
from my write buffer without waiting (should work)...\n", 
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** write another 3 cells
    */
    num_cells = 3;
    TRdisplay("%@ CUT test: Master: writing %d cells to write buffer...\n",
	num_cells);
    status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: buffer head at %d, writing the following:\n"
	, cut_buf_info.cut_buf_head);
    for (i = 0; i < num_cells; i++)
    {
	TRdisplay("%@ CUT test: Master: Cell %d = %s\n", 
	    (cut_buf_info.cut_buf_head + i)%cut_buf[WRITE_BUF].cut_num_cells,
	    buf_dat+(cut_buf[WRITE_BUF].cut_cell_size*i));
    }

    status = cut_write_buf(&num_cells, &cut_buf[WRITE_BUF], buf_dat, TRUE, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to write buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell both threads to read all 3 cells concurrently
    */
    comm_buf.thread = 0;
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 3;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing all threads to read %d cells \
from my write buffer without waiting (should work)...\n", comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf,
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell both threads to write 5 cells to my read buffer
    */
    comm_buf.thread = 0;
    comm_buf.command = WRITE_NOWAIT;
    comm_buf.value = 5;
    TRdisplay("%@ CUT test: Master: Instructing all threads to write %d cells \
to my read buffer without waiting (should work)...\n", comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** read 10 cells from read buffer
    */
    status = cut_info(CUT_INFO_BUF, &cut_buf[READ_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[READ_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    num_cells = 10;
    TRdisplay("%@ CUT test: Master: reading %d cells with wait, buffer tail is at %d, (should work)...\n", num_cells, cut_buf_info.cut_buf_tail);
    status = cut_read_buf(&num_cells, &cut_buf[READ_BUF], buf_dat, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: error reading from read buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** print out buffer
    */
    for (i = 0; i < num_cells; i++)
    {
	TRdisplay("%@ CUT test: Master: Cell %d = %s\n", 
	    (cut_buf_info.cut_buf_tail+i)%cut_buf[READ_BUF].cut_num_cells, 
	    buf_dat + (i*cut_buf[READ_BUF].cut_cell_size));
    }

    /*
    ** try to read an empty buffer
    */
    TRdisplay("%@ CUT test: Master: trying to read an empty buffer with no \
wait (should fail)\n");
    num_cells = 1;
    status = cut_read_buf(&num_cells, &cut_buf[READ_BUF], buf_dat, FALSE, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: BAD NEWS!!! A read on an empty \
buffer succeeded, terminating test.\n");
	status = E_DB_ERROR;
	(VOID)cut_signal_status(&cut_buf[READ_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);

    }
    else if (status == E_DB_WARN && error.err_code == W_CU0305_NO_CELLS_READ)
    {
	TRdisplay("%@ CUT test: Master: Read returned warning, there are no \
cells, this is correct\n");
    }
    else
    {
	TRdisplay("%@ CUT test: Master: error reading from read buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** fill write buffer
    */
    MEfill(9, '1', buf_dat);
    buf_dat[9] = 0;
    MEfill(9, '2', buf_dat+10);
    buf_dat[19] = 0;
    MEfill(9, '3', buf_dat+20);
    buf_dat[29] = 0;
    MEfill(9, '4', buf_dat+30);
    buf_dat[39] = 0;
    MEfill(9, '5', buf_dat+50);
    buf_dat[49] = 0;
    num_cells = 5;
    TRdisplay("%@ CUT test: Master: writing %d cells to write buffer...\n",
	num_cells);
    status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: buffer head at %d, writing the following:\n"
	, cut_buf_info.cut_buf_head);
    for (i = 0; i < num_cells; i++)
    {
	TRdisplay("%@ CUT test: Master: Cell %d = %s\n", 
	    (cut_buf_info.cut_buf_head + i)%cut_buf[WRITE_BUF].cut_num_cells,
	    buf_dat+(cut_buf[WRITE_BUF].cut_cell_size*i));
    }
    status = cut_write_buf(&num_cells, &cut_buf[WRITE_BUF], buf_dat, 
	FALSE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to write buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    /*
    ** try to write more without wait (should fail)
    */
    TRdisplay("%@ CUT test: Master: trying to write to full buffer without \
wait (should fail).\n");
    num_cells = 1;
    status = cut_write_buf(&num_cells, &cut_buf[WRITE_BUF], buf_dat, 
	FALSE, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: BAD NEWS !!!, write to a full \
buffer succeeded, terminating test\n");
	status = E_DB_ERROR;
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    else if (status == E_DB_WARN && error.err_code == W_CU0307_NO_CELLS_WRITTEN)
    {
	TRdisplay("%@ CUT test: Master: write returned warning, buffer is full \
this is correct\n");
    }
    else
    {
	TRdisplay("%@ CUT test: Master: error writing to write buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell thread 1 to read 5 cells, buffer should still be full
    */
    comm_buf.thread = thread_id[0];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 5;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read %d cells \
from my write buffer without waiting (should work)...\n",
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** try to write more (should fail)
    */
    TRdisplay("%@ CUT test: Master: trying to write to full buffer without \
wait (should fail).\n");
    num_cells = 1;
    status = cut_write_buf(&num_cells, &cut_buf[WRITE_BUF], buf_dat, 
	FALSE, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: BAD NEWS !!!, write to a full \
buffer succeeded, terminating test\n");
	status = E_DB_ERROR;
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    else if (status == E_DB_WARN && error.err_code == W_CU0307_NO_CELLS_WRITTEN)
    {
	TRdisplay("%@ CUT test: Master: write returned warning, buffer is full \
this is correct\n");
    }
    else
    {
	TRdisplay("%@ CUT test: Master: error writing to write buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell second thread to read 3 cells after waiting 5 seconds
    */
    comm_buf.thread = thread_id[1];
    comm_buf.command = COMM_SLEEP;
    comm_buf.value = 5;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to wait for \
%d seconds...\n", comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    comm_buf.thread = thread_id[1];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 3;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read %d cells \
from my write buffer without waiting (should work)...\n",
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** try to write with wait, we will wait until second thread has read
    ** then return O.K.
    */
    TRdisplay("%@ CUT test: Master: trying to write to full buffer with \
wait, should wait for thread %x then write...\n", thread_id[1]);
    num_cells = 1;
    status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: buffer head at %d, writing the following:\n"
	, cut_buf_info.cut_buf_head);
    TRdisplay("%@ CUT test: Master: Cell %d = %s\n", cut_buf_info.cut_buf_head,
	buf_dat);

    status = cut_write_buf(&num_cells, &cut_buf[WRITE_BUF], buf_dat, 
	TRUE, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: write returned O.K.\n");
    }
    else
    {
	TRdisplay("%@ CUT test: Master: error writing to write buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell thread 1 to wait 5 seconds, then write 5 cells
    */
    comm_buf.thread = thread_id[0];
    comm_buf.command = COMM_SLEEP;
    comm_buf.value = 5;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to wait for \
%d seconds...\n", comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    comm_buf.thread = thread_id[0];
    comm_buf.command = WRITE_NOWAIT;
    comm_buf.value = 5;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to write %d cells \
to my read buffer without waiting (should work)...\n", 
	comm_buf.thread, comm_buf.value);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** now read 3 with wait, we should wait for the write then return O.K.
    */
    TRdisplay("%@ CUT test: Master: trying to read from empty buffer with \
wait, should wait for thread %x then read...\n", thread_id[0]);
    num_cells = 3;
    status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: buffer tail at %d, reading the following:\n"
	, cut_buf_info.cut_buf_tail);
    status = cut_read_buf(&num_cells, &cut_buf[READ_BUF], buf_dat, 
	TRUE, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: read returned O.K.\n");
    }
    else
    {
	TRdisplay("%@ CUT test: Master: error reading from buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    for (i = 0; i < num_cells; i++)
    {
	TRdisplay("%@ CUT test: Master: Cell %d = %s\n", 
	    (cut_buf_info.cut_buf_tail + i) % 
	        (cut_buf[READ_BUF].cut_num_cells + 1),
	    buf_dat + (i*10));
    }

    /*
    ** try to detach with unread cells (should return error), then read 
    ** the remainder
    */
    TRdisplay("%@ CUT test: Master: Attempting to detach from my read buffer \
with unread cells (should fail)\n");
    status = cut_detach_buf(&cut_buf[READ_BUF], 0, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: BAD NEWS !!!, detach from a buffer \
with unread data suceeded, test terminating\n");
	status = E_DB_ERROR;
	(VOID)cut_signal_status(&cut_buf[READ_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    else if (status == E_DB_ERROR && error.err_code == E_CU0202_READ_CELLS)
    {
	TRdisplay("%@ CUT test: Master: detach failed, this is correct\n");
    }
    else
    {
	TRdisplay("%@ CUT test: error recieved when detaching from buffer \
,test terminating\n");
	(VOID)cut_signal_status(&cut_buf[READ_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    num_cells = 2;
    TRdisplay("%@ CUT test: Master: Reading the the remaining %d cells from my \
read buffer\n", num_cells);
    status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], (PTR)&cut_buf_info, 
	&error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't get buffer info, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    TRdisplay("%@ CUT test: Master: buffer tail at %d, reading the following:\n"
	, cut_buf_info.cut_buf_tail);
    status = cut_read_buf(&num_cells, &cut_buf[READ_BUF], buf_dat, 
	FALSE, &error);
    if (status == E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: read returned O.K.\n");
    }
    else
    {
	TRdisplay("%@ CUT test: Master: error reading from buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    for (i = 0; i < num_cells; i++)
    {
	TRdisplay("%@ CUT test: Master: Cell %d = %s\n", 
	    (cut_buf_info.cut_buf_tail + i) % 
	        (cut_buf[READ_BUF].cut_num_cells + 1), 
	    buf_dat + (i*10));
    }

    /*
    ** tell second thread to detatch, should produce warning because of
    ** unread cells
    */
    comm_buf.thread = thread_id[1];
    comm_buf.command = COMM_DETACH;
    comm_buf.value = READ_BUF;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to detach from my \
write buffer (should produce warning because of unread cells)...\n", 
	comm_buf.thread);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell second thread to re-attatch, there will now be no cells to read
    */
    comm_buf.thread = thread_id[1];
    comm_buf.command = COMM_ATTACH;
    comm_buf.value = READ_BUF;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to attach to my \
write buffer (should return O.K.)...\n",
	comm_buf.thread);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell second thread to read without wait, should get warning
    */
    comm_buf.thread = thread_id[1];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 1;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read from my \
write buffer (should produce warning, no cells to read)...\n", comm_buf.thread);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** send async status to threads
    */
    TRdisplay("%@ CUT test: Master: Instructing all threads to sleep for \
5 seconds\n");
    comm_buf.thread = 0;
    comm_buf.command = COMM_SLEEP;
    comm_buf.value = 5;
    num_cells = 1;
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    
    TRdisplay("%@ CUT test: Master: Sending async status to threads, they \
should recive it when they wake up\n");
    status = E_DB_INFO;
    status = cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: Error signalling status to cooperating \
threads, test terminating\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /* re-set my status */
    status = cut_signal_status(&cut_buf[COMMAND_BUF], E_DB_OK, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: Error re-setting status, test \
terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    /*
    ** tell first thread to read last cell without wait
    */
    comm_buf.thread = thread_id[0];
    comm_buf.command = READ_NOWAIT;
    comm_buf.value = 1;
    num_cells = 1;
    TRdisplay("%@ CUT test: Master: Instructing thread %x to read from my \
write buffer (should return O.K.)...\n", comm_buf.thread);
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** tell threads to detach from buf and terminate
    */
    TRdisplay("%@ CUT test: Master: Instructing all threads to detach from all \
buffers (should work O.K)...\n");
    comm_buf.thread = 0;
    comm_buf.command = COMM_DETACH;
    comm_buf.value = READ_BUF;
    num_cells = 1;
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], 
	(PTR)&comm_buf, TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    comm_buf.value = WRITE_BUF;
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], 
	(PTR)&comm_buf, TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    TRdisplay("%@ CUT test: Master: Instructing all threads to closedown\n");
    comm_buf.thread = 0;
    comm_buf.command = COMM_SHUTDOWN;
    comm_buf.value = 0;
    num_cells = 1;
    status = cut_write_buf(&num_cells, &cut_buf[COMMAND_BUF], (PTR)&comm_buf, 
	TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: can't write to command buffer, \
test terminating\n");
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
 
    /*
    ** cleanup
    */
    TRdisplay("%@ CUT test: Master: test complteed successfully, cleaning \
up\n");
    status = cut_thread_term(TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Master: Error terminating thread in CUT\n");
    }
    if (buf_dat)
	MEfree(buf_dat);

    if (buf_list)
	MEfree((PTR)buf_list);

    return(E_DB_OK);
}

/*
** Name: thread_child - CUT test child thread
**
** Description:
**	main function for CUT test child threads. these threads sequence 
**	a series of commands sent by the master thread after connecting
**	to the provided buffers
**
** Inputs:
**	ftx		Thread execution control block
**
** Outputs:
**	None
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-98 (stephenb)
**	    Initial creation
**	3-jun-99 (stephenb)
**	    Async status can be returned from any CUT call, check for it and
**	    take appropriate action.
*/
static DB_STATUS
thread_child(SCF_FTX	*ftx)
{
    CUT_RCB		cut_buf[NUM_BUFS];
    BUF_LIST		*buf_list;
    PTR			cut_thread;
    DB_ERROR		error;
    CS_SID		sid;
    i4			num_cells;
    COMM_BUF		comm_buf;
    PTR			buf_dat;
    STATUS		cl_stat;
    i4			i;
    bool		done = FALSE;
    DB_STATUS		status;
    SCD_SCB		*scb;
    CUT_THREAD_INFO	thread_info;
    CUT_BUF_INFO	cut_buf_info;

    CSget_sid(&sid);
    CSget_scb((CS_SCB **)&scb);

    /*
    ** Grab CUT buffers, init control blocks and connect
    ** child read buffer is parent write and vice-versa.
    */
    buf_list = (BUF_LIST *)ftx->ftx_data;
    cut_buf[READ_BUF].cut_buf_handle = buf_list[WRITE_BUF].buf_handle;
    cut_buf[READ_BUF].cut_buf_use = CUT_BUF_READ;
    cut_buf[WRITE_BUF].cut_buf_handle = buf_list[READ_BUF].buf_handle;
    cut_buf[WRITE_BUF].cut_buf_use = CUT_BUF_WRITE;
    cut_buf[COMMAND_BUF].cut_buf_handle = buf_list[COMMAND_BUF].buf_handle;
    cut_buf[COMMAND_BUF].cut_buf_use = CUT_BUF_READ;

    status = cut_thread_init(&cut_thread, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Child %8x: Error initializing thread, test \
terminating\n", sid);
	return(E_DB_ERROR);
    }

    TRdisplay("%@ CUT test: Child %8x: Initializing and attaching to buffers\n",
	sid);
    status = cut_attach_buf(&cut_buf[READ_BUF], &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Child %8x: Error attaching to read buffer, \
thread terminating\n", sid);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    status = cut_attach_buf(&cut_buf[WRITE_BUF], &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Child %8x: Error attaching to write buffer, \
thread terminating\n", sid);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    status = cut_attach_buf(&cut_buf[COMMAND_BUF], &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Child %8x: Error attaching to command buffer, \
thread terminating\n", sid);
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }
    /*
    ** get data area
    */
    buf_dat = MEreqmem(0, 100, TRUE, &cl_stat);
    if (cl_stat != OK || buf_dat == NULL)
    {
	sc0e_0_put(E_SC0023_INTERNAL_MEMORY_ERROR, 0);
	TRdisplay("%@ CUT Test: Master: Can't get memory, test terminated\n");
	(VOID)cut_thread_term(TRUE, &error);
	return(E_DB_ERROR);
    }

    /*
    ** sequence commands
    */
    while (!done)
    {
	/*
	** check for thread status
	*/
	if (scb->scb_sscb.sscb_interrupt == SCS_CTHREAD)
	{
	    status = cut_info(CUT_INFO_THREAD, &cut_buf[COMMAND_BUF], 
		(PTR)&thread_info, &error);
	    if (status != E_DB_OK && error.err_code != E_CU0204_ASYNC_STATUS)
	    {
		TRdisplay("%@ CUT test: Child %8x: Can't get info from CUT, \
child terminating\n", sid);
		(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, &error);
		(VOID)cut_thread_term(TRUE, &error);
		return(E_DB_ERROR);
	    }
	    TRdisplay("%@ CUT test: Child %8x: Recieved asynchronous status \
%d\n", sid, thread_info.cut_thread_status);
	    if (thread_info.cut_thread_status > E_DB_WARN)
	    {
		TRdisplay("%@ CUT test: Child %8x: Terminating because another \
cooperating thread has signalled a bad status\n", sid);
		(VOID)cut_thread_term(TRUE, &error);
		return(E_DB_ERROR);
	    }
	    else if (thread_info.cut_thread_status > E_DB_OK)
	    {
		TRdisplay("%@ CUT test: Child %8x: Resetting warning/Info \
status and continuing\n", sid);
		(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], E_DB_OK, &error);
		scb->scb_sscb.sscb_interrupt = 0;
	    }
	}
	/*
	** read command
	*/
	num_cells = 1;
	status = cut_info(CUT_INFO_THREAD, &cut_buf[COMMAND_BUF], 
	    (PTR)&thread_info, &error);
	for (;;)
	{
	    TRdisplay("%@ CUT test: Child %8x: Reading command at cell %d\n",
		sid, thread_info.cut_current_cell);
	    status = cut_read_buf(&num_cells, &cut_buf[COMMAND_BUF], 
		(PTR)&comm_buf, TRUE, &error);
	    if (status != E_DB_OK)
	    {
		if (error.err_code == E_CU0204_ASYNC_STATUS)
		{
		    if (status > E_DB_WARN)
		    {
			TRdisplay("%@ CUT test: Child %8x: Terminating because \
another cooperating thread has signalled a bad status\n", sid);
			(VOID)cut_thread_term(TRUE, &error);
			return(status);
		    }
		    else
		    {
			TRdisplay("%@ CUT test: Child %8x: Resetting \
warning/Info status and continuing\n", sid);
			(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], E_DB_OK, 
			    &error);
			scb->scb_sscb.sscb_interrupt = 0;
			/* re-read command */
			continue;
		    }
		}
		else
		{
		    TRdisplay("%@ CUT Test: Child %8x: Error reading command, \
child terminating\n", sid);
		    (VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, 
			&error);
		    (VOID)cut_thread_term(TRUE, &error);
		    return(E_DB_ERROR);
		}
	    }
	    break;
	}


	/* is it for me ? */
	if (comm_buf.thread != 0 && comm_buf.thread != sid)
	{
	    TRdisplay("%@ CUT test: Child %8x: Ignoring command for %x\n",
		sid, comm_buf.thread);
	    continue;
	}

	/*
	** execute command
	*/
	switch(comm_buf.command)
	{
	    case COMM_TEST:
	    {
		/* test command, just print that we received it */
		TRdisplay("%@ CUT test: Child %8x: Received test command, \
value = %d\n", sid, comm_buf.value);
		break;
	    }
	    case READ_NOWAIT:
	    {
		/* read (no wait) value indicates # of cells */
		TRdisplay("%@ CUT test: Child %8x: reading %d cells from \
buffer with no wait...\n", sid, comm_buf.value);
		status = cut_info(CUT_INFO_THREAD, &cut_buf[READ_BUF], 
		    (PTR)&thread_info, &error);
		if (status != E_DB_OK)
		{
		    TRdisplay("%@ CUT test: Child %8x: Can't get info from \
CUT, child terminating\n", sid);
		    (VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, 
			&error);
		    (VOID)cut_thread_term(TRUE, &error);
		    return(E_DB_ERROR);
		}
		for (;;)
		{
		    TRdisplay("%@ CUT test: Child %8x: Current position at %d, \
reading the following\n", sid, thread_info.cut_current_cell);
		    status = cut_read_buf(&comm_buf.value, &cut_buf[READ_BUF],
			buf_dat, FALSE, &error);
		    if (status == E_DB_OK)
		    {
			TRdisplay("%@ CUT test: Child %8x: Read succeeded for \
all cells\n", sid);
		    }
		    else if (status == E_DB_WARN && 
			error.err_code == W_CU0306_LESS_CELLS_READ)
		    {
			TRdisplay("%@ CUT test: Child %8x: only read %d \
cells\n",
			    sid, comm_buf.value);
		    }
		    else if (status == E_DB_WARN &&
			error.err_code == W_CU0305_NO_CELLS_READ)
		    {
			TRdisplay("%@ CUT test: Child %8x: No cells read\n", 
			    sid);
		    }
		    else if (error.err_code == E_CU0204_ASYNC_STATUS)
		    {
			if (status > E_DB_WARN)
			{
			    TRdisplay("%@ CUT test: Child %8x: Terminating \
because another cooperating thread has signalled a bad status\n", sid);
			    (VOID)cut_thread_term(TRUE, &error);
			    return(status);
			}
			else
			{
			    TRdisplay("%@ CUT test: Child %8x: Resetting \
warning/Info status and continuing\n", sid);
			    (VOID)cut_signal_status(&cut_buf[COMMAND_BUF], 
				E_DB_OK, &error);
			    scb->scb_sscb.sscb_interrupt = 0;
			    /* re-read buffer */
			    continue;
			}
		    }
		    else
		    {
			TRdisplay("%@ CUT test: Child %8x: Error, failed to \
read from buffer, thread terminating\n", sid);
			(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, 
			    &error);
			(VOID)cut_thread_term(TRUE, &error);
			MEfree(buf_dat);
			return(E_DB_ERROR);
		    }
		    break;
		}
		/* print cells */
		for (i = 0; i < comm_buf.value; i++)
		{
		    TRdisplay("%@ CUT test: Child %8x: Cell %d = %s\n",
			sid, (thread_info.cut_current_cell + i) % 
			    cut_buf[READ_BUF].cut_num_cells, 
			buf_dat + (i*10));
		}
		break;
	    }
	    case WRITE_NOWAIT:
	    {
		/* write (no wait) value indicated # of cells */
		TRdisplay("%@ CUT test: Child %8x: Writing %d cells to buffer \
with no wait...\n", sid, comm_buf.value);
		for (i = 0; i < comm_buf.value; i++)
		{
		    MEfill(9, (char)i+49, buf_dat+(i*10));
		    buf_dat[(10*i)+9] = 0;
		}
		status = cut_info(CUT_INFO_BUF, &cut_buf[WRITE_BUF], 
		    (PTR)&cut_buf_info, &error);
		if (status != E_DB_OK)
		{
		    TRdisplay("%@ CUT test: Child %8x: can't get buffer info, \
child terminating\n", sid);
		    (VOID)cut_signal_status(&cut_buf[WRITE_BUF], status, 
			&error);
		    (VOID)cut_thread_term(TRUE, &error);
		    return(E_DB_ERROR);
		}
		for (;;)
		{
		    TRdisplay("%@ CUT test: Child %8x: buffer head at %d, \
writing the following:\n", sid, cut_buf_info.cut_buf_head);
		    status = cut_write_buf(&comm_buf.value, &cut_buf[WRITE_BUF],
			buf_dat, FALSE, &error);
		    if (status == E_DB_OK)
		    {
			TRdisplay("%@ CUT test: Child %8x: Wrote all cells\n", 
			    sid);
		    }
		    else if (status == E_DB_WARN && 
			error.err_code == W_CU0308_LESS_CELLS_WRITTEN)
		    {
			TRdisplay("%@ CUT test: Child %8x: Only wrote %d \
cells\n",
			    sid, comm_buf.value);
		    }
		    else if (status == E_DB_WARN &&
			error.err_code == W_CU0307_NO_CELLS_WRITTEN)
		    {
			TRdisplay("%@ CUT test: Child %8x: Buffer full, no \
cells written\n", sid);
		    }
		    else if (error.err_code == E_CU0204_ASYNC_STATUS)
		    {
			if (status > E_DB_WARN)
			{
			    TRdisplay("%@ CUT test: Child %8x: Terminating \
because another cooperating thread has signalled a bad status\n", sid);
			    (VOID)cut_thread_term(TRUE, &error);
			    return(status);
			}
			else
			{
			    TRdisplay("%@ CUT test: Child %8x: Resetting \
warning/Info status and continuing\n", sid);
			    (VOID)cut_signal_status(&cut_buf[COMMAND_BUF], 
				E_DB_OK, &error);
			    scb->scb_sscb.sscb_interrupt = 0;
			    /* re-try write */
			    continue;
			}
		    }
		    else
		    {
			TRdisplay("%@ CUT test: Child %8x: Error writing \
buffer, thread terminating.\n", sid);
			(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, 
			    &error);
			(VOID)cut_thread_term(TRUE, &error);
			MEfree(buf_dat);
			return(E_DB_ERROR);
		    }
		    break;
		}
		for (i = 0; i < comm_buf.value; i++)
		{
		    TRdisplay("%@ CUT test: Child %8x: Cell %d = %s\n",
			sid, (cut_buf_info.cut_buf_head + i) %
			    cut_buf[WRITE_BUF].cut_num_cells,
			buf_dat + (i*cut_buf[WRITE_BUF].cut_cell_size));
		}
		break;
	    }
	    case COMM_SLEEP:
	    {
		/* sleep, values indicates # of seconds */
		TRdisplay("%@ CUT test: Child: %8x: Sleeping for %d seconds\n",
		    sid, comm_buf.value);
		CSsuspend(CS_TIMEOUT_MASK, comm_buf.value, (PTR)NULL);
		TRdisplay("%@ CUT test: Child %8x: Waking after sleep\n", sid);
		break;
	    }
	    case COMM_SHUTDOWN:
	    {
		/* shutdown thread */
		TRdisplay("%@ CUT test: Child %8x: Terminating thread\n", sid);
		done = TRUE;
		break;
	    }
	    case COMM_ATTACH:
	    {
		/* attach to buffer, value indicates buffer # */
		TRdisplay("%@ CUT test: Child %8x: Attaching to buffer %d\n",
		    sid, comm_buf.value);
		status = cut_attach_buf(&cut_buf[comm_buf.value], &error);
		if (status != E_DB_OK)
		{
		    if (error.err_code == E_CU0204_ASYNC_STATUS)
		    {
			if (status > E_DB_WARN)
			{
			    TRdisplay("%@ CUT test: Child %8x: Terminating \
because another cooperating thread has signalled a bad status\n", sid);
			    (VOID)cut_thread_term(TRUE, &error);
			    return(status);
			}
			else
			{
			    TRdisplay("%@ CUT test: Child %8x: Resetting \
warning/Info status and continuing\n", sid);
			    (VOID)cut_signal_status(&cut_buf[COMMAND_BUF], 
				E_DB_OK, &error);
			    scb->scb_sscb.sscb_interrupt = 0;
			}
		    }
		    else
		    {
			TRdisplay("%@ CUT test: Child %8x: Error, can't attach \
to buffer %d, thread terminating\n", sid, comm_buf.value);
			(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, 
			    &error);
			(VOID)cut_thread_term(TRUE, &error);
			MEfree(buf_dat);
			return(E_DB_ERROR);
		    }
		}
		TRdisplay("%@ CUT test: Child %8x: Buffer attached\n", sid);
		break;
	    }
	    case COMM_DETACH:
	    {
		for (;;)
		{
		    /* detach from buffer, value indicates buffer # */
		    TRdisplay("%@ CUT test: Child %8x: Detaching from \
buffer %d\n",
			sid, comm_buf.value);
		    status = cut_detach_buf(&cut_buf[comm_buf.value], 0, &error);
		    if (status == E_DB_WARN && 
			error.err_code == W_CU0303_UNREAD_CELLS)
		    {
			TRdisplay("%@ CUT test: Child %8x: Buffer detach \
returned a warning that there were unread cells, ignoring and continuing\n", 
			    sid);
		    }
		    else if (status != E_DB_OK)
		    {
			if (error.err_code == E_CU0204_ASYNC_STATUS)
			{
			    if (status > E_DB_WARN)
			    {
				TRdisplay("%@ CUT test: Child %8x: Terminating \
because another cooperating thread has signalled a bad status\n", sid);
				(VOID)cut_thread_term(TRUE, &error);
				return(status);
			    }
			    else
			    {
				TRdisplay("%@ CUT test: Child %8x: Resetting \
warning/Info status and continuing\n", sid);
				(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], 
				    E_DB_OK, &error);
				scb->scb_sscb.sscb_interrupt = 0;
				/* re-try detach */
				continue;
			    }
			}
			else
			{
			    TRdisplay("%@ CUT test: Child %8x: Error, can't \
detach from buffer %d, thread terminating\n", sid, comm_buf.value);
			    (VOID)cut_signal_status(&cut_buf[COMMAND_BUF], 
				status, &error);
			    (VOID)cut_thread_term(TRUE, &error);
			    MEfree(buf_dat);
			    return(E_DB_ERROR);
			}
		    }
		    break;
		}
		TRdisplay("%@ CUT test: Child %8x: Buffer detached\n", sid);
		break;
	    }
	    default:
	    {
		TRdisplay("%@ CUT test: Child %8x Bad command %d ignored\n",
		    sid, comm_buf.command);
		break;
	    }
	}
    }
    /*
    ** cleanup
    */
    if (buf_dat)
	MEfree(buf_dat);
	
    status = cut_thread_term(TRUE, &error);
    if (status != E_DB_OK)
    {
	TRdisplay("%@ CUT test: Child %8x: Error terminating thread in CUT\n", 
	    sid);
	(VOID)cut_signal_status(&cut_buf[COMMAND_BUF], status, 
	    &error);
    }
	
    return(status);
}
#endif
