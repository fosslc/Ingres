/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <adf.h>

# include <iiapi.h>
# include <api.h>
# include <apige2ss.h>
# include <apitrace.h>

/*
** Name: apige2ss.c
**
** Description:
**	This file defines the generic error to SQLSTATE functions.
**
**      IIapi_gen2IntSQLState  Convert from general error to integer
**                             SQLSTATE.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	16-Dec-94 (gordy)
**	    Enhance tracing.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	17-Jan-96 (gordy)
**	    Added environment handles.
**	 2-Oct-96 (gordy)
**	    Added new tracing levels.
**	03-Feb-00 (loera01) Bug 100348
**   	    Substituted SQLSTATE 5000R (run-time logical error) for 22011
**	    (substring error) mapping to generic error -33000 (logic error).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	2-jan-2004 (lunbr01) bug 111540, problem EDDB2#12
**	    Correct bad offset to GE 30210 which caused erroneous mapping to
**          SQL State (SS) 42502 rather than SS 50001.  Also, change the
**          SS mapping from 50001 to 23501, per JDBC specs for integrity
**          constraint violations (should be in 23xxx range) and to match
**          sqlstate returned by Ingres releases after 6.4.
*/

typedef struct GE2SQLSTATEMAP
{
    char	*(*ge_cnvtFunc)(II_LONG);
    char	*ge_sQLState;
} GE2SQLSTATEMAP;

typedef struct GE2SQLOFFSET
{
    II_LONG	    go_base;
    II_ULONG	    go_entryCount;
    GE2SQLSTATEMAP  *go_mapBase;
} GE2SQLOFFSET;

# define OFFSET_0     0
# define OFFSET_50    1
# define OFFSET_100   2
# define OFFSET_30100 3
# define OFFSET_30110 4
# define OFFSET_30120 5
# define OFFSET_30130 6
# define OFFSET_30140 7
# define OFFSET_30200 8
# define OFFSET_30210 9
# define OFFSET_31000 10
# define OFFSET_31100 11
# define OFFSET_31200 12
# define OFFSET_32000 13
# define OFFSET_33000 14
# define OFFSET_34000 15
# define OFFSET_36000 16
# define OFFSET_36100 17
# define OFFSET_36200 18
# define OFFSET_37000 19
# define OFFSET_38000 20
# define OFFSET_38100 21
# define OFFSET_39000 22
# define OFFSET_39100 23
# define OFFSET_40100 24
# define OFFSET_40200 25
# define OFFSET_40300 54
# define OFFSET_40400 55
# define OFFSET_40500 56
# define OFFSET_40600 57
# define OFFSET_40700 58
# define OFFSET_41000 59
# define OFFSET_41200 60
# define OFFSET_41300 61
# define OFFSET_41500 62
# define OFFSET_49900 63

# define OFFSET_CNT 36


/*
** Name: ge2SQLStateMapTBL
**
** Description:
**     This table provides the corresponding SQLSTATE or coversion
**     function for a specific generic error.  If there is an one
**     to one mapping for Generic error to SQLSTATE, then a SQLSTATE
**     is specified in this table.  If there is one to n mapping for
**     Generic error to SQLSTATE, then a conversion function is
**     specified.
**
**     ge_cnvtFunc   convertion function to convert a generic error
**                   to the appropriate SQLSTATE based on the local
**                   error.
**     ge_sQLState   SQLSTATE for the generic error when there is an
**                   one to one mapping.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
*/

