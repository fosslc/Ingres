/*
** Copyright (c) 2007 Ingres Corporation
*/

#ifndef EQSQLDA_H_INCLUDED
#define EQSQLDA_H_INCLUDED

/**
+*  Name: eqsqlda.h - Defines the SQLDA and some useful macros.
**
**  Description:
** 	The SQLDA is a structure for holding data descriptions, used by 
**	embedded program and INGRES run-time system during execution of
**	dynamic SQL statements.
**
**  Defines:
**	IISQLDA_TYPE	- Macro to declare type or variable for SQLDA.
**	IISQLDA		- Actual SQLDA typedef that programs use.
**	Constants and type codes required for using the SQLDA.
**
**  History:
**	09-sep-1987	- Rewritten with size-tunable macros. (neil)
**	27-apr-1990	- Updated IISQ_MAX_COLS to 300. (barbara)
**	19-oct-1992	- Added new datatype codes for large objects, and 
**			  struct typedef (IISQLHDLR) for DATAHANDLER. (kathryn)
**	20-oct-1992	- Added datatype code for W4GL objects (Mike S)
**	17-sep-1993	- Added new byte datatype codes. (sandyd)
**      04-Dec-1998     (merja01)
**           Changed sqldabc from long to int for Digital UNIX.  Note,
**           this change had been previously sparse branched.
**      04-May-1999     (hweho01)
**           Changed sqldabc from long to int for ris_u64 (AIX 64-bit port).
**	19-apr-1999 (hanch04)
**           Changed sqldabc from long to int for everything
**	07-jan-2002	- Updated IISQ_MAX_COLS to 1024. (toumi01)
**	16-sep-2002 (abbjo03)
**	    Added IISQ_NCHR/NVCHR/LNVCHR_TYPE for NCHAR related types.
**      17-Nov-2003 (hanal04) Bug 111309 INGEMB113
**          Changed sqllen to unsigned to prevent overflow when
**          using NVARCHARs.
**	10-Feb-2005 (hanje04)
**	    BUG 113875
**	    C++ requires that function prototypes match function definitions
**	    even if the prototype doesn't declare any arguments. (C does't
**	    care). For C++ programs, define the data handler function pointer 
**	    in SQLHDLR as sqlhdlr(void *) and force users to always declare
**	    an argument even if it isn't used. (Documentation needs to be
**	    updated. Argument is still optional for regular C programs.
**	10-Dec-2006 (gupsh01)
**	    Added definitions for Ansi date/time datatypes.
**	11-Oct-2007 (kiria01) b118421
**	    Include guard against repeated inclusion.
**      18-nov-2009 (joea)
**          Add IISQ_BOO_TYPE.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      17-Aug-2010 (thich01)
**          Add geospatial types to IISQ types skipped 60 for SECL consistency.
**/

/*
** IISQLVAR - Single Element of SQLDA variable as described in manual.
*/
typedef struct sqlvar_ {
	short	sqltype;
	unsigned short 	sqllen;
	char	*sqldata;
	short	*sqlind;
	struct {
	    short sqlnamel;
	    char  sqlnamec[258];
	} sqlname;
} IISQLVAR;

/*
** IISQLDA_TYPE - Macro that declares an SQLDA structure object (typedef or
**		  variable) with tag 'sq_struct_tag' and object name
** 'sq_sqlda_name'.  The number of SQLDA variables is specified by
** 'sq_num_vars'.  The SQLDA structure object is defined in the manual.
**
**
** Usage Example:
**	static IISQLDA_TYPE(small_sq_, small_sqlda, 10);
**		Declares a static variable (small_sqlda) with 10 vars.
**
**	typedef IISQLDA_TYPE(xsq_, MY_SQLDA, 50);
**		Defines a type (MY_SQLDA) with 50 vars.
*/

# define	IISQLDA_TYPE(sq_struct_tag, sq_sqlda_name, sq_num_vars)	\
						\
    struct sq_struct_tag {			\
	char		sqldaid[8];		\
	int		sqldabc;		\
	short		sqln;			\
	short		sqld;			\
	IISQLVAR	sqlvar[sq_num_vars];	\
    } sq_sqlda_name

/*
** IISQLDA - Type needed for generic SQLDA allocation and processing.
*/
# define	IISQ_MAX_COLS	1024

typedef IISQLDA_TYPE(sqda_, IISQLDA, IISQ_MAX_COLS);


/*
** IISQLHDLR - Structure type with function pointer and function argument
** 	       for speicfying DATAHANDLER.
*/

