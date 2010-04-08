
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	vifrbft.h - common include for vifred and rbf.
**
** Description:
**	Types used in communication between RBF and VIFRED.
**
** History:
**	??? - written.
**	12-apr-87 (rdesmond) - deleted num_ags from AGG_INFO.
**	16-oct-89 (cmr) -	deleted copt_whenprint, copt_skip
**				from COPT.
**  11/27/89 (martym) - Added the field 'sec_y_count' to the 
**      Sec_node structure, to keep track of the Y positions 
**      within a section.
**	22-nov-89 (cmr)	Added agformat to AGG_INFO, deleted
**			apage, areport and abreak.
**	30-jan-90 (cmr)	Added new field to Sec_node to deal with
**			underlining.
**	19-apr-90 (cmr)	Added new field to COPT (agu_list) which
**			is a linked list of unique agg fields.
**	5/15/90 (martym) Added copt_whenprint, & copt_newpage to COPT.
**	9-mar-1993 (rdrane) Added copt_var_name to COPT.  This introduces a
**			    dependency on fe.h (FE_MAXNAME definition);
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Added function prototypes.
**	09-Apr-2005 (lakvi01)
**	    Moved the declaration of Reset_copt() into #ifdef NT_GENERIC since
**	    it was causing compilation problems on non-windows compilers.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/*
**	The SEC structure contains the Y coordinate of the 
**		starts of the sections of the RBF frame.
*/

typedef struct sec_node
{
	struct sec_node	*next;
	struct sec_node	*prev;
	char		*sec_name;
	int		sec_type;
	int		sec_y;
	int		sec_brkseq;
	i2		sec_brkord;
	int     	sec_y_count;
	char		sec_under;
} Sec_node;

typedef struct
{
	Sec_node	*head;
	Sec_node	*tail;
	int		count;
} Sec_List;


/*
**	The CS structure contains the link between a linked list of
**		field structures and
**		a linked list of trim structures.  There will
**		be an array En_n_attribs i4  of these structures.
*/

typedef struct
{
	LIST	*cs_flist;		/* start of linked list of FIELD
					** structures for this attribute */
	LIST	*cs_tlist;		/* start of linked list of TRIM
					** structures for this attribute */
	char	*cs_name;		/* Name of Attribute  */
}  CS;

/*
**	The AGG_INFO structure contains the information associated with
**		one aggregate function for one attribute of a relation.  
**		In addition there is a pointer to an ADF_AGNAMES structure, 
**		which contains the various names for the aggregate and
**		a prime flag.
*/

typedef struct
{
	ADF_AGNAMES	*agnames;	/* names for aggregate, uniqueness */
	FMT		*agformat;
} AGG_INFO;

/*
**	The COPT structure contains the storage of options associated
**		with an attribute.  These are entered by the user in
**		the column options form and the report structure form.
*/

typedef struct
{
	i2		copt_sequence;	/* Sort sequence for attribute */
	char		copt_order;	/* sort order (a/d) */
	char		copt_break;	/* y/n if attribute is break */
	char            copt_whenprint; /* when to print (b/t/p/a) */
	bool            copt_newpage;   /* do a new page after a break or not */
	char		copt_brkhdr;	/* y/n if break footer */
	char		copt_brkftr;	/* y/n if break footer */
	char		copt_select;	/* selection criteria (v/r/n) */
	AGG_INFO	*agginfo;	/* aggregate information */
	i4		num_ags;	/* number of aggs for attribute */
	LIST		*agu_list;	/* linked list of unique agg fields */
	char		copt_var_name[(FE_MAXNAME + 1)];
					/*
					** Unique name for generated RW
					** variables since the base attribute
					** name may "collapse" (delim id's).
					** Assumes value of an empty string						** unused or not yet set.
					*/
}   COPT;

