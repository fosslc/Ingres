/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ervg.msg -	Message file for class VG
**
** Description:
**	This file defines the messages for class VG.
**
** History:
**	 6-aug-1987 (bobm)
**		Created.
**	16-aug-88 (bruceb)
**		Added new messages F_VG00C[EF]_ftext[12] to replace
**		F_VG008[ABCDEF]_ftext[12345].  The new messages are
**		designed to center in two lines of 70 spaces each.
**	10/02/92 (dkh) - Added messages to support owner.table and delimids.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**/

# define	_VG_CLASS	76
/*
**	topframe.qc
*/
/*
**	Menu items for Catalog Frame
*/
F_VG0001_CreateBlank	"CreateBlank"
F_VG0002_Sketch	"Sketch"
F_VG0003_Profile	"Profile"
F_VG0004_MapData	"MapData"
F_VG0005_Attribute	"Attributes"
/*
**	topframe.qc
*/
/*
**	status message used during graph retrieval.
*/
F_VG0006_Retrieving	"Retrieving graph %0c from the database . . ."
/*
**	Error message for command line choice of non-existent graph
*/
E_VG0001_Graph_not_found	"Graph not found: %0c. Will edit new graph."
/*
**	Details frame title
*/
S_VG0002_Graph_Details	"VIGRAPH - MoreInfo about a Graph"
/*
**	Message used following plot, if there is a sketch on the screen,
**	while re-retrieving sketched graph from database
*/
S_VG0003_Recovering_sketch	"Recovering sketch of %0c"
/*
**	Used with "find" on an empty table
*/
S_VG0004_No_graphs_in_catalog	"No graphs in catalog"
/*
**	Fatal error - not supposed to happen
*/
S_VG0005_Default_dat	"Can't set up default data"	/* DIAG */
/*
**	options.qc
*/
/*
**	Message used while saving profile
*/
S_VG0006_Saving_profile	"Saving profile in the database"
E_VG0007_Pr_Save_failed	"Save failed.  Return to continue."
/*
**	layout.qc
*/
/*
**	Status line messages for layout
**	Fast because status line messages are only stored as pointers,
**	and these two are inevitably used
*/
F_VG0007_Editing_entire_graph	"Editing entire graph"
F_VG0008_Editing_new_graph	"Editing new graph"
/*
**	Menu items for main layout
*/
F_VG0009_Size	"Size"
/*
**	layout.qc
*/
F_VG000A_Layer	"Layer"
/*
**	Menu items for types of objects on "Create" choice
*/
F_VG000B_BarChart	"BarChart"
F_VG000C_LineChart	"LineChart"
F_VG000D_ScatterChart	"ScatterChart"
F_VG000E_Piechart	"PieChart"
F_VG000F_Legend	"Legend"
F_VG0010_Trim	"Trim"
F_VG0011_Field	"Field"
/*
**	Menu items for "Move" choices
*/
F_VG0012_Left	"Left"
F_VG0013_Center	"Center"
F_VG0014_Horizontal	"Horizontal"
F_VG0015_Vertical	"Vertical"
F_VG0016_Both	"Both"
F_VG0017_Right	"Right"
F_VG0018_Top	"Top"
F_VG0019_Bottom	"Bottom"
F_VG001A_Back	"Back"
F_VG001B_Front	"Front"
/*
**	Status line message while saving graph
*/
S_VG0008_Saving_graph	"Saving graph - currently named %0c"
/*
**	Chart types - visible on "MapData" screen
*/
F_VG001C_bar	"bar"
F_VG001D_line	"line"
F_VG001E_scatter	"scatter"
F_VG001F_pie	"pie"
F_VG0020_empty	"empty"
F_VG0021_text	"text"
F_VG0022_composite	"composite"
/*
**	Error messages used from main layout
*/
E_VG0009_Del_no_comp	"No component chosen to delete."
E_VG000A_Ed_no_comp	"No component chosen to edit."
E_VG000B_Ed_group	"Can't edit a group of components"
E_VG000C_Move_no_comp	"No component chosen to move."
E_VG000D_Size_no_comp	"No component chosen to have size changed."
E_VG000E_1_layer	"There is only one layer."
E_VG000F_Too_far_right	"Too far right for upper left corner"
E_VG0010_Too_far_down	"Too far down for upper left corner"
E_VG0011_Empty_column_name	"Empty column name - converting into trim"
E_VG0012_Illegal_name	"Illegal name"
E_VG0013_A_field_exists	"A field with that name already exists"
E_VG0014_Already_on_top	"Already on top."
/*
** not supposed to happen
*/
S_VG0015_Edit_of_bad_component	"Edit of bad component type"	/* DIAG */
/*
**	Verbs for "Do you really want to **** without saving?" routine
*/
F_VG0023_end	"end"
F_VG0024_quit	"quit"
/*
**	layout.qc
*/
/*
**	Status line messages
*/
S_VG0016_Type_of_obj	"Specify type of object to create"
S_VG0017_Type_of_move	"Specifying type of move"
S_VG0018_Direction_to_center	"Specify direction in which to center"
/*
**	Prompt for lower right box corner on component creation
*/
S_VG0019_Point_to_lower_right	"Point to lower right corner of box for new component"
/*
**	Field name prompt
*/
S_VG001A_fld_name_prompt	"Internal name for new field:"
/*
**	Prompt for entry of new trim text
*/
S_VG001B_Enter_text	"Enter text for new trim:"
/*
**	Prompt for title of new legend
*/
S_VG001C_Legend_Title	"Legend Title:"
/*
**	Edit submenu menu items
**	Attached comments designate menu items which only apply to the
**	given component type - AXILE means LINE / SCATTER / BAR collectively
*/
F_VG0025_Labels	"Labels"
F_VG0026_Cluster	"Cluster"	/* BAR */
F_VG0027_Stack	"Stack"			/* BAR */
F_VG0028_Vertical	"Vertical"	/* BAR */
F_VG0029_Horizontal	"Horizontal"	/* AXILE */
F_VG002A_CreateAxis	"CreateAxis"	/* AXILE */
F_VG002B_DeleteAxis	"DeleteAxis"	/* AXILE */
F_VG002C_EditAxis	"EditAxis"	/* AXILE */
F_VG002D_Margin	"Margin"
F_VG002E_ShowDetail	"ShowDetail"
F_VG002F_Color	"Color"
F_VG0030_ChangeType	"ChangeType"	/* AXILE */
F_VG0031_LineChart	"LineChart"	/* AXILE */
F_VG0032_ScatterChart	"ScatterChart"	/* AXILE */
F_VG0033_BarChart	"BarChart"	/* AXILE */
F_VG0034_LeftAxis	"LeftAxis"	/* AXILE */
F_VG0035_BottomAxis	"BottomAxis"	/* AXILE */
F_VG0036_RightAxis	"RightAxis"	/* AXILE */
F_VG0037_TopAxis	"TopAxis"	/* AXILE */
F_VG0038_AxisHeader	"AxisHeader"	/* AXILE */
F_VG0039_ChooseColor	"ChooseColor"
F_VG003A_ShowColors	"ShowColors"
F_VG003B_ChooseFont	"ChooseFont"
F_VG003C_ShowFonts	"ShowFonts"
F_VG003D_ChooseHatch	"ChooseHatch"
F_VG003E_ShowHatches	"ShowHatches"
F_VG003F_Placement	"Placement"
F_VG0040_Font	"Font"
F_VG0041_Format	"Format"
F_VG0042_Background	"Background"
F_VG0043_TitleText	"TitleText"	/* LEGEND */
F_VG0044_SeriesText	"SeriesText"	/* LEGEND */
F_VG0045_Title	"Title"			/* LEGEND */
F_VG0046_NoFill	"NoFill"		/* LINE */
F_VG0047_Fill	"Fill"			/* LINE */
F_VG0048_Left_margin	"Left"
F_VG0049_Right_margin	"Right"
F_VG004A_Top_margin	"Top"
F_VG004B_Bottom_margin	"Bottom"
F_VG004C_Explode	"Explode"	/* PIE */
F_VG004D_Rotate	"Rotate"		/* PIE */
F_VG004F_Box	"Box"
F_VG0050_AxisLines	"AxisLines"	/* AXILE */
F_VG0051_HeaderText	"HeaderText"	/* AXILE */
F_VG0052_WholeAxis	"WholeAxis"	/* AXILE */
F_VG0053_Header	"Header"		/* AXILE */
F_VG0054_Text	"Text"
F_VG0055_Rename	"Rename"		/* FIELD */
F_VG0056_NoBox	"NoBox"
F_VG0057_Box	"Box"
F_VG0058_Transparent	"Transparent"
F_VG0059_Opaque	"Opaque"
/*
**	Status line messages for edit submenus
*/
/*
**	edbar.qc
*/
S_VG001D_Editing_bar_chart	"Editing bar chart"
S_VG001E_Choose_series_of_bars	"Choose series of bars"
S_VG001F_new_chart_type	"Choose a new chart type"
/*
**	edchcr.qc
*/
S_VG0020_Creating_new_subcomp	"Creating new subcomponent"
S_VG0021_new_hdr_text	"Entering new header text"
/*
**	edcolor.qc
*/
S_VG0022_Choose_color	"Choose color for %0c"
/*
**	edfont.qc
*/
S_VG0023_Choose_font	"Choose font for %0c"
/*
**	edhatch.qc
*/
S_VG0024_Choose_hatch	"Choose hatch for %0c"
/*
**	edlab.qc
*/
S_VG0025_Editing_data_labels	"Editing data labels"
S_VG0026_Editing_format_string	"Editing format string"
/*
**	edleg.qc
*/
S_VG0027_Editing_legend	"Editing legend"
S_VG0028_leg_color	"Choose legend component to color"
S_VG0029_leg_font	"Choose legend component for font change"
S_VG002A_Editing_legend_title	"Editing legend title"
/*
**	edline.qc
*/
S_VG002B_Editing_line_chart	"Editing line chart"
/*
**	edmargin.qc
*/
S_VG002C_Specify_a_margin_area	"Specify a margin area"
/*
**	edpie.qc
*/
S_VG002D_Editing_pie_chart	"Editing pie chart"
/*
**	edscat.qc
*/
S_VG002E_Editing_scatter_chart	"Editing scatter chart"
/*
**	edscomp.qc
*/
S_VG002F_comp_color	"Specify component to color"
S_VG0030_Choose_what_to_delete	"Choose what to delete"
S_VG0031_Editing_axis	"Editing %0c axis"
S_VG0032_Enter_new_header_text	"Enter new header text"
S_VG0033_Editing_axis_hdr	"Editing %0c axis header"
S_VG0034_ax_comp_color	"Specify component of axis to color"
S_VG0035_Editing_header_text	"Editing header text"
S_VG0036_Editing_ax_lab	"Editing %0c axis labels"
/*
**	edtrim.qc
*/
S_VG0037_Editing_field	"Editing field"
S_VG0038_Editing_trim	"Editing trim"
S_VG0039_Editing_trim_text	"Editing trim text"
/*
**	edutil.c
*/
/*
**	edbar.qc
*/
/*
**	Names for parts of graphs used in edit submenus.  These
**	Will be the arguments to the status line messages and prompts
**	which take arguments.
*/
F_VG005A_Bar	"Bar"
F_VG005B_labels	"labels"
F_VG005C_legend_background	"legend background"
F_VG005D_legend_title	"legend title"
F_VG005E_legend_descriptions	"legend descriptions"
F_VG005F_Line	"Line"
F_VG0060_left	"left"		/* margin reference */
F_VG0061_right	"right"		/* margin reference */
F_VG0062_top	"top"		/* margin reference */
F_VG0063_bottom	"bottom"	/* margin reference */
F_VG0064_pie_chart_background	"pie chart background"
F_VG0065_DataSet	"DataSet"
F_VG0066_background	"background"
F_VG0067_axis_labels	"axis labels"
F_VG0068_axis_lines	"axis lines"
F_VG0069_axis_hdr_bck	"axis header background"
F_VG006A_axis_header_text	"axis header text"
F_VG006B_data_label	"data label"
F_VG006C_axis_header	"axis header"
F_VG006D_text	"text"
/*
**	Error strings used in the edit submenus
**
**	Those marked DIAG are fatal conditions which are not
**	supposed to happen.
*/
E_VG003A_Axis_has_hdr	"Axis already has a header"
E_VG003B_not_axis	"You aren't pointing to an axis"
E_VG003C_No_margin_change	"No change in the margin"
S_VG003D_no_point_color	"Not pointing at anything to have color changed"
S_VG003E_Bad_subidx_item_on	"Bad subidx item on axis subcomponent" /* DIAG */
E_VG003F_No_point_delete	"Not pointing at anything to delete"
E_VG0040_No_point_edit	"Not pointing to an editable subcomponent"
E_VG0041_No_header_entered	"No header entered"
E_VG0042_fld_name_rqd	"A name for the field is required"
E_VG0043_Illegal_field_name	"Illegal field name"
S_VG0044_chart_swap_bad_types	"chart swap: bad types %0d to %1d" /* DIAG */
S_VG0045_swap_type_map	"Fatal map_view error while changing type" /* DIAG */
/*
**	Prompt strings used in the edit submenus
**
**	"Point" prompts will be displayed while user is expected to point
**	on screen.
**
**	Prompts for numbers are fast strings for internal reasons
*/
S_VG0046_Point_to_color	"Point to color for %0c"
F_VG006E_Color_Number	"Color Number"
S_VG0047_Point_to_font	"Point to font for %0c"
F_VG006F_Font_Number	"Font Number"
S_VG0048_Point_to_hatch	"Point to hatch for %0c"
F_VG0070_Hatch_Number	"Hatch Number"
S_VG0049_Point_to_margin	"Point to new %0c margin"
F_VG0071_Percentage_to_explode	"Percentage to explode"
F_VG0072_first_slice	"Number of first slice to explode"
F_VG0073_last_slice	"Number of last slice to explode"
F_VG0074_Degrees_to_rotate	"Degrees to rotate"
S_VG004A_Internal_Name	"Internal Name:"
/*
**	atfmxln.qc
*/
/*
**	xpoint.qc
*/
/*
**	Display message for pointing routine
*/
F_VG0075_Position_cursor	"Position cursor - use <MENU KEY> to indicate choice"
/*
**	uats.c
*/
/*
**	Fatal errors in undo handling - not supposed to happen
*/
S_VG004B_Undo_depth	"Undo attribute stack depth too great" /* DIAG */
S_VG004C_Undo_pop	"Undo attribute stack popped past zero" /* DIAG */
S_VG004D_undo_map	"Fatal error in map_frame during undo" /* DIAG */
S_VG004E_Bad_undo_type	"Bad object type on undo stack" /* DIAG */
S_VG004F_undo_set_bad_op	"undo_set: bad operation %0d" /* DIAG */
/*
**	Error message for choice of "Undo" with nothing to undo
*/
E_VG0050_Nothing_to_undo	"Nothing to undo"
/*
** More fatal undo errors - not supposed to happen
*/
S_VG0051_undo_bad_op	"undo: bad operation %0d" /* DIAG */
S_VG0052_undo_swap_no_node	"undo_swap: no node" /* DIAG */
S_VG0053_undo_swap_wrong_obj	"undo_swap: wrong object" /* DIAG */
S_VG0054_undo_cannot_chain	"undo: can't put node back on chain" /* DIAG */
/*
**	checkfrm.qc
*/
/*
**	Diagnostic message unused in production code
*/
S_VG0055_reading_form	"reading form from database . . . "
/*
**	Fatal error - not supposed to happen
*/
S_VG0056_unknown_form	"unknown form %0c" /* DIAG */
/*
**	dialogue.qc
*/
/*
**	prompt string (currently not used)
*/
S_VG0057_Graph_name	"Graph name %0c - satisfactory ?"
/*
**	Plot confirmation prompts
*/
S_VG0058_plot_term	"OK to plot on terminal?"
S_VG0059_plot_aterm	"OK to plot on terminal, as device %0c?"
S_VG005A_plot_term_file	"OK to plot on %0c, device type identical to terminal?"
S_VG005B_plot_dev	"OK to plot on %0c, device type %1c?"
/*
**	Fatal error - not supposed to happen
*/
S_VG005C_bad_cnf_check	"bad parameter to cnf_check: %0d" /* DIAG */
/*
**	Prompt for graph name
*/
S_VG005D_Graph_name	"Graph name:"
/*
**	Plot location prompt string
*/
S_VG005E_Plot_Location	"Plot Location:"
S_VG005F_Plot_Device_Type	"Plot Device Type:"
/*
**	Prompt for numeric quantity with default value
*/
F_VG0076_decimal_prompt	"%0c (return for %1d):"
/*
**	Warning issued for round off on entry of decimal where integer
**	is expected
*/
E_VG0060_Rounding_off	"Rounding off to whole number"
/*
**	Error message containing legal range of values for bad
**	numeric input - also contains original prompt
*/
E_VG0061_Numeric_error	"Numeric answer, %0d - %1d required.  %2c:"
F_VG0077_Using_decimal	"Using %0d."
/*
**	Prompts for floating point input.  The same idea as the decimal
**	set, except that the floating point values have been converted
**	to strings before being output, so we have c formats instead of d
*/
F_VG0078_Float	"%0c (return for %1c):"
E_VG0062_Float_error	"Numeric answer %0c - %1c required.  %2c:"
F_VG0079_Using_Float	"Using %0c."
/*
**	glob.c
*/
/*
**	Axis names
*/
F_VG007A_left	"left"
F_VG007B_right	"right"
F_VG007C_top	"top"
F_VG007D_bottom	"bottom"
/*
**	layer.qc
*/
/*
**	Layering frame status messages
*/
S_VG0063_layer_1	"Examining layer 1 of %0d"
S_VG0064_layer	"Examining layer %0d of %1d"
S_VG0065_Everything_deleted	"Everything deleted"
/*
**	Layer frame menu items
*/
F_VG007E_Cover	"Cover"
F_VG007F_Reveal	"Reveal"
E_VG0066_No_component_chosen_t	"No component chosen to delete"
E_VG0067_No_component_chosen_t	"No component chosen to move"
S_VG0068_Point_to_new_location	"Point to new location"
E_VG0069_Moved_above	"Moved above current layer."
E_VG006A_No_more_layers	"No more layers are available."
E_VG006B_Bottom_layer	"Bottom layer."
S_VG006C_empty_push	"Must be on a component to push back."
E_VG006D_Empty_pull	"Must be on a component to pull forward."
E_VG006E_Already_on_top	"Already on top."
/*
**	mapchlp.qc
*/
/*
**	Menu item for choosing names from tablefield
*/
F_VG0080_Select	"Select"
/*
**	mapdata.qc
*/
/*
**	Terminal monitor menu items for mapdata frame
*/
F_VG0081_IQUEL	"IQUEL"
F_VG0082_ISQL	"ISQL"
/*
**	Message used to indicate use of default data
*/
S_VG006F_No_view	"No view/table specified.  Graph will be drawn from default data."
/*
**	Fatal - Not supposed to happen.
*/
S_VG0071_Fields_savearea	"Fields savearea damaged." /* DIAG */
/*
**	Message used on mapdata frame for invalid name
*/
E_VG0072_Not_a_valid_view	"Not a valid view or table name."
/*
**	Message used when user tries to make a tablefield choice when
**	not on a ablefield
*/
S_VG0073_First_move_cursor	"First move cursor to tablefield."
E_VG0074_Not_good_fld	"Not an appropriate field for list selection."
E_VG0075_No_view	"No view or table specified to select columns from."
E_VG0076_Error_from_Quel	"Error %0d from Quel terminal monitor."
E_VG0078_Error_from_SQL	"Error %0d from SQL terminal monitor."
/*
**	Fatal error - not supposed to happen
*/
S_VG0079_mapdata_alloc	"mapdata: memory allocation failure" /* DIAG */
/*
**	Mapdata form text dependent on chart type.  Explains X / Y / Z
**	columns.
*/
/*
**	CAUTION:  MUST right justify these strings to 32 char. width
*/
F_VG0083_pie_x	"           Pie slice labels (X):"
F_VG0084_pie_y	"            Pie slice sizes (Y):"
F_VG0085_hbar_x	"              Vertical axis (X):"
F_VG0086_hbar_y	"            Horizontal axis (Y):"
F_VG0087_series_z	"     Optional series column (Z):"
F_VG0088_x	"            Horizontal axis (X):"
F_VG0089_y	"              Vertical axis (Y):"
/*
**	Explanatory text placed on form if fields exist in the graph.
**	There are five rows available on the form, though the current
**	text fits in 4 rows.  Do not make strings any longer than the
**	longest of these without checking the width available on the form
*/
F_VG008A_ftext1	"Each \"field\" in the graph"
F_VG008B_ftext2	"layout will display the data"
F_VG008C_ftext3	"obtained from the first row"
F_VG008D_ftext4	"of its corresponding column."
F_VG008E_ftext5	""
/*
**	Menu item used in mapdata frame to bring up tablefield of
**	available items to fill in a field
*/
F_VG008F_ListChoices	"ListChoices"
/*
**	maphlp.qc
*/
/*
**	move.c
*/
/*
**	Fatal error - not supposed to happen
*/
S_VG007A_mv_comp	"mv_comp: illegal type %0d" /* DIAG */
/*
**	Error message for attemp to move object off screen
*/
E_VG007B_Move_off_screen	"Move would be off screen"
E_VG007C_Move_nothing	"Move doesn't do anything"
/*
**	plotcall.qc
*/
/*
**	Messages used during Plot operations
*/
F_VG0090_Plotting	"Plotting . . . "
F_VG0091_plotted	"Graph plotted."
E_VG007D_Plot_failed	"Plot failed."
/*
**	pushpull.c
*/
E_VG007E_on_bottom	"already on the bottom"
/*
**	size.qc
*/
/*
**	Status line message for size operation
*/
S_VG007F_Choose_corner_to_move	"Choose corner to move"
/*
**	Corner choosing menu items for size operation
*/
F_VG0092_ULeft	"ULeft"
F_VG0093_URight	"URight"
F_VG0094_LLeft	"LLeft"
F_VG0095_LRight	"LRight"
/*
**	lower/upper right/left words for sizing status line message
*/
F_VG0096_right	"right"
F_VG0097_left	"left"
F_VG0098_upper	"upper"
F_VG0099_lower	"lower"
S_VG0080_Point_to_new_corner	"Point to new %0c %1c corner"
E_VG0081_No_change_in_size	"No change in size"
/*
**	svutil.qc
*/
/*
**	Save Frame title
*/
F_VG009A_Save_Frame	"VIGRAPH - Saving a Graph"
/*
**	String used to confirm end / quit when changes have not been saved
*/
S_VG0082_Changes_not_saved	"Changes have not been saved.  Do you want to %0c anyway ?"
/*
**	vig.c
*/
/*
**	Fatal error - not supposed to happen
*/
S_VG0083_test_init	"vig: cannot initialize for testing, \"%0c\"" /* DIAG */
S_VG0084_cannot_open_dump	"vig: cannot open dump file, %0c\n" /* DIAG */
S_VG0085_exc_handle	"vig: could not set exception handler" /* DIAG */
/*
**	vghelp.c
*/
/*
**	Help screen titles
*/
S_VG0086_Cat_frame	"VIGRAPH Catalog Frame"
S_VG0087_Cat_util	"VIGRAPH Catalog Utilities Frame"
S_VG0088_profile	"VIGRAPH Catalog Profile Frame"
S_VG0089_layout	"VIGRAPH Main Layout Frame"
S_VG008A_move	"VIGRAPH - Move"
S_VG008B_Move_Center	"VIGRAPH - Move - Center"
S_VG008C_Save	"VIGRAPH - Save"
S_VG008D_layer	"VIGRAPH - Layer"
S_VG008E_Save_Frame	"VIGRAPH - Saving a Graph"
S_VG008F_Map_Frame	"VIGRAPH - Map Frame"
S_VG0090_Sort_Frame	"VIGRAPH - Sort Frame"
S_VG0091_Create	"VIGRAPH - Create"
S_VG0092_Trim_color	"VIGRAPH - Trim - Color"
S_VG0093_Trim	"VIGRAPH - Trim"
S_VG0094_Field_color	"VIGRAPH - Field - Color"
S_VG0095_Field	"VIGRAPH - Field"
S_VG0096_Bar_newchart	"VIGRAPH - BarChart - NewChart"
S_VG0097_Barchart	"VIGRAPH - BarChart"
S_VG0098_Margin	"VIGRAPH - Border"
S_VG0099_Color	"VIGRAPH - Color"
S_VG009A_line_dash	"VIGRAPH - LineChart - Dash"
S_VG009B_line_connect	"VIGRAPH - LineChart - Connect"
S_VG009C_LineChart	"VIGRAPH - LineChart"
S_VG009D_scatter_mark	"VIGRAPH - ScatterChart - Mark"
S_VG009E_scatter_newchart	"VIGRAPH - ScatterChart - NewChart"
S_VG009F_ScatterChart	"VIGRAPH - ScatterChart"
S_VG00A0_PieChart	"VIGRAPH - PieChart"
S_VG00A1_Font	"VIGRAPH - Font"
S_VG00A2_Label	"VIGRAPH - Label"
S_VG00A3_Legend	"VIGRAPH - Legend"
S_VG00A4_leg_color	"VIGRAPH - Legend - Color"
S_VG00A5_leg_font	"VIGRAPH - Legend - Font"
S_VG00A6_Cr_ax	"VIGRAPH - Create"
S_VG00A7_Axis	"VIGRAPH - Axis"
S_VG00A8_Ax_hdr	"VIGRAPH - Axis Header"
S_VG00A9_Ax_lab	"VIGRAPH - Axis label"
S_VG00AA_bar_attr	"VIGRAPH - BarChart - Attributes"
S_VG00AB_pie_attr	"VIGRAPH - PieChart - Attributes"
S_VG00AC_line_attr	"VIGRAPH - LineChart - Attributes"
S_VG00AD_scat_attr	"VIGRAPH - ScatterChart - Attributes"
S_VG00AE_leg_attr	"VIGRAPH - Legend - Attributes"
S_VG00AF_ax_attr	"VIGRAPH - Axis - Attributes"
S_VG00B0_fld_attr	"VIGRAPH - Field - Attributes"
S_VG00B1_trim_attr	"VIGRAPH - Trim - Attributes"
S_VG00B2_ch_ax_attr	"VIGRAPH - Chart - AxisAttributes"
S_VG00B3_List_tab	"VIGRAPH - Map Frame - ListTables"
S_VG00B4_Select_corner	"VIGRAPH - Size - Select corner"
S_VG00B5_ax_sty	"VIGRAPH - Axis style"
S_VG00B6_ax_sty_range	"VIGRAPH - Axis style - Range"
S_VG00B7_ax_sty_grid	"VIGRAPH - Axis style - Grid lines"
S_VG00B8_ax_sty_tick	"VIGRAPH - Axis style - Ticks"
S_VG00B9_hatch	"VIGRAPH - Hatch"
S_VG00BA_line_newchart	"VIGRAPH - LineChart - NewChart"
/*
**	vghelp.c
*/
S_VG00BB_line_mark	"VIGRAPH - LineChart - Mark"
S_VG00BC_list_col	"VIGRAPH - Map Frame - ListColumns"
/*
**	Status line message for help selection
*/
S_VG00BD_Help_Selection	"Help Selection"
/*
**	atfmxln.qc
*/
/*
**	Axis Header justification keywords expected on form
*/
F_VG009B_center	"center"
F_VG009C_left	"left"
F_VG009D_right	"right"
F_VG009E_bottom	"bottom"
F_VG009F_top	"top"
/*
**	Keywords used on attribute form for axis label styles
*/
F_VG00A0_none	"none"
F_VG00A1_all	"all"
F_VG00A2_outside	"outside"
F_VG00A3_middle	"middle"
F_VG00A4_firstdrop	"firstdrop"
F_VG00A5_lastdrop	"lastdrop"
/*
**	Keywords for axis label placements on axis attributes form
*/
F_VG00A6_on	"on"
F_VG00A7_between	"between"
/*
**	Keywords for axis grid style on axis attributes form
*/
F_VG00A8_none	"none"
F_VG00A9_solid	"solid"
F_VG00AA_dashed	"dashed"
/*
**	Keywords for axis tick placement on axis attributes form
*/
F_VG00AB_outside	"outside"
F_VG00AC_inside	"inside"
F_VG00AD_cross	"cross"
F_VG00AE_none	"none"
/*
**	Keywords for line connection types on linechart attributes form
*/
F_VG00AF_straight	"straight"
F_VG00B0_step	"step"
F_VG00B1_regression	"regression"
F_VG00B2_curve	"curve"
F_VG00B3_none	"none"
/*
**	Keywords for types of marks on line / scatter chart attributes form
*/
F_VG00B4_star	"star"
F_VG00B5_dot	"dot"
F_VG00B6_cross	"cross"
F_VG00B7_circle	"circle"
F_VG00B8_x	"x"
F_VG00B9_square	"square"
F_VG00BA_none	"none"
/*
**	Keywords for line types on linechart attributes form
*/
F_VG00BB_solid	"solid"
F_VG00BC_dashed	"dashed"
F_VG00BD_dotted	"dotted"
F_VG00BE_both	"both"
/*
**	Value to display on attributes form for unknown value.  Indicates
**	bad data in the graph definition, and should not normally appear
*/
F_VG00BF_BadVal_Key	"?"
/*
**	Fatal error - not supposed to happen
*/
S_VG00BE_chget_bad_type	"chget: bad type %0d" /* DIAG */
S_VG00BF_chput_bad_type	"chput: bad type %0d" /* DIAG */
S_VG00C0_dpget_bad_type	"dpget: bad type %0d" /* DIAG */
/*
**	Indicator used on attribute form tablefields for series which do
**	not exist in the current data mapping.  18 characters available,
**	2 digits needed for number.
*/
F_VG00C0_Unmapped_series	"Unmapped series %0d"
/*
** Fatal error - not supposed to happen
*/
S_VG00C1_dpput_bad_type	"dpput: bad type %0d" /* DIAG */
/*
**	atfax.qc
*/
/*
**	Label for RIGHT, LEFT, etc axis
*/
F_VG00C1_Axis_format	"%0c AXIS"
/*
**	Menu items for Axis Attributes Form
*/
F_VG00C2_NextAxis	"NextAxis"
F_VG00C3_PreviousAxis	"PreviousAxis"
/*
**	Fatal error - not supposed to happen
*/
S_VG00C2_atf_axsub_bad_mask	"atf_axsub: bad mask - %0x" /* DIAG */
/*
**	atfbar.qc
*/
/*
**	Menu item used to reach axis attributes from axile chart forms
*/
F_VG00C4_AxisAttributes	"AxisAttributes"
/*
**	atffield.qc
*/
/*
**	error for blank field name on attributes form
*/
E_VG00C3_No_fld_name	"Field name must be specified"
E_VG00C4_invalid_fld_name	"%0c is not a valid field name"
/*
**	atfleg.qc
*/
/*
**	atfline.qc
*/
/*
**	Tablefield column titles used on linechart attributes form
*/
F_VG00C5_Series	"Series"
F_VG00C6_Line_Color	"Line Color"
F_VG00C7_Fill_Color	"Fill Color"
F_VG00C8_Fill_Hatch	"Fill Hatch"
F_VG00C9_Connection_Type	"Connection Type"
F_VG00CA_Dash	"Dash"
F_VG00CB_Mark	"Mark"
/*
**	Fatal error - not supposed to happen
*/
S_VG00C5_attr_top_bad_type	"attr_top: bad type" /* DIAG */
S_VG00C6_attr_top_bad_mask	"attr_top: bad mask - %0x" /* DIAG */
/*
**	Undo & MoreInfo menu items
*/
F_VG00CC_Undo	"Undo"
F_VG00CD_MoreInfo	"MoreInfo"
/*
**	Fatal error - not supposed to happen.
*/
S_VG00C7_Could_not_get_form	"Could not get form %0c"	/* DIAG */
/*
** topframe.qc
*/
E_VG00C8_Could_not_Retrieve	"Could not retrieve graph %0c from database"
/*
** mapdata.qsc
*/
F_VG00CE_ftext1
"        Each \"field\" in the graph layout will display the data"
F_VG00CF_ftext2
"       obtained from the first row of its corresponding column."
/*%
** Explanation:
**	These messages explain the meaning of the 'fields' tablefield on
**	the mapdata form.  The fields tablefield is displayed if fields
**	exist in the graph.  There are two rows of 70 characters each
**	available for use in this explanation.  Centering the text is up
**	to this message since such control doesn't exist in the forms
**	system.
**
** System Status:
**	Explanatory text on a form.
**
** User Action:
**	None.
*/

