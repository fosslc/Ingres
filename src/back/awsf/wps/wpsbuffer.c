/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wpsbuffer.h>
#include <wsf.h>
#include <erwsf.h>
#include <wpsfile.h>

/*
**
**  Name: wpsbuffer.c - HTML buffer 
**
**  Description:
**		All functions of this file aims at providing tools to store HTML data 
**		cpmposed of the header and the html text.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Fixup compiler warnings on unix.
**      17-May-1999 (fanra01)
**          Modified the opening and reading of results, previously paged to
**          disk.  Replace WPSOpenFile with WCSOpen to specify a read mode
**          instead of a write mode.
**      27-Sep-1999 (fanra01)
**          Bug 98957
**          Change the re-open from WPSOpenFile to WCSOpen as being re-opened
**          for read.
**      05-Jun-2000 (fanra01)
**          Bug 101745
**          Correct heap corruption when reading results from a file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-sep-2000 (hanch04)
*	    Changed MEcopylng to MEcopy.
**/

GLOBALDEF i4  max_memory_block = 100;
GLOBALDEF i4  block_unit_size = 2048;

/*
** Name: WPSOpenFile() - 
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WPSOpenFile (	
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
	bool		status;

	if (buffer->file == NULL)
	{
		WCSRequest (
				0,
				NULL, 
				SYSTEM_ID, 
				NULL, 
				WSF_ICE_SUFFIX, 
				&buffer->file, 
				&status);
	}
	if (err == GSTAT_OK)
		err = WCSOpen (buffer->file, "w", SI_BIN, 0);
return(err);
}

/*
** Name: WPSCloseFile() - 
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WPSCloseFile (	
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;

	CLEAR_STATUS( WCSClose(buffer->file) );
	CLEAR_STATUS( WCSLoaded(buffer->file, TRUE) );
return(err);
}

/*
** Name: WPSReleaseFile() - 
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WPSReleaseFile (	
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
	WCSRelease(&buffer->file);
	buffer->file = NULL;
return(err);
}

/*
** Name: WPSHeaderAllocate() - 
**
** Description:
**      Check the current size of the block and allocate more memory if necessary
**
** Inputs:
**	WPS_PBUFFER	: buffer
**	u_i4		: needed length
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WPSHeaderAllocate ( 
	WPS_PBUFFER	 buffer, 
	u_i4		 length) 
{
	GSTATUS err = GSTAT_OK;
	char *tmp;
	i4 add = 0;

	add = (buffer->header_position + length) - buffer->header_size;
	if (add > 0)
	{
		tmp = buffer->header;
		buffer->header_size += length;
		err = G_ME_REQ_MEM(0, buffer->header, char, buffer->header_size);
		if (err == GSTAT_OK && tmp != NULL)
		{
			MECOPY_VAR_MACRO(tmp, buffer->header_position, buffer->header);
			MEfree((PTR)tmp);
		}
	}
return(err);
}

/*
** Name: WPSBlockAllocate() - 
**
** Description:
**      Check the current size of the block and allocate more memory if necessary
**
** Inputs:
**	WPS_PBUFFER	: buffer
**	u_i4		: needed length
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	31-Jul-2000 (hanje04)
**	    Changed MECOPY_VAR_MACRO to MEcopylng when copying 
**	    buffer->block to tmp to avoid data corruption when 
**	    buffer->block > MAXi2
*/
GSTATUS 
WPSBlockAllocate ( 
	WPS_PBUFFER buffer,
	u_i4		length) 
{
    GSTATUS err = GSTAT_OK;
    char *tmp;
    i4  add = 0;
    i4  size;

    add = (buffer->position + length) - buffer->size;
    if (add > 0)
    {
	size = buffer->size + (((add / buffer->block_size) + 1) * buffer->block_size);
	if (size > (max_memory_block * block_unit_size))
	{
	    if (buffer->file == NULL)
		err = WPSOpenFile(buffer);

	    if (err == GSTAT_OK)
	    {
		i4		count = 0;
		err = WCSWrite(buffer->position - 1, buffer->block, &count, buffer->file);
		if (err == GSTAT_OK)
		{
		    buffer->size = (max_memory_block * block_unit_size);
		    MEfree((PTR)buffer->block);
		    err = G_ME_REQ_MEM(0, buffer->block, char, buffer->size);
		    WPS_BUFFER_EMPTY_BLOCK(buffer);
		}
	    }
	}
	else
	{
	    tmp = buffer->block;
	    err = G_ME_REQ_MEM(0, buffer->block, char, size);
	    if (err != GSTAT_OK)
		buffer->block = tmp;
	    else
	    {
		if (tmp != NULL) 
		{
		    MEcopy(tmp, buffer->position, buffer->block);
		    MEfree((PTR)tmp);
		}
		else
		    WPS_BUFFER_EMPTY_BLOCK(buffer);
		buffer->size = size;
	    }
	}
    }
    return(err);
}

