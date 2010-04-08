/*
** Copyright (c) 2004 Ingres Corporation 
*/
/*
** Name: fortunoff.sc
**
PROGRAM = fortunoff

NEEDLIBS = LIBINGRES
**
** Description:
**      Read configuration files to load fortunoff demo tables.
**
** History:
**      07-Apr-1999 (fanra01)
**          Created. CL'ized for marol01.
*/
# include <compat.h>
# include <cv.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <pc.h>
# include <si.h>
# include <st.h>

/*
** Declare the SQLCA structure and the SQLDA typedef
*/
EXEC SQL INCLUDE SQLCA;
EXEC SQL INCLUDE SQLDA;

#define BLOCK_SIZE 1000
#define LINE_SIZE 4096
#define COMMENT_SIZE 2048
#define NAME_SIZE 33
#define PATH_SIZE 257
#define URL_SIZE 257

# ifdef NT_GENERIC
# define CHR_PATH_DELIM '\\'
# define STR_PATH_DELIM "\\"
# else
# ifdef UNIX
# define CHR_PATH_DELIM '/'
# define STR_PATH_DELIM "/"
# endif
# endif

void    II_Err ();
void    putHandler( char *filename );
char*   GetWord( char **line );
STATUS  loadArticle ( char *conf_file );
STATUS  loadArticleComment ( char *conf_file );
STATUS  loadMenu ( char *conf_file );
STATUS  loadImage ( char *conf_file );
STATUS  loadLocation( char *conf_file );
STATUS  CreateCommand();
STATUS  Guestbook();
STATUS  loadArticleMenu ( char *conf_file );

char *ImagePath;

