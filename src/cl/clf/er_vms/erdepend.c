/*
** Copyright (c) 1986, 2008 Ingres Corporation
**
*/

#include	<compat.h>
#include	<gl.h>
#include	<er.h>
#include	<lo.h>
#include	<me.h>
#include	<st.h>
#include	<fab.h>
#include	<rab.h>
#include	<ssdef.h>
#include	<starlet.h>
#include	<cs.h>	    /* Needed for "erloc.h" */
#include	"erloc.h"

/**
** Name:    ERdepend.c - The group of internal used machin dependent 
**			    routines in ER.
**
** Description:
**      This is a group of internal used machine dependent routines in 
**	ER compatibility library.
**	If you use these routine in your program, you must be included
**	'erloc.h' header file.
**
**	cer_close	close the file
**	cer_open	open the file
**	cer_getdata	get data from file
**	cer_dataset	read in block and set data in buffer
**	cer_sysgetmsg	get system message
**
** History:    
**      01-Oct-1986 (kobayashi) - first written
**	27-sep-88 (thurston)
**	    Incorporated a `retry' mechanism in cer_getdata() to account for
**	    the problem where one thread in the DBMS attempts to post a read to
**	    the error message file while another thread's previous read has not
**	    yet completed.  RMS does not allow a process to have two reads
**	    posted on the same channel.
**	21-oct-88 (thurston)
**	    Added semaphore stuff around the sys$read() loop in the VMS version
**	    of cer_dataset().   Also used the static buffer _buf instead of the
**	    local tbuf.  This is to avoid a similar problem here, as we had
**	    with cer_getdata(), where sys$read() occassionally decides to read
**	    *MORE* than you asked for.  Finally, corrected all of the casting
**	    for ME calls.
**	22-feb-89 (greg)
**	    CL_ERR_DESC member error must be initialized to OK
**	07-mar-89 (Mike S)
**	    Map fast memory file into memory
**	05-oct-89 (pasker)
**	    SYS$GETMSG requires a 2-byte integer for the length, but our
**	    calling sequence is for a 4-byte integer.  Initialize the return
**	    length to 0 to make sure that any junk in the high bytes are 
**	    cleared out before calling sys$getmsg.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	15-mar-2001 (kinte01)
**	    Use errnum instead of errno to avoid conflict now that
**	    errno.h is included in compat.h
**	05-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	13-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Replace uses of
**	    CL_SYS_ERR by CL_ERR_DESC.  Remove non-VMS code.
**/


/*}
** Name: DESCRIPTOR - A VMS string descriptor.
**
** Description:
**      A structure describing a variable length item in a VMS system
**	service call.
**
** History:
**     02-oct-1985 (derek)
**          Created new for 5.0.
*/
typedef struct _DESCRIPTOR
{
    int             desc_length;        /* Length of the item. */
    char            *desc_value;        /* Pointer to item. */
}   DESCRIPTOR;

#define IOBLOCK	512

static	char	_buf[4096];	/* OK because of semaphore */


/*{
**  Name: cer_close - close the file
**
**  Description:
**	This function closes the file whose internal file id is
**	parameter 'file'->w_ifi.
**
**  Input:
**	ERFILE	*file		    the pointer of file descriptor.
**
**  Output:
**	none.
**	
**	Return:
**	    STATUS  OK	
**		    FAIL
**
**  Side Effect:
**	none.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
*/

STATUS
cer_close(file)
ERFILE	*file;
{
    struct  FAB	fab;
    i4		st = OK;
    i4	status;

    if (file->w_ifi != 0)
    {
	/* close run time file */
	
	MEfill((u_i2)sizeof(fab), (u_char)0, (PTR)&fab);
	fab.fab$b_bid = FAB$C_BID;
	fab.fab$b_bln = FAB$C_BLN;
	fab.fab$w_ifi = file->w_ifi;
	status = sys$close(&fab);
	if ((status & 1) == 0)
	{
	    st = FAIL;
	}
	file->w_ifi = 0;
	file->w_isi = 0;
    }
    return (st);
}


/*{
**  Name: cer_open - open the file
**
**  Description:
**	This function opens the file whose name is filename and fors is 
**	ER_FASTSIDE or ER_SLOWSIDE.
**
**  Input:
**	LOCATION *loc		location pointer of file.
**	ERFILE *file		pointer of file descriptor
**
**  Output:
**	CL_ERR_DESC *err_code	error code structure
**	Return:
**	    STATUS  OK
**		    ER_BADOPEN
**
**  Side Effect:
**	changes to err_code.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
**	07-mar-1989 (Mike S)
**		Clear "mapped" flag
**
*/

