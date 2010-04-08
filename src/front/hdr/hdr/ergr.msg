/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ergr.msg -	Message file for class GR
**
** Description:
**	This file defines the messages for class GR.
**
** History:
**	 5-aug-1987 (bobm)
**		Created.
**	10/02/92 (dkh) - Added messages for owner.table support.
**/

# define	_GR_CLASS	72
/*
**	gr.c
*/
/*
**	fatal error - not supposed to happen
*/
S_GR0001_STalloc_err	"Error from STalloc in catSortString" /* DIAG */
/*
**	grcoerce.c
*/
/*
**	fatal error - not supposed to happen
*/
S_GR0002_format_error	"GRcoerce_type: error encountered setting up format" /* DIAG */
S_GR0003_adc_isnull	"GRcoerce_type: adc_isnull failure" /* DIAG */
S_GR0004_adc_call	"GRcoerce_type: bad adc calls in NULL date" /* DIAG */
S_GR0005_fmt_fail	"GRcoerce_type: fmt_format failure" /* DIAG */
S_GR0006_afe_clinstd	"GRcoerce_type: unexpected failure of afe_clinstd" /* DIAG */
S_GR0007_bad_type	"GRcoerce_type: bad type %0d" /* DIAG */
S_GR0008_adc_fail	"GRcoerce_type: unexpected adc failure" /* DIAG */
S_GR0009_bad_type	"GRzero_pt: bad type %0d" /* DIAG */
/*
**	grnameok.qc
*/
E_GR000A_invalid_name	"%0c is an invalid name for a Graph."
E_GR000B_already_exists	"Graph %0c already exists for owner %1c."
/*
**	grstore.c
*/
/*
**	fatal error - not supposed to happen
*/
S_GR0010_bad_class	"GRretobj bad class: %0d" /* DIAG */
/*
**	maperr.c
*/
/*
**	fatal error - not supposed to happen
*/
S_GR0013_MAPRECOVER	"MAPRECOVER - unable to graph with default data - abort" /* DIAG */
/*
**	mapview.qc
*/
/*
**	fatal errors - not supposed to happen
*/
S_GR0014_x_flag	"MAPVIEW: flag for x column not found" /* DIAG */
S_GR0015_tmpfil_close	"MAPVIEW: xyzmap unable to close tmpfile prior to read" /* DIAG */
S_GR0016_tmpfil_open	"MAPVIEW: xyzmap unable to open temp file to read" /* DIAG */
S_GR0017_tmpfil_close	"MAPVIEW: xyzmap unable to close tmpfile after read" /* DIAG */
S_GR0018_ds_bad_type	"MAPVIEW: datastore - unknown type %0d" /* DIAG */
S_GR0019_Num_ser_err	"MAPVIEW: Number of series error" /* DIAG */
S_GR001A_depdat_array	"MAPVIEW: depdat array description error" /* DIAG */
S_GR001B_EOF	"MAPVIEW: rdfrtmp unexpected file end" /* DIAG */
S_GR001C_Write	"MAPVIEW: wrtotmp unable to write file" /* DIAG */
S_GR001D_adc_compare	"MAPVIEW: adc_compare returns failure" /* DIAG */
S_GR001E_adc_isnull	"MAPVIEW: adc_isnull returns failure" /* DIAG */
/*
**	qryutil.c
*/
/*
**	fatal errors - not supposed to happen
*/
S_GR0021_afe_opid	"QRYUTIL: afe_opid failure" /* DIAG */
S_GR0022_afe_cancoerce	"QRYUTIL: afe_cancoerce failure" /* DIAG */
/*
** Format string used to fill in unmapped fields - argument is field name
** Keep in mind that this string is DRAWN as graphics, so don't make it
** too long.
*/
F_GR0001_Field_map	"Field: %0c"
/*
**	grstore.c
*/
/*
** Fatal error - not supposed to happen
*/
S_GR0023_GRgetobj_bad_class	"GRgetobj bad class: %0d" /* DIAG */
/*
**	error.23
*/
E_GR0025_Unsupported_device	"Unsupported graphics device, '%0c'."
E_GR0026_Cannot_open_file	"Cannot open file for graphics output.\nFile name is '%0c'."
E_GR0027_No_graph	"Graph '%0c' does not exist or is not owned by you."
E_GR0028_No_mouse	"Device '%0c' does not support locator device.\nCheck \"Gl\" capability in TERMCAP."
E_GR0029_Not_edit_device	"Device '%0c' does not support editing:  graphics are not erasable.\nCheck \"Gi\" capability in TERMCAP."
E_GR002A_Device_window	"Device '%0c' graphics window defined incorrectly.\nCheck TERMCAP."
E_GR002B_Error_saving	"Error saving graph '%0c'."
E_GR002C_Bad_encoded_graph_fil	"Bad encoded graph file '%0c'."
/*%
** Explanation
**	User specified or default copygraph file does not contain proper
**	encoding data.  File was probably not produced with "copygraph out".
**
** System Status
**	Copygraph will abort
**
** User Action
**	Specify a file that contains an encoded graph.
*/
E_GR002D_Error_decoding_graph	"Error decoding graph from file '%0c'."
E_GR002E_Graph_exists	"Graph '%0c' already exists."
E_GR002F_name_choice	"'%0c' is not a valid choice for a graph name."
E_GR0030_too_many_series	"Warning: Too many series values.  Extra series ignored."
E_GR0031_too_many_strings	"Warning: Too many data string values.  Excess ignored."
E_GR0032_too_many_points	"Warning: Too many data points to plot.  Extra points ignored."
E_GR0033_dup_x_y	"Warning: Duplicate X values within a single series were ignored."
E_GR0034_bars_created	"Warning: Bars were created for missing X values."
E_GR0035_before_1902	"Warning: Data points with dates prior to 1902 were ignored."
E_GR0036_NULL_x_y	"Warning: Data points with NULL X or Y values were ignored."
E_GR0037_intervals	"Warning: Data points with date intervals were ignored."
E_GR0038_table_retrieve	"Unable to retrieve table or view '%0c'."
E_GR0039_independent	"Unable to retrieve independent axis column '%0c'."
E_GR003A_dependent	"Unable to retrieve dependent axis column '%0c'."
E_GR003B_series	"Unable to retrieve series column '%0c'."
E_GR003C_no_rows	"Table or view '%0c' has no rows."
E_GR003D_temp_file	"Unable to create temporary file for data retrieval."
E_GR003E_default_data	"Unable to retrieve specified data. Default data will be used."
E_GR003F_RANGE	"The specified range of values for the axis is invalid.\nThe maximum value may not equal the minimum value for manually\nscaled axes."
E_GR0040_Graph_already_convert	"Graph already converted."
E_GR0041_converting_query	"Error converting query for graph."
E_GR0042_graph_struct	"Error converting graph structures for graph."
E_GR0043_Sort_spec	"Error converting sort specification for graph."
E_GR0044_NoCGFile	"Encoded graph file '%0c' does not exist."
/*%
** Explanation
**	User specified or default copygraph file does not exist.
**
** System Status
**	Copygraph will abort
**
** User Action
**	Specify a file that exists.
*/
E_GR0045_bad_plot_dev	"Illegal plot device type '%0c' in TERMCAP"
E_GR0046_no_mf_driver	"No metafile driver in TERMCAP for device '%0c'"
E_GR0047_bad_mf_driver	"Illegal metafile driver type '%0c' in TERMCAP"

