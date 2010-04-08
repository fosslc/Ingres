/*
**	Copyright (c) 2004, 2010 Ingres Corporation
*/
#include <compat.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <pc.h>
#include <nm.h>
#include <cm.h>
#include <id.h>
#include <evset.h>
#include <me.h>
#include <gc.h>
#include <gl.h>
#include <iicommon.h>
#include <mu.h>
#include <pm.h>
#include <gca.h>

/*
**  Name: mkexcept - Problem database update interface
**
**  Description:
**      This module implements the mkexcept interface which allows updates
**      to be made to the iiexcept.opt file.
**
**      The program takes as its single parameter either the name of a file
**      containing details of updates to be made or the flag "-list" to list
**      the contents of the problem database in a form suitable for reinsertion
**
**      The file format is as follows:-
**
**              *<error ids>
**              <actions>
**
**      Where <error ids> is a list of error numbers (in hexadeciaml) separated
**      by commas. Ranges can be specified by separating the start and end with
**      hyphens.
**
**      The <actions> each appear on a separate line and are one of the
**      following:-
**
**              IGNORE           Ignore this error
**              DUMP             Produce a dump
**              DUMP_ON_FATAL    Produce a dump on fatal error
**              EXEC <script>    Execute the specified shell line
**
**      Any line starting with a hash is treated as a comment and ignored
**
**      This information is encoded into records of the form:-
**
**         <from><to><action><params>
**
**      Where <from> and <to> are 8 digit hex numbers, <action> is I for ignore,
**      D for dump and E for EXEC and <params> is only present for EXEC entries
**      and is the script to execute.
**
**  History:
**	22-feb-1996 -- Initial port to Open Ingres
**	01-Apr-1996 -- (prida01)
**			   Add sunOS define's
**      19-May-1998 -- (horda03)
**                       ported to VMS.
**      17-jun-1998 (horda03)
**          Allow any user with the MONITOR provilege to use this
**          utility.
**	01-jul-1998 (ocoro02)
**	    mkexcept needs DIAGLIB to link. This includes newly created
**	    chk_priv symbol. Added NEEDLIBS to create mingfile properly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Jun-2002 (thaju02)
**	    IDname() was revised to determine whether Ingres is
**	    part of a TNG installation, which was accomplished by
**	    calls to PMinit() and PMload(). If PMinit() is called
**	    once again, do not report PM_DUP_INIT. (B108079)
**    25-Oct-2005 (hanje04)
**        Add prototype for parse_range(), parse_action(), to_hex(),
**	  insert_entries(), list_iiexcept() and write_iiexcept().
**	  Also remove cast of arg 4 in ERlookup() call, both to prevent
**	  compiler errors with GCC 4.0 due to conflict with GCC 4.0
**    25-Jan-2009 (horda03) Bug 121811
**        Port to Windows. Clean up warnings.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
*/

/*
NEEDLIBS =      DIAGLIB
*/

static STATUS new_entry();
static STATUS new_action();
static VOID error() ;
static STATUS parse_range();
static STATUS parse_action();
static i4 to_hex();
static STATUS insert_entries();
static STATUS list_iiexcept();
static STATUS write_iiexcept();

STATUS chk_priv( char *user_name, char *priv_name );


