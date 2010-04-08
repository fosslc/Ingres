/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddfcom.h>
#include <erddf.h>

/**
**
**  Name: ddfcom.c - Data Dictionary Services Facility Common Function
**
**  Description:
**		Memory, Error and trace management for DDF module
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**  27-May-98 (marol01)
**          Add tracing info.
**	29-Jul-1998 (fanra01)
**	    Add stdarg.h header for variable-arg functions.
**	    Fix compiler warning.
**      15-Apr-1999 (fanra01)
**          Add a test for length of data and offset to check.  This was
**          previously returning incorrect file types.
**      05-Jan-2000 (fanra01)
**          Bug 99959
**          Correct message define spelling.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-Nov-00 (fanra01)
**          Bug 103307
**          Pass correct length to ERlog as it performs a writes
**          to the errlog.log file.
**	16-May-2002 (hanch04)
**	    A SIZE_TYPE variable should be passed to MEsize, not i4
**/

GLOBALDEF struct _GSTATUS out_of_memory = { E_DF0001_OUT_OF_MEMORY, "" };
GLOBALDEF u_i4	 ddf_trace_level = 0;
GSTATUS GOutOfMemory() { return &out_of_memory; }
u_i4 GTraceLevel() { return ddf_trace_level; }

static  char*   DDFhostname = NULL;

/*
** Name: GAlloc()	- Allocate memory by checking the current block size
**
** Description:
**		That function check is the allocated block size is greater than the 
**		requested length. If Yes a new block will allocated
**
** Inputs:
**	PTR*		: pointer to the memory block (NULL if no current block)
**	u_nat		: length required
**
** Outputs:
**  PTR			: pointer with the request block length
**
** Returns:
**	GSTATUS		: E_OG0001_OUT_OF_MEMORY
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	29-Jul-1998 (fanra01)
**	    Cast argument to MEsize to be signed.
**	    Changed object type to void to match prototype.
*/

GSTATUS 
GAlloc(
	PTR 	*object, 
	u_i4	length,
	bool	preserved) 
{
    GSTATUS err = GSTAT_OK;
    SIZE_TYPE size = 0; 	

  if (*object)
		MEsize(*object, &size); 

  if (size < length) 
  { 
		char* ptr = NULL;
		
		err = G_ME_REQ_MEM(0, ptr, char, length);

		if (err == GSTAT_OK)
		{
			if (preserved == TRUE && size > 0)
				MECOPY_VAR_MACRO(*object, size, ptr);
			MEfree(*object);
			*object = ptr;
		}
  }
return(err);
}

/*
** Name: DDFStatusAlloc()	- Allocate memory and initialize the error number
**
** Description:
**
** Inputs:
**	DB_sTATUS	: error number
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTATUS
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
DDFStatusAlloc (
	DB_STATUS status)
{
  GSTATUS err;

  if (G_ME_REQ_MEM(0, err, struct _GSTATUS, 1) == &out_of_memory)
		err = &out_of_memory;
  else
		err->number = status;
return(err);
}

/*
** Name: DDFStatusInfo()	- set status information
**
** Description:
**
** Inputs:
**	DB_sTATUS	: error number
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTATUS
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
DDFStatusInfo (
	GSTATUS status,
	char *fmt,
	...
	)
{
  GSTATUS err = GSTAT_OK; 
	char *parm[20];
  char	*tmp;
  u_i4	length = 0;
	u_i4	counter = 0;
  va_list	ap;
	
  va_start( ap, fmt );
  tmp = fmt;
  while (tmp[0] != EOS && tmp[1] != EOS)
  {
		if (*tmp++ == '%')
		{
			if (*tmp == '%')
			{
				tmp++;
				length++;
			}
			else
			{
				parm[counter++] = va_arg( ap, char*);
				if (*tmp == '*')
					tmp++;
				switch (*tmp)
				{
				case 'x':
					length += NUM_MAX_ADD_STR;
					break;
				case 'd':
				case 'l':
				case 'f':
				case 'e':
				case 'g':
					length += NUM_MAX_INT_STR;
					break;
				case 'c':
					length++;
					break;
			  case 's':
 	        length += STlength(parm[counter-1]);
					break;
		    default:;
		    }
			}
		}
		else
	    length++;
  }
  va_end(ap);

  length += 10;

  err = G_ME_REQ_MEM(0, status->info, char, length);

  if (err == GSTAT_OK)
  {
		STprintf(status->info, fmt, 
								parm[0], 
								parm[1],
								parm[2], 
								parm[3],
								parm[4], 
								parm[5],
								parm[6], 
								parm[7],
								parm[8], 
								parm[9],
								parm[10], 
								parm[11],
								parm[12], 
								parm[13],
								parm[14], 
								parm[15],
								parm[16], 
								parm[17],
								parm[18], 
								parm[19]);
  }
  else
		DDFStatusFree(TRC_EVERYTIME, &status); 
return(err);
}

/*
** Name: DDFStatusFree() - set status information
**
** Description:
**
** Inputs:
**      DB_STATUS: error number
**
** Outputs:
**
** Returns:
**      GSTATUS: GSTATUS
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      28-May-98 (marol01)
**          Created.
**      09-Nov-98 (fanra01)
**          Replace hardcoded node with hostname.
**      22-Nov-00 (fanra01)
**          Pass actual length of buffer to ERlog.
*/

