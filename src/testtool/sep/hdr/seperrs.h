/*
**  Seperrs.h
**  
**  History:
**      ##-###-#### (donj)
**	    Created.
**	14-jan-1992 (donj)
**	    Added error values to pass to listexec's executor.exe so it knows
**	    why we failed. Elaborated on IO errors.
**	16-jan-1992 (donj)
**	    Added SEPerr_Cant_find_test so listexec can report when SEP
**	    couldn't find a test script listed in the list file.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	07-jun-1994 (donj)
**	    Added a SEPerr for ABORT due to missing canon.
*/

/*
**      SEP Error Codes.
**
*/

#define         SEPerr_no_IISYS            -1
#define         SEPerr_bad_cmdline         -3
#define         SEPerr_SEPfileInit_fail    -4
#define         SEPerr_No_TST_SEP          -5

#define         SEPerr_cmd_load_fail       -7
#define         SEPerr_trmcap_load_fail    -8
#define         SEPerr_OpnHdr_fail         -9
#define         SEPerr_CpyHdr_fail         -10
#define         SEPerr_LogOpn_fail         -11
#define         SEPerr_CtrlC_entered       -12
#define         SEPerr_Unk_exception       -13
#define         SEPerr_TM_timed_out        -14
#define         SEPerr_FE_timed_out        -15
#define         SEPerr_Cant_Spawn          -16
#define         SEPerr_TM_Cant_connect     -17
#define         SEPerr_Cant_Trans_Token    -18
#define         SEPerr_Cant_Opn_Commfile   -19
#define         SEPerr_Cant_Opn_ResFile    -20
#define         SEPerr_Cant_Set_FRSFlags   -21
#define         SEPerr_Cant_Set_Peditor    -22
#define         SEPerr_Cant_Connect_to_FE  -23
#define         SEPerr_Cant_find_test      -24
#define         SEPerr_question_in_batch   -25
#define         SEPerr_ABORT_canon_matched -26
#define         SEPerr_ABORT_canon_missing -27