main(argc,argv)
i4      argc;
char    **argv;
{
    LOCATION my_loc;
    FILE *iiexcept;
    FILE *input;
    char buffer[200];
    bool do_list=FALSE;
    char name[32];
    PTR name_ptr=name;
    STATUS status=OK;

    /* Check we are ingres */

    IDname(&name_ptr);

    status = PMinit();
    if ( (status != OK ) && (status != PM_DUP_INIT) )
       error ( status, NULL, 0, NULL);

    if ( (status = PMload( (LOCATION *) NULL, (PM_ERR_FUNC *) NULL)) != OK)
       error( status, NULL, 0, NULL );

    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, PMhost() );

    if ( (status = chk_priv( name, GCA_PRIV_MONITOR )) != OK)
       error( status, NULL, 0, NULL );

    /* Make sure we have enough parameters */

    if(argc!=2)
    {
        error(E_USAGE,NULL,0,0,0,0);
    }

    /* Check the parameter */

    if(!STcompare(argv[1],"-list"))
    {
        do_list=TRUE;
    }
    else if(argv[1][0]=='-')
    {
        error(E_UNKNOWN_FLAG,NULL,STlength(argv[1]),argv[1],0,0);
    }
    else
    {
        /* Open the steering file */

        if(LOfroms(PATH&FILENAME,argv[1],&my_loc) ||
           SIfopen(&my_loc,"r",SI_TXT,sizeof(buffer),&input))
        {
            error(E_OPEN_FAIL,NULL,STlength(argv[1]),argv[1],0,0);
        }
    }

    /* Open problem database file (if it exists) */

    if(NMloc(ADMIN,FILENAME,"iiexcept.opt",&my_loc)==OK &&
       SIfopen(&my_loc,"r",SI_TXT,sizeof(buffer),&iiexcept)==OK)
    {
        /* Read problem database into memory */

        u_i4 last_from=0;
        u_i4 last_to=0;

        while(SIgetrec(buffer,sizeof(buffer),iiexcept)==OK)
        {
            u_i4 from=0;
            u_i4 to=0;
            i4  c;

            /* Remove the newline and decode */

            buffer[STlength(buffer)-1]=EOS;
            for(c=0;c<8;++c)
                from=(from*16)+to_hex(&buffer[c]);
            for(c=8;c<16;++c)
                to=(to*16)+to_hex(&buffer[c]);

            /* Insert entries into in store copy of problem database */

            if(last_from!=from || last_to!=to)
            {
                insert_entries();
                new_entry(from,to);
                last_from=from;
                last_to=to;
            }

            new_action(buffer[16],buffer[16]=='E'?buffer+17:NULL);
        }

        insert_entries();

        SIclose(iiexcept);
    }

    if(!do_list)
    {
        /* Merge in entries from steering file */

        i4  line=0;

        while(SIgetrec(buffer,sizeof(buffer),input)==OK && status==OK)
        {
            buffer[STlength(buffer)-1]=EOS;
            line++;

            switch(buffer[0])
            {
                case '#':
                    /* A comment - ignore the line */
                    break;
                case '*':
                    /* Start of a new entry - insert the previous entries */
                    /* and create the new set                             */
                    insert_entries();
                
                    status=parse_range(buffer+1);
                    break;
                default:
                    /* An action - process it */

                    status=parse_action(buffer);
                    break;
            }
        }

        if(status!=OK)
        {
            switch(status)
            {
                case E_SYNTAX_ERROR:
                    error(status,NULL,sizeof(i4),(PTR)&line,
                        STlength(buffer),buffer);
                    break;
                case E_UNKNOWN_ACTION:
                    error(status,NULL,STlength(buffer),buffer,0,0);
                    break;
                default:
                    error(status,NULL,0,0,0,0);
                    break;
            }
        }

        insert_entries();

        SIclose(input);

        /* Create new problem database */

        if(NMloc(ADMIN,FILENAME,"iiexcept.opt",&my_loc)!=OK ||
           SIfopen(&my_loc,"w",SI_TXT,sizeof(buffer),&iiexcept)!=OK)
        {
            error(E_CREATE_FAIL,NULL,0,0,0,0);
        }

        write_iiexcept(iiexcept);

        SIclose(iiexcept);
    }
    else
    {
        list_iiexcept();
    }
}

/*{
**  Name: parse_range - Parse an error number range list
**
**  Description:
**      This routine parses an error number range list which is a list of error
**      number ranges separated by commas. Each entry in the list
**      can be a single number or two numbers separated by a hypen. All numbers
**      are in hexadecimal.
**
**      New entrys are created for each range found via a call of the new_entry
**      routine below. These will be actually inserted into the problem database
**      after their actions have been added (with new_action) via the
**      insert_entries routine
**
**  Inputs:
**      PTR buffer        Buffer to parse
**
**  Returns:
**     STATUS
**
**  History:
*/