void
DDFStatusFree(
    u_i4 trace_level,
    GSTATUS *status)
{
    GSTATUS err = GSTAT_OK;

    if (*status != NULL && *status != &out_of_memory)
    {
        CL_ERR_DESC clError;
        i4         nMsgLen;
        char        szMsg[ERROR_BLOCK];

        char *info = ((*status)->info) ? (*status)->info : "";

        if (ERslookup (
            (*status)->number,
            NULL,
            0,
            NULL,
            szMsg,
            ERROR_BLOCK,
            -1,
            &nMsgLen,
            &clError,
            0,
            NULL) == OK)
        {
                    szMsg[nMsgLen] = EOS;
        }

        switch (trace_level)
        {
            case TRC_EVERYTIME:
            {
                SYSTIME stime;
                char    date[TM_SIZE_STAMP+1];

                u_i4   length = STlength(info) + 50;
                char*   buffer;

                TMnow(&stime);
                TMstr(&stime, date);

                length += (STlength(szMsg) + STlength(date));

                err = G_ME_REQ_MEM(0, buffer, char, length);
                if (err == GSTAT_OK)
                {
                    STprintf(buffer,
                        "%-8.8s::[                , 00000000]: %s %s %s",
                        DDFhostname,
                        date,
                        szMsg,
                        info);
                    length = STlength( buffer );
                    ERlog(buffer, length, &clError);
                    MEfree((PTR)buffer);
                }
            }
            default:
                G_TRACE(6)("%s %s", szMsg, info);
        }

        MEfree((PTR)(*status)->info);
        MEfree((PTR)(*status));
        *status = GSTAT_OK;
    }
}

/*
** Name: DDFTraceInit() - Initialize trace system
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**      GSTATUS: GSTATUS
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      3-Sep-98 (marol01)
**          Created.
**      09-Nov-98 (fanra01)
**          Add initialisation of hostname for tracing.
*/

void
DDFTraceInit(
    u_i4    level,
    char*    filename)
{
    CL_ERR_DESC cl_err;
    char hostname[MAX_LOC];

    ddf_trace_level = level;
    if (filename != NULL && filename[0] != EOS)
        TRset_file(TR_F_APPEND, filename, STlength(filename) + 1, &cl_err);
    GChostname(hostname, sizeof(hostname));
    DDFhostname = STalloc(hostname);
}

/*
** Name: DDFTrace() - trace text
**
** Description:
**
** Inputs:
**   char*	format
**	 ...		values
**
** Outputs:
**
** Returns:
**      GSTATUS: GSTATUS
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      3-Sep-98 (marol01)
**          Created.
*/

