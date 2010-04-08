/%
**	Copyright (c) 2007 Ingres Corporation
%/
/*
**  Preprocessor input for the OSL header file.
**
**  This file is passed through ccpp -- the C preprocessor with all CONFIG
**  symbols defined -- in order to create the deliverable OSL header file.
**  The ming rule then uses sed to strip all `@#' signs at beginnings of
**  lines, and to convert all strings `/@' and `@/' to the appropriate
**  C comment delimiters.  Real comments, like this one, and real 
**  preprocessor commands, are removed and processed, respectively, by the 
**  preprocessor.
**
**  History:
**	6-mar-1992 (jonb)
**		Adapted for preprocessing from original oslhdr.h.
**	22-apr-1992 (mgw)
**		Make ALIGN_RESTRICT dependent on it's definition in
**		bzarch.h by letting that value replace ALIGN_RESTRICT
**		here via ccpp and change DUMMY_ALIGN to ALIGN_RESTRICT
**		in the sed script in the MINGH.
**	16-sep-92 (deannaw)
**		Changed @ to % since AIX cpp does not like @'s.  Changed
**		MINGH to accept this change.
**	16-Apr-97 (merja01)
**		#ifdef correct sizes for axp_osf 64 bit i4 and long values
**		to int.  On axp_osf a long is 8 bytes and an int is 4 bytes.
**		This change originated from (kchin) and had been previously
**		sparse branched.
**      25-may-99 (hweho01)
**              Defined the correct sizes of i4 and i4 for
**              ris_u64 (AIX 64-bit port).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-sep-2000 (hanch04)
**	    i4 should be defined as an int.
**	7-Jan-2004 (schka24)
**	    Discover and update yet another instance of a DB_DATA_VALUE
**	    definition.  These things are like lice.
**	24-Sep-2007 (kiria01) b119029
**	    Remove NULL definition and replace with standard includes.
**      09-jan-2008 (smeke01) b119637
**          Added definitions for 8-byte integers (copied from compat.h).
**      12-Feb-2009 (horda03) Bug 121646
**          Include "string.h" to obtain prototype for memset().
*/
%#ifndef OSLHDR_H_INCLUDED
%#define OSLHDR_H_INCLUDED
%
%#include <stdio.h>
%#include <stdlib.h>
%#include <string.h>
%
%# define	DB_MAXNAME	24
%
/%
** The following definitions and typedefs should be changed for each
** installation to match what is in compat.h, targetsys.h and dbms.h.
%/
%# define	i1		char
%# define	u_i1		char
%# define	i2		short
%# define	i4		int
%# define	f4		float
%# define	f8		double
%# define	u_i4		unsigned int
%# ifdef LP64
typedef		long		i8;
typedef		unsigned long	u_i8;
%#else
typedef		long long 	i8;
typedef		unsigned long long	u_i8;
%# endif  /* LP64 */
%
%# define	DUMMY_ALIGN	ALIGN_RESTRICT
%
%# define	DB_CNTSIZE	2
%
typedef		char *		PTR;
typedef		unsigned short	u_i2;
typedef		i2		DB_DT_ID;
%
%#ifdef WSC
%#define _PTR_
%#else
%#define _PTR_	(PTR)
%#endif
%
typedef struct
{
	PTR		db_data;
	i4		db_length;
	DB_DT_ID	db_datatype;
	i2		db_prec;
	i2		db_collID;
} DB_DATA_VALUE;
%
typedef struct
{
	i4		db_offset;
	i4		db_length;
	DB_DT_ID	db_datatype;
	i2		db_prec;
} IL_DB_VDESC;
%
typedef struct
{
	i2	fe_version;
	i2	fe_i1_align;
	i2	fe_i2_align;
	i2	fe_i4_align;
	i2	fe_f4_align;
	i2	fe_f8_align;
	i2	fe_char_align;
	i2	fe_max_align;
	i2	adf_hi_version;
	i2	adf_lo_version;
	i2	adf_max_align;
} FRMALIGN;
%
typedef struct
{
	char		*fdv2_name;
	char		*fdv2_tblname;
	char		fdv2_visible;
	i2		fdv2_order;
	i4		fdv2_dbdvoff;
	i2		fdv2_flags;
	char		*fdv2_typename;
} FDESCV2;
%
typedef struct
{
	i4	fdsc_zero;
	i4	fdsc_version;
	i4	fdsc_cnt;
	FDESCV2	*fdsc_ofdesc;
} FDESC;
%
typedef struct {
    PTR	    qs_var;
    u_i2    qs_index;
    u_i1    qs_base;
    u_i1    qs_type;
} QRY_SPEC;
%
typedef struct
{
    PTR		qsg_value;
    i4		(*qsg_gen)();
} QSGEN;
%
typedef struct {
    DB_DATA_VALUE   *qv_dbval;
    u_i2	    qv_index;
    u_i1	    qv_base;
    char	    *qv_name;
} QRY_ARGV;
%
typedef struct _qdesc {
    char	*qg_new;
    char	*qg_name;
    u_i1	qg_fill[2];
    u_i1	qg_version;
    u_i1	qg_dml;
%
    u_i1	qg_mode;
%
    u_i4	qg_num[2];
%
    QRY_ARGV    *qg_argv;
%
    QRY_SPEC    *qg_tlist;
    QRY_SPEC    *qg_from;
    QRY_SPEC    *qg_where;
    QRY_SPEC    *qg_order;
%
    struct _qdesc   *qg_child;
    struct qgint    *qg_internal;
%
    i4              qg_bcnt;
    DB_DATA_VALUE   *qg_base[2];
} QDESC;
%
%# define	QI_BREAK	0101
%# define	QI_GET		0102
%# define	QI_START	0103
%# define	QI_NMASTER	0104
%
typedef struct qrytype
{
	i4		qr_zero;
	i4		qr_version;
	QRY_ARGV	*qr_argv;
	char		*qr_putparam;
	char		*qr_form;
	char		*qr_table;
	struct qrytype	*qr_child;
	QDESC		*qr_qdesc;
} QRY;
%
typedef struct
{
	char	*tf_name;
	char	*tf_column;
	i4	*tf_expr;
	i4	*tf_value;
} ABRTSTFLD;
typedef struct
{
	char		*pr_tlist;
	char		**pr_argv;
	ABRTSTFLD	*pr_tfld;
	QRY		*pr_qry;
} ABRTSOPRM;
typedef struct
{
	i4		abpvtype;
	char		*abpvvalue;
} OABRTSPV;
typedef struct
{
	i4		abpvtype;
	char		*abpvvalue;
	i4		abpvsize;
} ABRTSPV;
typedef struct
{
	i4		pr_zero;
	i4		pr_version;
	ABRTSOPRM	*pr_oldprm;
	ABRTSPV		*pr_actuals;
	char		**pr_formals;
	i4		pr_argcnt;
	i4		pr_rettype;
	i4		*pr_retvalue;
	i4		pr_flags;
} ABRTSPRM;
%
typedef struct
{
	char	*abprtable;
	char	*abprfld;
	char	*abprexpr;
	i4	abprtype;
} ABRTSPR;
%
typedef struct
{
    char	*qf_field;
    char	*qf_expr;
} ABRT_QFLD;
%
typedef struct
{
    i4		qu_type;
    char	*qu_form;
    i4		qu_count;
    ABRT_QFLD	*qu_elms;
} ABRT_QUAL;
%
char	*IIARtocToCstr();
%
%#endif /% OSLHDR_H_INCLUDED %/