/*
** Name: main
**
** Description:
**      Connect to specified database and create and load tables as specified
**      in the cfg files.
**
** Input:
**      argv[1]     dbname      Database to load
**      argv[2]     CFGpath     Path to configuration files
**      argv[3]     ImagePath   Path to blobs to store in database
**
** Output:
**      None.
**
** Returns:
**      OK      completed successfully
**      FAIL    on error
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
main( i4 argc, char **argv )
{
    STATUS  status = OK;
    exec sql begin declare section;
        char *dbname;
    exec sql end declare section;
    char *CfgPath;
    char *name;

    if (argc != 4)
    {
        SIfprintf( stderr,
            ERx("usage : %s <database name> <cfg directory> <image directory>\n"),
            argv[0] );
        PCexit( FAIL );
    }

    if ((dbname = STalloc( argv[1] )) == NULL)
    {
        SIfprintf( stderr,
            ERx("Not enought memory to load the fortunoff application.\n") );
        PCexit( FAIL );
    }

    if ((CfgPath = STalloc( argv[2] )) == NULL)
    {
        SIfprintf( stderr,
            ERx("Not enought memory to load the fortunoff application.\n") );
        PCexit( FAIL );
    }

    if ((ImagePath = STalloc( argv[3] )) == NULL)
    {
        SIfprintf( stderr,
            ERx("Not enought memory to load the fortunoff application.\n") );
        PCexit( FAIL );
    }

    SIfprintf( stderr, "connection to %s\n", dbname );
    EXEC SQL CONNECT :dbname identified by icedbuser;

    if (sqlca.sqlcode != 0)
    {
        SIfprintf( stderr, ERx("Unable to open the database\n") );
        PCexit( FAIL );
    }

    EXEC SQL WHENEVER sqlerror call II_Err;

    if ((name = MEreqmem( 0, strlen(CfgPath) + 20, TRUE, NULL )) == NULL)
    {
        SIfprintf( stderr,
            ERx("Not enought memory to load the fortunoff application.\n") );
        PCexit( FAIL );
    }

    STprintf( name, "%s%cmenu.cfg", CfgPath, CHR_PATH_DELIM );
    loadMenu( name );
    STprintf( name, "%s%cimages.cfg", CfgPath, CHR_PATH_DELIM );
    loadImage( name );
    STprintf( name, "%s%carticles.cfg", CfgPath, CHR_PATH_DELIM );
    loadArticle( name );
    STprintf( name, "%s%cartcomments.cfg", CfgPath, CHR_PATH_DELIM );
    loadArticleComment( name );
    STprintf( name, "%s%cartmenu.cfg", CfgPath, CHR_PATH_DELIM );
    loadArticleMenu( name );
    STprintf( name, "%s%clocation.cfg", CfgPath, CHR_PATH_DELIM );
    loadLocation( name );
    CreateCommand();
    Guestbook();

    EXEC SQL ROLLBACK;
    EXEC SQL DISCONNECT;

    MEfree( name );
    MEfree( dbname );
    MEfree( CfgPath );
    MEfree( ImagePath );
    return(status);
}

/*
** Name: II_Err
**
** Description:
**      SQL Error handler.  Displays error message and rolls back transaction.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
void
II_Err()
{
    exec sql begin declare section;
        char text[2000];
        i4     error;
    exec sql end declare section;

    exec sql inquire_sql( :error = errorno, :text = errortext );

    if (error != 0 && error != 30100)
    {
        SIfprintf( stderr, "error : %d '%s'\n", error, text );
        exec sql rollback;
        PCexit( FAIL );
    }
}

/*
** Name: File_Err
**
** Description:
**      File open error handler.  Rolls back transaction.
**
** Input:
**      filename    Path and filename being opened.
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
void
File_Err( char *name )
{
    SIfprintf( stderr, "error opening file : %s\n", name );
    exec sql rollback;
    PCexit( FAIL );
}

/*
** Name: putHandler
**
** Description:
**      blob put handler.
**
** Input:
**      filename    name of blob file
**
** Output:
**      None.
**
** Returns:
**      None.
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
void
putHandler( char *filename )
{
    exec sql begin declare section;
        char    segment[BLOCK_SIZE];
        i4      length = 0;
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;

    if (*filename != '\0')
    {
        STcopy( filename, locpath);
        LOfroms (PATH & FILENAME, locpath, &loc);
        if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
        {
            SIfprintf( stdout, "opened the %s file \n", filename );
            while (status == OK)
            {
                status = SIread( fd, BLOCK_SIZE, &length, segment );
                if (((status == OK) ||(status == ENDFILE)) && length != 0)
                {
                    exec sql put data (segment = :segment,
                        segmentlength = :length);
                }
            }
            SIclose( fd );
        }
        else
        {
            File_Err(filename);
        }
    }
    exec sql put data (dataend = 1);  /* required by esql */
    return;
}