/* Product Name */
F_VG00D0_VIGRAPH		"VIGRAPH"	

/*
** mapdata.qsc
*/
S_VG00C9_Mapping	"Retrieving data to graph . . ."
/*%
** Explanation:
**	used while retrieving data to graph from the database
*/

/*
**	Create a new frame (in topframe.qsc)
*/
F_VG00D1_CreateGraph	"Creating a Graph"
F_VG00D2_CopyGraph	"Choosing a Graph to copy"
F_VG00D3_Retrieving	"Retrieving graph names from the database . . ."
F_VG00D4_GraphName	"Graph Name"
F_VG00D5_graphs		"graphs"
F_VG00D6_Graph_from_Graph	"Create Graph from Existing Graph"


S_VG00D0_Duplicate      "Duplicate"
S_VG00D1_Duplicate_expl "Create a graph by copying an existing graph"
S_VG00D2_Blank  	"Blank"
S_VG00D3_Blank_expl     "Create a graph starting with a blank screen"
E_VG00D4_no_graph
"A graph name must be entered.\n
A graph name needs to be entered here in order to create a new
graph.  If you do not wish to create a graph based upon an existing 
graph, use the 'Cancel' menuitem."
/*%
** Explanation:
**	The user chose "OK" when the graph name field was empty.
**
** System Status:
**	Not an error.
**
** User Action:
**	Enter a graph name or choose "Cancel".
*/



