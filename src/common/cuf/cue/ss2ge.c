/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <generr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <sqlstate.h>

static  VOID	init_ch_2_base36(i1 arr[]);

/*
** Name:	ss2ge.c		- Routines for translating SQLSTATEs
** 				  to GENERIC errors
**
** Description:
**	Contains routines for translating SQLSTATEs to GENERIC errors
**
**		ss_2_ge		- Translate SQLSTATE to GENERIC error
**		ss_encode	- convert SQLSTATE from string to base36 number
**		ss_decode	- convert base36 number back into a SQLSTATE
**		init_ch_2_base36- initialize array used for base 36 conversion
**
**  History:
**	02-nov-92 (andre)
**	    written
**	11-oct-93 (rblumer)
**	    added separate error mapping for subclass 23501.
**	10-mar-94 (andre)
**	    added code to map SS5000N_OPERAND_TYPE_MISMATCH,
**	    SS5000O_INVALID_FUNC_ARG_TYPE, SS5000P_TIMEOUT_ON_LOCK_REQUEST,
**	    SS5000Q_DB_REORG_INVALIDATED_QP and SS5000R_RUN_TIME_LOGICAL_ERROR
**	15-mar-94 (andre)
**	    removed mappings for 37000- and 2A000-series SQLSTATEs which have 
**	    been discontinued.
**	08-may-1997 (canor01)
**	    In ss_encode(), correctly cast *sqlstate to unsigned before 
**	    using it to index into the array.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
ss_2_ge(char *sql_state, i4 dbms_err)
{
    i4	gen_err;
    i4		ss_class, ss_subclass;
    i4	encoded_ss;
    static struct
    {
	i4		dbmserr;
	i4		generr;
    } numeric_exception[] =
    {
	{ /* E_AD1127_DECOVF_ERROR */      0x021127L,  E_GE9D0B_DATAEX_NUMOVR },
	{ /* E_GW7015_DECIMAL_OVF */       0x0F7015L,  E_GE9D0B_DATAEX_NUMOVR },
	{ /* E_AD1013_EMBEDDED_NUMOVF */   0x021013L,  E_GE9D1C_DATAEX_FIXOVR },
	{ /* E_CL0503_CV_OVERFLOW */       0x010503L,  E_GE9D1C_DATAEX_FIXOVR },
	{ /* E_GW7000_INTEGER_OVF */       0x0F7000L,  E_GE9D1C_DATAEX_FIXOVR },
	{ /* E_US1068_4200 */              0x001068L,  E_GE9D1C_DATAEX_FIXOVR },
	{ /* E_GW7001_FLOAT_OVF */         0x0F7001L,  E_GE9D1D_DATAEX_EXPOVR },
	{ /* E_GW7004_MONEY_OVF */         0x0F7004L,  E_GE9D1D_DATAEX_EXPOVR },
	{ /* E_US106A_4202 */              0x00106AL,  E_GE9D1D_DATAEX_EXPOVR },
	{ /* E_CL0502_CV_UNDERFLOW */      0x010502L,  E_GE9D21_DATAEX_FXPUNF },
	{ /* E_GW7002_FLOAT_UND */         0x0F7002L,  E_GE9D22_DATAEX_EPUNF },
	{ /* E_GW7005_MONEY_OVF */         0x0F7005L,  E_GE9D22_DATAEX_EPUNF },
	{ /* E_US106C_4204 */              0x00106CL,  E_GE9D22_DATAEX_EPUNF },
	{ -1L, -1L},
    };

    static struct
    {
	i4         dbmserr;
	i4         generr;
    } division_by_zero[] =
    {
	{ /* E_US1069_4201 */              0x001069L,  E_GE9D1E_DATAEX_FPDIV },
	{ /* E_US106B_4203 */              0x00106BL,  E_GE9D1F_DATAEX_FLTDIV },
	{ /* E_AD1126_DECDIV_ERROR */      0x021126L,  E_GE9D20_DATAEX_DCDIV },
	{ -1L, -1L},
    };

    /*
    ** convert SQLSTATE class (treated as a base 36 number) to a base 10 integer
    */
    encoded_ss = ss_encode(sql_state);
    
    ss_class = ((encoded_ss >> 24) & 0x3F) * 36 + ((encoded_ss >> 18) & 0x3F);
    
    ss_subclass =   ((encoded_ss >> 12) & 0x3F) * 36 * 36
		  + ((encoded_ss >> 6) & 0x3F) * 36
		  + (encoded_ss & 0x3F);

    switch (ss_class)
    {
        case SS_3C000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_3C000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_21000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_21000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_08000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_08003_SUBCLASS:
                {
                    gen_err = SS_08003_GE_MAP;
                    break;
                }
                case SS_08006_SUBCLASS:
                {
                    gen_err = SS_08006_GE_MAP;
                    break;
                }
                case SS_08002_SUBCLASS:
                {
                    gen_err = SS_08002_GE_MAP;
                    break;
                }
                case SS_08001_SUBCLASS:
                {
                    gen_err = SS_08001_GE_MAP;
                    break;
                }
                case SS_08004_SUBCLASS:
                {
                    gen_err = SS_08004_GE_MAP;
                    break;
                }
                case SS_08007_SUBCLASS:
                {
                    gen_err = SS_08007_GE_MAP;
                    break;
                }
                case SS_08500_SUBCLASS:
                {
                    gen_err = SS_08500_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_08000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_22000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_22021_SUBCLASS:
                {
                    gen_err = SS_22021_GE_MAP;
                    break;
                }
                case SS_22008_SUBCLASS:
                {
                    gen_err = SS_22008_GE_MAP;
                    break;
                }
                case SS_22012_SUBCLASS:
                {
                    i4     i;

                    for (i = 0;
                         (   division_by_zero[i].dbmserr != -1L
                          && division_by_zero[i].dbmserr != dbms_err);
                         i++)
                    ;

                    if (division_by_zero[i].dbmserr == -1L)
                        gen_err = SS_22012_GE_MAP;
                    else
                        gen_err = division_by_zero[i].generr;

                    break;
                }
                case SS_22005_SUBCLASS:
                {
                    gen_err = SS_22005_GE_MAP;
                    break;
                }
                case SS_22022_SUBCLASS:
                {
                    gen_err = SS_22022_GE_MAP;
                    break;
                }
                case SS_22015_SUBCLASS:
                {
                    gen_err = SS_22015_GE_MAP;
                    break;
                }
                case SS_22018_SUBCLASS:
                {
                    gen_err = SS_22018_GE_MAP;
                    break;
                }
                case SS_22007_SUBCLASS:
                {
                    gen_err = SS_22007_GE_MAP;
                    break;
                }
                case SS_22019_SUBCLASS:
                {
                    gen_err = SS_22019_GE_MAP;
                    break;
                }
                case SS_22025_SUBCLASS:
                {
                    gen_err = SS_22025_GE_MAP;
                    break;
                }
                case SS_22023_SUBCLASS:
                {
                    gen_err = SS_22023_GE_MAP;
                    break;
                }
                case SS_22009_SUBCLASS:
                {
                    gen_err = SS_22009_GE_MAP;
                    break;
                }
                case SS_22002_SUBCLASS:
                {
                    gen_err = SS_22002_GE_MAP;
                    break;
                }
                case SS_22003_SUBCLASS:
                {
                    i4     i;

                    for (i = 0;
                         (   numeric_exception[i].dbmserr != -1L
                          && numeric_exception[i].dbmserr != dbms_err);
                         i++)
                    ;

                    if (numeric_exception[i].dbmserr == -1L)
                        gen_err = SS_22003_GE_MAP;
                    else
                        gen_err = numeric_exception[i].generr;

                    break;
                }
                case SS_22026_SUBCLASS:
                {
                    gen_err = SS_22026_GE_MAP;
                    break;
                }
                case SS_22001_SUBCLASS:
                {
                    gen_err = SS_22001_GE_MAP;
                    break;
                }
                case SS_22011_SUBCLASS:
                {
                    gen_err = SS_22011_GE_MAP;
                    break;
                }
                case SS_22027_SUBCLASS:
                {
                    gen_err = SS_22027_GE_MAP;
                    break;
                }
                case SS_22024_SUBCLASS:
                {
                    gen_err = SS_22024_GE_MAP;
                    break;
                }
                case SS_22500_SUBCLASS:
                {
                    gen_err = SS_22500_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_22000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_2B000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_2B000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_07000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_07003_SUBCLASS:
                {
                    gen_err = SS_07003_GE_MAP;
                    break;
                }
                case SS_07008_SUBCLASS:
                {
                    gen_err = SS_07008_GE_MAP;
                    break;
                }
                case SS_07009_SUBCLASS:
                {
                    gen_err = SS_07009_GE_MAP;
                    break;
                }
                case SS_07005_SUBCLASS:
                {
                    gen_err = SS_07005_GE_MAP;
                    break;
                }
                case SS_07006_SUBCLASS:
                {
                    gen_err = SS_07006_GE_MAP;
                    break;
                }
                case SS_07001_SUBCLASS:
                {
                    gen_err = SS_07001_GE_MAP;
                    break;
                }
                case SS_07002_SUBCLASS:
                {
                    gen_err = SS_07002_GE_MAP;
                    break;
                }
                case SS_07004_SUBCLASS:
                {
                    gen_err = SS_07004_GE_MAP;
                    break;
                }
                case SS_07007_SUBCLASS:
                {
                    gen_err = SS_07007_GE_MAP;
                    break;
                }
                case SS_07500_SUBCLASS:
                {
                    gen_err = SS_07500_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_07000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_0A000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_0A001_SUBCLASS:
                {
                    gen_err = SS_0A001_GE_MAP;
                    break;
                }
                case SS_0A500_SUBCLASS:
                {
                    gen_err = SS_0A500_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_0A000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_23000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_23501_SUBCLASS:
                {
                    gen_err = SS_23501_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_23000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_28000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_28000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_3D000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_3D000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_2C000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_2C000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_35000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_35000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_2E000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_2E000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_34000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_34000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_24000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_24000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_3F000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_3F000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_33000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_33000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_26000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_26000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_25000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_25000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_2D000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_2D000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_02000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_02000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_HZ000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_HZ000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_00000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_00000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_42000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_42500_SUBCLASS:
                {
                    gen_err = SS_42500_GE_MAP;
                    break;
                }
                case SS_42501_SUBCLASS:
                {
                    gen_err = SS_42501_GE_MAP;
                    break;
                }
                case SS_42502_SUBCLASS:
                {
                    gen_err = SS_42502_GE_MAP;
                    break;
                }
                case SS_42503_SUBCLASS:
                {
                    gen_err = SS_42503_GE_MAP;
                    break;
                }
                case SS_42504_SUBCLASS:
                {
                    gen_err = SS_42504_GE_MAP;
                    break;
                }
                case SS_42505_SUBCLASS:
                {
                    gen_err = SS_42505_GE_MAP;
                    break;
                }
                case SS_42506_SUBCLASS:
                {
                    gen_err = SS_42506_GE_MAP;
                    break;
                }
                case SS_42507_SUBCLASS:
                {
                    gen_err = SS_42507_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_42000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_40000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_40002_SUBCLASS:
                {
                    gen_err = SS_40002_GE_MAP;
                    break;
                }
                case SS_40001_SUBCLASS:
                {
                    gen_err = SS_40001_GE_MAP;
                    break;
                }
                case SS_40003_SUBCLASS:
                {
                    gen_err = SS_40003_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_40000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_27000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_27000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_01000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_01001_SUBCLASS:
                {
                    gen_err = SS_01001_GE_MAP;
                    break;
                }
                case SS_01002_SUBCLASS:
                {
                    gen_err = SS_01002_GE_MAP;
                    break;
                }
                case SS_01008_SUBCLASS:
                {
                    gen_err = SS_01008_GE_MAP;
                    break;
                }
                case SS_01005_SUBCLASS:
                {
                    gen_err = SS_01005_GE_MAP;
                    break;
                }
                case SS_01003_SUBCLASS:
                {
                    gen_err = SS_01003_GE_MAP;
                    break;
                }
                case SS_01007_SUBCLASS:
                {
                    gen_err = SS_01007_GE_MAP;
                    break;
                }
                case SS_01006_SUBCLASS:
                {
                    gen_err = SS_01006_GE_MAP;
                    break;
                }
                case SS_0100A_SUBCLASS:
                {
                    gen_err = SS_0100A_GE_MAP;
                    break;
                }
                case SS_01009_SUBCLASS:
                {
                    gen_err = SS_01009_GE_MAP;
                    break;
                }
                case SS_01004_SUBCLASS:
                {
                    gen_err = SS_01004_GE_MAP;
                    break;
                }
                case SS_01500_SUBCLASS:
                {
                    gen_err = SS_01500_GE_MAP;
                    break;
                }
                case SS_01501_SUBCLASS:
                {
                    gen_err = SS_01501_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_01000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_44000_CLASS:
        {
            switch (ss_subclass)
            {
                default:
                {
                    gen_err = SS_44000_GE_MAP;
                    break;
                }
            }

            break;
        }
        case SS_50000_CLASS:
        {
            switch (ss_subclass)
            {
                case SS_50001_SUBCLASS:
                {
                    gen_err = SS_50001_GE_MAP;
                    break;
                }
                case SS_50002_SUBCLASS:
                {
                    gen_err = SS_50002_GE_MAP;
                    break;
                }
                case SS_50003_SUBCLASS:
                {
                    gen_err = SS_50003_GE_MAP;
                    break;
                }
                case SS_50004_SUBCLASS:
                {
                    gen_err = SS_50004_GE_MAP;
                    break;
                }
                case SS_50005_SUBCLASS:
                {
                    gen_err = SS_50005_GE_MAP;
                    break;
                }
                case SS_50006_SUBCLASS:
                {
                    gen_err = SS_50006_GE_MAP;
                    break;
                }
                case SS_50007_SUBCLASS:
                {
                    gen_err = SS_50007_GE_MAP;
                    break;
                }
                case SS_50008_SUBCLASS:
                {
                    gen_err = SS_50008_GE_MAP;
                    break;
                }
                case SS_50009_SUBCLASS:
                {
                    gen_err = SS_50009_GE_MAP;
                    break;
                }
                case SS_5000A_SUBCLASS:
                {
                    gen_err = SS_5000A_GE_MAP;
                    break;
                }
                case SS_5000B_SUBCLASS:
                {
                    gen_err = SS_5000B_GE_MAP;
                    break;
                }
                case SS_5000C_SUBCLASS:
                {
                    gen_err = SS_5000C_GE_MAP;
                    break;
                }
                case SS_5000D_SUBCLASS:
                {
                    gen_err = SS_5000D_GE_MAP;
                    break;
                }
                case SS_5000E_SUBCLASS:
                {
                    gen_err = SS_5000E_GE_MAP;
                    break;
                }
                case SS_5000F_SUBCLASS:
                {
                    gen_err = SS_5000F_GE_MAP;
                    break;
                }
                case SS_5000G_SUBCLASS:
                {
                    gen_err = dbms_err;
                    break;
                }
                case SS_5000H_SUBCLASS:
                {
                    gen_err = SS_5000H_GE_MAP;
                    break;
                }
                case SS_5000I_SUBCLASS:
                {
                    gen_err = SS_5000I_GE_MAP;
                    break;
                }
                case SS_5000J_SUBCLASS:
                {
                    gen_err = SS_5000J_GE_MAP;
                    break;
                }
                case SS_5000K_SUBCLASS:
                {
                    gen_err = SS_5000K_GE_MAP;
                    break;
                }
                case SS_5000L_SUBCLASS:
                {
                    gen_err = SS_5000L_GE_MAP;
                    break;
                }
                case SS_5000M_SUBCLASS:
                {
                    gen_err = SS_5000M_GE_MAP;
                    break;
                }
                case SS_5000N_SUBCLASS:
                {
                    gen_err = SS_5000N_GE_MAP;
                    break;
                }
                case SS_5000O_SUBCLASS:
                {
                    gen_err = SS_5000O_GE_MAP;
                    break;
                }
                case SS_5000P_SUBCLASS:
                {
                    gen_err = SS_5000P_GE_MAP;
                    break;
                }
                case SS_5000Q_SUBCLASS:
                {
                    gen_err = SS_5000Q_GE_MAP;
                    break;
                }
                case SS_5000R_SUBCLASS:
                {
                    gen_err = SS_5000R_GE_MAP;
                    break;
                }
                default:
                {
                    gen_err = SS_50000_GE_MAP;
                    break;
                }
            }

            break;
        }
        default:
        {
            gen_err = E_GE98BC_OTHER_ERROR;
            break;
        }
    }

    return(gen_err);
}

/*
** Name: ss_encode	encode SQLSTATE represented as a 5-byte string into a
**			(almost) base-36 number
**
** Description:
**	Encode SQLSTATE status code as an integer.  We will convert every
**	character into a base-36 i1 and OR it at appropriate offset into a
**	i4
**
** Input:
**	sqlstate	sqlstate status to encode
**
** Output:
**	none
**
** Side effects:
**	whan called for the first time, will call init_ch_2_base36() to
**	initialize ch_2_base36 char->base36 integer map
**	
** Returns:
**	encoded sqlstate
**
** History:
**
**	02-nov-92 (andre)
**	    written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	08-may-1997 (canor01)
**	    Correctly cast *sqlstate to unsigned before using it to
**	    index into the array.
*/
i4
ss_encode(char *sqlstate)
{
    i4	    res = 0L;
    i4		    shift;
    static  i1	    ch_2_base36[256];
    static  bool    encode_arr_inited = FALSE;
    
    if (!encode_arr_inited)
    {
	init_ch_2_base36(ch_2_base36);
	encode_arr_inited = TRUE;
    }

    for (shift = (DB_SQLSTATE_STRING_LEN - 1) * 6;
	 shift >= 0;
	 sqlstate++, shift -= 6)
    {
	res |= (((i4) ch_2_base36[(unsigned char)*sqlstate]) << shift);
    }

    return(res);
}

/*
** Name:    init_ch_2_base36	- initialize array mapping characters 0-9A-Z
**				  into a number between 0 and 35 (so we can
**				  treat them as base-36 digits)
**
** Input:
**	arr	array to be initialized
**
** Output
**	arr	initialized array
**
** Side effect:
**	none
**
** Returns:
**	none
**
** History:
**
**	02-nov-92 (andre)
**	    written
*/
static VOID
init_ch_2_base36(i1 arr[])
{
    i4	    i;

    for (i = 0; i < 256; i++)
	arr[i] = 0;

    arr['1'] = 0x01;
    arr['2'] = 0x02;
    arr['3'] = 0x03;
    arr['4'] = 0x04;
    arr['5'] = 0x05;
    arr['6'] = 0x06;
    arr['7'] = 0x07;
    arr['8'] = 0x08;
    arr['9'] = 0x09;
    arr['A'] = 0x0A;
    arr['B'] = 0x0B;
    arr['C'] = 0x0C;
    arr['D'] = 0x0D;
    arr['E'] = 0x0E;
    arr['F'] = 0x0F;
    arr['G'] = 0x10;
    arr['H'] = 0x11;
    arr['I'] = 0x12;
    arr['J'] = 0x13;
    arr['K'] = 0x14;
    arr['L'] = 0x15;
    arr['M'] = 0x16;
    arr['N'] = 0x17;
    arr['O'] = 0x18;
    arr['P'] = 0x19;
    arr['Q'] = 0x1A;
    arr['R'] = 0x1B;
    arr['S'] = 0x1C;
    arr['T'] = 0x1D;
    arr['U'] = 0x1E;
    arr['V'] = 0x1F;
    arr['W'] = 0x20;
    arr['X'] = 0x21;
    arr['Y'] = 0x22;
    arr['Z'] = 0x23;

    return;
}

/*
** Name: ss_decode	decode SQLSTATE encoded by ss_encode() into a 5-char
**			string
**
** Description:
**	decode SQLSTATE encoded by ss_encode()
**
** Input:
**	encoded_ss	encoded sqlstate value
**
** Output:
**	sqlstate	sqlstate value represented as a 5-char string
**
** Side effects:
**	none
**	
** Returns:
**	encoded sqlstate
**
** History:
**
**	02-nov-92 (andre)
**	    written
*/
VOID
ss_decode(char *sqlstate, i4 encoded_ss)
{
    static char base36_2_ch[] =
    {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
	'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z',
    };
    i4		    shift;
    i4		    i;
    
    for (shift = (DB_SQLSTATE_STRING_LEN - 1) * 6;
	 shift >= 0;
	 sqlstate++, shift -= 6)
    {
	i = (encoded_ss >> shift) & 0x3F;
	
	*sqlstate = (i > sizeof(base36_2_ch) - 1)
	    ? '?'			    /* this should never happen */
	    : base36_2_ch[i];
    }

    return;
}