/*
** Name: GetWord
**
** Description:
**      Return the next word from the line.
**
** Input:
**      line    pointer to input line.
**
** Output:
**      line    updated line pointer.
**
** Returns:
**      pointer to word.
**      no error return.
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
char *
GetWord( char **line )
{
    char *tmp;
    char *word;

    tmp = *line;
    while (*tmp == ' ' && *tmp == '\t') tmp++;
    word = tmp;

    while (*tmp != ';' && *tmp != '\r' && *tmp != '\n' && *tmp != '\0') tmp++;
    *line = tmp+1;

    while (*tmp == ' ' || *tmp == '\t') tmp--;
    tmp[0] = '\0';

    return(word);
}

/*
** Name: loadMenu
**
** Description:
**      Create menu table and load the menu images into it.
**
** Input:
**      conf_file   path and filename to menu config file.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
loadMenu (char *conf_file)
{
    exec sql begin declare section;
        char name[NAME_SIZE];
        char ref[URL_SIZE];
        char alt[COMMENT_SIZE];
        char image[PATH_SIZE];
        char line[LINE_SIZE];
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;
    char *tmp;

    SIfprintf( stdout, "deleting existing menu table\n" );
    exec sql drop table menu;

    SIfprintf( stdout, "creating menu table\n" );
    exec sql create table menu (name varchar(32), ref varchar(256), alt varchar(256), image long byte);

    SIfprintf(stdout, "loading data from the %s file \n", conf_file);
    STcopy( conf_file, locpath);
    LOfroms (PATH & FILENAME, locpath, &loc);
    if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
    {
        MEfill(LINE_SIZE, 0, line);
        while ((status = SIgetrec( line, LINE_SIZE, fd)) == OK )
        {
            tmp = line;
            STcopy( GetWord( &tmp ), name);
            STcopy( GetWord( &tmp ), ref);
            STcopy( GetWord( &tmp ), alt);
            STprintf( image, "%s%c%s", ImagePath, CHR_PATH_DELIM,
                GetWord(&tmp) );
            SIfprintf( stdout,
                "loading : (name : %s, ref : %s, alt : %s, image : %s)\n",
                name, ref, alt, image);

            exec sql insert into menu (name, ref, alt, image)
                values (:name, :ref, :alt, datahandler(putHandler(image)));
            MEfill(LINE_SIZE, 0, line);
        }
        SIclose(fd);
        if (status == ENDFILE)
            status = OK;
    }
    else
    {
        File_Err(conf_file);
    }
    EXEC SQL COMMIT;
    return (status);
}

/*
** Name: loadImage
**
** Description:
**      Create image table and load the images into it.
**
** Input:
**      conf_file   path and filename to menu config file.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
**      02-Oct-2000 (fanra01)
**          Add a commit in between insert and modify.
*/
STATUS
loadImage( char *conf_file )
{
    exec sql begin declare section;
        char name[NAME_SIZE];
        char image[PATH_SIZE];
        char line[LINE_SIZE];
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;
    char *tmp;

    SIfprintf(stdout, "deleting existing images table\n");
    exec sql drop table images;

    SIfprintf(stdout, "creating image table\n");
    exec sql create table images (name varchar(32), image long byte);
    EXEC SQL COMMIT;
    exec sql modify images to btree unique on name;

    SIfprintf(stdout, "loading data from the %s file \n", conf_file);
    STcopy( conf_file, locpath);
    LOfroms (PATH & FILENAME, locpath, &loc);
    if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
    {
        MEfill(LINE_SIZE, 0, line);
        while ((status = SIgetrec( line, LINE_SIZE, fd)) == OK )
        {
            tmp = line;
            STcopy( GetWord( &tmp ), name );
            STprintf(image, "%s%c%s", ImagePath, CHR_PATH_DELIM,
                GetWord(&tmp));
            SIfprintf(stdout,
                "loading : (name : %s, image : %s)\n", name, image);

            exec sql insert into images (name, image)
                values (:name, datahandler(putHandler(image)));
            MEfill(LINE_SIZE, 0, line);
        }
        SIclose(fd);
        if (status == ENDFILE)
            status = OK;
    }
    else
    {
        File_Err(conf_file);
    }
    EXEC SQL COMMIT;
    return (status);
}