STATUS
cer_open(loc,file,err_code)
LOCATION *loc;
ERFILE *file;
CL_ERR_DESC  *err_code;
{
    struct FAB fab;
    struct RAB rab;
    i4 status;
    char *filename;

    CL_CLEAR_ERR(err_code);

    LOtos(loc,&filename);

    MEfill((u_i2)sizeof(fab), (u_char)0, (PTR)&fab);
    MEfill((u_i2)sizeof(rab), (u_char)0, (PTR)&rab);
    fab.fab$b_bid = FAB$C_BID;
    fab.fab$b_bln = FAB$C_BLN;
    rab.rab$b_bid = RAB$C_BID;
    rab.rab$b_bln = RAB$C_BLN;
    fab.fab$b_fac = FAB$M_BRO | FAB$M_GET;
    rab.rab$l_rop = RAB$M_BIO;
    rab.rab$l_fab = &fab;
    fab.fab$l_fna = filename;
    fab.fab$b_fns = STlength(filename);

    /* Open the message file */

    status = sys$open(&fab);
    if ((status & 1) == 0)
    {
	err_code->error = status;
	return (ER_BADOPEN);
    }

    /* Connect the message file for relative block I/O */

    status = sys$connect(&rab);
    if ((status & 1) == 0)
    {
	err_code->error = status;
	sys$close(&fab);
	return (ER_BADOPEN);
    }

    /* Set the IFI and ISI to ERmulti table */

    file->w_ifi = fab.fab$w_ifi;
    file->w_isi = rab.rab$w_isi;
    file->mapped = FALSE;
    return (OK);
}


/*{
**  Name:   cer_getdata - get data from file.
**
**  Description:
**	This function gets data for 'size' from file to 'pbuf'.
**
**  Input:
**	char *pbuf	    pointer of buffer to set.
**	i4  size	    size of data to set.
**	i4  bkt	    baket number
**	ERFILE *file	    pointer of file descriptor
**
**  Output:
**	CL_ERR_DESC *err_code	error code structure
**	Return:
**	    STATUS	OK
**			ER_BADREAD
**
**  Side Effect:
**	Changes to err_code.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
**	29-sep-88 (thurston)
**	    Now uses the semaphoring routines, if they were supplied to
**	    ERinit(), in order to prevent more than one session having
**	    outstanding reads on the message file.
**	17-oct-1988 (jhw)  Fixed apparent VMS bug with 'sys$read()' overwriting
**		the input buffer.  Use an internal buffer, which should be the
**		size of the records for message files.
**	5/90 (bobm) make bucket longnat
*/

STATUS
cer_getdata(pbuf,size,bkt,file,err_code)
char		*pbuf;
i4		size;
i4		bkt;
ERFILE		*file;
CL_ERR_DESC	*err_code;
{
    i4		    status;
    STATUS	    ret_stat;
    ER_SEMFUNCS	    *sem_funcs;
    struct RAB	    rab;

    CL_CLEAR_ERR(err_code);

    MEfill((u_i2)sizeof(rab), (u_char)0, (PTR)&rab);
    rab.rab$b_bid = RAB$C_BID;
    rab.rab$b_bln = RAB$C_BLN;
    rab.rab$l_ubf = _buf;
    rab.rab$w_usz = size;
    rab.rab$l_bkt = bkt;
    rab.rab$w_isi = file->w_isi;

    /*
    **	Before reading, check to see if we need to do semaphore protection.
    **	If so, do semaphore protection first to prevent more than one
    **	outstanding read on the message file.
    */

    if (cer_issem(&sem_funcs))
    {
	/* Request the ER semaphore */
	_VOID_ (*(sem_funcs->er_p_semaphore))((i4)TRUE, &sem_funcs->er_sem);
    }

    /* Now it is safe to do the read */
    status = sys$read(&rab);
    if ((status & 1) == 0)
    {
	err_code->error = status;
	ret_stat = ER_BADREAD;
    }
    else
    {
	ret_stat = OK;
	MEcopy((PTR)_buf, (u_i2)size, (PTR)pbuf);
    }

    if ( sem_funcs != NULL )
    {	/* Release the ER semaphore */
	_VOID_ (*(sem_funcs->er_v_semaphore))(&sem_funcs->er_sem);
    }

    return (ret_stat);
}


