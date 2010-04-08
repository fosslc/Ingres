/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ercuf.msg
**
** Description
**	Error messages for common!cuf
**
** History:
**	29-jul-98 (stephenb)
**	    Initial creation
**	20-aug-98 (stephenb)
**	    Add message E_CU0204_ASYNC_STATUS
**	3-May-2004 (schka24)
**	    Correct spelling typos.
**	15-Sep-2004 (schka24)
**	    %s notwork in error msgs, try %c.
*/

#define _CU_CLASS	19

NO_SQLSTATE_MAPPING

E_CU0101_BAD_BUFFER_COUNT
"An invalid buffer count %0d was passed to %1c, please report this problem
to your technical support representative."

E_CU0102_NULL_BUFFER
"A null buffer location for buffer %0d was passed to %1c, please report this
problem to your technical support representative."

E_CU0103_BAD_BUFFER_USE
"A bad buffer usage value of %0d for buffer %1d was passed to %2c, please 
report this problem to your technical support representative."

E_CU0104_BAD_CELLS
"A bad cell count (%0d) or size (%1d) was passed to %2c, please report this 
problem to your technical support representative."

E_CU0105_NO_MEM
"Unable to allocate %0d bytes of memory in %1c, please check errlog.log 
in II_CONFIG for more details."

E_CU0106_INIT_SEM
"Unable to initialize semaphore %0c, please check errlog.log in II_CONFIG
for more details."

E_CU0107_INIT_READ_COND
"Unable to initialize read condition for buffer %0c, please see errlog.log in 
II_CONFIG for more details."

E_CU0108_INIT_WRITE_COND
"Unable to initialize write condition for buffer %0c, please see errlog.log in
II_CONFIG for more details."

E_CU0109_INIT_EVENT_COND
"Unable to initialize event condition for buffer %0c, please see errlog.log in
II_CONFIG for more details."

E_CU010A_NAME_COND
"Unable to name condition for buffer %0c, please see errlog.log in II_CONFIG
for more details."

E_CU010B_CANT_GET_SEM
"An error occurred obtaining semaphore %0c, please see errlog.log in 
II_CONFIG for more details."

E_CU010C_INCONSISTENT_BUFFER
"Buffer at address %0x does not point to itself at address %1x, please report
this problem to your technical support representative."

E_CU010D_BUF_NOT_IN_LIST
"Buffer at address %0x, passed to %1c is not in the CUT master buffer list."

E_CU010E_NO_CELLS
"CUT was asked to read/write zero cells, or was passed a null cell count, no
cells will be read."

E_CU010F_NO_OUTPUT_BUF
"A null output buffer was passed to a CUT request to read cells, please report
this problem to your technical support representative."

E_CU0110_TO_MANY_CELLS
"A request was passed to CUT to read/write %0d cells from buffer %1c, this
buffer only contains %2d cells."

E_CU0111_NOT_ATTACHED
"A thread has asked to read/write from CUT buffer %0c which it is not yet 
attached to, please report this problem to your technical support 
representative."

E_CU0112_BAD_WRITE_CONDITION
"An error was returned to CUT when waiting on a buffer write condition for
buffer %0c, please see errlog.log in II_CONFIG for more details."

E_CU0013_NO_INPUT_BUF
"A null input buffer was passed to a CUT request to write cells, please
report this problem to your technical support representative."

E_CU0114_BAD_READ_CONDITION
"An error was returned to CUT when waiting on a buffer read condition for
buffer %0c, please see errlog.log in II_CONFIG for more details."

E_CU0115_INVALID_STATUS
"A status value of %0d was passed to cut_signal_status, please report this
problem to your technical support representative."

E_CU0116_BAD_EVENT
"CUT was asked to wait for event %0d which is unknown to it, please report this
problem to your technical support representative."

E_CU0117_BAD_EVENT_CONDITION
"An error was returned to CUT when waiting on a buffer event condition for
buffer %0c, please see errlog.log in II_CONFIG for more details."

E_CU0118_BAD_BUFFER
"CUT consistency check error, an attempt was made to obtain a semaphore for
a buffer that has no connected threads, please report this problem to your
technical support representative."

E_CU0119_BUF_NOT_IN_LIST
"%0c attempted to obtain a semaphore on a buffer that is not in the 
master CUT buffer list, please report this problem to your technical support
representative."

E_CU011A_BAD_BLOCK_TYPE
"CUT attempted to get a memory block of type %0d, this is an invalid block
type. Please report this problem to your technical support representative."

/*
** Errors that are not logged in CUF, simply returned to caller to handle
** i.e. these errors can be handled by the caller and recovered from
*/

E_CU0201_BAD_THREAD_STATE
"CUT cannot detach this thread from a shared buffer while the thread has a
state in CUT of %0d."

E_CU0202_READ_CELLS
"CUT cannot detach this thread from a shared buffer because it is the last
attached thread and there are still cells to be read."

E_CU0203_TERM_ATTACHED
"An attempt was made to terminate a thread in CUT while the thread was still
attached to buffers."

E_CU0204_ASYNC_STATUS
"An asynchronous status was signaled by a cooperating thread."

/*
** Warnings (not logged by cut)
*/
W_CU0301_THREAD_ATTACHED
"Thread is already attached to buffer."

W_CU0302_THREAD_NOT_ATTACHED
"Thread is not attached to buffer."

W_CU0303_UNREAD_CELLS
"Warning, detatching thread from a shared buffer while there are still cells
to be read."

W_CU0304_NO_BUFFERS
"There are no buffers in the CUT master buffer list."

W_CU0305_NO_CELLS_READ
"A request to CUT to read cells resulted in no cells being read because the
caller asked not to wait, and the buffer is empty."

W_CU0306_LESS_CELLS_READ
"CUT read less cells than requested because there were not enough cells in the
buffer, and the caller asked not to wait."

W_CU0307_NO_CELLS_WRITTEN
"A request to CUT to write cells resulted in no cells being written because the
caller asked not to wait, and the buffer is full."

W_CU0308_LESS_CELLS_WRITTEN
"CUT wrote less cells than requested because there was not enough room in the
buffer, and the caller asked not to wait."

W_CU0309_TOO_FEW_THREADS
"CUT was asked to wait for less than 2 threads to attach, no waiting
was done."
/*
** Informational messages
*/

I_CU0401_LAST_BUFFER
"Last qualifying buffer in CUT buffer list."

I_CU0402_THREAD_INITIALIZED
"A call was made to initialize a thread that has already been initialized."