static STATUS
parse_range(buffer)
PTR buffer;
{
    PTR pointer=buffer;
    STATUS status=OK;
#define STATE_START 0
#define STATE_VAL1  1
#define STATE_VAL2  2
    i4  state=STATE_START;
    i4  digit;
    u_i4 val1;
    u_i4 val2;

    while(*pointer && status==OK)
    {
        /* Ignore white space */

        if(CMwhite(pointer))
        {
           ++pointer;
           continue;
        }

        switch(state)
        {
            case STATE_START:
                if((digit=to_hex(pointer))!= -1)
                {
                    val1=digit;
                    state=STATE_VAL1;
                }
                else
                {
                    status=E_SYNTAX_ERROR;
                }
                break;
            case STATE_VAL1:
                if(*pointer==',')
                {
                    new_entry(val1,val1);
                    state=STATE_START;
                }
                else if((digit=to_hex(pointer))!= -1)
                {
                    val1=(val1*16)+digit;
                }
                else if(*pointer=='-')
                {
                   val2=0;
                   state=STATE_VAL2;
                }
                else
                {
                    status=E_SYNTAX_ERROR;
                }
                break;
            case STATE_VAL2:
                if(*pointer==',')
                {
                    new_entry(val1,val2);
                }
                else if((digit=to_hex(pointer))!= -1)
                {
                    val2=(val2*16)+digit;
                }
                else
                {
                    status=E_SYNTAX_ERROR;
                }
                break;
            default:
                SIprintf(
                   "mkexcept: internal error parse_range state %d\n",state);
                PCexit(2);
        }

        ++pointer;
    }

    if(status==OK)
    {
        switch(state)
        {
            case STATE_VAL1:
                new_entry(val1,val1);
                break;
            case STATE_VAL2:
                new_entry(val1,val2==0?0xffffffff:val2);
                break;
        }
    }

    return(status);
}

/*{
**  Name: parse_action - Parse action line
**
**  Description:
**      This routine parses an action line and calls new_action to add its
**      details to the appropriate entries. 
**
**      The following actions are recognised:-
**
**      IGNORE      Ignore this error
**      DUMP        Dump the process in error
**      DUMP_ON_FATAL        Dump the process on fatal error
**      EXEC script Execute a script
**
**  Inputs:
**      PTR buffer         Buffer containing line to parse
**
**  Returns:
**     STATUS
**
**  History:
*/

static STATUS
parse_action(buffer)
PTR buffer;
{
    PTR pointer=buffer;
    STATUS status=OK;

    /* Skip any leading white space */

    while(CMwhite(pointer))
        pointer++;

    if((i4)STlength(pointer) >= 13 &&
		!STbcompare(pointer,13,"DUMP_ON_FATAL",13,TRUE))
    {
        new_action('F',NULL);
    }
    else if((i4)STlength(pointer) >=4 && !STbcompare(pointer,4,"DUMP",4,TRUE))
    {
        new_action('D',NULL);
    }
    else if((i4)STlength(pointer) >= 4 && 
		!STbcompare(pointer,4,"EXEC",4,TRUE))
    {
        /* Skip white space before script */

        pointer+=4;

        while(CMwhite(pointer))
            pointer++;

        new_action('E',pointer);
    }
    else if((i4)STlength(pointer) >= 6 && 
		!STbcompare(pointer,6,"IGNORE",6,TRUE))
    {
        new_action('I',NULL);
    }
    else
    {
        status=E_UNKNOWN_ACTION;
    }

    return(status);
}

/*{
**  Name: to_hex - Convert an character digit to a number 0-15
**
**  Description:
**      This routine is used to convert a character representation of
**      a hex digit into a number in the range 0-15
**
**  Inputs:
**      char *pointer       Pointer to the digit to convert
**
**  Returns:
**      -1       Invalid hex digit
**     0-15      Value of hex digit
**
**  History:
*/

