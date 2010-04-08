/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include <tm.h>

/**
** Name:	tmtz.h	- Define TMGL function externs
**
** Specification:
**
** Description:
**	Contains TMGL function externs
**
** History:
**	14-nov-1995 (nick)
**	    Created history and added II_DATE_CENTURY_BOUNDARY support.
**	06-jun-1995 (emmag)
**	    Problems with tzname and Microsoft Visual C++ 4.0.  Rename to
**	    ii_tzname.
**	15-aug-1997 (canor01)
**	    Add prototypes for TMtz_str() and TMtz_break().
**	28-aug-1997 (canor01)
**	    Clean up compiler complaint about redefinition of 'tzname'.
**	13-apr-06 (dougi)
**	    Added TMtz_isdst() to implement isdst() scalar function.
**/

# ifdef NT_GENERIC
# undef  tzname
# define tzname ii_tzname
# endif

#define TM_MAX_TIMECNT 370
#define TM_MAX_TZLBL   50
#define TM_MAX_TZTYPE  10
#define TM_MAX_TZNAME  48

typedef  i4                TM_TIMETYPE;     /* Input timetype */
#define  TM_TIMETYPE_GMT   0            
#define  TM_TIMETYPE_LOCAL 1

#define  TM_NO_TZNAME       (E_CL_MASK + E_TM_MASK + 0x05)
#define  TM_NO_TZFILE       (E_CL_MASK + E_TM_MASK + 0x06)
#define  TM_TZFILE_OPNERR   (E_CL_MASK + E_TM_MASK + 0x07)
#define  TM_TZFILE_BAD      (E_CL_MASK + E_TM_MASK + 0x08)
#define  TM_TZFILE_NOMEM    (E_CL_MASK + E_TM_MASK + 0x09)
#define  TM_TZLKUP_FAIL     (E_CL_MASK + E_TM_MASK + 0x0A)
#define  TM_PMFILE_OPNERR   (E_CL_MASK + E_TM_MASK + 0x0B)
#define  TM_PMFILE_BAD      (E_CL_MASK + E_TM_MASK + 0x0C)
#define  TM_PMNAME_BAD      (E_CL_MASK + E_TM_MASK + 0x0D)
#define  TM_PMVALUE_BAD     (E_CL_MASK + E_TM_MASK + 0x0E)
#define  TM_YEARCUT_BAD     (E_CL_MASK + E_TM_MASK + 0x10)

/* default II_DATE_CENTURY_BOUNDARY ( i.e. none ) */
# define TM_DEF_YEAR_CUTOFF	-1
/* max II_DATE_CENTURY_BOUNDARY */
# define TM_MAX_YEAR_CUTOFF	100


FUNC_EXTERN STATUS TMtz_init(PTR *cb);
FUNC_EXTERN STATUS TMtz_lookup(char *tz_name, PTR *cb);
FUNC_EXTERN STATUS TMtz_load(char *tz_name, PTR *cb);
FUNC_EXTERN i4 TMtz_search(PTR cb, TM_TIMETYPE tm_timetype, i4 tm_time);
FUNC_EXTERN STATUS TMtz_year_cutoff(i4 *year_cutoff);
FUNC_EXTERN VOID TMtz_str(SYSTIME *st, PTR cb, char *cp);
FUNC_EXTERN STATUS TMtz_break(SYSTIME *st, PTR cb, struct TMhuman *th);
FUNC_EXTERN i4 TMtz_isdst(PTR cb, i4 tm_time);


typedef struct _TM_TZINFO
{
    i4             gmtoff;               /* GMT offset */
    u_char         isdst;                /* DST or not */
    i2             abbrind;              /* Offset to Timezone lable */
} TM_TZINFO;


typedef struct _TM_TZ_CB
{
    PTR             next_tz_cb;             /* 
                                            ** PTR to next TZ table.  NULL if
                                            ** this is the last table.
                                            */
    char            tzname[TM_MAX_TZNAME];  /*
                                            ** TZ table name
                                            */
    i4		    timecnt;                /* 
					    ** If timecnt = 0, then we have
					    ** constant offset value (GMT8).
					    ** Otherwise, timecnt = number of 
					    ** intervals 
					    */
    TM_TZINFO       tzinfo[TM_MAX_TZTYPE];  /* 
					    ** Different types of timezone
					    ** settings.
					    */
    char            tzlabel[TM_MAX_TZLBL];
    char            *tm_tztype;            /* Pointer to the 1st element of 
					    ** type array.
					    */
    i4              *tm_tztime;            /* Pointer to the 1st element of
					    ** date array.
					    */
} TM_TZ_CB;