static GE2SQLSTATEMAP
ge2SQLStateMapTBL[] =
{
    /* 0 */
{NULL         ,II_SS00000_SUCCESS               }, /* GE_OK              */
    
    /* 50 */
{NULL         ,II_SS01000_WARNING               }, /* GE_WARNING         */
    
    /* 100 */
{NULL         ,II_SS02000_NO_DATA               }, /* GE_NO_MORE_DATA    */
    
    /* 30100 */
{NULL         ,II_SS42500_TBL_NOT_FOUND         }, /* GE_TABLE_NOT_FOUND */
    
    /* 30110 */
{NULL         ,II_SS42501_COL_NOT_FOUND         }, /* GE_COLUMN_UNKNOWN  */
    
    /* 30120 */
{NULL         ,II_SS42504_UNKNOWN_CURSOR        }, /* GE_CURSOR_UNKNOWN  */
    
    /* 30130 */
{NULL         ,II_SS26000_INV_SQL_STMT_NAME     }, /* GE_NOT_FOUND       */
    
    /* 30140 */
{NULL         ,II_SS5000H_UNAVAILABLE_RESOURCE  }, /* GE_UNKNOWN_OBJECT  */
    
    /* 30200 */
{NULL         ,II_SS42502_DUPL_OBJECT           }, /* GE_DEF_RESOURCE    */
    
    /* 30210 */
{NULL         ,II_SS23501_UNIQUE_CONS_VIOLATION }, /* GE_INS_DUP_ROW     */
    
    /* 31000 */
{NULL         ,II_SS42000_SYN_OR_ACCESSERR      }, /* GE_SYNTAX_ERROR    */
    
    /* 31100 */
{NULL         ,II_SS42506_INVALID_IDENTIFIER    }, /* GE_INVALID_IDENT   */
    
    /* 31200 */
{NULL         ,II_SS0A500_INVALID_QRY_LANG      }, /* GE_UNSUP_LANGUAGE  */
    
    /* 32000 */
{NULL         ,II_SS5000A_QUERY_ERROR           }, /* GE_QUERY_ERROR     */
    
    /* 33000 */
{NULL         ,II_SS5000R_RUN_TIME_LOGICAL_ERROR }, /* GE_LOGICAL_ERROR   */
    
    /* 340000 */
{NULL         ,II_SS42503_INSUF_PRIV            }, /* GE_NO_PRIVILEGE    */
    
    /* 36000 */
{NULL         ,II_SS50002_LIMIT_EXCEEDED        }, /* GE_SYSTEM_LIMIT    */
    
    /* 36100 */
{NULL         ,II_SS50003_EXHAUSTED_RESOURCE    }, /* GE_NO_RESOURCE     */
    
    /* 36200 */
{NULL         ,II_SS50004_SYS_CONFIG_ERROR      }, /* GE_CONFIG_ERROR    */
    
    /* 37000 */
{NULL         ,II_SS08006_CONNECTION_FAILURE    }, /* GE_COMM_ERROR      */
    
    /* 38000 */
{NULL         ,II_SS50005_GW_ERROR              }, /* GE_GATEWAY_ERROR   */
    
    /* 38100 */
{NULL         ,II_SS08004_CONNECTION_REJECTED   }, /* GE_HOST_ERROR      */
    
    /* 39000 */
{NULL         ,II_SS50006_FATAL_ERROR           }, /* GE_FATAL_ERROR     */
    
    /* 39100 */
{NULL         ,II_SS5000B_INTERNAL_ERROR        }, /* GE_OTHER_ERROR     */
    
    /* 40100 */
{NULL         ,II_SS21000_CARD_VIOLATION        }, /* GE_CARDINALITY     */
    
    /* 40200-40228 */
{NULL         ,II_SS22000_DATA_EXCEPTION        }, /* GE_DATAEX_NOSUB    */
{NULL         ,II_SS22001_STRING_RIGHT_TRUNC    }, /* GE_DATAEX_TRUNC    */
{NULL         ,II_SS22002_NULLVAL_NO_IND_PARAM  }, /* GE_DATAEX_NEED_IND */
{NULL         ,II_SS22003_NUM_VAL_OUT_OF_RANGE  }, /* GE_DATAEX_NUMOVR   */
{NULL         ,II_SS22005_ASSIGNMENT_ERROR      }, /* GE_DATAEX_AII_SSGN */
{NULL         ,II_SS5000C_FETCH_ORIENTATION     }, /* GE_DATAEX_FETCH0   */
{NULL         ,II_SS22008_DATETIME_FLD_OVFLOW   }, /* GE_DATAEX_DTINV    */
{NULL         ,II_SS22015_INTERVAL_FLD_OVFLOW   }, /* GE_DATAEX_DATEOVR  */
{NULL         ,0                                }, /* GE_DATAEX_RSV08    */
{NULL         ,II_SS22022_INDICATOR_OVFLOW      }, /* GE_DATAEX_INVIND   */
{NULL         ,II_SS5000D_INVALID_CURSOR_NAME   }, /* GE_DATAEX_CURSINV  */
{NULL         ,0                                }, /* 40211              */
{NULL         ,0                                }, /* 40212              */
{NULL         ,0                                }, /* 40213              */
{NULL         ,0                                }, /* 40214              */
{NULL         ,II_SS22500_INVALID_DATA_TYPE     }, /* GE_DATAEX_TYPEINV  */
{NULL         ,0                                }, /* 40216              */
{NULL         ,0                                }, /* 40217              */
{NULL         ,0                                }, /* 40218              */
{NULL         ,0                                }, /* 40219              */
{NULL         ,II_SS22003_NUM_VAL_OUT_OF_RANGE  }, /* GE_DATAEX_FIXOVR   */
{NULL         ,II_SS22003_NUM_VAL_OUT_OF_RANGE  }, /* GE_DATAEX_EXPOVR   */
{NULL         ,II_SS22012_DIVISION_BY_ZERO      }, /* GE_DATAEX_FPDIV    */
{NULL         ,II_SS22012_DIVISION_BY_ZERO      }, /* GE_DATAEX_FLTDIV   */
{NULL         ,II_SS22012_DIVISION_BY_ZERO      }, /* GE_DATAEX_DCDIV    */
{NULL         ,II_SS22003_NUM_VAL_OUT_OF_RANGE  }, /* GE_DATAEX_FXPUNF   */
{NULL         ,II_SS22003_NUM_VAL_OUT_OF_RANGE  }, /* GE_DATAEX_EPUNF    */
{NULL         ,0                                }, /* GE_DATAEX_DECUNF   */
{NULL         ,II_SS22003_NUM_VAL_OUT_OF_RANGE  }, /* GE_DATAEX_OTHER    */
    
    /* 40300 */
{NULL         ,II_SS23000_CONSTR_VIOLATION      }, /* GE_CONSTR_VIO      */
    
    /* 40400 */
{NULL         ,II_SS24000_INV_CURS_STATE        }, /* GE_CUR_STATE_INV   */
    
    /* 40500 */
{NULL         ,II_SS25000_INV_XACT_STATE        }, /* GE_TRAN_STATE_INV  */
    
    /* 40600 */
{NULL         ,II_SS50007_INVALID_SQL_STMT_ID   }, /* GE_CUR_STATE_INV   */
    
    /* 40700 */
{NULL         ,II_SS27000_TRIG_DATA_CHNG_ERR    }, /* GE_TRIGGER_DATA    */
    
    /* 41000 */
{NULL         ,II_SS28000_INV_AUTH_SPEC         }, /* GE_USER_ID_INV     */
    
    /* 41200 */
{NULL         ,II_SS50008_UNSUPPORTED_STMT      }, /* GE_SQL_STMT_INV    */
    
    /* 41300 */ 
{NULL         ,II_SS50009_ERROR_RAISED_IN_DBPROC},/* GE_RAISE_ERROR      */
    
    /* 41500 */ 
{NULL         ,II_SS5000E_DUP_STMT_ID           },/* GE_DUP_SQL_STMT_ID  */
    
    /* 49900 */ 
{NULL         ,II_SS40001_SERIALIZATION_FAIL    } /* GE_SERIALIZATION    */

};