static i4
to_hex(pointer)
PTR pointer;
{
    i4  ret_val;

    switch(*pointer)
    {
       case '0':
           ret_val=0;
           break;
       case '1':
           ret_val=1;
           break;
       case '2':
           ret_val=2;
           break;
       case '3':
           ret_val=3;
           break;
       case '4':
           ret_val=4;
           break;
       case '5':
           ret_val=5;
           break;
       case '6':
           ret_val=6;
           break;
       case '7':
           ret_val=7;
           break;
       case '8':
           ret_val=8;
           break;
       case '9':
           ret_val=9;
           break;
       case 'a':
       case 'A':
           ret_val=10;
           break;
       case 'b':
       case 'B':
           ret_val=11;
           break;
       case 'c':
       case 'C':
           ret_val=12;
           break;
       case 'd':
       case 'D':
           ret_val=13;
           break;
       case 'e':
       case 'E':
           ret_val=14;
           break;
       case 'f':
       case 'F':
           ret_val=15;
           break;
       default:
          ret_val= -1;
    }

    return(ret_val);
}

/*}
**  Name: In store copy of problem database
**
**  Description:
**      The instore copy of the problem database is kept as a linked list
**      of error ranges each with an associated list of actions.
**
**      If the list of actions on an entry is NULL then the action for that
**      error range is IGNORE - we try and merge adjacent entrys with an
**      IGNORE action to make it easier to decide if an error should be 
**      ignored (The backend builds a lookup table to do this)
**      - this merging is done as the entries are written back to the problem
**        database
**
**      We keep two lists:- one the actual problem database, the second a list
**      of entries being built for insertion into the problem database.
**
**      When new entries are inserted into the problem database existing
**      entries may need to be split or deleted as follows:-
**
**          1-10        Insert 5       1-4,5,6-10
**          1-4,5,6-10  Insert 1-10    1-10
**
**  History:
*/

typedef struct IIACTION
{
    struct 	IIACTION *next;
    char 	type;
    PTR 	param;
} ACTION;

typedef struct IIENTRY
{
    struct IIENTRY *next;
    u_i4 from;
    u_i4 to;
    ACTION *actions;
} ENTRY;

static ENTRY *entry_list=NULL;
static ENTRY *iiexcept=NULL;
static bool match_actions();

/*{
**  Name: new_entry - Create a new range entry
**
**  Description:
**      This routine creates a new range entry for later insertion into the
**      problem database with the insert_entries routine
**
**      Before insertion entries are queued and all actions defined (via the
**      new_action routine) are applied to all entries in the queue.
**
**  Inputs:
**      u_i4 from       Starting error number
**      u_i4 to         Ending error number
**
**  History:
*/

static STATUS
new_entry(from,to)
u_i4 from;
u_i4 to;
{
    ENTRY *new;
    STATUS status;

    new=(ENTRY *)MEreqmem(0,sizeof(ENTRY),FALSE,&status);

    if(status==OK)
    {
        /* Add new entry to end of list */

        if(entry_list==NULL)
        {
            entry_list=new;
        }
        else 
        {
            ENTRY *last=entry_list;

            while(last->next!=NULL)
                last=last->next;

            last->next=new;
        }

        new->next=NULL;
        new->from=from;
        new->to=to;
        new->actions=NULL;
    }

    return(status);
}

/*{
**  Name: new_action - Add action to current entry list
**
**  Description:
**      This routine adds an action to all entries in the current entry list
**      - new actions are always added on the end of an existing action list
**        so they will be activated in the order the user expects
**
**      The IGNORE action is optimised out if another action is already present
**      (the IGNORE action is indicated by the action list on an entry being
**      empty)
**      
**  Inputs:
**      char  action    Type of action
**                      I   Ignore
**                      D   Dump
**                      E   Execute
**      PTR   param     Script to execute for execute (or NULL)
**
**  Returns:
**     STATUS
**
**  History:
*/

