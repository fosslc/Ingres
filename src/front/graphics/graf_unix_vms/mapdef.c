/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<st.h>
# include	<graf.h>
# include	<grmap.h>
# include	<dvect.h>

/**
** Name:    mapdef.c	-	Do 'default' mappings for graphs w/o data
**
** Description:
**	<comments>.  This file defines:
**
**	<proc1>	       <description>
**	<proc2>	       <description>
**	<...>	     <description>
**
** History:
**	9/19/98 (bab)	changed 15 to 14.8 in bary2, 10 to 9.8 in bary1
**		so that autoscale function will always set range to
**		0-15.  (mainly for testing purposes)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

VOID	map_def();
VOID	pie_def();
VOID	bar_def();
VOID	line_def();
VOID	scat_def();

FUNC_EXTERN	GR_DVEC *GTdvcreate();
FUNC_EXTERN	char	*GRtxtstore();

static	i4	deftag = 0;
static	i4	npoints = 6;

static	char	xname[FE_MAXNAME+1];
static	char	yname[FE_MAXNAME+1];
static	char	zname[FE_MAXNAME+1];

static	struct	txtmap {
	char	*str;
	i4	val;
} labels[] =
{
	{ERx("Athens"),1},
	{ERx("London"),2},
	{ERx("New York"),3},
	{ERx("Paris"),4},
	{ERx("Rome"),5},
	{ERx("Tokyo"),6}
};

static	float	pieval[]	= { 40, 25, 10, 5, 15, 5 };
static	float	bary1[]		= { 5, 8, 9.8, 3, 3, 2 };
static	float	bary2[]		= { 14.8, 10, 7, 2, 10, 12 };
static	float	*liney1		= bary1;
static	float	*liney2		= bary2;

/*{
** Name:	map_def		-	get 'default' data for a chart
**
** Description:
**	Loop through a graph frame structure, finding charts and labels.
**	Create data vectors for all of them, using 'default' data derived
**	from local arrays.
**
**	This routine is used when editing/running/displaying any graph that does
**	not have a table/view associated with it - there is no way to 'plot'
**	the graph without giving it some data.
**
**
** Inputs:
**	frame		Graph frame to be mapped to default data.
**	map_ptr		Structure describing mappings
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		none
**
** Side Effects:
**
**	Destroys all existing data vectors (GTdvrestart), and creates new
**	ones for default data.	Associates data vector names with the
**	data-dependent elements of the charts and labels of the graph frame.
**
** History:
**	10-Dec-85	(drh)	First written.
*/
VOID
map_def( frame, map_ptr )
GR_FRAME	*frame;
register GR_MAP *map_ptr;
{
	register GR_OBJ		*comp_ptr;
	register GR_TEXT	*lab_ptr;
	char			*qry_funmap();
	VOID			GTdvrestart();

	/*  Destroy all existing data vectors */

	GTdvrestart();

	if ( deftag != 0 )
		FEfree( deftag );

	deftag = FEgettag();

	/*
	**	Do default 'name' assignments for all unnamed graphical
	**	objects
	*/

		if ( map_ptr->indep == NULL  || *map_ptr->indep == '\0')
			STcopy( ERx("x"), xname );
		else
		{
			STcopy( map_ptr->indep, xname );
			STtrmwhite( xname );
			if ( xname[0] == '\0' )
				STcopy( ERx("x"), xname );
		}

		if ( map_ptr->dep == NULL || *map_ptr->dep == '\0')
			STcopy( ERx("y"), yname );
		else
		{
			STcopy( map_ptr->dep, yname );
			STtrmwhite( yname );
			if ( yname[0] == '\0' )
				STcopy( ERx("y"), yname );
		}


		if ( map_ptr->series == NULL || *map_ptr->series == '\0')
			STcopy( ERx("z"), zname );
		else
		{
			STcopy( map_ptr->series, zname );
			STtrmwhite( zname );
			if ( zname[0] == '\0' )
				STcopy( ERx("z"), zname );
		}

	for ( comp_ptr = frame->head; comp_ptr != NULL;
		comp_ptr = comp_ptr->next )
	{
		switch ( comp_ptr->type )
		{

		   case GROB_TRIM:
			lab_ptr = ( GR_TEXT * ) comp_ptr->attr;
			if ( lab_ptr->name != NULL &&
				*(lab_ptr->name) != '\0' )
			{
				lab_ptr->text = qry_funmap(lab_ptr->name);
			}
			break;

		   case GROB_PIE:
			pie_def( ( PIECHART * ) comp_ptr->attr );
			break;

		   case GROB_BAR:
			bar_def( ( BARCHART * ) comp_ptr->attr );
			break;

		   case GROB_LINE:
			line_def( ( LINECHART * ) comp_ptr->attr );
			break;

		   case GROB_SCAT:
			scat_def( ( SCATCHART * ) comp_ptr->attr );
			break;

		   default:
			break;
		}

	}

	return;
}