/*
** Name: WPSHeaderInitialize() - 
**
** Description:
**      Initialize the header part with starting value
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WPSHeaderInitialize (
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
	buffer->page_type = WPS_HTML_BLOCK;
	buffer->header_position = 0;
	buffer->header_size = 0;
	MEfree(buffer->header);
	buffer->header = NULL;

	err = WPSHeaderAllocate(buffer, 2);
	buffer->header[0] = '\n';
	buffer->header[1] = EOS;
	buffer->header_position = 2;
return(err);
}

/*
** Name: WPSBlockInitialize() - 
**
** Description:
**      Initialize the block part with starting value
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WPSBlockInitialize (	
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;

	buffer->block_size = block_unit_size;
	buffer->position = 0;
	buffer->total = 0;
	buffer->size = 0;
	MEfree(buffer->block);
	buffer->block = NULL;
	
	err = WPSBlockAllocate(buffer, 1);

	WPS_BUFFER_EMPTY_BLOCK(buffer);
return(err);
}

/*
** Name: WPSBlockAppend() - Concat a string to the HTML block
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**	char*		: string
**	u_i4		: length
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WPSBlockAppend (	
	WPS_PBUFFER buffer, 
	char* str, 
	u_i4 length) 
{
    GSTATUS err = GSTAT_OK;
    u_i4 available = 0;
    i4 count;

    if (length > 0)
    {
	buffer->total += length;
	err = WPSBlockAllocate(buffer, length);
	while (err == GSTAT_OK && length > 0)
	{
	    available = buffer->size - buffer->position;
			
	    if (available < length)
	    {
		available++;
		MEcopy(str, available, buffer->block + buffer->position - 1);
		err = WCSWrite(buffer->size, buffer->block, &count, buffer->file);
		length -= available;
		str += available;
		WPS_BUFFER_EMPTY_BLOCK(buffer);
	    }
	    else
	    {
		MEcopy(str, length, buffer->block + buffer->position - 1);
		buffer->position += length;
		length = 0;
		buffer->block[buffer->position - 1] = EOS;
	    }
	}
    }
    return(err);
}

/*
** Name: WPSMoveBlock() - Copy and Close the buffer
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: dest
**	WPS_PBUFFER	: src
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      27-Sep-1999 (fanra01)
**          Bug 98957
**          Change the re-open from WPSOpenFile to WCSOpen as being re-opened
**          for read.
*/
GSTATUS
WPSMoveBlock (	
	WPS_PBUFFER dest, 
	WPS_PBUFFER src) 
{
    GSTATUS err = GSTAT_OK;

    if (src->position > 1)
    {
	if (src->file != NULL)
	{
	    i4 count, read;

	    err = WCSWrite(src->position - 1, src->block, &count, src->file);
	    CLEAR_STATUS( WPSCloseFile(src) );
			
	    if (err == GSTAT_OK)
                err = WCSOpen( src->file, "r", SI_BIN, 0 );

	    if (err == GSTAT_OK)
	    {
		do 
		{
		    err = WCSRead(src->file, src->size, &read, src->block);
		    if (err == GSTAT_OK && read > 0)
			err = WPSBlockAppend(dest, src->block, read);
		}
		while (err == GSTAT_OK && read > 0);
		CLEAR_STATUS( WCSClose(src->file) );
		CLEAR_STATUS( WPSReleaseFile(src) );
	    }
	    src->file = NULL;
	}
	else
	    err = WPSBlockAppend(dest, src->block, src->position - 1);
    }
    WPS_BUFFER_EMPTY_BLOCK(src);
    return(err);
}