static STATUS
new_action(action,param)
char action;
PTR  param;
{
    ENTRY *entry=entry_list;
    STATUS status=OK;

    while(entry!=NULL && status==OK)
    {
        bool ignore=FALSE;

        if(action=='I')
        {
            /* An ignore action is ignored (if the ACTION list we end up */
            /* with is NULL then it has an IGNORE action)                */

            ignore=TRUE;
        }
        else if(action=='D')
        {
            /* Check for an existing dump action - we don't want two */

            ACTION *action=entry->actions;

            while(action && action->type!='D')
                action=action->next;

            if(action!=NULL)
            {
                /* If there was already one - ignore this one */

                ignore=TRUE;
            }
        }
	else if(action=='F')
	{
	     	/* Check for an existing dump action - we don't want two */
	  	ACTION *action=entry->actions;
	       	while(action && action->type!='F')
		action=action->next;
	    	if(action!=NULL)
		{
			/* If there was already one - ignore this one */
			ignore=TRUE;
		}
	}


        /* If we have to create a new entry - do so now */

        if(!ignore)
        {
            ACTION *new=(ACTION *)MEreqmem(0,sizeof(ACTION),FALSE,&status);

            if(status==OK)
            {
                ACTION *end=entry->actions;

                if(end==NULL)
                {
                    entry->actions=new;
                }
                else
                {
                    while(end->next!=NULL)
                        end=end->next;

                    end->next=new;
                }

                new->next=NULL;
                new->type=action;
                new->param=(param==NULL)?NULL:STalloc(param);
            }
        }

        entry=entry->next;
    }

    return status;
}

/*{
**  Name: insert_entries - Insert entries on entry list into problem database
**
**  Description:
**      This routine inserts the list of entries built by the new_entry and 
**      new_action routines into the instore copy of the problem database
**
**      This instore copy is kept in sorted order with no overlap in the number
**      ranges so inserting may require existing entries to be split or deleted
**
**  Returns:
**     STATUS
**
**  History:
*/

static STATUS
insert_entries()
{
    STATUS status=OK;

    /* If the database is empty - insert an entry covering the entire */
    /* error number range - this makes the logic much simpler         */

    if(iiexcept==NULL)
    {
        iiexcept=(ENTRY *)MEreqmem(0,sizeof(ENTRY),FALSE,&status);

        if(status==OK)
        {
            iiexcept->next=NULL;
            iiexcept->from=0;
            iiexcept->to=0xffffffff;
            iiexcept->actions=NULL;
        }
    }

    while(entry_list!=NULL && status==OK)
    {
        ENTRY *next=entry_list->next;
        ENTRY *problem=iiexcept;
        ENTRY *last=NULL;


        /* Find insertion point */

        while(problem!=NULL && 
              entry_list->from > problem->from &&
              entry_list->from > problem->to)
        {
            last=problem;
            problem=problem->next;
        }


        if(entry_list->from==problem->from && entry_list->to==problem->to)
        {
            /* Replace */


            /* Free old action list, add new, delete old entry */

            problem->actions=entry_list->actions;
            MEfree((PTR)entry_list);
        }
        else if(entry_list->from==problem->from &&
                entry_list->to < problem->to)
        {
            /* Insert before */

            if(last==NULL)
                iiexcept=entry_list;
            else
                last->next=entry_list;

            entry_list->next=problem;

            problem->from=(entry_list->to)+1;
        }
        else if(entry_list->from > problem->from &&
                entry_list->to==problem->to)
        {
            /* Insert after */

            entry_list->next=problem->next;
            problem->next=entry_list;
            problem->to=(entry_list->from)-1;
        }
        else if(entry_list->from >= problem->from &&
                entry_list->to <= problem->to)
        {
            /* Split required */

            ENTRY *split=(ENTRY *)MEreqmem(0,sizeof(ENTRY),FALSE,&status);


            if(status==OK)
            {
                split->next=problem->next;
                entry_list->next=split;
                problem->next=entry_list;

                split->to=problem->to;
                split->from=(entry_list->to+1);
                problem->to=(entry_list->from-1); 

                /* Copy action list from problem to split */

                split->actions=problem->actions;
            }
        }
        else 
        {
            /* Must be an overlapping replace - find end */

            ENTRY *first=problem;


            problem=first->next;

            while(problem->to < entry_list->to)
            {
                ENTRY *temp=problem->next;

                /* Free problem->actions */
                MEfree((PTR)problem);
                problem=temp;
            }

            /* Check for an exact overlap of the start segment */

            if(first->from == entry_list->from)
            {
                if(last==NULL)
                    iiexcept=entry_list;
                else
                   last->next=entry_list;
                MEfree((PTR)first);
            }
            else
            {
                /* Get rid of overlap */

                first->next=entry_list;
                first->to=(entry_list->from-1);
            }

            /* Check for an exact overwrite of the end segment */

            if(problem->to == entry_list->to)
            {
                entry_list->next=problem->next;
                MEfree((PTR)problem);
            }
            else
            {
                entry_list->next=problem;
                problem->from=(entry_list->to+1);
            }
            
        }

        entry_list=next;
    }

    return(status);
}