typedef struct sq_datahdlr_
{
    char    *sqlarg;         /* optional argument to pass thru */
# ifdef __cplusplus
    int     (*sqlhdlr)(void *);    /* user-defined datahandler function */
# else
    int     (*sqlhdlr)();    /* user-defined datahandler function */
# endif
} IISQLHDLR;


/*
** Allocation sizes - When allocating an SQLDA for the size use:
**		IISQDA_HEAD_SIZE + (N * IISQDA_VAR_SIZE)
*/
# define	IISQDA_HEAD_SIZE 	16
# define	IISQDA_VAR_SIZE  	sizeof(IISQLVAR)

/*
** Define the precision and scale for packed decimal. High byte is
** the precision, low byte is the scale.
*/
# define	IISQL_PRCSN(len)	((len)/256)
# define	IISQL_SCALE(len)	((len)%256)
# define	IISQL_PACK(len, prec, scal)			\
		len = (prec)*256 + (scal)
# define	IISQL_UNPACK(len, prec, scal)			\
		prec = IISQL_PRCSN(len), scal = IISQL_SCALE(len)

/* Type codes */
# define	IISQ_DTE_TYPE   3	/* Date - Output */
# define	IISQ_MNY_TYPE   5	/* Money - Output */
# define	IISQ_DEC_TYPE   10	/* Decimal - Output */
# define        IISQ_LBIT_TYPE  16      /* Long Bit  - Input, Output */
# define	IISQ_CHA_TYPE	20	/* Char - Input, Output */
# define	IISQ_VCH_TYPE	21	/* Varchar - Input, Output */
# define        IISQ_LVCH_TYPE  22      /* Long Varchar - Output */
# define	IISQ_BYTE_TYPE	23	/* Byte - Input, Output */
# define	IISQ_VBYTE_TYPE	24	/* Byte Varying - Input, Output */
# define	IISQ_LBYTE_TYPE	25	/* Long Byte - Output */
# define	IISQ_NCHR_TYPE	26	/* Nchar - Output */
# define	IISQ_NVCHR_TYPE	27	/* Nvarchar - Output */
# define	IISQ_LNVCHR_TYPE 28	/* Long Nvarchar - Output */
# define	IISQ_INT_TYPE	30	/* Integer - Input, Output */
# define	IISQ_FLT_TYPE   31	/* Float - Input, Output */
# define	IISQ_CHR_TYPE   32	/* C - Not seen */
# define	IISQ_TXT_TYPE   37	/* Text - Not seen */
# define        IISQ_BOO_TYPE   38      /* Boolean - Input, Output */
# define        IISQ_HDLR_TYPE  46      /* IISQLHDLR type - Datahandler*/
# define	IISQ_TBL_TYPE   52	/* Table field - Output */
# define	IISQ_OBJ_TYPE   45	/* 4GL object - Output */
# define        IISQ_ADTE_TYPE   4      /* ANSI date type - Output */
# define        IISQ_TMWO_TYPE   6      /* time without time - Output */
# define        IISQ_TMW_TYPE    7      /* time with time - Output */
# define        IISQ_TME_TYPE    8      /* Ingres time - Output */
# define        IISQ_TSWO_TYPE   9      /* timestamp without - Output */
# define        IISQ_TSW_TYPE   18      /* timestamp with time - Output */
# define        IISQ_TSTMP_TYPE 19      /* Ingres timestamp - Output */
# define        IISQ_INYM_TYPE  33      /* interval year to month - Output */
# define        IISQ_INDS_TYPE  34      /* interval day to second - Output */


# define	IISQ_DTE_LEN    25  /* Date length */
# define	IISQ_ADTE_LEN   17  /* Ansi Date length */
# define	IISQ_TMWO_LEN   21  /* Time without time zone length */
# define	IISQ_TMW_LEN    31  /* Time with time zone length */
# define	IISQ_TME_LEN    31  /* Time with local time zone length */
# define	IISQ_TSWO_LEN   39  /* Timestamp without time zone length */
# define	IISQ_TSW_LEN    49  /* Timestamp with time zone length */
# define	IISQ_TSTMP_LEN  49  /* Timestamp with local time zone length */
# define	IISQ_INTYM_LEN  15  /* Interval year to month length */
# define	IISQ_INTDS_LEN  45  /* Interval day to second length */

# define	IISQ_GEOM_TYPE   56 /* Spatial types */
# define	IISQ_POINT_TYPE  57
# define	IISQ_MPOINT_TYPE 58
# define	IISQ_LINE_TYPE   59
# define	IISQ_MLINE_TYPE  61
# define	IISQ_POLY_TYPE   62
# define	IISQ_MPOLY_TYPE  63
# define	IISQ_NBR_TYPE    64
# define	IISQ_GEOMC_TYPE  65

#endif /* EQSQLDA_H_INCLUDED */