/*{
** Name:	pie_def		-	Get default pie chart data
**
** Description:
**	Build data vectors for a pie chart (labels and y values).
**
** Inputs:
**	pie_ptr		Pointer to pie attribute structure
**
** Outputs:
**	Returns:
**		VOID
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**
** History:
**	10-dec-85	(drh)	First written.
*/

VOID
pie_def( pie_ptr )
PIECHART	*pie_ptr;
{
	register GR_DVEC	*dptr;
	register i4		i;

	/* Allocate data vector for labels */

	pie_ptr->lab.l_field = FEtsalloc( deftag, xname );

	dptr = GTdvcreate( pie_ptr->lab.l_field, (i4) GRDV_STRING, npoints );

	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
		dptr->ar.sdat.val[i] = labels[i].val;
	}

	/* Allocate data vector for y-values */

	pie_ptr->ds.xname = pie_ptr->lab.l_field;
	pie_ptr->ds.field =  FEtsalloc( deftag, yname );

	dptr = GTdvcreate( pie_ptr->ds.field, (i4) GRDV_NUMB, npoints );

	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.fdat[i] = pieval[i];
	}

	return;
}

/*{
** Name:	bar_def		-	Create default bar chart data
**
** Description:
**
**
** Inputs:
**	bar_ptr		Pointer to attribute structure for bar chart
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**
** History:
**	12-dec-85 (drh) First written.
*/

VOID
bar_def( bar_ptr)
BARCHART	*bar_ptr;
{
	DEPDAT	*dep_ptr;
	GR_DVEC *dptr;
	LABEL	*lab_ptr;
	char	*namptr;
	char	nambuf[FE_MAXNAME+2];
	i4	i;
	i4	nseries;

	/*  Allocate data vectors for x- and y-series */

	nseries = 2;
	bar_ptr->ds_num = nseries;

	dep_ptr = &bar_ptr->ds[0];
	namptr = STpolycat( (i4) 2, zname, ERx("1"), nambuf );
	dep_ptr->desc = FEtsalloc( deftag, namptr );
	dep_ptr->field = FEtsalloc( deftag, ERx("bary1") );
	dptr = GTdvcreate( dep_ptr->field, (i4) GRDV_NUMB, npoints );
	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.fdat[i] = bary1[i];
	}

	dep_ptr->xname = FEtsalloc( deftag, ERx("barx1"));
	dptr = GTdvcreate( dep_ptr->xname, (i4) GRDV_STRING, npoints );

	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
		dptr->ar.sdat.val[i] = labels[i].val;
	}

	if ( nseries > 1 )
	{
		dep_ptr = &bar_ptr->ds[1];
		namptr = STpolycat( (i4) 2, zname, ERx("2"), nambuf );
		dep_ptr->desc = FEtsalloc( deftag, namptr );
		dep_ptr->field = FEtsalloc( deftag, ERx("bary2") );
		dptr = GTdvcreate( dep_ptr->field, (i4) GRDV_NUMB, npoints );
		for ( i = 0; i < npoints; i++ )
		{
			dptr->ar.fdat[i] = bary2[i];
		}
		dep_ptr->xname = FEtsalloc( deftag, ERx("barx2"));
		dptr = GTdvcreate( dep_ptr->xname, (i4) GRDV_STRING, npoints );

		for ( i = 0; i < npoints; i++ )
		{
			dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
			dptr->ar.sdat.val[i] = labels[i].val;
		}

	}

	/* There will not be a label series in default bar-chart data */

	for (i = 0; i < nseries; i++ )
	{
		bar_ptr->lab[i].l_field = NULL;
	}

	return;

}

/*{
** Name:	line_def	-	Create default line chart data
**
** Description:
**
**
** Inputs:
**	line_ptr	Pointer to attribute structure for line chart
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**
** History:
**	12-dec-85 (drh) First written.
*/

