/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ergt.msg -	Message file for class GT
**
** Description:
**	This file defines the messages for class GT.
**
** History:
**	30-jul-1987 (bobm)
**		Created.
**/

# define	_GT_CLASS	62
/*
**	explanatory messages displayed only in response to user "=" key
**	fast strings because logic causes these strings to be accessed on every
**	user keystroke which has an error
*/
F_GT0001_No_mouse	"No graphics pointer available"
F_GT0002_undefined_keystroke	"undefined keystroke"
F_GT0003_left_edge	"ran into left edge"
F_GT0004_bottom_edge	"ran into bottom edge"
F_GT0005_top_edge	"ran into top edge"
F_GT0006_top_edge	"ran into top edge"
F_GT0007_right_edge	"ran into right edge"
F_GT0008_no_subcomponents	"no subcomponents"
F_GT0009_no_next_comp	"no \"next\" component"
F_GT000A_no_prev_comp	"no \"previous\" component"
F_GT000B_refresh_fail	"unable to refresh"
/*
**	fatal error in GTedit - not supposed to happen.
*/
S_GT0001_GTedit_sort_err	"GTedit: sort error" /* DIAG */
S_GT0002_GTedit_fwd_fail	"GTedit: forward search failure" /* DIAG */
S_GT0003_GTedit_rev_fail	"GTedit: reverse search failure" /* DIAG */
/*
**	prompt format within GT routines
*/
F_GT000C_RET_format	"%0c  [PRESS RETURN]"
/*
**	gtdatset.c
*/
/*
**	fatal errors in setting up data - not supposed to happen.
*/
S_GT0004_DS_too_many	"GTdataset: too many datasets" /* DIAG */
S_GT0005_DS_no_vectors	"GTdataset: %0c, %1c non-existent data vector(s)" /* DIAG */
S_GT0006_DS_pair_alloc	"GTdataset(makepair): graphics data allocation failure" /* DIAG */
S_GT0007_DS_bad_Y_type	"GTdataset: bad data vector y type" /* DIAG */
S_GT0008_DS_bad_X_type	"GTdataset: bad data vector x type" /* DIAG */
S_GT0009_DS_bad_labfield	"GTdataset: non-existent label field \"%0c\"" /* DIAG */
S_GT000A_DS_bad_lab_type	"GTdataset: label field \"%0c\",type %1d - not string" /* DIAG */
S_GT000B_DS_lab_too_short	"GTdataset: label field \"%0c\", too short" /* DIAG */
/*
**	gtinit.c
*/
/*
**	status message while graphics is being drawn
**	Fast because of extremely heavy use.
*/
F_GT0010_Working	"Working . . ."
/*
**	fatal error - not supposed to happen
*/
S_GT000C_Bad_mode	"Mode switch: bad mode (%0d)" /* DIAG */
/*
**	gtlocdev.c
*/
/*
**	Messages used with mouse on, to choose alpha cursor again
**	or inform user they aren't pointing to anything
*/
F_GT0012_Alpha_Cursor	" Alpha Cursor"
F_GT0013_No_point	"Point is not in a meaningful location.  Try Again"
/*
**	gtmsgp.c
*/
/*
**	prompt format with leading return
*/
F_GT0014_r_RET_format	"\r%0c  [PRESS RETURN]"
/*
**	gttexted.c
*/
/*
**	messages used with text editing withing GT
**		%s format argument is either F_GT0017 or F_GT0018 for
**		"insert" or "overstrike" modes.
*/
F_GT0015_Box_too_small	"Box too small - will use menu line.  Press [return]"
F_GT0016_Bad_Alpha	"Bad terminal type - will use menu line.  Press [return]"
F_GT0017_Insert	"Insert"
F_GT0018_Overstrike	"Overstrike"
F_GT0019_on_menu_line	" on menu line, %0c mode"
F_GT001A_Mode_format	", %0c mode"
/*
**	gtdvec.c
*/
/*
**	fatal error - not supposed to happen
*/
S_GT000D_DV_alloc	"Data vector allocation failure" /* DIAG */
S_GT000E_DV_Bad_Type	"Bad data vector data type: %0d" /* DIAG */
/*
**	gtstatus.c
*/
/*
**	fatal error - not supposed to happen
*/
S_GT000F_Runaway_stack	"Runaway status line stack" /* DIAG */
S_GT0010_stack_underflow	"Status line stack underflow" /* DIAG */
/*
**	gtws.c
*/
/*
**	fatal error - not supposed to happen.  GTopen is routine name
*/
S_GT0011_No_mode_func	"GTopen:  'G_mfunc' not set." /* DIAG */
/*
**	gtxws.c
*/
S_GT0012_No_device	"GTchange_ws:  must have device" /* DIAG */
S_GT0013_No_loc	"GTchange_ws:  must have location" /* DIAG */
/*
**	gtdatset.c
*/
S_GT0014_DS_pair_alloc	"GTdataset(makepair): graphics data allocation failure" /* DIAG */
/*
**	gtdraw.c
*/
S_GT0015_Bad_object	"Bad object, GTdraw" /* DIAG */
/*
**	gtcchdef.c
*/
/*
** message used when data on pie chart sums to zero - only caught at actual
** plot time
*/
E_GT0016_pie_zero_sum	"Data error - sum of pie slice sizes is zero"
/*
** format string for reporting CCHART/GKS errors, arguments:
**	0 - "CCHART" or "GKS"
**	1 - error number (VE's numbers)
**	2 - VE set character string.
*/
E_GT0017_Draw_err_format	"Drawing error encountered (%0c error %1d - %2c)"
/*
** Fatal error - not supposed to happen
*/
S_GT0018_ws_open	"Can't open workstation for scr file plot" /* DIAG */
/*
**	gtinit.c
*/
/*
**  Message that is put out when we can't figure out what type of terminal
**  user is using.  This usually means that the envionrment logical 
**  defining the system terminal type is not set.
*/
E_GT0019_NO_TERM	"Can not determine user's terminal type.\n  Please
define your terminal type via %0c."
/*
**	gtws.c & gtxws.c
*/
/*
**  Message for telling user that we can't find user's terminal type
**  in the termcap file.
*/
E_GT001A_UNKNOWN	"Can not find terminal \"%0c\" in the termcap file\n.
Please check that you have set TERM_INGRES properly."
