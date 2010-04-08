/*
**	Copyright (c) 2004 Ingres Corporation
*/
typedef struct
{
	int type;
	int length;	/* applies to pointers in dat item */
	DB_ANYTYPE dat;
} GP_PARM;

GLOBALREF GP_PARM **IIFR_gparm;
GLOBALREF i4  IIFR_maxpid;

# define MSG_MINROW 4
# define MSG_MINCOL 16

# define GP_INTEGER(I) ((I < 0 || I >= IIFR_maxpid) ? 0 : (IIFR_gparm[I])->dat.db_i4type)

# define GP_FLOAT(I) ((I < 0 || I >= IIFR_maxpid) ? 0.0 : (IIFR_gparm[I])->dat.db_f8type)

# define GP_STRING(I) ((I < 0 || I >= IIFR_maxpid) ? "" : (IIFR_gparm[I])->dat.db_cptype)