/*
** Name: loadArticle
**
** Description:
**      Create the articles table and populate it.
**
** Input:
**      conf_file   path and filename to menu config file.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
**      02-Oct-2000 (fanra01)
**          Add a commit in between insert and modify.
*/
STATUS
loadArticle (char *conf_file)
{
    exec sql begin declare section;
        i4 siteld;
        i4 category;
        char cdProd[NAME_SIZE];
        char item[COMMENT_SIZE];
        char image[PATH_SIZE];
        char line[LINE_SIZE];
        char description[COMMENT_SIZE];
        i4 price;
        i4 special;
        i4 newitem;
        char AA[NAME_SIZE];
        char A0[NAME_SIZE];
        char A1[NAME_SIZE];
        char A2[NAME_SIZE];
        char A3[NAME_SIZE];
        char A4[NAME_SIZE];
        char A5[NAME_SIZE];
        char A6[NAME_SIZE];
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;
    char *tmp;

    SIfprintf(stdout, "deleting existing articles table\n");
    exec sql drop table articles;

    SIfprintf(stdout, "creating articles table\n");
    exec sql create table articles (
            siteld integer,
            productCode varchar(32),
            item varchar(256),
            category integer,
            description varchar(1000),
            price integer,
            special integer,
            newitem integer,
            AA varchar(32),
            A0 varchar(32),
            A1 varchar(32),
            A2 varchar(32),
            A3 varchar(32),
            A4 varchar(32),
            A5 varchar(32),
            A6 varchar(32),
            image long byte);

    SIfprintf(stdout, "loading data from the %s file \n", conf_file);
    STcopy( conf_file, locpath);
    LOfroms (PATH & FILENAME, locpath, &loc);
    if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
    {
        MEfill(LINE_SIZE, 0, line);
        while ((status = SIgetrec( line, LINE_SIZE, fd)) == OK )
        {
            tmp = line;
            status = CVan( GetWord(&tmp), &siteld );
            STcopy( GetWord( &tmp ), cdProd);
            STcopy( GetWord( &tmp ), item);
            STprintf(image, "%s%c%s", ImagePath, CHR_PATH_DELIM, GetWord(&tmp));
            status = CVan( GetWord(&tmp), &category );
            STcopy( GetWord( &tmp ), description);
            status = CVan( GetWord(&tmp), &price );
            status = CVan( GetWord(&tmp), &special );
            status = CVan( GetWord(&tmp), &newitem );
            STcopy( GetWord( &tmp ), AA);
            STcopy( GetWord( &tmp ), A0);
            STcopy( GetWord( &tmp ), A1);
            STcopy( GetWord( &tmp ), A2);
            STcopy( GetWord( &tmp ), A3);
            STcopy( GetWord( &tmp ), A4);
            STcopy( GetWord( &tmp ), A5);
            STcopy( GetWord( &tmp ), A6);

            exec sql insert into articles (
                    siteld,
                    productCode,
                    item,
                    category,
                    description,
                    price,
                    special,
                    newitem,
                    AA,
                    A0,
                    A1,
                    A2,
                    A3,
                    A4,
                    A5,
                    A6,
                    image)
                values (
                    :siteld,
                    :cdProd,
                    :item,
                    :category,
                    :description,
                    :price,
                    :special,
                    :newitem,
                    :AA,
                    :A0,
                    :A1,
                    :A2,
                    :A3,
                    :A4,
                    :A5,
                    :A6,
                    datahandler(putHandler(image)));
            MEfill(LINE_SIZE, 0, line);
        }
        SIclose(fd);
        if (status == ENDFILE)
            status = OK;
    }
    else
    {
        File_Err(conf_file);
    }
    EXEC SQL COMMIT;
    exec sql modify articles to btree on productCode;
    exec sql create index ArticlesCategory on articles(category)
        with structure=btree;

    EXEC SQL COMMIT;
    return (status);
}

