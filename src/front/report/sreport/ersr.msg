/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ersr.msg -	Message file for class SR
**
** Description:
**	This file defines the messages for class SR.
**
** History:
**	17-aug-1987 (peterk)
**		Created.
**	11-dec-1987 (rdesmond)
**		added '?' to S_SR0001
**	12/21/89 (elein)
**		Corrected $s -> $0c and %d to %0d for E_SR0004, E_SR000A
**	15-feb-90 (sylviap)
**		Changed ellipses according to the FRC decsision.
**	2/23/90 (martym)
**		Added E_SR0017.
**	11-jun-90 (sylviap)
**		Changed prompt "Report File?" to "Filename" to be consistent
**		with copyrep and other prompts (Database, etc.) with no 
**		question marks.
**/

# define	_SR_CLASS	96
/*
**	scrack.c
*/
S_SR0001_Report_File	"Filename"
/*
**	deadlock.c
*/
E_SR0002_Deadlock_occurred	"Deadlock occurred; retrying . . ."
/*
**	main.qc
*/
E_SR0003_Too_many_retries	"Too many retries with deadlock; Report was NOT written.\n"
/*
**	scopynew.qc
*/
E_SR0004_s_copy_new_Bad_close_	"s_copy_new:Bad close of file %0c.\n"
E_SR0005_s_copy_new_No_node	"s_copy_new: Can't get node for network.\n"
/*
**	sdelold.qc
*/
S_SR0006_Replacing_report	"Replacing specifications for report '%s' in database '%s' . . .\n"
S_SR0007_Adding_report	"Adding specifications for report '%s' to database '%s'...\n"
/*
**	serror.c
*/
E_SR0008_Line_in_error	"        Line in error:\n%s\n"
E_SR0009_Error_processing_cont	"^\n        Error processing continues . . .\n"
E_SR000A_s_error_Too_many_args	"s_error('%0d'): too many arguments.\n"
/*
**	srenset.c
*/
S_SR000B_Start_of_report	"File '%s': Start of specifications for Report '%s'.\n"
/*
**	sreset.c
*/
E_SR000C_s_reset_Too_many_args	"s_reset: too many arguments.\n"
/*
**	srsoadd.c
*/
E_SR000D_s_rso_add_Null_name	"s_rso_add: Null name pointer.\n"
/*
**	srsofind.c
*/
E_SR000E_s_rso_find_Null_name	"s_rso_find: Null name pointer.\n"
/*
**	ssbr.c
*/
E_SR000F_s_sbr_add_Null_name	"s_sbr_add: Null name pointer.\n"
E_SR0010_s_sbr_find_Null_name	"s_sbr_find: Null name pointer.\n"
/*
**	ssetcmd.c
*/
E_SR0011_s_set_cmd_Null_cmd	"s_set_cmd: Null pointer to command name.\n"
/*
**	swaction.c
*/
E_SR0012_s_w_action_Null_cmd	"s_w_action: Null pointer to command.\n"
/*
**	swrow.c
*/
E_SR0013_s_w_row_Bad_write	"s_w_row: Bad write to file.\n"
E_SR0017_s_w_row_Write_failed	"Can not write to file '%0c'.\nYou may have run out of disk space.\n"
/*
**	sinclude.c
*/
S_SR0014_including_file		"Including file '%s'.\n"
S_SR0015_good_include	        "File '%s' included successfully.\n"
S_SR0016_error_include	        "File '%s' included with %d error(s).\n"
/*
** Explanation: 
**	An empty file was used as an input .RW file to SREPORT.
**
** System Status:
**	Exit SREPORT
**
** User Action:
**	none.
*/

S_SR0018_empty_rw_file
"Premature end of file was found when reading file '%0c'.\n
Check to see if the file is empty or contains\n
no report commands.\nProcessing stops."

