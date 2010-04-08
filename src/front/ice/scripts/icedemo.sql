/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icedemo.sql
**
** Description:
**      File initializes the tables for the plays demo.
## History:
##      30-Oct-98 (fanra01)
##          Created.
##      06-Nov-98 (fanra01)
##          Add play_counters, new_order procedure and play_item.
##      12-Apr-99 (peeje01)
##          Add procedure newProduct
##          Product name changes
##	30-nov-1999 (somsa01)
##	    Corrected syntax for insertion of data into play_counters.
##	22-may-2003 (somsa01)
##	    Changed references to "next" , as this is now a reserved word.
##      29-June-2005 (zhahu02)
##          Updated for select permit on table plays (INGICE113/b114750).
*/

/*
** Create the plays table
*/
drop table plays;\p\g
create table plays (
    comporder integer,
    title varchar(45),
    playwright varchar(20),
    performed integer ,
    acts integer,
    type char(10)
);\p\g

insert into plays values (1,  'The Two Gentlemen of Verona', 'Shakespeare', 1598, 5, 'comedy' );\p\g
insert into plays values (2,  'The Taming of The Shrew'    , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (3,  'Henry VI part 1'            , 'Shakespeare', 1591, 5, 'history');\p\g
insert into plays values (4,  'Henry VI part 3'            , 'Shakespeare', 1595, 5, 'history');\p\g
insert into plays values (5,  'Titus Andronicus'           , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (6,  'Henry VI part 2'            , 'Shakespeare', 1592, 5, 'history');\p\g
insert into plays values (7,  'Richard III'                , 'Shakespeare', 1593, 5, 'history');\p\g
insert into plays values (8,  'The Comedy of Errors'       , 'Shakespeare', 1594, 5, 'comedy' );\p\g
insert into plays values (9,  'Loves Labours Lost'         , 'Shakespeare', 1594, 5, 'comedy' );\p\g
insert into plays values (10, 'A Midsummer Night''s Dream' , 'Shakespeare', 1595, 5, 'comedy' );\p\g
insert into plays values (11, 'Romeo and Julliet'          , 'Shakespeare', 1595, 5, 'tragedy');\p\g
insert into plays values (12, 'Richard II'                 , 'Shakespeare', 1595, 5, 'history');\p\g
insert into plays values (13, 'King John'                  , 'Shakespeare', 1596, 5, 'history');\p\g
insert into plays values (14, 'The Merchant of Venice'     , 'Shakespeare', 1598, 5, 'comedy' );\p\g
insert into plays values (15, 'Henry IV part 1'            , 'Shakespeare', 1598, 5, 'history');\p\g
insert into plays values (16, 'The Merry Wives of Windsor' , 'Shakespeare', 1597, 5, 'comedy' );\p\g
insert into plays values (17, 'Henry IV part 2'            , 'Shakespeare', 1597, 5, 'history');\p\g
insert into plays values (18, 'Much Ado About Nothing'     , 'Shakespeare', 1599, 5, 'comedy' );\p\g
insert into plays values (19, 'Henry V'                    , 'Shakespeare', 1599, 5, 'history');\p\g
insert into plays values (20, 'Julius Ceasar'              , 'Shakespeare', 1599, 5, 'tragedy');\p\g
insert into plays values (21, 'As You Like It'             , 'Shakespeare', 1600, 5, 'comedy' );\p\g
insert into plays values (22, 'Hamlet'                     , 'Shakespeare', 1600, 5, 'tragedy');\p\g
insert into plays values (23, 'Twelfth Night'              , 'Shakespeare', 1601, 5, 'comedy' );\p\g
insert into plays values (24, 'Troiles & Cressida'         , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (25, 'Measure for Measure'        , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (26, 'Othello'                    , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (27, 'Alls Well That Ends Well'   , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (28, 'Timon and Athens'           , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (29, 'King Lear'                  , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (30, 'Macbeth'                    , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (31, 'Anthony and Cleopatra'      , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (32, 'Pericles, Prince of Tyre'   , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (33, 'Coriolanus'                 , 'Shakespeare', NULL, 5, 'tragedy');\p\g
insert into plays values (34, 'The Winter''s Tale'         , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (35, 'Cymbeline'                  , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (36, 'The Tempest'                , 'Shakespeare', NULL, 5, 'comedy' );\p\g
insert into plays values (37, 'Henry VIII'                 , 'Shakespeare', 1613, 5, 'history');\p\g

/*
** Create the play_acts table
*/
drop table play_acts;\p\g
create table play_acts (playid integer, act integer, scenes integer);
insert into play_acts values ( 1, 1, 3 );\p\g
insert into play_acts values ( 1, 2, 7 );\p\g
insert into play_acts values ( 1, 3, 2 );\p\g
insert into play_acts values ( 1, 4, 4 );\p\g
insert into play_acts values ( 1, 5, 4 );\p\g

insert into play_acts values ( 2, 1, 2 );\p\g
insert into play_acts values ( 2, 2, 1 );\p\g
insert into play_acts values ( 2, 3, 2 );\p\g
insert into play_acts values ( 2, 4, 5 );\p\g
insert into play_acts values ( 2, 5, 2 );\p\g

insert into play_acts values ( 3, 1, 6 );\p\g
insert into play_acts values ( 3, 2, 5 );\p\g
insert into play_acts values ( 3, 3, 4 );\p\g
insert into play_acts values ( 3, 4, 7 );\p\g
insert into play_acts values ( 3, 5, 5 );\p\g

insert into play_acts values ( 4, 1, 4 );\p\g
insert into play_acts values ( 4, 2, 6 );\p\g
insert into play_acts values ( 4, 3, 3 );\p\g
insert into play_acts values ( 4, 4, 8 );\p\g
insert into play_acts values ( 4, 5, 7 );\p\g

insert into play_acts values ( 5, 1, 1 );\p\g
insert into play_acts values ( 5, 2, 4 );\p\g
insert into play_acts values ( 5, 3, 2 );\p\g
insert into play_acts values ( 5, 4, 4 );\p\g
insert into play_acts values ( 5, 5, 3 );\p\g

insert into play_acts values ( 6, 1, 4 );\p\g
insert into play_acts values ( 6, 2, 4 );\p\g
insert into play_acts values ( 6, 3, 3 );\p\g
insert into play_acts values ( 6, 4, 10);\p\g
insert into play_acts values ( 6, 5, 3 );\p\g

insert into play_acts values ( 7, 1, 4 );\p\g
insert into play_acts values ( 7, 2, 4 );\p\g
insert into play_acts values ( 7, 3, 7 );\p\g
insert into play_acts values ( 7, 4, 5 );\p\g
insert into play_acts values ( 7, 5, 5 );\p\g

insert into play_acts values ( 8, 1, 2 );\p\g
insert into play_acts values ( 8, 2, 2 );\p\g
insert into play_acts values ( 8, 3, 2 );\p\g
insert into play_acts values ( 8, 4, 4 );\p\g
insert into play_acts values ( 8, 5, 1 );\p\g

insert into play_acts values ( 9, 1, 2 );\p\g
insert into play_acts values ( 9, 2, 1 );\p\g
insert into play_acts values ( 9, 3, 1 );\p\g
insert into play_acts values ( 9, 4, 3 );\p\g
insert into play_acts values ( 9, 5, 2 );\p\g

insert into play_acts values (10, 1, 2 );\p\g
insert into play_acts values (10, 2, 2 );\p\g
insert into play_acts values (10, 3, 2 );\p\g
insert into play_acts values (10, 4, 2 );\p\g
insert into play_acts values (10, 5, 1 );\p\g

insert into play_acts values (11, 1, 5 );\p\g
insert into play_acts values (11, 2, 6 );\p\g
insert into play_acts values (11, 3, 5 );\p\g
insert into play_acts values (11, 4, 5 );\p\g
insert into play_acts values (11, 5, 3 );\p\g

insert into play_acts values (12, 1, 4 );\p\g
insert into play_acts values (12, 2, 4 );\p\g
insert into play_acts values (12, 3, 4 );\p\g
insert into play_acts values (12, 4, 1 );\p\g
insert into play_acts values (12, 5, 6 );\p\g

insert into play_acts values (13, 1, 1 );\p\g
insert into play_acts values (13, 2, 1 );\p\g
insert into play_acts values (13, 3, 4 );\p\g
insert into play_acts values (13, 4, 3 );\p\g
insert into play_acts values (13, 5, 7 );\p\g

insert into play_acts values (14, 1, 3 );\p\g
insert into play_acts values (14, 2, 9 );\p\g
insert into play_acts values (14, 3, 5 );\p\g
insert into play_acts values (14, 4, 2 );\p\g
insert into play_acts values (14, 5, 1 );\p\g

insert into play_acts values (15, 1, 3 );\p\g
insert into play_acts values (15, 2, 4 );\p\g
insert into play_acts values (15, 3, 2 );\p\g
insert into play_acts values (15, 4, 4 );\p\g
insert into play_acts values (15, 5, 5 );\p\g

insert into play_acts values (16, 1, 4 );\p\g
insert into play_acts values (16, 2, 3 );\p\g
insert into play_acts values (16, 3, 5 );\p\g
insert into play_acts values (16, 4, 6 );\p\g
insert into play_acts values (16, 5, 5 );\p\g

insert into play_acts values (17, 1, 3 );\p\g
insert into play_acts values (17, 2, 4 );\p\g
insert into play_acts values (17, 3, 2 );\p\g
insert into play_acts values (17, 4, 5 );\p\g
insert into play_acts values (17, 5, 5 );\p\g

insert into play_acts values (18, 1, 3 );\p\g
insert into play_acts values (18, 2, 3 );\p\g
insert into play_acts values (18, 3, 5 );\p\g
insert into play_acts values (18, 4, 2 );\p\g
insert into play_acts values (18, 5, 4 );\p\g

insert into play_acts values (19, 1, 2 );\p\g
insert into play_acts values (19, 2, 4 );\p\g
insert into play_acts values (19, 3, 7 );\p\g
insert into play_acts values (19, 4, 8 );\p\g
insert into play_acts values (19, 5, 2 );\p\g

insert into play_acts values (20, 1, 3 );\p\g
insert into play_acts values (20, 2, 4 );\p\g
insert into play_acts values (20, 3, 3 );\p\g
insert into play_acts values (20, 4, 3 );\p\g
insert into play_acts values (20, 5, 5 );\p\g

insert into play_acts values (21, 1, 3 );\p\g
insert into play_acts values (21, 2, 7 );\p\g
insert into play_acts values (21, 3, 5 );\p\g
insert into play_acts values (21, 4, 3 );\p\g
insert into play_acts values (21, 5, 4 );\p\g

insert into play_acts values (22, 1, 5 );\p\g
insert into play_acts values (22, 2, 2 );\p\g
insert into play_acts values (22, 3, 4 );\p\g
insert into play_acts values (22, 4, 7 );\p\g
insert into play_acts values (22, 5, 2 );\p\g

insert into play_acts values (23, 1, 5 );\p\g
insert into play_acts values (23, 2, 5 );\p\g
insert into play_acts values (23, 3, 4 );\p\g
insert into play_acts values (23, 4, 3 );\p\g
insert into play_acts values (23, 5, 1 );\p\g

insert into play_acts values (24, 1, 3 );\p\g
insert into play_acts values (24, 2, 3 );\p\g
insert into play_acts values (24, 3, 3 );\p\g
insert into play_acts values (24, 4, 5 );\p\g
insert into play_acts values (24, 5, 10);\p\g

insert into play_acts values (25, 1, 4 );\p\g
insert into play_acts values (25, 2, 4 );\p\g
insert into play_acts values (25, 3, 2 );\p\g
insert into play_acts values (25, 4, 6 );\p\g
insert into play_acts values (25, 5, 1 );\p\g

insert into play_acts values (26, 1, 3 );\p\g
insert into play_acts values (26, 2, 3 );\p\g
insert into play_acts values (26, 3, 4 );\p\g
insert into play_acts values (26, 4, 3 );\p\g
insert into play_acts values (26, 5, 2 );\p\g

insert into play_acts values (27, 1, 3 );\p\g
insert into play_acts values (27, 2, 5 );\p\g
insert into play_acts values (27, 3, 7 );\p\g
insert into play_acts values (27, 4, 5 );\p\g
insert into play_acts values (27, 5, 3 );\p\g

insert into play_acts values (28, 1, 2 );\p\g
insert into play_acts values (28, 2, 2 );\p\g
insert into play_acts values (28, 3, 6 );\p\g
insert into play_acts values (28, 4, 3 );\p\g
insert into play_acts values (28, 5, 4 );\p\g

insert into play_acts values (29, 1, 5 );\p\g
insert into play_acts values (29, 2, 4 );\p\g
insert into play_acts values (29, 3, 7 );\p\g
insert into play_acts values (29, 4, 7 );\p\g
insert into play_acts values (29, 5, 3 );\p\g

insert into play_acts values (30, 1, 7 );\p\g
insert into play_acts values (30, 2, 4 );\p\g
insert into play_acts values (30, 3, 6 );\p\g
insert into play_acts values (30, 4, 3 );\p\g
insert into play_acts values (30, 5, 8 );\p\g

insert into play_acts values (31, 1, 4 );\p\g
insert into play_acts values (31, 2, 4 );\p\g
insert into play_acts values (31, 3, 4 );\p\g
insert into play_acts values (31, 4, 6 );\p\g
insert into play_acts values (31, 5, 3 );\p\g

insert into play_acts values (32, 1, 4 );\p\g
insert into play_acts values (32, 2, 5 );\p\g
insert into play_acts values (32, 3, 4 );\p\g
insert into play_acts values (32, 4, 6 );\p\g
insert into play_acts values (32, 5, 3 );\p\g

insert into play_acts values (33, 1, 10);\p\g
insert into play_acts values (33, 2, 3 );\p\g
insert into play_acts values (33, 3, 3 );\p\g
insert into play_acts values (33, 4, 7 );\p\g
insert into play_acts values (33, 5, 6 );\p\g

insert into play_acts values (34, 1, 2 );\p\g
insert into play_acts values (34, 2, 3 );\p\g
insert into play_acts values (34, 3, 3 );\p\g
insert into play_acts values (34, 4, 3 );\p\g
insert into play_acts values (34, 5, 3 );\p\g

insert into play_acts values (35, 1, 6 );\p\g
insert into play_acts values (35, 2, 5 );\p\g
insert into play_acts values (35, 3, 7 );\p\g
insert into play_acts values (35, 4, 4 );\p\g
insert into play_acts values (35, 5, 5 );\p\g

insert into play_acts values (36, 1, 2 );\p\g
insert into play_acts values (36, 2, 2 );\p\g
insert into play_acts values (36, 3, 3 );\p\g
insert into play_acts values (36, 4, 1 );\p\g
insert into play_acts values (36, 5, 1 );\p\g

insert into play_acts values (37, 1, 4 );\p\g
insert into play_acts values (37, 2, 4 );\p\g
insert into play_acts values (37, 3, 2 );\p\g
insert into play_acts values (37, 4, 2 );\p\g
insert into play_acts values (37, 5, 5 );\p\g


/*
** Create the online purchase table
*/
drop table play_order;\p\g
create table play_order(
    order_nr integer default NULL,
    user_id varchar(30) default NULL,
    product_id integer default NULL,
    status integer default NULL,
    timestamp date default NULL
)
with duplicates,
    page_size = 4096,
    location = (ii_database),
    security_audit=(table,norow)
;\p\g

modify play_order to hash on
    order_nr
    with minpages = 10,
    fillfactor = 50,
    extend = 16,
    allocation = 4,
    page_size = 4096
;\p\g

/*
** Table play_counters
*/
drop table play_counters;\p\g
create table play_counters(name char(8) not null not default,
    value integer not null default 1);\p\g

modify play_counters to btree unique on name;\p\g
insert into play_counters (name, value) values ('order', 1001);\p\g

/*
** Procedure new_order
*/
drop procedure new_order;
create procedure new_order as
declare
next1 integer not null;
begin
    select value into :next1 from play_counters where name = 'order';
    next1 = next1 + 1 ;
    update play_counters set value = :next1 where name = 'order';
    return :next1;
end;\p\g

/*
** Create shop table
*/
drop table play_item\p\g

create table play_item (
    id integer not null unique ,
    name varchar(200),
    cost integer,
    stock integer,
    insertor varchar(40)
);\p\g

insert into play_item values (  1, 'Globe Model'                            ,  50, 100, 'ingres');\p\g
insert into play_item values (  2, 'Tape Measure (for Measure)'             ,  15, 600, 'ingres');\p\g
insert into play_item values (  3, 'Tame Shrew Paper Weight'                ,  45,  80, 'ingres');\p\g
insert into play_item values (  4, 'Comedy Error Eraser'                    ,  17,  19, 'ingres');\p\g
insert into play_item values (  5, 'Venitian Merchant''s Heart T-Shirt'     ,  17,  19, 'ingres');\p\g
insert into play_item values (  6, 'All''s Well Book Ends (pair)'           ,  53,  27, 'ingres');\p\g
insert into play_item values (  7, 'Tony and Cleo''s Asses Milk Bubble Bath',  53,  27, 'ingres');\p\g
insert into play_item values (  8, 'Pericles Foot Pump'                     ,  53,  27, 'ingres');\p\g
insert into play_item values (  9, 'Tempest Wind Break'                     ,  53,  27, 'ingres');\p\g
insert into play_item values ( 10, 'Julius'' Bumper Salad Recipe Book'      ,  53,  27, 'ingres');\p\g
insert into play_item values ( 11, 'Brutus Letter Opener'                   ,  53,  27, 'ingres');\p\g
insert into play_item values ( 12, 'Hamlet 2B Pencil'                       ,   1,  53, 'ingres');\p\g
insert into play_item values ( 13, 'Gruoch''s All Purpose Spot Remover'     ,  99,  53, 'ingres');\p\g
insert into play_item values ( 14, 'A guide to parenting by King Lear'      , 100,   5, 'ingres');\p\g
insert into play_item values ( 15, 'Romeo Fragrance for Men'                ,  25,  30, 'ingres');\p\g
insert into play_item values ( 16, 'Juliet Parfum'                          ,  45, 100, 'ingres');\p\g
insert into play_item values ( 17, 'On Choosing and Purchasing A Horse -Richard III'            ,  60,  10, 'ingres');\p\g
insert into play_item values ( 18, 'Bottom''s Up ! tankard'                 ,  10,   5, 'ingres');\p\g
/*
** Procedure new_order
*/
drop procedure new_order;
create procedure new_order as
declare
next1 integer not null;
begin
    select value into :next1 from play_counters where name = 'order';
    next1 = next1 + 1 ;
    update play_counters set value = :next1 where name = 'order';
    return :next1;
end;\p\g

/*
** Procedure newProduct
*/
/* Create counter value for products */
insert into play_counters (name, value)
    select'item', max(id) from play_item;\p\g

drop procedure newProduct;
create procedure newProduct
    (name varchar(200) not null, cost integer not null,
     stock integer not null, insertor varchar(40) not null) as
declare
next1 integer not null;
status integer not null;
begin
    /* Get next item id */
    select value into :next1 from play_counters where name = 'item';
    next1 = next1 + 1 ;
    update play_counters set value = :next1 where name = 'item';

    /* Insert item into the play_item table */
    insert into play_item (id, name, cost, stock, insertor)
        values (:next1, :name, :cost, :stock, :insertor);
    status = iierrornumber;
    return :status;
end;\p\g

/* permits */
grant select on plays to public;\p\g
