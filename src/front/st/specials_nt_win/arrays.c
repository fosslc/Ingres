/********************************************************************
//
//  Copyright (C) 1995-1999  Ingres Corporation
//
//    Project : Visual DBA
//
//    string arrays of the assistant.
//
//    translation : please only replace quoted strings
//                  and NOT the syntax keywords
//              
//
********************************************************************/

[portion to be reintegrated by development]

SQLCOMP SQLDataConv[] =
{
 {TRUE,"BYTE",        F_DT_BYTE     ,"BYTE(expr,[,len])", "Converts the expression to BYTE binary data.",""},
 {TRUE,"C",           F_DT_C        ,"C(expr,[,len])"   , "Converts argument to c string."              ,""},
 {TRUE,"CHAR",        F_DT_CHAR     ,"CHAR(expr,[,len])", "Converts argument to char string."           ,""},
 {TRUE,"DATE",        F_DT_DATE     ,"DATE(expr)"       , "Converts a c, char, varchar or text string"  ,"to internal date representation."},
 {TRUE,"DECIMAL",     F_DT_DECIMAL  ,"DECIMAL(expr,[,precision[,scale]])","Converts any numeric expression to a decimal value.","" },
 {TRUE,"DOW",         F_DT_DOW      ,"DOW(expr)"        , "Converts an absolute date into its day of week","(for example, 'Mon', 'Tue')."},
 {TRUE,"FLOAT4",      F_DT_FLOAT4   ,"FLOAT4(expr)"     , "Converts the specified expression to float4." ,""},
 {TRUE,"FLOAT8 ",     F_DT_FLOAT8   ,"FLOAT8(expr)"     , "Converts the specified expression to float8.",""},
 {TRUE,"HEX",         F_DT_HEX      ,"HEX(expr)"        , "Returns the hexadecimal representation of"   ,"the argument string."},
 {TRUE,"INT1",        F_DT_INT1     ,"INT1(expr)"       , "Converts the specified expression to"        ,"integer1."},
 {TRUE,"INT2",        F_DT_INT2     ,"INT2(expr)"       , "Converts the specified expression to"        ,"smallint."},
 {TRUE,"INT4",        F_DT_INT4     ,"INT4(expr)"       , "Converts the specified expression to"        ,"integer."},
 {TRUE,"LONG_BYTE",   F_DT_LONGBYTE ,"LONG_BYTE(expr[,len])","Converts the expression to long byte","binary data."},
 {TRUE,"LONG_VARCHAR",F_DT_LONGVARC ,"LONG_VARCHAR(expr)","Converts the expression to a long varchar."   ,""},
 {TRUE,"MONEY",       F_DT_MONEY    ,"MONEY(expr)"      , "Converts the specified expression to"        ,"internal money representation."},
 {TRUE,"OBJECT_KEY",  F_DT_OBJKEY   ,"OBJECT_KEY(expr)" , "Converts the operand to an object_key."      ,""},
 {TRUE,"TABLE_KEY",   F_DT_TABKEY   ,"TABLE_KEY(expr)"  , "Converts the operand to a table_key."        ,""},
 {TRUE,"TEXT",        F_DT_TEXT     ,"TEXT(expr[,len])" , "Converts argument to text string."           ,""},
 {TRUE,"VARBYTE ",    F_DT_VARBYTE  ,"VARBYTE(expr[,len])","Converts argument to byte varying binary data." ,""},
 {TRUE,"VARCHAR",     F_DT_VARCHAR  ,"VARCHAR(expr[,len])","Converts argument to varchar string."       ,""},
 {0,0,             0             ,0,                   0,0 }
};