/*
** Function declarations
*/
#ifdef NT_GENERIC
FUNC_EXTERN VOID	Reset_copt(PTR);
#else
FUNC_EXTERN VOID	Reset_copt();
#endif
FUNC_EXTERN VOID	IIVFgetmu();
FUNC_EXTERN VOID	IIVFrvxRebuildVxh();
FUNC_EXTERN STATUS	IIVFdrg_DumpReg();
FUNC_EXTERN STATUS	IIVFdtb_DumpTab();
FUNC_EXTERN STATUS	IIVFcfd_CopyFds();
FUNC_EXTERN VOID	unLinkPos();
FUNC_EXTERN VOID	lfree();
FUNC_EXTERN VOID	errOver();
FUNC_EXTERN VOID	Remove_agu();
FUNC_EXTERN VOID	editTitle();
FUNC_EXTERN VOID	editData();
FUNC_EXTERN VOID	RebuildLines();
FUNC_EXTERN VOID	prTrim();
FUNC_EXTERN VOID	prFld();
FUNC_EXTERN VOID	VifAgguHandler();
FUNC_EXTERN VOID	vfrbfinit();
FUNC_EXTERN VOID	vfgrf();
FUNC_EXTERN VOID	vfTestDump();
FUNC_EXTERN VOID	vf_newFrame();
FUNC_EXTERN VOID	vfhelp();
FUNC_EXTERN VOID	vfstdel();
FUNC_EXTERN VOID	vfdmpon();
FUNC_EXTERN VOID	vfdmpoff();
FUNC_EXTERN VOID	vfTrigUpd();
FUNC_EXTERN VOID	vfPutFrmAtr();
FUNC_EXTERN VOID	vfSetPopFlg();
FUNC_EXTERN VOID	vfSetBordFlg();
FUNC_EXTERN VOID	vfPutStartRowCol();
FUNC_EXTERN VOID	vfPutNumRowsCols();
FUNC_EXTERN VOID	vfVisualAdj();
FUNC_EXTERN VOID	vfhelpsub();
FUNC_EXTERN VOID	vfFormExtent();
FUNC_EXTERN VOID	vfChkPopup();
FUNC_EXTERN VOID	vfFormMove();
FUNC_EXTERN VOID	vfFrmBotResize();
FUNC_EXTERN VOID	vfFrmRightResize();
FUNC_EXTERN VOID	vfFormResize();
FUNC_EXTERN VOID	vfFrmLeftResize();
FUNC_EXTERN VOID	vfFrmTopResize();
FUNC_EXTERN VOID	vfFormExtent();
FUNC_EXTERN VOID	vfFormHorzAdjust();
FUNC_EXTERN VOID	vfMoveBottomDown();
FUNC_EXTERN VOID	vfboundary();
FUNC_EXTERN VOID	vfposarray();
FUNC_EXTERN VOID	vfdmpoff();
FUNC_EXTERN VOID	vfdelnm();
FUNC_EXTERN VOID	vflmove();
FUNC_EXTERN VOID	vfshrlen();
FUNC_EXTERN VOID	vfexplen();
FUNC_EXTERN VOID	vfwmove();
FUNC_EXTERN VOID	vfshrwid();
FUNC_EXTERN VOID	vfnewLine();
FUNC_EXTERN VOID	vfsvImage();
FUNC_EXTERN VOID	vfwchg();
FUNC_EXTERN VOID	vf_wchg();
FUNC_EXTERN VOID	vfexpwid();
FUNC_EXTERN VOID	vfwider();
FUNC_EXTERN VOID	vflchg();
FUNC_EXTERN VOID	vfTabdisp();
FUNC_EXTERN VOID	vfsetcolor();
FUNC_EXTERN i4		vfgetcolor();
FUNC_EXTERN i4		vffnmchk();
FUNC_EXTERN i4		vfnmunique();
FUNC_EXTERN i4		vfToFrm();
FUNC_EXTERN i4		vflfind();
FUNC_EXTERN i4		vfwfind();
FUNC_EXTERN i4		vfBoxCount();
FUNC_EXTERN i4		vfexit();
FUNC_EXTERN VOID	vftorbf();
FUNC_EXTERN VOID	vfunique();
FUNC_EXTERN VOID	vfFDFmtstr();
FUNC_EXTERN VOID	vfBoxDisp();
FUNC_EXTERN VOID	vfdispData();
FUNC_EXTERN VOID	vfgetDataWin();
FUNC_EXTERN VOID	vfFmtdisp();
FUNC_EXTERN VOID	vfseqnext();
FUNC_EXTERN VOID	vfTitledisp();
FUNC_EXTERN VOID	vfersPos();
FUNC_EXTERN VOID	vfresImage();
FUNC_EXTERN VOID	vfchkmsg();
FUNC_EXTERN VOID	vfcursor();
FUNC_EXTERN VOID	vfdisplay();
FUNC_EXTERN VOID	vfBoxEdit();
FUNC_EXTERN VOID	vfBoxAttr();
FUNC_EXTERN VOID	vffrmcount();
FUNC_EXTERN VOID	vfgetSize();
FUNC_EXTERN VOID	vffillType();
FUNC_EXTERN VOID	vfnewCs();
FUNC_EXTERN VOID	vfnewAssoc();
FUNC_EXTERN VOID	vfnextCs();
FUNC_EXTERN VOID	vfFlddisp();
FUNC_EXTERN VOID	vfmu_put();
FUNC_EXTERN VOID	vfseqbuild();
FUNC_EXTERN VOID	vfAscTrim();
FUNC_EXTERN VOID	vfdispReg();
FUNC_EXTERN VOID	vfcolcount();
FUNC_EXTERN VOID	vfshiftmode();
FUNC_EXTERN VOID	vfclosemode();
FUNC_EXTERN VOID	vfcolsize();
FUNC_EXTERN VOID	vfClearFld();
FUNC_EXTERN i4		onEdge();
FUNC_EXTERN VOID	ndfree();
FUNC_EXTERN VOID	spcVTrim();
FUNC_EXTERN VOID	spcTrrbld();
FUNC_EXTERN VOID	spcBldtr();
FUNC_EXTERN VOID	unVarInit();
FUNC_EXTERN VOID	delFeat();
FUNC_EXTERN VOID	delLine();
FUNC_EXTERN VOID	newFeature();
FUNC_EXTERN VOID	newFrame();
FUNC_EXTERN bool	inLimit();
FUNC_EXTERN bool	moveField();
FUNC_EXTERN VOID	moveSide();
FUNC_EXTERN VOID	moveFld();
FUNC_EXTERN VOID	moveBox();
FUNC_EXTERN VOID	moveSubComp();
FUNC_EXTERN VOID	moveComp();
FUNC_EXTERN VOID	moveTrim();
FUNC_EXTERN VOID	moveBack();
FUNC_EXTERN VOID	movePosBack();
FUNC_EXTERN VOID	moveDown();
FUNC_EXTERN VOID	mvSide();
FUNC_EXTERN VOID	mvCol();
FUNC_EXTERN VOID	insert();
FUNC_EXTERN VOID	prFeature();
FUNC_EXTERN VOID	editTrim();
FUNC_EXTERN VOID	putEdTrim();
FUNC_EXTERN VOID	putEnTrim();
FUNC_EXTERN VOID	copyBytes();
FUNC_EXTERN VOID	FDToSmlfd();
FUNC_EXTERN VOID	saveSection();
FUNC_EXTERN i4		changeSec();
FUNC_EXTERN i4		rFcoptions();
FUNC_EXTERN VOID	resetMoveLevel();
FUNC_EXTERN i4		getMoveLevel();
FUNC_EXTERN VOID	tfattr();
FUNC_EXTERN VOID	fitPos();
FUNC_EXTERN i4		fdqbfmap();
FUNC_EXTERN bool	testHorizontal();
FUNC_EXTERN bool	sideWays();
FUNC_EXTERN bool	testCol();
FUNC_EXTERN VOID	nextMoveLevel();
FUNC_EXTERN VOID	setGlMove();
FUNC_EXTERN VOID	doInsLine();
FUNC_EXTERN VOID	doDelLine();
FUNC_EXTERN VOID	delColumn();
FUNC_EXTERN VOID	moveColumn();
FUNC_EXTERN VOID	unLinkCol();
FUNC_EXTERN VOID	ersCol();
FUNC_EXTERN VOID	wrOver();
FUNC_EXTERN VOID	linit();
FUNC_EXTERN VOID	ndinit();
FUNC_EXTERN VOID	createTrim();
FUNC_EXTERN VOID	createLine();
FUNC_EXTERN VOID	setGlobUn();
FUNC_EXTERN VOID	addColumn();
FUNC_EXTERN VOID	spcTrim();
FUNC_EXTERN VOID	allocFrmAttr();
FUNC_EXTERN VOID	insSpecLine(i4 x, i4 y, char *str);
FUNC_EXTERN VOID	insLine();
FUNC_EXTERN VOID	insTrim();
FUNC_EXTERN VOID	insColumn();
FUNC_EXTERN VOID	insPos();
FUNC_EXTERN VOID	insFld();
FUNC_EXTERN VOID	insTab();
FUNC_EXTERN VOID	insReg();
FUNC_EXTERN VOID	insPos();
FUNC_EXTERN VOID	rFcols();
FUNC_EXTERN VOID	spcRbld();
FUNC_EXTERN VOID	trimLines();
FUNC_EXTERN VOID	sec_list_remove_all();
FUNC_EXTERN VOID	sec_list_append();
