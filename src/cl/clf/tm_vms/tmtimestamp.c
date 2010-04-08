/*
**    Copyright (c) 1986, 2003 Ingres Corporation
*/

#include    <compat.h>
#include    <cm.h>
#include    <gl.h>
#include    <tm.h>
#include    <st.h>
#include    <tmtz.h>
#include    <starlet.h>

/**
**
**  Name: TM.C - Time routines.
**
**  Description:
**      This module contains the TM cl routines.
**
**	    TMcmp_stamp - Compare timestamps.
**          TMget_stamp - Get timestamp.
**	    TMstamp_str - Convert stamp to string.
**	    TMstr_stamp - COnvert string to stamp.
**	    TMsecs_to_stamp - Convert seconds since 1-Jan-1970 to TM_STAMP
**
**
**  History:    $Log-for RCS$
**      20-jan-1987 (Derek)
**          Created.
**	28-jun-1989 (rogerk)
**	    In TMcmp_stamp, use unsigned local variables to do comparisons as 
**	    timestamps are unsigned values.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

#define DAYS_BTW_BASEDATES		40587UL
#define SECS_BTW_BASEDATES		(DAYS_BTW_BASEDATES * 86400UL)
#define HUNDRED_NANOSEC_ADJ		10000000L

/*{
** Name: TMcmp_stamp	- Compare two timestamps.
**
** Description:
**      Compare two time stamps.  Returns the logical subtraction
**	of the time stamps.  
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**      time1                           First timestamp.
**	time2				Second timestamp.
**
** Outputs:
**	Returns:
**	    -1			    time1 < time2
**	    0			    time1 == time2
**	    +1			    time1 > time2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1987 (Derek)
**          Created.
**	28-jun-1989 (rogerk)
**	    Use unsigned local variables to do comparisons as timestamps
**	    are unsigned values.  Otherwise we compare incorrectly if one
**	    value has the MSB set.
*/
i4
TMcmp_stamp(
TM_STAMP	*time1,
TM_STAMP	*time2)
{
    u_i4	    *t1 = (u_i4 *)time1;
    u_i4	    *t2	= (u_i4 *)time2;

    if (t1[1] != t2[1])
    {
	if (t1[1] < t2[1])
	    return (-1);
	return (1);
    }

    if (t1[0] != t2[0])
    {
	if (t1[0] < t2[0])
	    return (-1);
	return (1);
    }

    return (0);
}

/*{
** Name: TMget_stamp	- Return timestamp.
**
** Description:
**      Return a TM timestamp.
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**
** Outputs:
**      time                            Pointer to location to return stamp.
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1986 (Derek)
**	    Created.
*/
void
TMget_stamp(
TM_STAMP	*time)
{
    sys$gettim(time);
}

/*{
** Name: TMstamp_str	- Convert a stamp to a string.
**
** Description:
**      Convert a stamp to a string of the form:
**(	    dd-mmm-yyyy hh:mm:ss.cc
**)
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**      time                            Pointer to timestamp.
**
** Outputs:
**      string                          Pointer to output string.
**					Must be TM_SIZE_STAMP bytes long.
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1987 (Derek)
**          Created.
*/
void
TMstamp_str(
TM_STAMP	*time,
char		*string)
{
    struct
    {
	i4	    length;
	char	    *buffer;
    }		    desc = { 24, string };

    sys$asctim(0, &desc, time, 0);
    string[23] = 0;
}

