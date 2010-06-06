/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include <sicl.h>
#include <me.h>
#include <st.h>
#include <sl.h>

/**
**
**  Name: ULTRACE.C - Routines to associate values with trace bits
**
**  Description:
**      This file contains the routines which associate values with 
**      particular trace flags.  For all of these routines, some number
**	of value pairs is kept.  If an attempt is made to add more values
**	than that, an error is returned.  There is no LRU algorithm
**	employed by these routines.
**
**          ult_getval - retrieve (or select) values from the trace vector
**          ult_putval - place a value in the trace vector
**          ult_clrval - remove a value from the trace vector
**
**
**  History:    $Log-for RCS$
**      15-Apr-86 (fred)    
**          Created on Jupiter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Jun-2008 (kibro01)
**	    Add SC930 tracing controlled by ult_always_trace()
**      10-Dec-2008 (horda03) Bug 120571
**          Removed include of stdio.h added on 25-Jun-2008.
**          Using LO functions to manipulate LOCATION variable
**          becasue LOCATION's have different fields on different
**          platforms.
**	13-oct-2009 (maspa05) sir 122725
**          Make timestamp output of ult_print_tracefile conditional
**          In order to do this and for general tidiness
**          ult_print_tracefile now uses integer constants instead of string
**          for type parameter - SC930_LTYPE_PARM instead of "PARM" and so on
**       2-Dec-2009 (hanal04) Bug 122137
**          Change the last field separator for QUEL, QUERY, REQUEL and
**          REQUERY from ':' to '?'. This allows lexical parsers to
**          tokenize everything after the '?' as query text.
**	25-May-2010 (kschendel)
**	    Tighten up declarations a little, () -> (void).
**/

/*{
** Name: ult_getval -	get value from the array indicated in the vector
**
** Description:
**      This routine searches the vector give to find the values for a 
**      particular flag.  If they are present, they are returned, and 
**      a status of TRUE is returned.  Otherwise, FALSE is returned. 
**
**
** Inputs:
**      vector                          the trace vector in question
**      bit                             which trace flag we are looking for
**      v1, v2                          pointers to where to put the values
**
** Outputs:
**      *v1, *v2                        filled with the values as above
**	Returns:
**	    bool
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**	15-Apr-86 (fred)
**          Created on Jupiter.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	17-Feb-2004 (schka24)
**	    Don't return values if the caller v1/v2 pointers are NULL.
**	    It's probably a call buried in a ult-check-macro usage
**	    and the caller doesn't expect values.
**	    (There are a bunch of scf calls this way, easier to fix here.)
*/
bool
ult_getval( ULT_TVECT *vector, i4  bit, i4 *v1, i4 *v2 )
{
    i4		    i;

    for (i = 0; i < vector->ulvao; i++)
    {
	if (vector->ulvp->val_elem[i].elem_flag == bit)
	    break;
    }
    if (i >= vector->ulvao)
    {
	return(0);
    }
    else
    {
	/* Just in case the caller doesn't bother with the result
	** pointers.  (My problems with this were probably a build
	** screw-up, but SCF *does* do ult-check-macro's without any
	** int result pointers, so this is safety.)  (schka24)
	*/
	if (v1 != NULL)
	    *v1 = vector->ulvp->val_elem[i].elem_v1;
	if (v2 != NULL)
	    *v2 = vector->ulvp->val_elem[i].elem_v2;
	return(1);
    }
}

/*{
** Name: ult_putval	- put a value in the list
**
** Description:
**      This routine searches the value list for a free space. 
**      If a free space is found, then the values are placed there. 
**      (If a space is found which currently is holding values for this
**      flag, these values are replaced.) 
**      If there is no space, an FALSE is returned. 
**      Otherwise, the space is claimed and TRUE is returned.
**
** Inputs:
**      vector                          the trace vector on which to operate
**      flag                            which flag to operate on.
**      v1, v2                          the values in question
**
** Outputs:
**      *vect                           is updated.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Apr-86 (fred)
**          Created on Jupiter.
*/
DB_STATUS
ult_putval( ULT_TVECT *vector, i4  flag, i4 v1, i4 v2 )
{
    i4		i;


    for (i = 0; i < vector->ulvao; i++)
    {
	if ((vector->ulvp->val_elem[i].elem_flag == flag)
	    || (vector->ulvp->val_elem[i].elem_flag == 0))
	    break;
    }
    if (i >= vector->ulvao)
    {
	/* first unset the bit set by the macro, then return false */

	vector->ulbp->btm_elem[flag/ULBPL] &= ~(ULBTM(flag));
	return(E_DB_ERROR);
    }
    else
    {
	vector->ulvp->val_elem[i].elem_v1 = v1;
	vector->ulvp->val_elem[i].elem_v2 = v2;
	vector->ulvp->val_elem[i].elem_flag = flag;
	return(E_DB_OK);
    }
}