E_GR0048_bad_tblname
"The name of table or view '%0c' is an SQL keyword\n
The name of the table or view to be graphed is a reserved SQL keyword.  This
makes it impossible to access the table or view.  Choose another table or
view to graph."

E_GR0049_bad_colname
"The name of column '%0c' is an SQL keyword\n
The table or view to be graphed contains a column whose name is a reserved
SQL keyword.  This makes it impossible to access the table or view.  Choose
another table or view to graph."

E_GR004A_BAD_ID_TABLE
"'%0c' is a bad identifier.\n  The name entered for the table is
an invalid identifier.  Please check the spelling or if the identifier
needs to be quoted as a delimited identifier."

E_GR004B_BAD_ID_X
"'%0c' is a bad identifier.\n  The name entered as the independent
axis column is an invalid identifier.  Please check the spelling or
if the identifier needs to be quoted as a delimited identifier."

E_GR004C_BAD_ID_Y
"'%0c' is a bad identifier.\n  The name entered as the dependent axis
column is an invalid identifier.  Please check the spelling or if the
identifier needs to be quoted as a delimited identifier."

E_GR004D_BAD_ID_Z
"'%0c' is a bad identifier.\n  The name entered as the series column is
an invalid identifier.  Please check the spelling or if the identifier
needs to be quoted as a delimited identifier."

E_GR004E_BAD_X_DTYPE
"Unsupported datatype column used.\n  Independent axis column '%0c' is defined
on a datatype that is not supported by Vigraph.  Please use another column
for the independent axis."

E_GR004F_BAD_Y_DTYPE
"Unsupported datatype column used.\n  Dependent axis column '%0c' is defined
on a datatype that is not supported by Vigraph.  Please use another column
for the dependent axis."

E_GR0050_BAD_Z_DTYPE
"Unsupported datatype column used.\n  Series column '%0c' is defined
on a datatype that is not supported by Vigraph.  Please use another column
for the series column."