E_VG00D5_Graph_not_found	
"Graph not found\n
There is no graph named '%0c' in the graph catalogs."
/*%
** Explanation:
**	The user chose to duplicate a non-existent graph.
**
** System Status:
**	Not an error.
**
** User Action:
**	Choose another graph.
*/

/*
** layout.qsc -- exiting save frame 
*/
/* Help banner */
F_VG00D7_ExitSaveFrameBanner	"Exiting Save Frame"

/* Menuitem Explanations */
F_VG00D8_Catalog		"Return to graph catalogs"
F_VG00D9_Edit			"Edit current graph"
F_VG00DA_Quit			"Quit VIGRAPH"

/*
** attrtop.qsc: error attempting to edit the attributes of an empty graph
*/
E_VG00DB_empty_graph
"Graph is empty.\n
You cannot edit the attributes of an empty graph. Before selecting this option, edit the graph and create some trim, a field or a type of chart."

E_VG00DC_BAD_X_NAME
"Invalid column name specified.\n
'%0c' is not a valid column name for the independent axis.
Please check the name for correct
spelling as well as proper use of quotes if using delmited identifiers."

E_VG00DD_BAD_Y_NAME
"Invalid column name specified.\n
'%0c' is not a valid column name for the dependent axis.
Please check the name for correct
spelling as well as proper use of quotes if using delmited identifiers."

E_VG00DE_BAD_Z_NAME
"Invalid column name specified.\n
'%0c' is not a valid column name for the optional series.
Please check the name for correct
spelling as well as proper use of quotes if using delmited identifiers."

E_VG00DF_BAD_NAME
"Invalid column name specified.\n
'%0c' is not a valid column name.
Please check the name for correct
spelling as well as proper use of quotes if using delmited identifiers."

E_VG00E0_BAD_TABLE_NAME
"Invalid table name specified.\n
'%0c' is not a valid table name.  Please check the name for correct
spelling as well as proper use of quotes if using delmited identifiers."