/*{
** Name: ult_clrval	- remove values from the list
**
** Description:
**      This routine removes values from the list. 
**      If they are not present, this is a NO-OP.
**
** Inputs:
**      vector                          trace vector
**      flag                            which flag to remove values for
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Apr-86 (fred)
**          Created on Jupiter.
*/
DB_STATUS
ult_clrval( ULT_TVECT *vector, i4  flag )
{
    i4	    i;


    for (i = 0; i < vector->ulvao; i++)
    {
	if (vector->ulvp->val_elem[i].elem_flag == flag)
	    break;
    }
    if (i <= vector->ulvao)
    {
	vector->ulvp->val_elem[i].elem_v1 = 0L;
	vector->ulvp->val_elem[i].elem_v2 = 0L;
    }
    return(E_DB_OK);
}


/*{
** Name: ult_always_trace	- Is SC930 tracing switched on?
**
** Description:
**      Returns TRUE if SC930 has been set ("set trace point sc930 1")
**
** Inputs:
**      None
**
** Outputs:
**	Returns:
**	    TRUE if SC930 tracing is on, FALSE otherwise.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
*/

static bool always_tracing = FALSE;
static i4 ult_pid = 0;
bool
ult_always_trace(void)
{
      return (always_tracing);
}


/*{
** Name: ult_set_always_trace	- Set whether SC930 tracing is on
**
** Description:
**      Sets the always_tracing static variable for this server.
**
** Inputs:
**      value to set the always_tracing flag to
**      pid to use in the open filename
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-May-2009 (kibro01) b122062
**	    Log in dbms.log the fact that SC930 has switched on or off.
*/

void
ult_set_always_trace(bool value,i4 pid)
{
      if (value != always_tracing)
      {
	TRdisplay("SC930 tracing %s on %d (ver=%d)\n",value?"ENABLED":"DISABLED",pid,
			SC930_VERSION);
      }
      always_tracing = value;
      ult_pid = pid;
}

/*{
** Name: ult_set_trace_qep	- Set whether SC930 tracing outputs QEPs too
**
** Description:
**      Sets the trace_qep static variable for this server.
**
** Inputs:
**      value to set the trace_qep flag to
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/

static bool trace_qep = FALSE;
void
ult_set_trace_qep(bool value)
{
      if (value != trace_qep)
      {
	TRdisplay("SC930 QEP %s on\n",value?"ENABLED":"DISABLED");
      }
      trace_qep = value;
}

/*{
** Name: ult_trace_qep	- Return whether SC930 traces QEPs
**
** Description:
**      Returns TRUE if SC930 tracing also applies to QEPs
**
** Inputs:
**
** Outputs:
**	Returns:
**	    trace_qep (true/false)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/

bool
ult_trace_qep(void)
{
	return (trace_qep);
}


/*{
** Name: ult_set_tracefile	- Set location of tracefile
**
** Description:
**      Set the directory name to use for tracing
**
** Inputs:
**      value to set the directory value to
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
*/

static char trace_dir [MAX_LOC+1];

static LOCATION trace_loc;
static LOCATION *tracefile = NULL;

void
ult_set_tracefile(char *newvalue)
{
    STcopy(newvalue, trace_dir);

    LOfroms( PATH, trace_dir, &trace_loc);
    tracefile = &trace_loc;
}


/*{
** Name: ult_tracefile_loc	- Get location of tracefile
**
** Description:
**      Gets the directory for tracing, defaulting to II_SC930_LOC environment
**	variable or NULL if not set.
**
** Inputs:
**      value to set the directory value to
**
** Outputs:
**	Returns:
**	    directory LOCATION to use for SC930 tracing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
*/

static LOCATION *
ult_tracefile_loc(void)
{
	char *tmp = NULL;
	if (tracefile)
		return (tracefile);
	NMgtAt("II_SC930_LOC",&tmp);
	if (!tmp || (*tmp == EOS))
		return (NULL);
	ult_set_tracefile(tmp);
	return (tracefile);
}


/*{
** Name: ult_open_tracefile	- Open the session trace file
**
** Description:
**      Open the session tracefile
**
** Inputs:
**      session code to use (address of sscb block)
**
** Outputs:
**	Returns:
**	    file pointer to be passed into other ult functions
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-May-2009 (kibro01) b122062
**	    Make this function threadsafe by copying the LOCATION structure.
*/

void *
ult_open_tracefile(void *code)
{
	FILE *f;
	char fname[MAX_LOC+1];
	LOCATION *tname = ult_tracefile_loc();
	char fname_tmp[MAX_LOC+1];
	LOCATION loc_copy;

	if (!tname)
	{
		ult_set_always_trace(FALSE,0);
		return(NULL);
	}
	LOcopy(tname, fname_tmp, &loc_copy);
	STprintf(fname,"sess_%d_%x", ult_pid, code);

        LOfstfile( fname, &loc_copy );
         
	if (SIopen(&loc_copy,"a",&f))
		return ((void*)NULL);
	return ((void*)f);
}


