/*
**  Forward and/or External typedef/struct references.
**
**  History:
**	21-mar-90 (jrb)
**	    Created.
**	19-jun-90 (edwin)
**	    Add RMS_7NUMSTR_BAD, RMS_8NUMSTR_MIN, RMS_9NUMSTR_MAX.
*/

typedef struct _RMS_ADFFI_CVT	    RMS_ADFFI_CVT;


/*
**  This section defines indicies into the global Rms_adffi_arr array which
**  caches ADF function instance id's for later use.
*/
#define                 RMS_00CHA2DTE_CVT	    0
#define                 RMS_01FLT2MNY_CVT	    1
#define                 RMS_02CHA2MNY_CVT	    2
#define                 RMS_03MNY2VCH_CVT	    3
#define                 RMS_04DTE2VCH_CVT	    4

/*
[@group_of_defined_constants@]...
*/

/*
[@global_variable_reference@]...
*/


/*}
** Name: RMS_QUADWORD - This is for both a signed and unsigned 8-byte integer
**
** Description:
**
** History:
**	21-mar-90 (jrb)
**	    Created.
*/
typedef struct _RMS_QUADWORD
{
    u_i4		quad_chunk[2];
} RMS_QUADWORD;


/*}
** Name: RMS_OCTAWORD - This is for both a signed and unsigned 16-byte integer
**
** Description:
**
** History:
**	21-mar-90 (jrb)
**	    Created.
*/
typedef struct _RMS_OCTAWORD
{
    u_i4		octa_chunk[4];
} RMS_OCTAWORD;


/*}
** Name: RMS_GFLOAT - This is a VAX G-floating type
**
** Description:
**
** History:
**	28-mar-90 (jrb)
**	    Created.
*/
typedef struct _RMS_GFLOAT
{
    u_i4		gfloat_chunk[2];
} RMS_GFLOAT;


/*}
** Name: RMS_HFLOAT - This is a VAX H-floating type
**
** Description:
**
** History:
**	28-mar-90 (jrb)
**	    Created.
*/
typedef struct _RMS_HFLOAT
{
    u_i4		hfloat_chunk[4];
} RMS_HFLOAT;


/*}
** Name: RMS_VAXDATE - This is a VAX date type
**
** Description:
**
** History:
**	13-apr-90 (jrb)
**	    Created.
*/
typedef struct _RMS_VAXDATE
{
    u_i4		hfloat_chunk[2];
} RMS_VAXDATE;


/*}
** Name: RMS_ADFFI_CVT - ADF function instance id's for type conversions
**
** Description:
**	This typedef describes an ADF function instance for converting from
**	one INGRES type to another.  We cache these so that we can use ADF
**	to do conversions for us when necessary.
**
** History:
**	13-apr-90 (jrb)
**	    Created.
*/
typedef struct _RMS_ADFFI_CVT
{
    DB_DT_ID		rms_fromdt;
    DB_DT_ID		rms_todt;
    ADI_FI_ID		rms_fiid;
};


/*
** References to globals variables declared in other C files.
*/

GLOBALCONSTREF	char		Rms_numstr_map[][2];
GLOBALCONSTREF	char		Rms_deczon_map[];
GLOBALCONSTREF	char		Rms_decovr_map[];
/***GLOBALREF	RMS_ADFFI_CVT	Rms_adffi_arr[];***/

/*
**  Parameters of Rms_numstr_map[][], defined in gwrmsnum.roc
*/

#define	    RMS_7NUMSTR_BAD	'X'
#define	    RMS_8NUMSTR_MIN	0x30
#define	    RMS_9NUMSTR_MAX	0x7d

