/*	grerr.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
** use one of the string codes to see that we have ergr.h in
*/
# ifndef E_GR0025_Unsupported_device
# include <ergr.h>
# endif

/**
** Name:	grerr.h -	Graphics System Error Number Definitions
**
**	Mapping of Old error code file numbers to ER codes
*/

# define BAD_DEVICE	E_GR0025_Unsupported_device
# define BAD_FILE	E_GR0026_Cannot_open_file
# define BAD_GRAPH	E_GR0027_No_graph
# define BAD_LOCATOR	E_GR0028_No_mouse
# define BAD_EDEVICE	E_GR0029_Not_edit_device
# define BAD_DEVWIN	E_GR002A_Device_window
# define BAD_SAVE	E_GR002B_Error_saving
# define BAD_CGFILE	E_GR002C_Bad_encoded_graph_fil
# define BAD_DECOMPILE	E_GR002D_Error_decoding_graph
# define GR_EXISTS	E_GR002E_Graph_exists
# define BAD_GRNAME	E_GR002F_name_choice
# define MAX_DEP	E_GR0030_too_many_series
# define MAX_STR	E_GR0031_too_many_strings
# define MAX_DATA	E_GR0032_too_many_points
# define DUP_POINTS	E_GR0033_dup_x_y
# define ADD_BAR	E_GR0034_bars_created
# define TOO_EARLY	E_GR0035_before_1902
# define NULL_X_OR_Y	E_GR0036_NULL_x_y
# define IS_INTERVAL	E_GR0037_intervals
# define GDNOTAB	E_GR0038_table_retrieve
# define GDNOX	E_GR0039_independent
# define GDNOY	E_GR003A_dependent
# define GDNOZ	E_GR003B_series
# define GDNOROWS	E_GR003C_no_rows
# define GDNOTEMP	E_GR003D_temp_file
# define GDDEFLT	E_GR003E_default_data
# define GDBADTBL 	E_GR0048_bad_tblname	
# define GDBADCOL 	E_GR0049_bad_colname	
# define SAME_HILO	E_GR003F_RANGE
# define CVALREADY	E_GR0040_Graph_already_convert
# define CVBAD_SORT	E_GR0043_Sort_spec
# define CVBAD_QUERY	E_GR0041_converting_query
# define CVBAD_GRAF	E_GR0042_graph_struct
# define GDTABBAD	E_GR004A_BAD_ID_TABLE
# define GDXBAD		E_GR004B_BAD_ID_X
# define GDYBAD		E_GR004C_BAD_ID_Y
# define GDZBAD		E_GR004D_BAD_ID_Z
# define GDXDTYPE	E_GR004E_BAD_X_DTYPE
# define GDYDTYPE	E_GR004F_BAD_Y_DTYPE
# define GDZDTYPE	E_GR0050_BAD_Z_DTYPE