SQLCOMP SQLNumFuncs[] =
{
 {TRUE,"ABS",         F_NUM_ABS      , "ABS(n)"  , "Absolute Value of n."   ,""},
 {TRUE,"ATAN",        F_NUM_ATAN     , "ATAN(n)" , "Arctangent of n;"      ,"returns value from (-pi/2) to pi/2 ."},
 {TRUE,"COS",         F_NUM_COS      , "COS(n)"  , "Cosine of n;"          ,"returns a value from -1 to 1 ."},
 {TRUE,"EXP",         F_NUM_EXP      , "EXP(n)"  , "Exponential of n."      ,""},
 {TRUE,"LOG",         F_NUM_LOG      , "LOG(n)"  , "Natural Logarithm of n.",""},
 {TRUE,"MOD",         F_NUM_MOD      , "MOD(n,b)", "n Modulo b."           ,""},
 {TRUE,"SIN",         F_NUM_SIN      , "SIN(n)"  , "Sine of n;"            ,"returns a value from -1 to 1 ."},
 {TRUE,"SQRT",        F_NUM_SQRT     , "SQRT(n)" , "Square Root of n."      ,""},
 {0,0,             0              ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLMathFuncs[] =
{
 {TRUE,"@ABS",        OF_MATH_ABS      , "@ABS(x)",       "Absolute Value of x."                     ,""},
 {TRUE,"@ACOS",       OF_MATH_ACOS     , "@ACOS(x)",      "Returns arc-cosine of x in radians;"      ,"x must be in the range [-1,1]"},
 {TRUE,"@ASIN",       OF_MATH_ASIN     , "@ASIN(x)",      "Returns arc-sine of x in radians;"        ,"x must be in the range [-1,1]"},
 {TRUE,"@ATAN",       OF_MATH_ATAN     , "@ATAN(x)",      "Returns arc-tangent of x in radians;"     ,""},
 {TRUE,"@ATAN2",      OF_MATH_ATAN2    , "@ATAN2(x,y)",   "Returns the arc-tangent of y/x"           ,""},
 {TRUE,"@COS",        OF_MATH_COS      , "@COS(x)",       "Cosine of x, where x is in radians;"      ,""},
 {TRUE,"@EXP",        OF_MATH_EXP      , "@EXP(x)",       "Returns the Natural Logarithm (base e)"   ,"raised to the x power"},
 {TRUE,"@FACTORIAL",  OF_MATH_FACTORIAL, "@FACTORIAL(x)", "Factorial of x."                          ,
                                                          "x must be integer and non-negative (>=0)"},
 {TRUE,"@INT",        OF_MATH_INT      , "@INT(x)",       "Returns integer portion of x. If x < 0,"  ,"the decimal portion is truncated."},
 {TRUE,"@LN",         OF_MATH_LN       , "@LN(x)",        "Returns Natural Logarithm of"             ,"(positive) x"},
 {TRUE,"@LOG",        OF_MATH_LOG      , "@LOG(x)",       "Base 10 logarithm of"                     ,"(positive) x"},
 {TRUE,"@MOD",        OF_MATH_MOD      , "@MOD(x,y)",     "modulo (remainder) of x/y"                 ,""},
 {FALSE,"@PI",        OF_MATH_PI       , "@PI",           "Returns the value Pi (3.14159265)"        ,""},
 {TRUE,"@ROUND",      OF_MATH_ROUND    , "@ROUND(x,n)",   "Rounds the number x with n decimal places",""},
 {TRUE,"@SIN",        OF_MATH_SIN      , "@SIN(x)",       "Sine of x, where x is in radians"         ,""},
 {TRUE,"@SQRT",       OF_MATH_SQRT     , "@SQRT(x)",      "Square root of (positive or null) x"      ,""},
 {TRUE,"@TAN",        OF_MATH_TAN      , "@TAN(x)",       "Tangent of x, where x is in radians"      ,""},
 {0,0,             0                ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLFinFuncs[] =
{
 {TRUE,"@CTERM", OF_FIN_CTERM   , "@CTERM(int,fv,pv)" ,"Number of compounding periods to an investment of",
                                                       "present value pv to grow to fv,with interest rate int"},
 {TRUE,"@FV",    OF_FIN_FV      , "@FV(pmt,int,n)"    ,"Future Value of series of equal payments pmt",
                                                       "earning periodic interest rate int,over n periods"},
 {TRUE,"@PMT",   OF_FIN_PMT     , "@PMT(ppal,int,per)","Amount of each per.payment needed to pay off loan princ",
                                                       "ppal, periodic interest rate int, over per periods"},
 {TRUE,"@PV",    OF_FIN_PV      , "@PV(pmt,int,n)"    ,"Present Value of series of equal payments pmt,",
                                                       "with periodic interest rate int, over n periods"},
 {TRUE,"@RATE",  OF_FIN_RATE    , "@RATE(fv,pv,n)"    ,"rate for investment of present value pv to grow to fv",
                                                       "over number of compounding periods n."},
 {TRUE,"@SLN",   OF_FIN_SLN     , "@SLN(cost, salv,lif)", "Straight-line depreciation of asset for each period",
                                                         "given base cost,predicted salvage value,expected life of asset"},
 {FALSE,"@SYD",  OF_FIN_SYD     , "@SYD(cost,sal,lif,pe)", "Sum-of-the-years'-Digits depreciation allowance",
                                                           "given base cost,pred.salvage value,exp.life of asset,period"},
 {TRUE,"@TERM",  OF_FIN_TERM    , "@TERM(pmt,int,fv)" , "Number of payment periods, given amount of each",
                                                        "payment, periodic interest rate, and future value"},
 {0,0,             0              ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLDateTime[] =
{
 {TRUE,"@DATE" ,      OF_DT_DATE        , "@DATE(year,month,day)"    ,"converts the arguments to a date."                 ,""},
 {TRUE,"@DATETOCHAR", OF_DT_DATETOCHAR  , "@DATETOCHAR(date,picture)","Applies the editing specified by picture"    ,
                                                                 "to a DATE,TIME or DATETIME"},
 {TRUE,"@DATEVALUE",  OF_DT_DATEVALUE   , "@DATEVALUE(datestring)"   ,"Converts the argument to a date."                  ,""},
 {TRUE,"@DAY",        OF_DT_DAY         , "@DAY(date)"               ,"Returns the day of the month (between 1 and 31)"   ,""},
 {TRUE,"@HOUR",       OF_DT_HOUR        , "@HOUR(date)"              ,"Returns the hour of the day (between 0 and 23)"    ,""},
 {TRUE,"@MICROSECOND",OF_DT_MICROSECOND , "@MICROSECOND(date)"       ,"Returns the microsecond value in a datetime or"    ,"time value"},
 {TRUE,"@MINUTE",     OF_DT_MINUTE      , "@MINUTE(date)"            ,"Returns the minute of the hour (between 0 and 59)" ,""},
 {TRUE,"@MONTH",      OF_DT_MONTH       , "@MONTH(date)"             ,"Returns the month of the year (between 1 and 12)"  ,""},
 {TRUE,"@MONTHBEG",   OF_DT_MONTHBEG    , "@MONTHBEG(date)"          ,"Returns the first day of the month represented"    ,"by the date"},
 {FALSE,"@NOW",       OF_DT_NOW         , "@NOW"                     ,"Returns current date and time"                     ,""},
 {TRUE,"@QUARTER",    OF_DT_QUARTER     , "@QUARTER(date)"           ,"Returns the quarter (between 1 and 4)"             ,""},
 {TRUE,"@QUARTERBEG", OF_DT_QUARTERBEG  , "@QUARTERBEG(date)"        ,"Returns the first day of the quarter"        ,
                                                                      "represented by the date"},
 {TRUE,"@SECOND",     OF_DT_SECOND      , "@SECOND(date)"            ,"Returns the second of the minute(between 0 and 59)",""},
 {TRUE,"@TIME",       OF_DT_TIME        , "@TIME(hour,min,sec)"      ,"Returns a time value, given hour, min, and sec",""},
 {TRUE,"@TIMEVALUE",  OF_DT_TIMEVALUE   , "@TIMEVALUE(time)"         ,"Returns a time value, given a string"           ,""},
 {TRUE,"@WEEKBEG",    OF_DT_WEEKBEG     , "@WEEKBEG(date)"           ,"Returns the date of the Monday of the week"     ,"containing the date"},
 {TRUE,"@WEEKDAY",    OF_DT_WEEKDAY     , "@WEEKDAY(date)"           ,"Returns the day of the week (between 0 and 6)"  ,""},
 {TRUE,"@YEAR",       OF_DT_YEAR        , "@YEAR(date)"              ,"Returns the year relative to 1900"              ,
                                                                       "(between -1900 and 2000)"},
 {TRUE,"@YEARBEG",    OF_DT_YEARBEG     , "@YEARBEG(date)"           ,"Returns the first day of the year"              ,
                                                                       "represented by the date"},
 {TRUE,"@YEARNO",     OF_DT_YEARNO      , "@YEARNO(date)"            ,"Returns the calendar year (4-digit number)"    ,""},
 {0,0,             0              ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLStrFuncs[] =
{
 {TRUE,"@CHAR" ,      OF_STR_CHAR      , "@CHAR(number)"  ,"Returns the ASCII character for a decimal code."   ,""},
 {TRUE,"@CODE",       OF_STR_CODE      , "@CODE(string)"  ,"Returns the ASCII decimal code of the first"   ,"character in the string"},
 {FALSE,"@DECODE",    OF_STR_DECODE    , "@DECODE(exp,s1,r1,s2,r2,...,[def])","If exp equals any sxx, returns rxx;","if not, returns default"},
 {TRUE,"@EXACT",      OF_STR_EXACT     , "@EXACT(string1,string2)"           ,"Returns 1 if strings are identical;","otherwise, returns 0"},
 {TRUE,"@FIND",       OF_STR_FIND      , "@FIND(string1,string2,pos)","Returns position of string1 in string2,",
                                                                 "starting at pos. If not found, returns -1"},
 {TRUE,"@LEFT",       OF_STR_LEFT      , "@LEFT(string,length)"      ,"Returns a string for the specified length",
                                                                 "starting with the first characters of string"},
 {TRUE,"@LENGTH",     OF_STR_LENGTH    , "@LENGTH(string)"           ,"Returns the length of the string"   ,""},
 {TRUE,"@LOWER",      OF_STR_LOWER     , "@LOWER(string)"            ,"Converts uppercase alphabetic characters"   ,"to lowercase"},
 {TRUE,"@MID",        OF_STR_MID       , "@MID(string,start,length)" ,"Returns a string of specified length","from string, starting at <start>"},
 {TRUE,"@NULLVALUE",  OF_STR_NULLVALUE , "@NULLVALUE(x,y)"           ,"Returns y if x is NULL"   ,""},
 {TRUE,"@PROPER",     OF_STR_PROPER    , "@PROPER(string)"           ,"Converts first character to uppercase",
                                                                      "and other characters to lowercase"},
 {TRUE,"@REPEAT",     OF_STR_REPEAT    , "@REPEAT(string,number)"    ,"Returns Concatenation of string with itself for",
                                                                 "the specified number of times"},
 {FALSE,"@REPLACE",   OF_STR_REPLACE   , "@REPLACE(s1,pos,len,s2)","Returns string in which chars from s1 are replaced with",
                                                              "chars of s2, starting at pos, len chars are removed"},
 {TRUE,"@RIGHT",      OF_STR_RIGHT     , "@RIGHT(string,length)"  ,"Returns the rightmost length characters of string"   ,""},
 {TRUE,"@SCAN",       OF_STR_SCAN      , "@SCAN(string,pattern)"  ,"Returns the numeric position of the first occurence of pattern" ,
                                                              "in string; returns -1 if no occurence, NULL if string is NULL"},
 {TRUE,"@STRING",     OF_STR_STRING    , "@STRING(number,scale)"  ,"Converts a number into a string with scale number",
                                                              "of decimal places. Numbers are rounded when appropriate."},
 {TRUE,"@SUBSTRING",  OF_STR_SUBSTRING , "@SUBSTRING(string,pos[,len])"  ,"Returns substring of string, starting at pos","with length len"},
 {TRUE,"@TRIM",       OF_STR_TRIM      , "@TRIM(string)"          ,"strips leading and trailing blanks from string"   
                                                             ,"and compresses multiple spaces into single spaces"},
 {TRUE,"@UPPER",      OF_STR_UPPER     , "@UPPER(string)"         ,"converts lowercase letters to uppercase"   ,""},
 {TRUE,"@VALUE",      OF_STR_VALUE     , "@VALUE(string)"         ,"converts string into the number represented"   ,"by that string"},
 {0,0,             0              ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLAggFuncs[] =
{
 {TRUE,"AVG",          F_AGG_AVG     , "AVG([distinct|all] expr)"    , "Average (sum/count)."             ,""},
 {TRUE,"COUNT",        F_AGG_COUNT   , "COUNT([distinct|all] expr)"  , "Count of occurrences."            ,""},
 {TRUE,"MAX",          F_AGG_MAX     , "MAX(expr)"                   , "Maximum value."                   ,""},
 {TRUE,"@MEDIAN",     OF_AGG_MEDIAN  , "@MEDIAN([distinct|all] expr)", "Middle value in a set of values;" ,""},
 {TRUE,"MIN",          F_AGG_MIN     , "MIN(expr)"                   , "Minimum value."                   ,""},
 {TRUE,"SUM",          F_AGG_SUM     , "SUM([distinct|all] expr)"    , "Column total."                    ,""},
 {TRUE,"@SDV",        OF_AGG_SDV     , "@SDV([distinct|all] expr)"   , "Standard deviation for the set of values",""},
 {0,0,             0              ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLSpeFuncs[] =
{
 {FALSE,"@CHOOSE" ,   OF_SPE_CHOOSE  , "@CHOOSE(sel_num,v1,v2,.. ,vm)", "Selects v1 if sel_num=0, v2 if sel_num=1, ...",""},
 {TRUE,"@DECIMAL",    OF_SPE_DECIMAL , "@DECIMAL(string)"             , "Returns the decimal equivalent for the given",
                                                                        "hexadecimal number string"},
// {"@DECRYPT",    OF_SPE_DECRYPT , "@DECRYPT(password)"  , "decrypts a password stored in the PASSWORD","column in SYSADM.SYSUSERAUTH"},
 {TRUE,"@DECODE",     OF_STR_DECODE  , "@DECODE(exp,s1,r1,s2,r2,...,[def])","If exp equals any sxx, returns rxx;","if not, returns default"},
 {TRUE,"@HEX",        OF_SPE_HEX     , "@HEX(number)" , "Returns the hexadecimal equivalent for  (number)"         ,""},
 {TRUE,"@LICS",       OF_SPE_LICS    , "@LICS(string)", "Uses international character set for sorting its" ,
                                                   "argument, instead of ASCII character set"},
 {0,0,             0              ,  0        , 0                       ,0 }
};

SQLCOMP OIDTSQLDB_Objects[] =
{
 {TRUE,"COLUMN",     F_DB_COLUMNS       ,"Column"       , "Returns a column of current table."       ,""},
 {TRUE,"TABLE",      F_DB_TABLES        ,"Table"        , "Returns a Table of current database."     ,""},
 {TRUE,"DATABASE",   F_DB_DATABASES     ,"Database"     , "Returns a Database of current node."      ,""},
 {TRUE,"DBAREA",     OF_DB_DBAREA       ,"DBArea"       , "Returns a Database Area of current node." ,""},
 {TRUE,"STOGROUP",   OF_DB_STOGROUP     ,"Storage Group" ,"Returns a Storage Group of current node." ,""},
 {0,0,             0                  ,0,          0, 0}
};


SQLCOMP SQLStrFuncs[] =
{
 {TRUE,"CHAREXTRACT", F_STR_CHAREXT  , "CHAREXTRACT(c1,n)" , "Returns the nth byte of c1."               ,""},
 {TRUE,"CONCAT",      F_STR_CONCAT   , "CONCAT(c1,c2)"     , "Concatenates one string to another."        ,""},
 {TRUE,"LEFT",        F_STR_LEFT     , "LEFT(c1,len)"      , "Returns the leftmost len characters of c1." ,""},
 {TRUE,"LENGTH",      F_STR_LENGTH   , "LENGTH(c1)"        , "Length of c1 without trailing blanks, or",
                                                        "number of chars if variable-length string."},
 {TRUE,"LOCATE",      F_STR_LOCATE   , "LOCATE(c1,c2)"     , "Returns the location of first occurrence"   ,"of c2 within c1."},
 {TRUE,"LOWERCASE",   F_STR_LOWERCS  , "LOWERCASE(c1)"     , "Converts all uppercase characters in c1"    ,"to lowercase."},
 {TRUE,"PAD",         F_STR_PAD      , "PAD(c1)"           , "Returns c1 with trailing blanks appended"   ,"to c1."},
 {TRUE,"RIGHT",       F_STR_RIGHT    , "RIGHT(c1,len)"     , "Returns the rightmost len characters of c1.",""},
 {TRUE,"SHIFT",       F_STR_SHIFT    , "SHIFT(c1,nshift)"  , "Shifts the string nshift places to the "    ,
                                                        "right or to the left (nshift >0 or <0 )."},
 {TRUE,"SIZE",        F_STR_SIZE     , "SIZE(c1)"          , "Returns the declared size of c1,"           ,
                                                        "without removal of trailing blanks."},
 {TRUE,"SQUEEZE",     F_STR_SQUEEZE  , "SQUEEZE(c1)"       , "Compresses white spaces."                    ,""},
 {TRUE,"TRIM",        F_STR_TRIM     , "TRIM(c1)"          , "Returns c1 without trailing blanks."        ,""},
 {TRUE,"NOTRIM",      F_STR_NOTRIM   , "NOTRIM(c1)"        , "Retains trailing blanks when placing"       ,
                                                        "a value in a char column."},
 {TRUE,"UPPERCASE",   F_STR_UPPERCS  , "UPPERCASE(c1)"     , "Converts all lowercase characters in c1"    ,"to uppercase."},
 {0,0,             0              ,0                    , 0,0 }
};

SQLCOMP SQLDateFuncs[] =
{
 {TRUE,"DATE_TRUNC",  F_DAT_DATETRUNC , "DATE_TRUNC(unit,date)", "Returns a date value truncated"             ,"to the specified unit."},
 {TRUE,"DATE_PART",   F_DAT_DATEPART  , "DATE_PART(unit,date)" , "Returns an integer containing the specified",
                                                            "(unit) component of the input date."},
 {TRUE,"DATE_GMT",    F_DAT_GMT       , "DATE_GMT(date)"       , "Converts an absolute date into the GMT"     ,"character equivalent."},
 {TRUE,"INTERVAL",    F_DAT_INTERVAL  , "INTERVAL(unit,date_interval)","Converts a date interval into a floating",
                                                                  "point constant expressed according to (unit)"},
 {TRUE,"_DATE",       F_DAT_DATE      , "_DATE(s)"             , "Returns string giving the date s seconds"   ,
                                                            "after January 1, 1970 GMT."},
 {TRUE,"_TIME",       F_DAT_TIME      , "_TIME(s)"             , "Returns string giving the time s seconds"   ,
                                                            "after January 1, 1970 GMT."},
 {0,0,             0               ,0                       , 0,0 }
};
SQLCOMP SQLAggFuncs[] =
{
 {TRUE,"ANY",          F_AGG_ANY        , "ANY([distinct|all] expr)"  , "Returns 1 if any row in the table fulfills",
                                                                        "the where clause, else 0 ."},
 {TRUE,"COUNT",        F_AGG_COUNT      , "COUNT([distinct|all] expr)", "Count of occurrences."                       ,""},
 {TRUE,"SUM",          F_AGG_SUM        , "SUM([distinct|all] expr)"  , "Column total."                              ,""},
 {TRUE,"AVG",          F_AGG_AVG        , "AVG([distinct|all] expr)"  , "Average (sum/count)."                       ,""},
 {TRUE,"MAX",          F_AGG_MAX        , "MAX(expr)"                 , "Maximum value."                             ,""},
 {TRUE,"MIN",          F_AGG_MIN        , "MIN(expr)"                 , "Minimum value."                             ,""},
 {0,0,              0                ,0                            , 0,0 }
};

SQLCOMP SQLIfnFuncs[] =
{
 {TRUE,"IFNULL",       F_IFN_IFN       , "IFNULL(v1,v2)", "Returns a non-null value v2 when a null","is encountered for v1, else returns v1."},
 {0,0,             0             ,0,                 0,0 }
};

SQLCOMP SQLPredFuncs[] =
{
 {TRUE,"Comparison",  F_PRED_COMP      , "expr1 comparison_operator expr2"       ,"Example : amount > 300"                            ,""},
 {TRUE,"Like",        F_PRED_LIKE      , "expr LIKE pattern [ESCAPE escape_char]","Example : name like 'a%'"                          ,""},
 {TRUE,"Between",     F_PRED_BETWEEN   , "y BETWEEN x AND z ."                     ,"",""},
 {TRUE,"In",          F_PRED_IN        , "y IN (x,...,z) or y IN (subquery)"     ,"is TRUE if y is equal to one of the values in the" ,
                                                                             "list, or returned by the subquery."},
 {TRUE,"Any",         F_PRED_ANY       , "ANY(subquery)"                         ,"Example: ... where dept = any(select dno from dept",
                                                                             "where floor = 3);"},
 {TRUE,"All",         F_PRED_ALL       , "ALL(subquery)"                         ,"A predicate using ALL is true if the specified comparison",
                                                                             "is true for all values y returned by the subquery."},
 {TRUE,"Exists",      F_PRED_EXIST     , "EXISTS(subquery)"                      ,"is TRUE if the set returned by the subquery"       ,
                                                                             "is not empty."},
 {TRUE,"Is null",     F_PRED_ISNULL    , "x is [not] null"                       ,"is TRUE if x is a null",""},
 {0,0,             0             ,0,                 0,0 }
};

SQLCOMP SQLPredCombFuncs[] =
{
 {TRUE,"AND",         F_PREDCOMB_AND   , "Predicate AND Predicate", "true if both predicates are true,",
                                                               "unknown if one (or both) are unknown."},
 {TRUE,"OR",          F_PREDCOMB_OR    , "Predicate OR Predicate" , "true if one (or both) are true,"   ,
                                                               "unknown if both are unknown."},
 {TRUE,"NOT",         F_PREDCOMB_NOT   , "NOT Predicate"          , "Not(true) is false, Not(false) is true,",
                                                               "and not(unknown) is unknown."},
 {0,0,             0                , 0                        , 0,0 }
};

SQLCOMP SQLDB_Objects[] =
{
 {TRUE,"COLUMN",     F_DB_COLUMNS       ,"Column"   , "Returns a column of current table."    ,""},
 {TRUE,"TABLE",      F_DB_TABLES        ,"Table"    , "Returns a Table of current database."  ,""},
 {TRUE,"DATABASE",   F_DB_DATABASES     ,"Database" , "Returns a Database of current node."   ,""},
 {TRUE,"USER",       F_DB_USERS         ,"User"     , "Returns a User of current node."       ,""},
 {TRUE,"GROUP",      F_DB_GROUPS        ,"Group"    , "Returns a Group of current node."      ,""},
 {TRUE,"ROLE",       F_DB_ROLES         ,"Role"     , "Returns a Role of current node."       ,""},
 {TRUE,"PROFILE",    F_DB_PROFILES      ,"Profile"  , "Returns a Profile of current node."    ,""},
 {TRUE,"LOCATION",   F_DB_LOCATIONS     ,"Location" , "Returns a Location of current node."   ,""},
 {0,0,             0                  ,0,          0, 0}
};

SQLCOMP SQLSelFuncs[] =
{
 {TRUE,"SELECT",     F_SUBSEL_SUBSEL    , "(Sub)Select Statement", "Enter or build a (sub)select statement",
                                                              "Can take the place of some expressions in some predicates."},
 {0,0,             0             ,0,                 0,0 }
};

// family structure
SQLFAM sFamily[] =
{
 {"Data Type Conversion",       FF_DTCONVERSION     , (LPSQLCOMP)SQLDataConv     },
 {"Numeric functions",          FF_NUMFUNCTIONS     , (LPSQLCOMP)SQLNumFuncs     },
 {"String Functions",           FF_STRFUNCTIONS     , (LPSQLCOMP)SQLStrFuncs     },
 {"Date Functions",             FF_DATEFUNCTIONS    , (LPSQLCOMP)SQLDateFuncs    },
 {"Aggregate Functions",        FF_AGGRFUNCTIONS    , (LPSQLCOMP)SQLAggFuncs     },
 {"IfNull Function",            FF_IFNULLFUNC       , (LPSQLCOMP)SQLIfnFuncs     },
 {"Search Condition Predicate", FF_PREDICATES       , (LPSQLCOMP)SQLPredFuncs    },
 {"Predicate Combinations",     FF_PREDICCOMB       , (LPSQLCOMP)SQLPredCombFuncs},
 {"Database Objects",           FF_DATABASEOBJECTS  , (LPSQLCOMP)SQLDB_Objects   },
 {"SubQueries",                 FF_SUBSELECT        , (LPSQLCOMP)SQLSelFuncs     },
 {0,                            0                   , 0}
};

SQLFAM OIDTsFamily[] =
{
// {"Data Type Conversion",       FF_DTCONVERSION     , (LPSQLCOMP)SQLDataConv     },
 {"Math functions",             OFF_MATHFUNCTIONS     , (LPSQLCOMP)OIDTSQLMathFuncs     },
 {"Finance functions",          OFF_FINFUNCTIONS     , (LPSQLCOMP)OIDTSQLFinFuncs     },
 {"String Functions",           OFF_STRFUNCTIONS     , (LPSQLCOMP)OIDTSQLStrFuncs     },
 {"Date/Time Functions",        OFF_DATEFUNCTIONS    , (LPSQLCOMP)OIDTSQLDateTime     },
 {"Aggregate Functions",        OFF_AGGRFUNCTIONS    , (LPSQLCOMP)OIDTSQLAggFuncs     },
 {"Special Functions",          OFF_SPEFUNCTIONS     , (LPSQLCOMP)OIDTSQLSpeFuncs     },
 {"IfNull Function",            OFF_IFNULLFUNC       , (LPSQLCOMP)SQLIfnFuncs     },
 {"Search Condition Predicate", OFF_PREDICATES       , (LPSQLCOMP)SQLPredFuncs    },
 {"Predicate Combinations",     OFF_PREDICCOMB       , (LPSQLCOMP)SQLPredCombFuncs},
 {"Database Objects",           OFF_DATABASEOBJECTS  , (LPSQLCOMP)OIDTSQLDB_Objects},
 {"SubQueries",                 OFF_SUBSELECT        , (LPSQLCOMP)SQLSelFuncs     },
 {0,                            0                   , 0}
};

