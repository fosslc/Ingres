/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<er.h>
#include      <cs.h>      /* Needed for "erloc.h" */
#include	"erloc.h"


/**
** Name:    ERget.c    - ERget program source code in CL
**
** Description:
**      This is a ERget program that can be used in order message 
**	when we will create internationalizetional INGRES.
**
**      ERget           Return string pointer corresponding to ER_MSGID.
**
** History:    
**      01-Oct-1986 (kobayashi) - first written
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-sep-2001 (somsa01)
**	    If we're using semaphores, use it here too. This way, we
**	    ERget() and ERslookup() in sync.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.
**/

/*
**			RUN TIME MESSAGE FILE STRUCTURE
**
** * Fast message file
**
**    +------ Max class number puls 1 (i4). Max class number is 169. (now)
*    /+--- class 0 ---+--- class 1 ---+
* +-/-+----+----+-----+----+----+-----+ --+
* |100|0002|0012|0x800|0004|0021|0x812|   |
* +---+----+----+---\-+----+----+-----+   |              class No.0 ~ 169
* |       	     +-------------------------- control record (ERCONTROL[170])
* |		        	      |   |              ERCONTROL is structure.
* |	   	            	      |   |		member is tblsize (i4)
* |		 	              |   +-- CONTROL part	  areasize (i4)
* |		 		      |   |	(2048byte)	  offset (i4).
* |			 	      |+-------- filler (i4) Null.
* |			 	      /   |
* |		    	         +---/+   |
* 0x800	       0x80C	         | \0 |   |
* +--------------+---------------+----+ --+
* |Help\0Append\0|Edit\0Blank\0EXit\0Q|   |
* +-----+--------+--------------------+   |
* |uit\0|  		              |   |
* +-----+-----------------------------+   |
* |			              |   +-- TEXT part
* |                                   |   |	(2048 * ? byte)
* |                                   |   |	all of message string.
* |                                   |   |	Terminater of each string is
* |			              |   |	Null.
* |			              |   |
* |			              |   |
* +-----------------------------------+ --+
*
* In above example, class 0 has two message (Help,Append) and class 1 has four
* message (Edit,Blank,EXit,Quit). And there are two class information (tblsize 
* -- number of message, areasize -- store size of a group of message, offset -- 
* distance from header of file to a group of message) in CONTROL part.
*
* Note: When user creates new Class number, the number must be the sequential 
*       order. Because it get the offset of class information to calculate.
*      
*       If user want to use the class number more than 169, they need to change
*       the structure of CONTROL part in fast message file. They need to change
*       the size of logical page (WRITE_SIZE in ercompile source code) according
*       to the maximum class number that you want to use. And then they need to 
*       change the class_size (CLASS_SIZE in ercompile source code) from 170 to
*       the maximum class number that you want to use. And they need to adjust
*       filler (Null) to do logical page boundary.
*       Then they don't need to change ER run time routines, but need to 
*       recompile ERcompile program.
*
* * Slow message file
*
*     +------ index_count (i4) Number of index entries.
*    /
* +-/--+----+----+----+---------------+ --+
* |0003|0050|0100|0160|               |   |
* +----+-\--+----+----+---------------+   |
* |       +-------------------------------------- index_key[511] (i4)
* |		        	      |   |        Highest message in
* |	   	            	      |   |        corresponding bucket.
* |		 	              |   +-- INDEX_PAGE part
* |		 		      |   |	(2048byte)
* |			 	      |   |
* |			 	      |   |
* |		    	              |   |
* |    	            	              |   |
* +--+--+----+--+---------------------+ --+
* |36|07|0000|15|QBF0000_SCUCESS-scuce|   |
* +--+--+--+-++-+--+--+---------------+   |
* |ss000|28|04|0001|12|QBF0001_HELP-He|   |
* +---+-+--+--+----+--+---------------+   |
* |lp0|			              |   +-- MESSAGE_ENTRY part
* +---+-------------------------------+   |	(2048 * ? byte)
* |                                   |   |   message_entry is structure.
* |                                   |   |     me_length (i2) Length of the
* |			              |   |        message entry.
* |			              |   |     me_text_length (i2) Length of the
* |			              |   |        test portion.
* +-----------------------------------+ --+     me_msg_number (i4) The message
*						    number for this message.
* 					        me_name_length (i2) Length of
*						    the name field.
*						me_name_text[me_name_length+me_
*						    text_length+1] (char) Area
*						    for message name
*						    and message text.
*
* In above example, there are two message in message_entry part. 
* (QBF0000_SCUCESS, QBF0001_HELP) And name and text in the me_name_text are 
* separated by '-'.
* 
* 
* 		DATA STRUCTURE OF FAST MESSAGE SYSTEM
* 
*		ERMULTI
* 	+---+---+---+---+---+
* 	|0x |   |   |   |   | -- *class (ER_CLASS_TABLE)
* 	|100|   |   |   |   |		The base pointer of class table.
* 	/---+---+---+---+---+
*      /|   |   |   |   |   | -- number_class (u_i4)
*     / | 3 |   |   |   |   |		The number of class table.
*    +	+---+---+---+---+---+
*    |	|   |   |   |   |   | -- language (i4)
*    |	| 0 |   |   |   |   |		Language internal id.
*    |	+---+---+---+---+---+
*    |	|   |   |   |   |   | -- deflang (bool)
*    |	| 1 |   |   |   |   |		The flag of default language.
*    |	+---+---+---+---+---+
*    |	|   |   |   |   |   | -- ERfile[2] (ERFILE)
*    |	|   |   |   |   |   |		Machine dependent file descriptor.
*    |	+---+---+---+---+---+
*    |
*    |		ER_CLASS_TABLE (English - language code 0)
*    +	  class 0   class 1   class 2  
*0x100\	+---------+---------+---------+
*      \|	  |	    |	      |
*	|   4     |	    |	   -------- number_message (u_i4)
*	|	  |	    |	      |		The number of message
*	+---------+---------+---------+
*	|	  |	    |	      |
*	| 0x140   |	    | 	   -------- **message_pointer (char)
*	|    /    |	    |	      |	   The pointer of message table header.
*	+---/-----+---------+---------+
*          /
*         /
*        +
*        |
*   0x140|  message table of class 0
*      	+|----+-----+-----+-----+
*       |     |     |     |     |  *(message_pointer) (char)
*	|0x300|0x305|0x309|0x310|
*	+-|---+-|---+-----+-----+
*	  |     |
*   0x300 +     +	In memory (Area of message)
*	+/-----/----------------------------------------------+
*	|Help\0Add\0Delete\0Modify\0 - - - - - - - - - - - - -|
*	|						      |
*	|						      |
*	|						      |
*	+-----------------------------------------------------+
*
* In above example, English language has three classes and class 0 has four
* message (Help,Add,Delete,Modify).
* If you want to search the message that is 'Help', in internally, first search
* class_table pointer from ERmulti according to language code (this case is 
* English 0), at next search message table pointer from class_table according to
* class number (this case is class 0), and the context in what is *(message 
* table pointer + message number) is the pointer of message that you want to 
* search.
**
*/