/*
** Name: ge2SQLOffsetTBL
**
** Description:
**     This table provides offset to the ge2SQLStateMapTBL for a specific
**     generic error.
**
**     ge_cnvtFunc   convertion function to convert a generic error
**                   to the appropriate SQLSTATE based on the local
**                   error.
**     ge_sQLState   SQLSTATE for the generic error when there is an
**                   one to one mapping.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
*/

static GE2SQLOFFSET
ge2SQLOffsetTBL[] =
{
    /* BASE   # OF ENTRIES     POINTER TO ge2SQLStateMapTBL */
{     0,             1,     ge2SQLStateMapTBL+OFFSET_0     },
{    50,             1,     ge2SQLStateMapTBL+OFFSET_50    },
{   100,             1,     ge2SQLStateMapTBL+OFFSET_100   },
{ 30100,             1,     ge2SQLStateMapTBL+OFFSET_30100 },
{ 30110,             1,     ge2SQLStateMapTBL+OFFSET_30100 },
{ 30120,             1,     ge2SQLStateMapTBL+OFFSET_30100 },
{ 30130,             1,     ge2SQLStateMapTBL+OFFSET_30100 },
{ 30140,             1,     ge2SQLStateMapTBL+OFFSET_30100 },
{ 30200,             1,     ge2SQLStateMapTBL+OFFSET_30200 },
{ 30210,             1,     ge2SQLStateMapTBL+OFFSET_30210 },
{ 31000,             1,     ge2SQLStateMapTBL+OFFSET_31000 },
{ 31100,             1,     ge2SQLStateMapTBL+OFFSET_31100 },
{ 31200,             1,     ge2SQLStateMapTBL+OFFSET_31200 },
{ 32000,             1,     ge2SQLStateMapTBL+OFFSET_32000 },
{ 33000,             1,     ge2SQLStateMapTBL+OFFSET_33000 },
{ 34000,             1,     ge2SQLStateMapTBL+OFFSET_34000 },
{ 36000,             1,     ge2SQLStateMapTBL+OFFSET_36000 },
{ 36100,             1,     ge2SQLStateMapTBL+OFFSET_36100 },
{ 36200,             1,     ge2SQLStateMapTBL+OFFSET_36200 },
{ 37000,             1,     ge2SQLStateMapTBL+OFFSET_37000 },
{ 38000,             1,     ge2SQLStateMapTBL+OFFSET_38000 },
{ 38100,             1,     ge2SQLStateMapTBL+OFFSET_38100 },
{ 39000,             1,     ge2SQLStateMapTBL+OFFSET_39000 },
{ 39100,             1,     ge2SQLStateMapTBL+OFFSET_39100 },
{ 40100,             1,     ge2SQLStateMapTBL+OFFSET_40100 },
{ 40200,            29,     ge2SQLStateMapTBL+OFFSET_40200 },
{ 40300,             1,     ge2SQLStateMapTBL+OFFSET_40300 },
{ 40400,             1,     ge2SQLStateMapTBL+OFFSET_40400 },
{ 40500,             1,     ge2SQLStateMapTBL+OFFSET_40500 },
{ 40600,             1,     ge2SQLStateMapTBL+OFFSET_40600 },
{ 40700,             1,     ge2SQLStateMapTBL+OFFSET_40700 },
{ 41000,             1,     ge2SQLStateMapTBL+OFFSET_41000 },
{ 41200,             1,     ge2SQLStateMapTBL+OFFSET_41200 },
{ 41300,             1,     ge2SQLStateMapTBL+OFFSET_41300 },
{ 41500,             1,     ge2SQLStateMapTBL+OFFSET_41500 },
{ 49900,             1,     ge2SQLStateMapTBL+OFFSET_49900 }
};