/*
** Name: loadArticleComment
**
** Description:
**      Create and populate the article comment table.
**
** Input:
**      conf_file   path and filename to menu config file.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
loadArticleComment (char *conf_file)
{
    exec sql begin declare section;
        int category;
        char name[NAME_SIZE];
        char comment[COMMENT_SIZE];
        char image[PATH_SIZE];
        char line[LINE_SIZE];
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;
    char *tmp;

    SIfprintf(stdout, "deleting existing article comments table\n");
    exec sql drop table articleComment;

    SIfprintf(stdout, "creating article comments table\n");
    exec sql create table articleComment (category integer, name varchar(32),
        comment varchar(1000), image long byte);

    exec sql modify articleComment to btree unique on category;

    SIfprintf(stdout, "loading data from the %s file \n", conf_file);
    STcopy( conf_file, locpath);
    LOfroms (PATH & FILENAME, locpath, &loc);
    if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
    {
        MEfill(LINE_SIZE, 0, line);
        while ((status = SIgetrec( line, LINE_SIZE, fd)) == OK )
        {
            tmp = line;
            status = CVan( GetWord(&tmp), &category );
            STcopy( GetWord( &tmp ), name );
            STcopy( GetWord( &tmp ), comment );
            STprintf( image, "%s%c%s", ImagePath, CHR_PATH_DELIM, GetWord(&tmp) );
            SIfprintf( stdout,
                "articleComment : (caterogy : %d, name : %s, comment : %s, image : %s)\n",
                category, name, comment, image);

            exec sql insert into articleComment (category, name, comment, image)
                values (:category, :name, :comment, datahandler(putHandler(image)));
            MEfill(LINE_SIZE, 0, line);
        }
        SIclose(fd);
        if (status == ENDFILE)
            status = OK;
    }
    else
    {
        File_Err(conf_file);
    }
    EXEC SQL COMMIT;
    return (status);
}

/*
** Name: loadLocation
**
** Description:
**      Create and populate the locations table.
**
** Input:
**      conf_file   path and filename to menu config file.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
loadLocation( char *conf_file )
{
    exec sql begin declare section;
        char name[NAME_SIZE];
        char comment[COMMENT_SIZE];
        char image1[PATH_SIZE];
        char image2[PATH_SIZE];
        char line[LINE_SIZE];
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;
    char *tmp;

    SIfprintf(stdout, "deleting existing locations table\n");
    exec sql drop table locations;

    SIfprintf(stdout, "creating locations table\n");
    exec sql create table locations (name varchar(32), comment varchar(1000),
        image1 long byte, image2 long byte);

    SIfprintf(stdout, "loading data from the %s file \n", conf_file);
    STcopy( conf_file, locpath);
    LOfroms (PATH & FILENAME, locpath, &loc);
    if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
    {
        MEfill(LINE_SIZE, 0, line);
        while ((status = SIgetrec( line, LINE_SIZE, fd)) == OK )
        {
            tmp = line;
            STcopy( GetWord( &tmp ), name );
            STcopy( GetWord( &tmp ), comment );
            STprintf( image1, "%s%c%s", ImagePath, CHR_PATH_DELIM, GetWord(&tmp) );
            STprintf( image2, "%s%c%s", ImagePath, CHR_PATH_DELIM, GetWord(&tmp) );
            SIfprintf(stdout, "locations : (name : %s, image : %s)\n",
                name, image1);

            exec sql insert into locations (name, comment, image1, image2)
                values (:name, :comment, datahandler(putHandler(image1)), datahandler(putHandler(image2)));
            MEfill(LINE_SIZE, 0, line);
        }
        SIclose(fd);
        if (status == ENDFILE)
            status = OK;
    }
    else
    {
        File_Err(conf_file);
    }
    EXEC SQL COMMIT;
    return (status);
}

/*
** Name: Guestbook
**
** Description:
**      Create and populate the guest book table.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
Guestbook()
{
    STATUS status = OK;

    SIfprintf(stdout, "deleting existing GuestBook tables\n");
    exec sql drop table GuestBook;

    SIfprintf(stdout, "creating GuestBook tables\n");
    exec sql create table GuestBook(
            sess varchar(32) not default not null,
            sess_day date default 'now',
            fname varchar(32) default '',
            lname varchar(32) default '',
            apt varchar(5) default '',
            street varchar(150) default '',
            city varchar(50) default '',
            state varchar(50) default '',
            zip varchar(50) default '',
            Email varchar(100) default '',
            upd varchar(10) default 'FALSE',
            catalog varchar(10) default 'FALSE',
            credit varchar(10) default 'FALSE',
            purchase varchar(10) default 'YES',
            find varchar(250) default '',
            real varchar(250) default '',
            look varchar(250) default '',
            suggest varchar(250) default ''
        );

    EXEC SQL COMMIT;
    return (status);
}

/*
** Name: CreateCommand
**
** Description:
**      Create the commands table for order handling.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
CreateCommand()
{
    STATUS status = OK;

    SIfprintf(stdout, "deleting existing commands tables\n");
    exec sql drop table commands;
    exec sql drop table orders;
    exec sql drop table bill;

    SIfprintf(stdout, "creating commands tables\n");
    exec sql create table commands (sess varchar(32), amount integer, code varchar(32));
    exec sql create table orders (sess varchar(32), amount integer, code varchar(32));
    exec sql create table bill (
            sess varchar(32) not default not null,
            sess_day date default 'now',
            name varchar(32) default '',
            company varchar(32) default '',
            address varchar(150) default '',
            Email varchar(100) default '',
            city varchar(50) default '',
            state varchar(50) default '',
            zip varchar(50) default '',
            country varchar(50) default '',
            dayPhone varchar(12) default '',
            nightPhone varchar(12) default '',
            cardType varchar(32) default 'Visa',
            cardNumber varchar(32) default '',
            cardexp varchar(32) default '',
            shname varchar(32) default '',
            shaddress varchar(150) default '',
            shcity varchar(50) default '',
            shstate varchar(50) default '',
            shzip varchar(50) default '',
            shcountry varchar(50) default '',
            Inst varchar(500) default ''
        );

    exec sql modify commands to btree on sess, code;
    exec sql modify orders to btree on sess;
    exec sql modify bill to btree on sess;

    EXEC SQL COMMIT;
    return (status);
}


/*
** Name: loadArticleMenu
**
** Description:
**      Create and load the artmenu table.
**
** Input:
**      conf_file   path and filename to menu config file.
**
** Output:
**      None.
**
** Returns:
**      OK  successful
**      !0  failure
**
** History:
**      07-Apr-1999 (fanra01)
**          CL'ized for marol01 and add comments.
*/
STATUS
loadArticleMenu (char *conf_file)
{
    exec sql begin declare section;
        char name[NAME_SIZE];
        char ref[URL_SIZE];
        char alt[COMMENT_SIZE];
        char image[PATH_SIZE];
        char line[LINE_SIZE];
    exec sql end declare section;
    FILE *fd = NULL;
    LOCATION loc;
    char locpath[MAX_LOC + 1];
    STATUS status = OK;
    char *tmp;

    SIfprintf(stdout, "deleting existing article menu table\n" );
    exec sql drop table artmenu;

    SIfprintf(stdout, "creating menu article table\n" );
    exec sql create table artmenu (name varchar(32), ref varchar(256),
        alt varchar(256), image long byte);

    SIfprintf(stdout, "loading data from the %s file \n", conf_file);
    STcopy( conf_file, locpath);
    LOfroms (PATH & FILENAME, locpath, &loc);
    if ((status = SIfopen( &loc, "r", SI_BIN, 0, &fd )) == OK)
    {
        MEfill(LINE_SIZE, 0, line);
        while ((status = SIgetrec( line, LINE_SIZE, fd)) == OK )
        {
            tmp = line;
            STcopy( GetWord( &tmp ), name);
            STcopy( GetWord( &tmp ), ref);
            STcopy( GetWord( &tmp ), alt);
            STprintf( image, "%s%c%s", ImagePath, CHR_PATH_DELIM,
                GetWord(&tmp) );
            SIfprintf(stdout, "loading : (name : %s, ref : %s, alt : %s, image : %s)\n",
                name, ref, alt, image);

            exec sql insert into artmenu (name, ref, alt, image)
                values (:name, :ref, :alt, datahandler(putHandler(image)));
            MEfill(LINE_SIZE, 0, line);
        }
        SIclose(fd);
        if (status == ENDFILE)
            status = OK;
    }
    else
    {
        File_Err(conf_file);
    }
    EXEC SQL COMMIT;
    return (status);
}