/*
** Name: WPSHeaderAppend() - Concat a string to the header block
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**	char*		: string
**	u_i4		: length
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
WPSHeaderAppend (	
	WPS_PBUFFER buffer, 
	char* str, 
	u_i4 length) 
{
	GSTATUS err = GSTAT_OK;

	if (buffer != NULL)
	{
		err = WPSHeaderAllocate(buffer, length); 
		if (err == GSTAT_OK)
		{
			MEcopy(str, length, buffer->header + buffer->header_position - 2);
			buffer->header_position += length;
			buffer->header[buffer->header_position-2] = '\n';
			buffer->header[buffer->header_position-1] = EOS;
		}
	}
return(err);
}

/*
** Name: WPSBufferGet() - Close and return information
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      17-May-1999 (fanra01)
**          Modified the opening and reading of results, previously paged to
**          disk.  Replace WPSOpenFile with WCSOpen to specify a read mode
**          instead of a write mode.
**      05-Jun-2000 (fanra01)
**          When reading from file into buffer ensure that the buffer is
**          large enough.
*/
GSTATUS 
WPSBufferGet (
    WPS_PBUFFER buffer,
    u_i4        *read,
    i4          *total,
    char*       *str,
    u_i4        keep)
{
    GSTATUS        err = GSTAT_OK;
    *read = 0;

    if (buffer != NULL)
    {
        *total = buffer->total;
        if (*str == NULL)
        {
            if (buffer->file != NULL)
            {
                i4    count = 0;

                err = WCSWrite(buffer->position - 1, buffer->block, &count,
                    buffer->file);
                if (err == GSTAT_OK)
                    err = WPSCloseFile(buffer);
                if (err == GSTAT_OK)
                {
                    LOINFORMATION locinfo;
                    i4 loflags = LO_I_SIZE;

                    /*
                    ** Get the length of the file
                    */
                    LOinfo( &buffer->file->loc, &loflags, &locinfo);

                    err = WCSOpen(buffer->file, "r", SI_BIN, 0);

                    /*
                    ** Set position to 1, to account for the EOS terminator
                    ** appended to the buffer, for length and position
                    ** calculations.
                    */
                    buffer->position = 1;
                    buffer->total = locinfo.li_size;
                    *total = buffer->total;
                    /*
                    ** if the file is bigger than the buffer allocate a
                    ** new buffer.
                    */
                    if ((u_i4)buffer->total > buffer->size)
                    {
                        i4 blocks;

                        blocks = buffer->total / buffer->block_size;
                        blocks += (buffer->total % buffer->block_size) ? 1 : 0;
                        buffer->size = blocks * buffer->block_size;
                        MEfree( buffer->block );
                        err = G_ME_REQ_MEM(0, buffer->block, char, buffer->size);
                        WPS_BUFFER_EMPTY_BLOCK(buffer);
                    }
                }
            }
            else
            {
                *read = buffer->position - 1;
            }
            *str = buffer->block;
        }
        else if (keep > 0)
        {
            MEcopy(buffer->block + (buffer->position - keep - 1),
               keep + 1,
               buffer->block);
            buffer->position = keep + 1;
        }

        if (err == GSTAT_OK && buffer->file != NULL)
        {
            err = WCSRead(buffer->file,
                  buffer->block_size - keep - 1,
                  (i4 *)read,
                  buffer->block + buffer->position - 1);
            buffer->position += *read;
            buffer->block[buffer->position - 1] = EOS;
        }

        if (*read == 0 && buffer->file != NULL)
        {
            CLEAR_STATUS( WPSCloseFile(buffer) );
            CLEAR_STATUS( WPSReleaseFile(buffer) );
            buffer->file = NULL;
        }
    }
    return(err);
}

/*
** Name: WPSFreeBuffer() - Clean the buffer
**
** Description:
**
** Inputs:
**	WPS_PBUFFER	: buffer
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WPSFreeBuffer (
	WPS_PBUFFER buffer) 
{
	GSTATUS err = GSTAT_OK;
	
	if (buffer != NULL)
	{
		if (buffer->file != NULL)
		{
			CLEAR_STATUS( WPSCloseFile(buffer) );
			CLEAR_STATUS( WPSReleaseFile(buffer) );
		}

		MEfree(buffer->header);
		buffer->header = NULL;
		MEfree(buffer->block);
		buffer->block = NULL;
	}
return(err);
}