/*{
** Name: TMstr_stamp	- Convert string to stamp.
**
** Description:
**      Convert a string of the form:
**(	    dd-mmm-yyyy hh:mm:ss.cc
**	    dd-mmm-yyyy:hh:mm:ss.cc
**)
**	to a timestamp.  Any partial suffix after the yyyy is legal.
**	(I.E  dd-mmm-yyyy means dd-mmm-yyy 00:00:00.00)
**      Note, this routine is for the sole use of DMF for 
**      auditing and rollforward.
**
** Inputs:
**      string                          String to convert.
**
** Outputs:
**      time                            Pointer to timestamp.
**	Returns:
**	    OK
**	    TM_SYNTAX
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-1987 (Derek)
**          Created.
**      23-mar-2004 (horda03) Bug 101206/INGSRV2769
**          Unless all parts of the time string is specified
**          VMS adds the current time to the timestamp, thus
**          the value returned may vary for different
**          invocations of the same time.
[@history_template@]...
*/
STATUS
TMstr_stamp(
char		*string,
TM_STAMP	*time)
{
    char	    time_buf[24];
    i4              status = OK;
    i4              i;
    char            *sp = string;
    char            *tp = time_buf;
    i4              dot = 0;
    i4              colon = 0;
    i4              space = 0;
    struct
    {
	i4	    length;
	char	    *buffer;
    }		    desc = { 0, time_buf };

    status = OK;

    /* Ignore leading whitespace */

    while (CMwhite(sp)) CMnext(sp);

    for (i = 0; i < 24 && *sp; i++, tp++)
    {
	*tp = *(sp++);

	if (*tp >= 'a' && *tp <= 'z')
        {
	    *tp += 'A' - 'a';
        }
	else if (*tp == ' ')
        {
	    if (!space)
            {
               space++;
            }
            else
            {
              /* Already processed a space, so
              ** ignore this space.
              */
              tp--;
              i--;
            }
           
            /* Trim additional whitespace */

            while (CMwhite(sp)) CMnext(sp);
        }
	else if (*tp == ':')
        {
            if (!space)
            {
	       *tp = ' ';
               space++;
	    }
            else
            {
               colon++;

               if ( (*(tp-1) == ' ') ||
                    (*(tp-1) == ':') )
               {
                  /* Need to insert a 0 */

                  *(tp++) = '0';
                  *tp     = ':';
                  i++;
               }
            }
        }
        else if (*tp == '.')
        {
           if ( dot || (colon != 2) )
           {
             /* Haven't specified a valid date */

             status = TM_SYNTAX;
             break;
           }

           if (*(tp-1) == ':')
           {
              /* Need to insert a 0 */

              *(tp++) = '0';
              *tp     = '.';
              i++;
           }

           dot++;
        }
    }

    while (!status)
    {
       if (!colon)
       {
          /* No time specified */

          if ( i > 15 )
          {
             /* Bad date specified. No room for time */

             break;
          }

          STprintf( tp, " 0:0:0.0");

          i += 8;
       }
       else if (colon != 2)
       {
          /* Bad date */

          break;
       }
       else if (!dot)
       {
          /* No millisecond specified, so add */


          if (*(tp-1) == ':')
          {
             /* Need to insert a 0 */

             *(tp++) = '0';
             i++;
          }

          *(tp++) = '.';
          *tp = '0';
          i += 2;
       }

       desc.length = i;
       status = sys$bintim(&desc, time);
       if (status & 1)
     	  return (OK);
    }
    return (TM_SYNTAX);
}

/*{
** Name: TMsecs_to_stamp - Convert seconds since 1-Jan-1970 to TM_STAMP
**
** Description:
**      Converts the number of seconds since 00:00:00 UTC, Jan. 1, 1970, to
**	a TM_STAMP timestamp. Note, this routine is for the sole use of DMF
**	for auditing and rollforward.
**
** Inputs:
**	secs		pointer to seconds to convert
**
** Outputs:
**	stamp		pointer to TM_STAMP structure
**
** Returns:
**	void
**
** Side Effects:
**	none
**
** History:
**	18-nov-2003 (abbjo03)
**	    Created.
*/
void
TMsecs_to_stamp(
i4		secs,
TM_STAMP	*stamp)
{
    PTR		tz_cb;

    if (TMtz_init(&tz_cb) == OK)
	secs += TMtz_search(tz_cb, TM_TIMETYPE_GMT, secs);

    *(__int64 *)stamp = ((__int64)secs + SECS_BTW_BASEDATES) *
			 HUNDRED_NANOSEC_ADJ;
    return OK;
}
