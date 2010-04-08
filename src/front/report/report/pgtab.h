/*
** Copyright (c) 2004 Ingres Corporation
*/
#define	ABORT		257
#define	ALL		258
#define	AND		259
#define	ANY		260
#define	APPEND		261
#define	AS		262
#define	AT		263
#define	AVG		264
#define	BEGIN		265
#define	BETWEEN		266
#define	BY		267
#define	COPY		268
#define	COUNT		269
#define	CREATE		270
#define	DELETE		271
#define	DISTINCT		272
#define	DROP		273
#define	END		274
#define	EXISTS		275
#define	FROM		276
#define	GROUP		277
#define	HAVING		278
#define	HELP		279
#define	INDEX		280
#define	INTO		281
#define	INSERT		282
#define	IN		283
#define	INTEGRITY		284
#define	IS		285
#define	JOURNALING		286
#define	RMAX		287
#define	RMIN		288
#define	MODIFY		289
#define	NOJOURNALING		290
#define	NOT		291
#define	OF		292
#define	ON		293
#define	OR		294
#define	ORDER		295
#define	PERMIT		296
#define	RELOCATE		297
#define	REPLACE		298
#define	RETRIEVE		299
#define	SAVE		300
#define	SAVEPOINT		301
#define	SELECT		302
#define	SET		303
#define	SQL		304
#define	SORT		305
#define	SUM		306
#define	TABLE		307
#define	TO		308
#define	TRANSACTION		309
#define	UNION		310
#define	UNIQUE		311
#define	UNTIL		312
#define	UPDATE		313
#define	VALUES		314
#define	VIEW		315
#define	WHERE		316
#define	IDENT		317
#define	ICONST		318
#define	FCONST		319
#define	SCONST		320
#define	LE		321
#define	GE		322
#define	NE		323
#define	EXP		324
#define	PUNARY		325
#define	PRELOP		326

typedef union 
{
	char	*st_name;
	SQSYM	*st_sym;
	SQNODE	*st_node;
} YYSTYPE;

extern YYSTYPE		yylval;