void
DDFTrace(
    char *format,
    ...)
{
	char *tmp = format;
	char *parm[20];
	va_list	ap;
	u_i4	counter = 0;

	va_start( ap, format );
	while (tmp[0] != EOS)
	{
		if (tmp[0] == '%')
		{
			if (tmp[1] == '%')
				tmp++;
			else
				parm[counter++] = va_arg(ap, char*);
		}
		tmp++;
	}
	va_end( ap );

	tmp = (char*) MEreqmem (0, STlength(format) + TM_SIZE_STAMP + 20, TRUE, NULL);
	if (tmp != NULL)
	{
		SYSTIME  stime;
		char date[TM_SIZE_STAMP + 1];

		TMnow(&stime);
		TMstr(&stime, date);
		STprintf(tmp, "ICE Trace: %s %s\n", date, format);
		TRdisplay(	tmp, 
								parm[0], 
								parm[1],
								parm[2], 
								parm[3],
								parm[4], 
								parm[5],
								parm[6], 
								parm[7],
								parm[8], 
								parm[9],
								parm[10], 
								parm[11],
								parm[12], 
								parm[13],
								parm[14], 
								parm[15],
								parm[16], 
								parm[17],
								parm[18], 
								parm[19]);
	}
	MEfree((PTR)tmp);
}

/*
**  structure for file extension determination
*/
typedef struct tagTYPEID
{
    i4     nOffset;                    /* offset in first buffer to compare */
    i4     nLen;                       /* length to compare                 */
    i4     nCmpType;                   /* type of compare                   */
# define FT_STR  0
# define FT_ESTR 1
# define FT_MEM  2
    char*   pTypeId;                    /* file format bytes to compare      */
    char*   pCat;                       /* extension for file                */
    char*   pExt;                       /* extension for file                */
}TYPEID, *PTYPEID;

static TYPEID FileHeadTable[] =
{
    0, 3, FT_MEM, "\x2E\x73\x6E\x64", "application",    "au",
    4, 4, FT_MEM, "\x6D\x6F\x6F\x76", "video",          "mov",
    4, 4, FT_MEM, "\x6D\x64\x61\x74", "video",          "mov",
    0, 4, FT_MEM, "\x47\x49\x46\x38", "image",          "gif",
    0, 4, FT_MEM, "\xFF\xd8\xff\xe0", "image",          "jpg",
    0, 4, FT_MEM, "\x23\x64\x65\x66", "application",    "xbm",
    0, 3, FT_STR, "ICE",              "application",    "octet-stream",
    8, 3, FT_STR, "AVI",              "video",          "avi",
    8, 3, FT_STR, "WAV",              "audio",          "wav",
    0, 2, FT_STR, "BM",               "image",          "bmp",
    0, 4, FT_ESTR,"HTML",             "application",    "htm",
    0, 2, FT_MEM, "\x0a\x05",         "image",          "pcx",
    0, 0, 0, "",NULL
};

/*
** Name: GuessFileType
**
** Description:
**      Attempts to guess the type of a file by examining the first
**      few bytes of the file, looking for the signatures of common
**      file types.
**
** Inputs:
**
** Outputs:
**
** Returns:
**      
**
** History:
**      14-Apr-1999 (fanra01)
**          Add length condition to check.  If the length is less or equal to
**          the offset the test fails.
*/

GSTATUS 
GuessFileType (
    char*   data,
    int     len,
    char**  category,
    char**  ext)
{
    int i;

    *ext = NULL;

    for (i=0; (FileHeadTable[i].pExt != NULL) && (*ext == NULL); i++)
    {
        switch (FileHeadTable[i].nCmpType)
        {
            case FT_STR:
                if ((len > FileHeadTable[i].nOffset) &&
                    (STbcompare (&data[FileHeadTable[i].nOffset],
                        len,
                        FileHeadTable[i].pTypeId, 0, TRUE) == 0))
                {
                    *category = FileHeadTable[i].pCat;
                    *ext = FileHeadTable[i].pExt;
                }
                break;
            case FT_ESTR:
                if ((len > FileHeadTable[i].nOffset) &&
                    (STstrindex (&data[FileHeadTable[i].nOffset],
                    FileHeadTable[i].pTypeId,
                    len,TRUE) != NULL))
                {
                    *category = FileHeadTable[i].pCat;
                    *ext = FileHeadTable[i].pExt;
                }
                break;
            case FT_MEM:
                if ((len > FileHeadTable[i].nOffset) &&
                    (MEcmp (FileHeadTable[i].pTypeId,
                    &data[FileHeadTable[i].nOffset],
                    FileHeadTable[i].nLen) == 0))
                {
                    *category = FileHeadTable[i].pCat;
                    *ext = FileHeadTable[i].pExt;
                }
                break;
            default:
                *category = NULL;
                *ext = NULL;
         }
    }
    return GSTAT_OK;
}
