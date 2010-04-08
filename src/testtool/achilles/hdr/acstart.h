#include <windows.h>
#include <stdio.h>
#include "resource.h"

enum { STRINGLENGTH	= 256 };
enum MSGTYPE { MSGTYPE_WARNING, MSGTYPE_ERROR };
extern	HANDLE	hInstance;
extern	DWORD	pid,	ret_val;
extern	BOOL	debug_sw,	interactive_sw,		repeat_sw,			output_sw, 
				user_sw,	get_out,			need_repeat,		need_output,	
				need_user,	need_cfg,			need_db,			need_time,		
				time_sw,	ach_result_exists,	ach_config_exists,	need_vnd,
				need_dbn,	need_svr,			have_vnd,			have_dbn,
				have_svr,	skip_vn,			need_prot,			need_remnode,
				need_listen;
 
extern	int		i,	repeat_val,		time_val;
extern	char 	output_val[STRINGLENGTH],		user_val[STRINGLENGTH], 
				test_case[STRINGLENGTH],		cfg_file[STRINGLENGTH], 
				achilles_config[STRINGLENGTH],	achilles_result[STRINGLENGTH], 
				dbspec[STRINGLENGTH],			host[STRINGLENGTH],
				fname[STRINGLENGTH],			fn_root[STRINGLENGTH],
				childlog[STRINGLENGTH],			log_file[STRINGLENGTH],
				rpt_file[STRINGLENGTH],			vnode[STRINGLENGTH],
				dname[STRINGLENGTH],			server[STRINGLENGTH],
				entry_type[STRINGLENGTH],		net_protocol[STRINGLENGTH],
				rem_node[STRINGLENGTH],			list_addr[STRINGLENGTH];
;

VOID	GetString(UINT, LPTSTR, int);
UINT	InitEnvironment();
int		String2Integer(char*);
BOOL	PrepareRun();
BOOL	ProcessParameters();
VOID	Message(enum MSGTYPE, DWORD, PCHAR);
DWORD	StartAchilles(BOOL);