/*
** Copyright (c) 2004 Ingres Corporation
*/
# include       <compat.h>
# include       <st.h>          /* 6-x_PC_80x86 */
# include       <gl.h>
# include       <sl.h>    
# include       <iicommon.h>
# include       <fe.h>
# include       "decls.h"
# include       <lo.h>
# include       <nm.h>
# include       <ug.h>
# include       <er.h>
# include       "erfc.h"
  
GLOBALDEF FILE  *fcoutfp = NULL;
GLOBALDEF char  *fcoutfile = NULL;
GLOBALDEF char  *fcnames[NUMFORMS] = {0};       /* changed {0} to {0} -nml */
GLOBALDEF char  **fcntable = fcnames;
GLOBALDEF char  *fcname = NULL; 
GLOBALDEF char  *fcdbname = NULL; 
GLOBALDEF i4    fclang = DSL_C;
GLOBALDEF i4    fceqlcall = 0;
GLOBALDEF FRAME *fcfrm = NULL; 
GLOBALDEF bool  fcrti = FALSE;   
GLOBALDEF bool  fcstdalone = FALSE;
GLOBALDEF char  *valstr = NULL;
GLOBALDEF i4    starttr = 0,  
                startfd = 0,  
                startns = 0;
 
GLOBALDEF bool  fcfileopened = FALSE;
