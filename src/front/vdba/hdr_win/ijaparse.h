/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

#ifdef __cplusplus
extern "C"
{
#endif

/*
** Name: outputline	- Gives the required line in the output.
**
** Description:
**      This routine returns the 'n'th line of the output. It will be used for 
**	parsing the output line by line. It will return the string corresponding to 
**	line n.
**
** Inputs/Globals:
**	  n			The line which you want to see in the output.
**    p			The char* that is generally the output of the auditdb command.
**
** Outputs/Globals:
**	Returns:
**	    String			This string corresponds to line n.
**	Errors:
**		Returns null on error.
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**		None
**
** History:
**      03-Mar-2004 (lakvi01)
**	    Created
**/
char* outputline(int line_no);

/*}
** Name:  ijaTxType - Holds the enum data that could be types of transactions.
**
** Description:
**      This enum contains all the values that describe the type of transaction detail.  
**
** History:
**      17-Mar-2004 (lakvi01)
**          Created.
*/
typedef enum 
{
	ijaT_UNKNOWN = 0, 
	ijaT_DELETE, 
	ijaT_INSERT, 
	ijaT_BEFOREUPDATE, 
	ijaT_AFTERUPDATE,
	ijaT_CREATETABLE,
	ijaT_ALTERTABLE,
	ijaT_DROPTABLE,
	ijaT_RELOCATE,
	ijaT_MODIFY,
	ijaT_INDEX
} ijaTxType;

/*}
** Name:  ParsedValues - Holds the data that was obtained from parsing.
**
** Description:
**      This structure has the data that is obtained from parsing. It is used later to
**	add in the list holding the parsed values.  
**
** History:
**      17-Mar-2004 (lakvi01)
**          Created.
*/
typedef struct _ParsedValues
{
	char csTransact[100];
	char csLSNBegin[100];
	char csLSNEnd[100];
	char csTransUser[100];
	char csTransBeginTime[100];
	char csTransEndTime[100];

	bool bCommitted;
	bool bAllJournaled;
	bool bIncludePartialRollBack;
	bool bDetailedFound;
	bool bDetailCollected;

	unsigned long l1, l2;
	unsigned long l_lsn_high;
	unsigned long l_lsn_low;
	unsigned long l_lsn_end_high;
	unsigned long l_lsn_end_low;
	unsigned long l_lsn_begin_high;
	unsigned long l_lsn_begin_low;
	unsigned long l_tid;
	unsigned long l_tid_new;
	unsigned long lsn_tp_high;
	unsigned long lsn_tp_low;
	ijaTxType txStatement;
	char csTable[100];
	char csSchema[100];
	char csRecord[100];
	char csBeforeRecord[100];
	char csAfterRecord[100];
}ParsedValues;

/*}
** Name:  item - This struct is the basic link in the linked list.
**
** Description:
**      This structure holds two things
**		a) The ParsedValues structure that has the values that were obtained from parsing.
**		b) The pointer to the next item.
**
** History:
**      17-Mar-2004 (lakvi01)
**          Created.
*/ 
typedef struct list_element
{
	ParsedValues p1;
	struct list_element* next;
}item;

/*{
** Name: ija_parse_transactions	- Parses the auditdb output by looking for transactions.
**
** Description:
**      This routine parses the output for transactions and various other transactions related
**	information. (LSN, user etc). It also understands different types of transactions like committed
**	rolled back transactions etc.
**
** Inputs/Globals:
**		&outputline(int line_no)	The address of outputline() function.
**		**item						The pointer to the linked list.
**		&buff						Address of the place where the function can write error messages.
**		int							The size of the above buffer.
** Outputs/Globals:
**	Returns:
**	    Boolean			Indicating whether the parsing was succesful or not.
**	Errors:
**		/*Have to write this
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**		Will write error messages into the address provided by the caller in case of parsing errors.
**
** History:
**      03-Mar-2004 (lakvi01)
**	    Created
**/

bool ija_parse_transactions(char* (*ptr2outputline) (int), item** h, char* buff_address, int sz_buff);

/*
** Name: free_up_memory	- Frees up the memory of the linked list.
**
** Description:
**      This routine frees up the memory used by the linked lists. This is important to avoid memory 
**	overflows. It is the responsibility of the caller to call this function at relevant places and free
**	up memory.
**
** Inputs/Globals:
**	  item**	Pointer to the linked list.
** Outputs/Globals:
**	Returns:
**	    Void
**	Errors:
**		None
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**		None
**
** History:
**      20-Mar-2004 (lakvi01)
**	    Created
**/
void free_up_memory(item** head);

#ifdef __cplusplus
}
#endif