/*
** Name: IIapi_gen2IntSQLState
**
** Description:
**     This function converts INGRES general error to SQLSTATE
**
**     genericError  INGRES generic error
**     localError    local DBMS error
**
** Return value:
**     sQLState      SQLSTATE in INGRES integer form.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 16-Dec-94 (gordy)
**	    Enhance tracing.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
*/

II_EXTERN char *
IIapi_gen2IntSQLState( II_LONG genericError, II_LONG localError )
{
    II_LONG		ge;
    i4			low = 0; 
    i4			hi = OFFSET_CNT - 1; 
    i4			mid;
    GE2SQLSTATEMAP	*mapBase;
    
    IIAPI_TRACE( IIAPI_TR_DETAIL )
	( "IIapi_gen2IntSQLState: converting generic (%d) to SQLSTATE\n",
	  genericError );
    
    while (low < hi)
    {
	if ( ( ge = genericError - ge2SQLOffsetTBL[hi].go_base ) < 0 )
	{
	    if ( ( ge = genericError - ge2SQLOffsetTBL[low].go_base ) < 0 )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_gen2IntSQLState: unknown generic error %d\n",
		      genericError );
		return( II_SS5000K_SQLSTATE_UNAVAILABLE );
	    }
	    
	    if ( ge < ge2SQLOffsetTBL[low].go_entryCount )
	    {
		mapBase = ge2SQLOffsetTBL[low].go_mapBase;
		break;
	    }
	    
	    if ( genericError < ge2SQLOffsetTBL[ low + 1 ].go_base )
	    {
		IIAPI_TRACE( IIAPI_TR_ERROR )
		    ( "IIapi_gen2IntSQLState: unknown generic error %d\n",
		      genericError );
		return( II_SS5000K_SQLSTATE_UNAVAILABLE );
	    }
	    
	    mid = ( hi + low ) / 2;

	    if ( ( ge = genericError - ge2SQLOffsetTBL[mid].go_base ) == 0 )
	    {
		mapBase = ge2SQLOffsetTBL[mid].go_mapBase;
		break;
	    }
	    
	    if ( ge > 0 )
		low = mid;
	    else
		hi = mid;
	}
	else
	{
	    if ( ge < ge2SQLOffsetTBL[hi].go_entryCount )
	    {
		mapBase = ge2SQLOffsetTBL[hi].go_mapBase;
		break;
	    }
	    
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_gen2IntSQLState: invalid generic error %d\n",
		  genericError );
	    return( II_SS5000K_SQLSTATE_UNAVAILABLE );
	}
    }
    
    if ( mapBase->ge_cnvtFunc )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "IIapi_gen2IntSQLState: calling conversion function for 0x%x\n",
	      localError );
	return( (*mapBase->ge_cnvtFunc)( localError ) );
    }

    return( mapBase->ge_sQLState );
}