VOID
line_def( line_ptr)
LINECHART	*line_ptr;
{
	DEPDAT	*dep_ptr;
	GR_DVEC *dptr;
	char	*namptr;
	char	nambuf[FE_MAXNAME+2];
	i4	i;
	i4	nseries;

	/*  Allocate data vectors for  y-series */

	nseries = 2;
	line_ptr->ds_num = nseries;

	dep_ptr = &line_ptr->ds[0];
	namptr = STpolycat( (i4) 2, zname, ERx("1"), nambuf );
	dep_ptr->desc = FEtsalloc( deftag, namptr );

	dep_ptr->field = FEtsalloc( deftag, ERx("liney1") );
	dptr = GTdvcreate( dep_ptr->field, (i4) GRDV_NUMB, npoints );
	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.fdat[i] = liney1[i];
	}

	dep_ptr->xname = FEtsalloc( deftag, ERx("linex1"));
	dptr = GTdvcreate( dep_ptr->xname, (i4) GRDV_STRING, npoints );

	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
		dptr->ar.sdat.val[i] = labels[i].val;
	}


	if ( nseries > 1 )
	{
		dep_ptr = &line_ptr->ds[1];
		dep_ptr->field = FEtsalloc( deftag, ERx("liney2") );
		namptr = STpolycat( (i4) 2, zname, ERx("2"), nambuf );
		dep_ptr->desc = FEtsalloc( deftag, namptr );
		dptr = GTdvcreate( dep_ptr->field, (i4) GRDV_NUMB, npoints );
		for ( i = 0; i < npoints; i++ )
		{
			dptr->ar.fdat[i] = liney2[i];
		}

		dep_ptr->xname = FEtsalloc( deftag, ERx("linex2"));
		dptr = GTdvcreate( dep_ptr->xname, (i4) GRDV_STRING, npoints );

		for ( i = 0; i < npoints; i++ )
		{
			dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
			dptr->ar.sdat.val[i] = labels[i].val;
		}

	}

	return;
}

/*{
** Name:	scat_def	-	Create default line chart data
**
** Description:
**
**
** Inputs:
**	scat_ptr	Pointer to attribute structure for line chart
**
** Outputs:
**
**	Returns:
**		VOID
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**
** History:
**	12-dec-85 (drh) First written.
*/

VOID
scat_def( scat_ptr)
SCATCHART	*scat_ptr;
{
	DEPDAT	*dep_ptr;
	GR_DVEC *dptr;
	char	*namptr;
	char	nambuf[FE_MAXNAME+2];
	i4	i;
	i4	nseries;

	/*  Allocate data vectors for y-series */


	nseries = 2;
	scat_ptr->ds_num = nseries;

	dep_ptr = &scat_ptr->ds[0];
	namptr = STpolycat( (i4) 2, zname, ERx("1"), nambuf );
	dep_ptr->desc = FEtsalloc( deftag, namptr );

	dep_ptr->field = FEtsalloc( deftag, ERx("scaty1") );
	dptr = GTdvcreate( dep_ptr->field, (i4) GRDV_NUMB, npoints );
	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.fdat[i] = liney1[i];
	}

	dep_ptr->xname = FEtsalloc( deftag, ERx("scatx1"));
	dptr = GTdvcreate( dep_ptr->xname, (i4) GRDV_STRING, npoints );

	for ( i = 0; i < npoints; i++ )
	{
		dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
		dptr->ar.sdat.val[i] = labels[i].val;
	}


	if ( nseries > 1 )
	{
		dep_ptr = &scat_ptr->ds[1];
		dep_ptr->field = FEtsalloc( deftag, ERx("scaty2") );
		namptr = STpolycat( (i4) 2, zname, ERx("2"), nambuf );
		dep_ptr->desc = FEtsalloc( deftag, namptr );
		dptr = GTdvcreate( dep_ptr->field, (i4) GRDV_NUMB, npoints );
		for ( i = 0; i < npoints; i++ )
		{
			dptr->ar.fdat[i] = liney2[i];
		}

		dep_ptr->xname = FEtsalloc( deftag, ERx("scatx1"));
		dptr = GTdvcreate( dep_ptr->xname, (i4) GRDV_STRING, npoints );

		for ( i = 0; i < npoints; i++ )
		{
			dptr->ar.sdat.str[i] = FEtsalloc( dptr->tag, labels[i].str );
			dptr->ar.sdat.val[i] = labels[i].val;
		}

	}

	return;
}