/*{
** Name:  cer_dataset - read data to use on VMS
**
** Description:
**	This procedure reads data from message file on VMS. Offset,datasize,
**	pointer of dataarea and internal stream id is set this procedure, then
**	data is set to dataarea from message file for datasize. Why this 
**	procedure is created? Because read on VMS is fix length block, unlike
**	UNIX. On UNIX,there is 'lseek', but on VMS, not.
**
**  Input:
**	longnat offset	    offset of header and class message in messagefile.
**	i4 size	    the size of dataarea to want set
**	char *dataarea	    the pointer of dataarea
**	ERFILE *file	    pointer of file descriptor
**
**  Output:
**	Return
**	    OK		    scucess
**	    ER_BADREAD	    fail sys$read system call
**
**	Exception
**	    none.
**  
**  Sideeffect:
**	none.
**
**  History:
**	01-Oct-1986 (kobayashi) - first written
**	21-oct-88 (thurston)
**	    Added semaphore stuff around sys$read() loop in the VMS version.
**	    Also used the static buffer _buf instead of the local tbuf.  This
**	    is to avoid a similar problem here, as we had with cer_getdata(),
**	    where sys$read() occassionally decides to read *MORE* than you
**	    asked for.
*/

STATUS
cer_dataset(offset,size,parea,file)
i4		offset;
i4		size;
char		*parea;
ERFILE		*file;
{
    STATUS	    ret_stat = OK;
    ER_SEMFUNCS	    *sem_funcs;
    char	    *pt;
    i4		    movesize;
    i4		    bkt;
    i4	    status;
    struct RAB	    rab;

    /*
    **	decide bkt number and new offset from head of bucket in whitch there
    **	is message string. 
    */

    bkt = (offset / IOBLOCK) + 1;
    offset %= IOBLOCK;
    
    /*
    **	Before reading, check to see if we need to do semaphore protection.
    **	If so, do semaphore protection first to prevent more than one
    **	outstanding read on the message file.
    */

    if (cer_issem(&sem_funcs))
    {
	/* Request the ER semaphore */
	_VOID_ (*(sem_funcs->er_p_semaphore))((i4)TRUE, &sem_funcs->er_sem);
    }

    /* Now it is safe to do the reads ... hold semaphore until all done. */

    while (size > 0)
    {
	/*
	**  set struct rab
	*/

	MEfill((u_i2)sizeof(rab), (u_char)0, (PTR)&rab);
	rab.rab$b_bid = RAB$C_BID;
	rab.rab$b_bln = RAB$C_BLN;
	rab.rab$l_ubf = _buf;	    /* Read into the static _buf ... This is
				    ** OK since we are always semaphored.
				    */
	rab.rab$w_usz = IOBLOCK;
	rab.rab$w_isi = file->w_isi;
	rab.rab$l_bkt = bkt;

	/*
	**  read one block
	*/

	status = sys$read(&rab);
	if ((status & 1) == 0)
	{
	    ret_stat = ER_BADREAD;
	    break;
	}

	pt = _buf;
	pt += offset; /* scan offset */

	movesize = (size > IOBLOCK - offset) ? IOBLOCK - offset : size; /*min*/
	offset = 0; /* From next time, read from head of bucket */

	/* move data to area */
	MEmove((u_i2)movesize, (PTR)pt, (char)0, (u_i2)movesize, (PTR)parea);
	parea += movesize; /* inclement pointer of area */
	size -= movesize;  /* declement size */
	++bkt;		   /* inclement bucket number */
    }

    if ( sem_funcs != NULL )
    {	/* Release the ER semaphore */
	_VOID_ (*(sem_funcs->er_v_semaphore))(&sem_funcs->er_sem);
    }

    return (ret_stat);
}


/*{
**  Name: cer_sysgetmsg	    - get system message 
**
**  Description:
**	It uses to get the system message according to errno in ERlookup 
**	procedure.
**
**  Input:
**	errno		The error number to want the message.
**	msg_desc	The pointer of message descriptor.
**
**  Output:
**	length		The message length
**	err		VMS system error
**	Return:
**	    OK
**	    ER_TRUNCATED
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
**  Side effects:
**	none
**
**  History:
**	24-NOV-1986 (kobayashi)
**	    Created new for 5.0 KANJI
**
*/

STATUS
cer_sysgetmsg(errnum,length,msg_desc,err_code)
i4		errnum;
i4		*length;
DESCRIPTOR	*msg_desc;
CL_ERR_DESC	*err_code;
{
    i4 status;
    unsigned short len;

    CL_CLEAR_ERR(err_code);

    status = sys$getmsg(errnum, &len, msg_desc, 0xf, 0);
    *length = (i4)len;
    if ((status & 1) == 0)
    {
        err_code->error = status;
        if (status == SS$_BUFFEROVF)
        {
	    return (ER_TRUNCATED);
	}
	else
	{
	    return (ER_BADPARAM);
	}
    }
    return (OK);
}