/*{
**  Name: list_iiexcept - List contents of in store problem database to screen
**
**  Description:
**      This routine lists the contents of the instore problem database to
**      stdout in a form suitable for reinserting after editting
**
**  Returns:
**     STATUS
**
**  History:
*/

static STATUS
list_iiexcept()
{
    ENTRY *problem=iiexcept;

    while(problem!=NULL)
    {
        ACTION *action=problem->actions;

        if(problem->from == problem->to)
            SIprintf("* %08x\n",problem->from);
        else
           SIprintf("* %08x-%08x\n",problem->from,problem->to);

        if(action==NULL)
        {
            SIprintf("IGNORE\n");
        }
        else
        {
            while(action!=NULL)
            {
                char *type;
                char *param="";

                switch(action->type)
                {
                    case 'D':
                        type="DUMP";
                        break;
                    case 'E':
                        type="EXEC";
                        param=action->param;
                        break;
		    case 'F':
			type="DUMP_ON_FATAL";
			break;
                    default:
                        error(E_UNKNOWN_TYPE,NULL,sizeof(i4),
                            &action->type,0,0);
                        PCexit(2);
                }

                SIprintf("%s %s\n",type,param);

                action=action->next;
            }
        }

        problem=problem->next;
    }

    return OK;
}

/*{
**  Name: write_iiexcept - Write in store problem database to file
**
**  Description:
**      This routine writes the instore problem database to a file
**
**      If two adjacent entries are found to have an IGNORE action they are
**      merged before being written
**
**  Inputs:
**      FILE *file      File to write to
**
**  Returns:
**     STATUS
**
**  History:
*/

static STATUS
write_iiexcept(file)
FILE *file;
{
    ENTRY *entry=iiexcept;
    STATUS status=OK;

    while(entry!=NULL && status==OK)
    {
        ACTION *action;
        i4  from=entry->from;

        /* Merge entries with identical action lists */

        while(entry->next!=NULL && match_actions(entry,entry->next))
           entry=entry->next;

        action=entry->actions;

        if(action==NULL)
        {

            SIfprintf(file,"%08x%08xI\n",from,entry->to);
        }
        else
        {

            while(action!=NULL && status==OK)
            {
                SIfprintf(file,"%08x%08x%c%s\n",from,entry->to,
                    action->type,action->param==NULL?"":action->param);
    
                action=action->next;
            }

        }

        entry=entry->next;
    }

    return(status);
}

/*{
**  Name: match_actions - Check if two entries have identical action lists
**
**  Description:
**     This routine checks two entries to determine if the actions to perform
**     on each are identical.
**
**  Inputs:
**     ENTRY *first        First entry
**     ENTRY *second       Second entry
**
**  Returns:
**     bool                TRUE they match
**                         FALSE they don't match
**
**  History:
*/