/*{
** Name:	ERget	- Return string pointer corresponding to ER_MSGID.
**
** Description:  
**	This procedure searches the binary message files (either fast or slow)
**	for the message corresponding to the ER_MSGID 'id' and returns
**	a pointer to a string which represents that 'id'.  The language
**	is determined by the setting of II_LANGUAGE for this process.
**
**	If the 'id' represents a slow message (high bit off), the return
**	string points to a static buffer containing the message, after
**	reading the message from the slow message file.  Unlike ERlookup,
**	the message name is not included in the returned string.
**
**	If the 'id' represents a fast message (high bit on), the return
**	string points to an in-memory version of the string, which will
**	stay in memory until the class of the 'id' is released either
**	through a call to ERrelease or a call to ERclose.
**
**	If the 'id' message cannot be found, a string is returned which
**	indicates an error condition.  If the message is longer than
**	ER_MAX_LEN bytes, it is silently truncated.
**
**	This call must not be used in server. Please use ERlookup in server.
**
** Inputs:
**	ER_MSGID id		key of message.
**
** Outputs:
**	Returns:
**		message string pointer (never NULL).
**	Exceptions:
**		return internal message and return from all error conditions:
**
**		(Please show cer_excep() for these definitions).
**
**		ER_DIRERR			if something cannot do LO call.
**		ER_NO_FILE			if something cannot do open 
**						on file
**		ER_BADREAD			if something cannot do read on
**						file
**		ER_NOALLOC			if something cannot do allocate
**		ER_MSGOVER			if message table is over flow
**
** Side Effects:
**	If slow message, this routine will overwrite the static buffer
**	for slow messages, so only one slow message can be used at a time.
**
** History:
**	01-Oct-1986 (kobayashi) - first written
**	21-sep-2001 (somsa01)
**	    If we're using semaphores, use it here too. This way, we
**	    ERget() and ERslookup() in sync.
*/
char *
ERget(ER_MSGID id)
{
    STATUS	status;
    char	*msg;
    ER_SEMFUNCS	*sems;

    /*
    **	In order to get message pointer 
    */

    if (cer_issem(&sems))
    {
	if (((sems->sem_type & MU_SEM) ?
	    (*sems->er_p_semaphore)(&sems->er_mu_sem) :
	    (*sems->er_p_semaphore)(1, &sems->er_sem)) != OK)
	{
	    sems = NULL;
	}
    }

    if((status = cer_get(id,&msg,ER_MAX_LEN,(i4)-1)) != OK)
    {
	/* Error status process */
	if (sems)
	{
	    if (sems->sem_type & MU_SEM)
		_VOID_ (*sems->er_v_semaphore)(&sems->er_mu_sem);
	    else
		_VOID_ (*sems->er_v_semaphore)(&sems->er_sem);
	}

	cer_excep(status,&msg);
    }

    if (sems)
    {
	if (sems->sem_type & MU_SEM)
	    _VOID_ (*sems->er_v_semaphore)(&sems->er_mu_sem);
	else
	    _VOID_ (*sems->er_v_semaphore)(&sems->er_sem);
    }

    /* return pointer setting message string */
    return(msg);
}
