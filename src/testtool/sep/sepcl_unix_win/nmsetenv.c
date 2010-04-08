#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <me.h>
#include <er.h>

#define nmsetenv_c

#include <sepdefs.h>
#include <fndefs.h>
/*
**  NMsetenv.c
**
**  History:
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**
**	14-sep-1995 (somsa01)
**	    On NT_GENERIC platforms, one is not allowed to change the 
**	    environment via the manipulation of the environ variable. Therefore,
**	    I added the use of the putenv() function on NT_GENERIC platforms.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
extern char **environ;

STATUS
NMsetenv (char *name,char *value)
{
	i4  namelen = 0 ;
	i4  i = 0 ;
	i4  num_envs = 0 ;
	bool found_it = FALSE ;
	char *newstring = NULL ;
	char **nenvp = NULL ;
#ifdef NT_GENERIC
	i4 status;
#endif


	newstring = SEP_MEalloc(SEP_ME_TAG_NODEL,
                                STlength(name)+STlength(value)+8, TRUE,
				(STATUS *) NULL);
	STprintf(newstring, ERx("%s=%s"), name, value);
#ifdef NT_GENERIC
	status = putenv (newstring);
	if (status == 0)
	  return(OK);
	else
	  return(FAIL);
#else
	namelen = STlength(name);
	for (i = 0 ; (environ[i]) && (*environ[i]) ; i++)
	{
		if (STbcompare(environ[i], STlength(environ[i]), name,
		  namelen, FALSE) == 0)
		{
			environ[i]= newstring;
			found_it = TRUE;
			break;
		}
		num_envs++;
	}
	if (!found_it)
	{
		nenvp = (char **) SEP_MEalloc(0, (num_envs+2)*sizeof(char *),
					      FALSE, (STATUS *) NULL);
		for (i = 0 ; i < num_envs ; i++)
			nenvp[i] = environ[i];
		environ = nenvp;
		environ[num_envs] = newstring;
		environ[num_envs+1] = NULL;
	}
	return(OK);
#endif /* NT_GENERIC */
} /* NMsetenv */

STATUS
NMdelenv (char *name)
{
#ifdef NT_GENERIC
	i4 status;
	char *newstring = NULL ;

	newstring = SEP_MEalloc(SEP_ME_TAG_NODEL,
                                STlength(name)+8, TRUE, (STATUS *) NULL);
	STprintf(newstring, ERx("%s="), name);
	status = putenv (newstring);
	if (status == 0)
	  return(OK);
	else
	  return(FAIL);
#else
	i4 namelen, i, j;

	namelen = STlength(name);
	for (i = 0 ; (environ[i]) && (*environ[i]) ; i++)
		if (STbcompare(environ[i], STlength(environ[i]), name,
		  namelen, FALSE) == 0)
		{
			if ( (environ[i+1]) && (*environ[i+1]) )
			{
				for (j = i+1 ; (environ[j]) && (*environ[j])
				  ; j++)
					;
				environ[i] = environ[j-1];
				environ[j-1] = NULL;
			}
			else
				environ[i] = NULL;
			return(OK);
		}
	return(FAIL);
#endif /* NT_GENERIC */
}  /* NMdelenv */