static bool
match_actions(first,second)
ENTRY *first;
ENTRY *second;
{
   ACTION *action1=first->actions;
   ACTION *action2=second->actions;

   while(action1!=NULL && action2!=NULL)
   {
      if(action1->type != action2->type)
         return(FALSE);

      if(action1->param !=NULL || action2->param!=NULL)
      {
         if(action1->param==NULL || action2->param==NULL)
            return(FALSE);

         if(STcompare(action1->param,action2->param))
            return(FALSE);
      }

      action1=action1->next;
      action2=action2->next;
   }

   if(action1!=NULL || action2!=NULL)
      return(FALSE);

   return(TRUE);
}

/*
**  Name: error - Report an error
**
**  Description:
**     This routine reports an error to stderr and terminates the
**     program
**
**  Inputs:
**     i4 msg_id  - The id to use in ERlookup call
**     CL_ERR_DESC err - Any reported CL error.
**     i4  len         - Length of parameter insert
**     PTR param       - Parameter insert for message
**
**  History:
*/
VOID static
error(msg_id,err,len1,param1,len2,param2)
STATUS msg_id;
CL_ERR_DESC *err;
i4  len1;
PTR param1;
i4  len2;
PTR param2;
{
	STATUS status=OK;
	i4 lang_code;
	CL_ERR_DESC err_desc;
        ER_ARGUMENT er_arg[2];
	char msg_buf[ ER_MAX_LEN];
	i4 msg_len;
	i4 length=0;

        /* Package up parameter insert */

        er_arg[0].er_size = len1;
        er_arg[0].er_value = param1;
        er_arg[1].er_size = len2;
        er_arg[1].er_value = param2;

	ERlangcode( ( char*) NULL, &lang_code);

	status = ERlookup( msg_id, CLERROR(msg_id) ? err : (CL_ERR_DESC *) 0,
		        ER_TEXTONLY, NULL, &msg_buf[length], 
                        ER_MAX_LEN,lang_code, &msg_len, &err_desc, 2, er_arg);
				
	if ( status)
	{
	    CL_ERR_DESC    junk;

	    STprintf(&msg_buf[length],
			"ERlookup failed to look up message %x ", msg_id);
	    length = STlength(msg_buf);

	    STprintf(&msg_buf[length], "(reason: ER error %x)\n",status);
	    length = STlength(msg_buf);
	    status = ERlookup( (i4) status,
				&err_desc,
				(i4) 0,
				NULL,
				&msg_buf[length],
				(i4) (sizeof(msg_buf) - length),
				(i4) lang_code,
				&msg_len,
				&junk,
				0,
				(ER_ARGUMENT *) NULL
			     );
	    if (status)
	    {
		STprintf(&msg_buf[length],
		    "... ERlookup failed twice:  status = %x", status);
		length = STlength(msg_buf);
	    }
	    else
		length += msg_len;
	}
	else
	    length += msg_len;

	/* Get system message text. */

	if ( err)
	{
	    msg_buf[length++] = '\n';

	    status = ERlookup(	(i4) 0,
				err,
				(i4) 0,
				NULL,
				&msg_buf[length],
				(i4) (sizeof(msg_buf) - length),
				(i4) lang_code,
				&msg_len,
				&err_desc,
				0,
				(ER_ARGUMENT *) NULL
			      );
	    if (status)
	    {
	        CL_ERR_DESC    junk;

		STprintf(&msg_buf[length],
			"ERlookup failed to look up system error ");
		length = STlength(msg_buf);

	        STprintf(&msg_buf[length],
		    "(reason: ER error %x)\n", status);
	        length = STlength(msg_buf);
	        status = ERlookup( (i4) status,
				    &err_desc,
				    (i4) 0,
				    NULL,
				    &msg_buf[length],
				    (i4) (sizeof(msg_buf) - length),
				    (i4) lang_code,
				    &msg_len,
				    &junk,
				    0,
				    (ER_ARGUMENT *) NULL
			         );
	        if (status != OK)
	        {
		    STprintf(&msg_buf[length],
		        "... ERlookup failed twice:  status = %x", status);
		    length = STlength(msg_buf);
	        }
		else
		    length += msg_len;
	    }
	    else
		length += msg_len;
	}

        SIfprintf(stderr,"%s\n",msg_buf);
        PCexit(FAIL);
}