/*{
** Name: ult_print_tracefile	- Print to trace file
**
** Description:
**      Print some stuff to the trace file
**
** Inputs:
**      file - file pointer returned by ult_open_tracefile()
**	type - text value to use to distinguish type of tracing
**	string - other text to append to tracing
**	prt_time - print timestamp?
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
*/

void
ult_print_tracefile(void *file, i2 type, char *string)
{
	FILE *f = (FILE*)file;
	char *type_str,type_str2[50];
	bool prt_timestamp=TRUE;
        char last_sep = ':';

	HRSYSTIME hr;
	
	if (!f)
	{
		ult_set_always_trace(FALSE,0);
		return;
	}

	switch (type)
	{
		case SC930_LTYPE_ADDCURSORID:
				type_str="ADD-CURSORID";
				break;
		case SC930_LTYPE_ENDTRANS:
				type_str="ENDTRANS";
				break;
		case SC930_LTYPE_COMMIT:
				type_str="COMMIT";
				break;
		case SC930_LTYPE_SECURE:
				type_str="SECURE";
				break;
		case SC930_LTYPE_ABORT:
				type_str="ABORT";
				break;
		case SC930_LTYPE_ROLLBACK:
				type_str="ROLLBACK";
				break;
		case SC930_LTYPE_ABSAVE:
				type_str="ABSAVE";
				break;
		case SC930_LTYPE_RLSAVE:
				type_str="RLSAVE";
				break;
		case SC930_LTYPE_BGNTRANS:
				type_str="BGNTRANS";
				break;
		case SC930_LTYPE_SVEPOINT:
				type_str="SVEPOINT";
				break;
		case SC930_LTYPE_AUTOCOMMIT:
				type_str="AUTOCOMMIT";
				break;
		case SC930_LTYPE_DDLCONCUR:
				type_str="DDLCONCUR";
				break;
		case SC930_LTYPE_CLOSE:
				type_str="CLOSE";
				break;
		case SC930_LTYPE_FETCH:
				type_str="FETCH";
				break;
		case SC930_LTYPE_DELCURSOR:
				type_str="DELETE CURSOR";
				break;
		case SC930_LTYPE_EXEPROCEDURE:
				type_str="EXECUTE PROCEDURE";
				break;
		case SC930_LTYPE_EXECUTE:
				type_str="EXECUTE";
				break;
		case SC930_LTYPE_TDESC_HDR:
				type_str="TDESC";
				prt_timestamp=FALSE;
				break;
		case SC930_LTYPE_TDESC_COL:
				type_str="COL";
				prt_timestamp=FALSE;
				break;
		case SC930_LTYPE_PARMEXEC:
				type_str="PARMEXEC";
				prt_timestamp=FALSE;
				break;
		case SC930_LTYPE_PARM:
				type_str="PARM";
				prt_timestamp=FALSE;
				break;
		case SC930_LTYPE_QEP:      
				type_str="QEP";
				prt_timestamp=FALSE;
				break;
		case SC930_LTYPE_BEGINTRACE:
				STprintf(type_str2,"SESSION BEGINS(%d)",SC930_VERSION);
				type_str=&type_str2[0];
				break;
		case SC930_LTYPE_QUEL:      
				type_str="QUEL";
                                last_sep='?';
				break;
		case SC930_LTYPE_QUERY:   
				type_str="QRY";
                                last_sep='?';
				break;
		case SC930_LTYPE_REQUEL: 
				type_str="REQUEL";
                                last_sep='?';
				break;
		case SC930_LTYPE_REQUERY:
				type_str="REQUERY";
                                last_sep='?';
				break;
		case SC930_LTYPE_UNKNOWN:
		default:
				type_str="UNKNOWN";
				prt_timestamp=FALSE;

	}
	if (prt_timestamp)
	{
	    TMhrnow(&hr);
	    SIfprintf(f,"%s:%ld/%ld%c%s\n",type_str,hr.tv_sec,hr.tv_nsec,last_sep,string);
	}
	else
	    SIfprintf(f,"%s%c%s\n",type_str,last_sep,string);

}


/*{
** Name: ult_close_tracefile	- Close trace file
**
** Description:
**      Close the trace file
**
** Inputs:
**      file - file pointer returned by ult_open_tracefile()
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
*/

void
ult_close_tracefile(void *file)
{
	FILE *f = (FILE*)file;
	if (!f)
	{
		ult_set_always_trace(FALSE,0);
		return;
	}
	SIclose(f);
}

