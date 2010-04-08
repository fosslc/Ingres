/*
** Copyright (c) 2004, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: connectstringconfig.cs
	**
	** Description:
	**	Implements Connection String Configuration class with IConfig interface.
	**
	**  Classes
	**
	**	ConnectStringConfig	Implements Connection String Configuration.
	**
	** History:
	**	25-Nov-03 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 5-nov-04 (thoda04) Bug 113421
	**	    Support quoted values inside connect string values.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	27-feb-07 (thoda04) SIR 117018
	**	    Add dbmsuser, dbmspassword, and char_encoding support.
	**	12-may-09 (thoda04)
	**	    Add support for parenthesis list for 
	**	    multiple host target support.
	**	16-feb-10 (thoda04) Bug 123298
	**	    Added SendIngresDates support to send .NET DateTime parms
	**	    as ingresdate type rather than as timestamp_w_timezone.
	*/


	/*
	** Name: ConnectStringConfig
	**
	** Description:
	**	Parse a connection string and build a configuarion name/value list.
	**
	**  Interface Methods:
	**
	**
	**  Public Methods:
	**
	** History:
	**	25-Nov-03 (thoda04)
	**	    Created.
	**	28-Nov-04 (thoda04)
	**	    Add connection string support for VNODE_USAGE, TIMEZONE,
	**	    DECIMAL_CHAR, DATE_FORMAT, MONEY_FORMAT, MONEY_PRECISION.
	*/

	/// <summary>
	/// Connection string configuration class.
	/// </summary>
	public sealed class ConnectStringConfig :
		System.Collections.Specialized.NameValueCollection, IConfig
	{
		static internal DictionaryEntry[]
			ConnectBuilderKeywords;
		static internal System.Collections.Specialized.StringCollection
			ConnectBuilderKeywordsLowerCase;
		static internal Hashtable
			NormalizedUserKeyword;

		static ConnectStringConfig()
		{
			ConnectBuilderKeywords = new DictionaryEntry[] { 
				new DictionaryEntry("Server",           ""),
				new DictionaryEntry("Port", "II7"),
				new DictionaryEntry("Database", ""),
				new DictionaryEntry("User ID", ""),
				new DictionaryEntry("Password", ""),
				new DictionaryEntry("Role ID", ""),
				new DictionaryEntry("Role Password", ""),
				new DictionaryEntry("DBMS User", ""),
				new DictionaryEntry("DBMS Password", ""),
				new DictionaryEntry("Character Encoding", ""),
				new DictionaryEntry("Group ID", ""),
				new DictionaryEntry("Timezone", ""),
				new DictionaryEntry("Decimal_Char", "."),
				new DictionaryEntry("Date_Format", ""),
				new DictionaryEntry("Money_Format", ""),
				new DictionaryEntry("Money_Precision",  ""),
				new DictionaryEntry("Blank Date", ""),
				new DictionaryEntry("Connect Timeout", (Int32)15),
				new DictionaryEntry("Max Pool Size", (Int32)100),
				new DictionaryEntry("Min Pool Size", (Int32)0),
				new DictionaryEntry("Vnode_Usage", "connect"),
				new DictionaryEntry("Enlist", (Boolean)true),
				new DictionaryEntry("Pooling", (Boolean)true),
				new DictionaryEntry("Persist Security Info", (Boolean)false),
				new DictionaryEntry("Cursor_Mode", ""),
				new DictionaryEntry("SendIngresDates", (Boolean)false),
			};

			//ConnectBuilderValuesDefault = new Object[ConnectBuilderKeywords.Length];
			//ConnectBuilderKeywords.Values.CopyTo(ConnectBuilderValuesDefault, 0);

			NormalizedUserKeyword = new Hashtable();
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_HOST,      "Server");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_PORT,      "Port");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_DB,        "Database");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_USR,       "User ID");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_PWD,       "Password");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_ROLE,      "Role ID");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_ROLEPWD,   "Role Password");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_DBUSR,     "DBMS User");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_DBPWD,     "DBMS Password");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_ENCODE,    "Character Encoding");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_GRP,       "Group ID");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_TIMEZONE,  "Timezone");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_DEC_CHAR,  "Decimal_Char");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_DATE_FRMT, "Date_Format");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_MNY_FRMT,  "Money_Format");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_MNY_PREC,  "Money_Precision");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_BLANKDATE, "Blank Date");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_TIMEOUT,   "Connect Timeout");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_MINPOOLSIZE,"Min Pool Size");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_MAXPOOLSIZE,"Max Pool Size");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_VNODE,     "Vnode_Usage");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_ENLISTDTC, "Enlist");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_CLIENTPOOL,"Pooling");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_PERSISTSEC,"Persist Security Info");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_CRSR,      "Cursor_Mode");
			NormalizedUserKeyword.Add(DrvConst.DRV_PROP_SENDINGRESDATES, "SendIngresDates");

			ConnectBuilderKeywordsLowerCase =
				new System.Collections.Specialized.StringCollection();
			foreach (DictionaryEntry entry in ConnectBuilderKeywords)
			{
				ConnectBuilderKeywordsLowerCase.Add(
					ToInvariantLower((String)entry.Key));
			}
		}

		static internal int TryGetOrdinal(String keyword)
		{
			if (keyword == null)
				throw new ArgumentNullException("keyword");
			String strLower = keyword.ToLowerInvariant();

			// Adjust for synonyms
			if (strLower == "blankdate")
				strLower =  "blank date";
			else
			if (strLower == "characterencoding"  ||
				strLower == "character_encoding" ||
				strLower == "char_encoding"      ||
				strLower == "charencoding"       ||
				strLower == "char encoding")
				strLower =  "character encoding";
			else
			if (strLower == "connection timeout"  ||
				strLower == "connectiontimeout"   ||
				strLower == "connecttimeout")
				strLower =  "connect timeout";
			else
			if (strLower == "db"               ||
				strLower == "initial catalog"  ||
				strLower == "initialcatalog")
				strLower =  "database";
			else
			if (strLower == "date_fmt"    ||
				strLower == "date fmt"    ||
				strLower == "dateformat"  ||
				strLower == "date format")
				strLower =  "date_format";
			else
			if (strLower == "decimal"     ||
				strLower == "decimalchar" ||
				strLower == "decimal char")
				strLower =  "decimal_char";
			else
			if (strLower == "groupid")
				strLower =  "group id";
			else
			if (strLower == "host"            ||
				strLower == "address"         ||
				strLower == "addr"            ||
				strLower == "network address" ||
				strLower == "networkaddress")
				strLower =  "server";
			else
			if (strLower == "maxpoolsize")
				strLower =  "max pool size";
			else
			if (strLower == "minpoolsize")
				strLower =  "min pool size";
			else
			if (strLower == "money_fmt"   ||
				strLower == "moneyfmt"    ||
				strLower == "moneyformat" ||
				strLower == "money format")
				strLower =  "money_format";
			else
			if (strLower == "money_prec"     ||
				strLower == "moneyprec"      ||
				strLower == "moneyprecision" ||
				strLower == "money precision")
				strLower =  "money_precision";
			else
			if (strLower == "persistsecurityinfo")
				strLower =  "persist security info";
			else
			if (strLower == "pwd")
				strLower =  "password";
			else
			if (strLower == "roleid")
				strLower =  "role id";
			else
			if (strLower == "role pwd"  ||
				strLower == "rolepwd"   ||
				strLower == "rolepassword")
				strLower =  "role password";
			else
			if (strLower == "dbms_user"  ||
				strLower == "dbmsuserid" ||
				strLower == "dbmsuser")
				strLower = "dbms user";
			else
			if (strLower == "dbms pwd" ||
				strLower == "dbmspwd" ||
				strLower == "dbmspassword"  ||
				strLower == "dbms_pwd"      ||
				strLower == "dbms_password")
				strLower = "dbms password";
			else
			if (strLower == "tz")
				strLower =  "timezone";
			else
			if (strLower == "uid"  ||
				strLower == "userid")
				strLower =  "user id";
			else
			if (strLower == "vnodeusage")
				strLower = "vnode_usage";
			else
			if (strLower == "send_ingres_dates"  ||
			    strLower == "send ingres dates"  ||
			    strLower == "send ingresdates")
			    strLower =  "sendingresdates";

			int i = ConnectBuilderKeywordsLowerCase.IndexOf(strLower);
			return i;
		}



		/// <summary>
		/// Constructor for a new connection string configuration.
		/// </summary>
		private ConnectStringConfig()
		{
		}

		/// <summary>
		/// Construct a new connection string configuration.
		/// </summary>
		/// <param name="capacity"></param>
		private ConnectStringConfig(int capacity) : base(capacity)
		{
		}

		/// <summary>
		/// Construct a new connection string configuration, given an old one.
		/// </summary>
		/// <param name="list"></param>
		public ConnectStringConfig(ConnectStringConfig list) : base(list)
		{
		}

		/// <summary>
		/// Construct a new connection string configuration, given an old one.
		/// </summary>
		/// <param name="list"></param>
		public ConnectStringConfig(IConfig list) : this((ConnectStringConfig)list)
		{
		}

		/// <summary>
		/// Return the value associated with the key.
		/// </summary>
		/// <param name="key"></param>
		/// <returns></returns>
		[CLSCompliant(false)]
		public new String Get (string key)
		{
			if (key == null)
				return null;

			string[] stringArray = GetValues(key);
			if (stringArray == null)
				return null;

			return stringArray[0];
		}

		/// <summary>
		/// Return the original key specified in the connection string.
		/// </summary>
		/// <param name="key"></param>
		/// <returns></returns>
		public String GetKey (string key)
		{
			if (key == null)
				return null;

			string[] stringArray = GetValues(key);
			if (stringArray == null)
				return null;

			if (stringArray.Length >= 2)
				return stringArray[1];  // return original key if present

			return DrvConst.GetKey(key);
		}

		String IConfig.get(string key)
		{
			return this.Get(key);
		}


		/*
		** Name: ParseConnectionString
		**
		** Description:
		**	Parse the connection string.
		**	Format it into a NameValueCollection for easy searching.
		**
		** Input:
		**	connection string
		**
		** Output:
		**	security-sanitized connection string (stripped of passwords)
		**
		** Returns:
		**	NameValueCollection (a collection of key/values pairs).
		**
		** History:
		**	24-Sep-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Parse the connection string keyword/values pairs
		/// into a configuration set.
		/// </summary>
		/// <param name="connStringParm"></param>
		/// <returns></returns>
		static public ConnectStringConfig
			ParseConnectionString(string connStringParm)
		{
			string sanitizedString = "";  // sanitized string to be discarded
			return ParseConnectionString(connStringParm, out sanitizedString, false);
		}

		/// <summary>
		/// Parse the connection string keyword/values pairs
		/// into a configuration set.
		/// </summary>
		/// <param name="connStringParm"></param>
		/// <param name="sanitizedString"></param>
		/// <returns></returns>
		static public ConnectStringConfig
			ParseConnectionString(
				string connStringParm,
				out string sanitizedString)
		{
			return ParseConnectionString(connStringParm, out sanitizedString, false);
		}

		/// <summary>
		/// Parse the connection string keyword/values pairs
		/// into a configuration set.
		/// </summary>
		/// <param name="connStringParm"></param>
		/// <param name="sanitizedString"></param>
		/// <param name="keepPWDKeyword"></param>
		/// <returns></returns>
		static public ConnectStringConfig
			ParseConnectionString(
				string connStringParm,
				out string sanitizedString,
				bool       keepPWDKeyword)
		{
			ConnectStringConfig
				nv = new ConnectStringConfig(10);

			sanitizedString = "";
			if (connStringParm == null  ||  connStringParm.Length == 0)
				return nv;

			bool insideQuoteString = false;
			bool insideParenString = false;
			bool scanningKey       = true;  // T if scanning key, F if value
			bool skippingWhitespace= true;  // drop whitespace before tokens
			char priorChar = Char.MinValue;
			char quoteChar = Char.MinValue; // single or double quote or '\0'
			char c;
			int  trailingSpaceCount = 0;    // number of trailing spaces in token
			System.Text.StringBuilder sbKey = new System.Text.StringBuilder(100);
			System.Text.StringBuilder sbVal = new System.Text.StringBuilder(100);
			System.Text.StringBuilder sbToken = sbKey;
			string strKey = "";
			string strVal = "";
			System.Text.StringBuilder sbSanitized = new System.Text.StringBuilder(100);
			// string to contain connection string without password
			int  sanitizedCount = 0;   // last known good string without a pwd
			int lastSanitizedEqualSign = 0;  // position of "=" in sanitized string
			string connString = String.Concat(connStringParm, ";");
			// guarantee a ";" at the end of the string
			// to force a flush of values to NameValueCollection

			foreach(char cCurrent in connString)
			{
				c = cCurrent;   // make a working and writeable copy of char
				sbSanitized.Append(c);   // build up sanitized string
				if (Char.IsWhiteSpace(c))
				{
					c = ' ';    // normalize whitespace to a blank
					if (skippingWhitespace)  // drop whitespace before tokens
						continue;
				}
				else
					skippingWhitespace = false;  // start or continue scan of token

				if (insideQuoteString)
				{
					// We are inside a quoted string.  If prior char 
					// was a quote char then that prior char was either
					// the terminating quote or the first quote for a
					// quote-quote sequence.  Look at the current char
					// to resolve the context.
					if (priorChar == quoteChar)  // if prior char was a quote char
					{                            // and current char is a quote char
						if (c == quoteChar) // then doubled quotes, append one quote
						{
							sbToken.Append(c);   // append to sbKey or sbVal
							priorChar = Char.MinValue;  // clear prior quote context
							continue;                   // get next character
						}
						else // we had a closing quote in the prior char
						{
							insideQuoteString = false;  // we out of quote string
							// fall through to normal token processing for current char
						}
					}
					else  // ordinary char inside string or possible ending quote
					{
						priorChar = c;      // remember this char as prior char
						if (c == quoteChar) // if ending quote (could be dbl quote)
							continue;       // drop ending quote from string
						                    // add ordinary char to key/value
						sbToken.Append(c);  // append to sbKey or sbVal
						continue;           // get next char
					}
				}

				if (c == '\"'  ||  c == '\'')
				{
					if (scanningKey)
						throw new ArgumentException(
							GcfErr.ERR_GC4203_CONNECTION_STRING_BAD_QUOTE.Msg);
					//"Invalid connection string. Only values, not keywords, "+
					//"may delimited by single or double quotes."
					insideQuoteString = true;   // we're inside the string
					quoteChar = c;
					priorChar = Char.MinValue;
					trailingSpaceCount = 0;
					continue;                   // drop starting from string
				}

				if (insideParenString)
				{
					// We are inside a parenthesised list string.
					sbToken.Append(c);  // append to sbKey or sbVal
					if (c == ')')  // if closing paren
						insideParenString = false;
					continue;           // get next char
				}

				if (c == '(')
					insideParenString = true;

				if (c == '=')
				{
					// Remember the last position of the "=" in sanitized string.
					// We'll use it to make "PWD=*" in the sanitized string.
					lastSanitizedEqualSign = sbSanitized.Length;

					if (scanningKey)           // stop scanning the key and
					{                          // start scanning the value
						scanningKey = false;
						sbToken = sbVal;
					}
					else // "Key = Value =" is a bad format
						throw new ArgumentException(
							GcfErr.ERR_GC4204_CONNECTION_STRING_DUP_EQUAL.Msg);
					//						"Invalid connection string has duplicate '=' character."
					skippingWhitespace= true;  // drop whitespace before tokens
					trailingSpaceCount = 0;
					continue;
				}

				if (c == ';')
				{
					if (sbKey.Length != 0  &&  scanningKey)
						// if key is missing its "=value". throw exception
						throw new ArgumentException(
							String.Format(
							GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
							sbKey, "<null>"));
					//				"Connection string keyword '{0}' has invalid value: '<null>'"

					skippingWhitespace= true;  // reset for new token
					scanningKey       = true;  // reset for scanning the next key
					sbToken = sbKey;

					strKey = sbKey.ToString().TrimEnd(null);
					strVal = sbVal.ToString(0, sbVal.Length - trailingSpaceCount);
					if (strKey.Length == 0)
						if (strVal.Length == 0)
							continue;
						else  // if key is missing but value is there then error
							throw new ArgumentException(
								GcfErr.ERR_GC4205_CONNECTION_STRING_MISSING_KEY.Msg);
					//							"Invalid connection string has " +
					//							"missing keyword before '=' character."

					string strKeyOriginal = strKey;  // save the user's original keyword
					// so we can reconstruct the original connection string if needed

					string strKeyUpper = ToInvariantUpper(strKey);
					string strValLower = ToInvariantLower(strVal);
					switch (strKeyUpper)  // allow synonyms
					{
						case "HOST":
						case "SERVER":
						case "ADDRESS":
						case "ADDR":
						case "NETWORKADDRESS":
							strKey = DrvConst.DRV_PROP_HOST;
							break;
						case "PORT":
							strKey = DrvConst.DRV_PROP_PORT;
							break;
						case "USERID":
						case "UID":
							strKey = DrvConst.DRV_PROP_USR;
							break;
						case "PWD":
						case "PASSWORD":
							strKey = DrvConst.DRV_PROP_PWD;
							if (keepPWDKeyword)  // keep the keyword for those who trace
							{
								// change "pwd=xxx;" to "pwd=*;"
								sbSanitized.Length=lastSanitizedEqualSign;
								sbSanitized.Append("*;");
							}
							else
								// chop "pwd=xxx;"
								sbSanitized.Length = sanitizedCount;
							break;
						case "INITIALCATALOG":
						case "DB":
						case "DATABASE":
							strKey = DrvConst.DRV_PROP_DB;
							break;
						case "ROLEID":
							strKey = DrvConst.DRV_PROP_ROLE;
							break;
						case "ROLEPWD":
						case "ROLEPASSWORD":
							strKey = DrvConst.DRV_PROP_ROLEPWD;
							if (keepPWDKeyword)  // keep the keyword for those who trace
							{
								// change "rolepwd=xxx;" to "rolepwd=*;"
								sbSanitized.Length=lastSanitizedEqualSign;
								sbSanitized.Append("*;");
							}
							else
								// chop "rolepwd=xxx;"
								sbSanitized.Length = sanitizedCount;
							break;
						case "DBMS_USER":
						case "DBMSUSER":
						case "DBMSUSERID":
							strKey = DrvConst.DRV_PROP_DBUSR;
							break;
						case "DBMS_PWD":
						case "DBMS_PASSWORD":
						case "DBMSPWD":
						case "DBMSPASSWORD":
							strKey = DrvConst.DRV_PROP_DBPWD;
							if (keepPWDKeyword)  // keep the keyword for those who trace
							{
								// change "dbmspwd=xxx;" to "dbmspwd=*;"
								sbSanitized.Length = lastSanitizedEqualSign;
								sbSanitized.Append("*;");
							}
							else
								// chop "dbmspwd=xxx;"
								sbSanitized.Length = sanitizedCount;
							break;
						case "CHARACTER_ENCODING":
						case "CHAR_ENCODING":
						case "CHARACTERENCODING":
						case "CHARENCODING":
							strKey = DrvConst.DRV_PROP_ENCODE;
							System.Text.Encoding encoding =
									CharSet.getEncoding(strVal);
							if (encoding == null)  // unknown .NET encodingName
								throw new ArgumentException(
									String.Format(
									GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
									strKey, strVal));
							break;
						case "GROUPID":
							strKey = DrvConst.DRV_PROP_GRP;
							break;
						case "TZ":
						case "TIMEZONE":
							strKey = DrvConst.DRV_PROP_TIMEZONE;
							break;
						case "DECIMAL":
						case "DECIMAL_CHAR":
						case "DECIMALCHAR":
							strKey = DrvConst.DRV_PROP_DEC_CHAR;
							break;
						case "DATE_FMT":
						case "DATE_FORMAT":
						case "DATEFORMAT":
							strKey = DrvConst.DRV_PROP_DATE_FRMT;
							break;
						case "MNY_FMT":
						case "MONEY_FORMAT":
						case "MONEYFORMAT":
							strKey = DrvConst.DRV_PROP_MNY_FRMT;
							break;
						case "MNY_PREC":
						case "MONEY_PRECISION":
						case "MONEYPRECISION":
							strKey = DrvConst.DRV_PROP_MNY_PREC;
							break;
						case "BLANKDATE":
							strKey = DrvConst.DRV_PROP_BLANKDATE;
							if (strValLower == "null")
								break;
							throw new ArgumentException(
								String.Format(
								GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
								strKey, strVal));
						case "TRACE":
							strKey = DrvConst.DRV_PROP_TRACE;
							break;
						case "TRACELEVEL":
							strKey = DrvConst.DRV_PROP_TRACELVL;
							goto case " Common check for good number";
						case "CONNECTTIMEOUT":
						case "CONNECTIONTIMEOUT":
							strKey = DrvConst.DRV_PROP_TIMEOUT;
							goto case " Common check for good number";
						case "MINPOOLSIZE":
							strKey = DrvConst.DRV_PROP_MINPOOLSIZE;
							goto case " Common check for good number";

						case "MAXPOOLSIZE":
							strKey = DrvConst.DRV_PROP_MAXPOOLSIZE;
							goto case " Common check for good number";

						case " Common check for good number":
							bool isGoodNumber = strVal.Length > 0;  // should be true
							foreach(char numc in strVal)
								isGoodNumber &= Char.IsDigit(numc); // should stay true

							if (isGoodNumber)
							{
								try
								{Int32.Parse(strVal);} // test convertion
								catch (OverflowException)
								{isGoodNumber = false;}
							}

							if (!isGoodNumber)
								throw new ArgumentException(
									String.Format(
									GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
									strKey, strVal));
							//"Connection string keyword '{0}' has invalid value: '{1}'"
							break;

						case "VNODE_USAGE":
						case "VNODEUSAGE":
							strKey = DrvConst.DRV_PROP_VNODE;
							if (strValLower == "connect"  ||
								strValLower == "login")
								break;
							throw new ArgumentException(
								String.Format(
								GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
								strKey, strVal));

						case "CURSOR_MODE":
						case "CURSORMODE":
							strKey = DrvConst.DRV_PROP_CRSR;
							if (strValLower == "dbms" ||
								strValLower == "readonly" ||
								strValLower == "update")
								break;
							throw new ArgumentException(
								String.Format(
								GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
								strKey, strVal));

						case "ENLIST":
							strKey = DrvConst.DRV_PROP_ENLISTDTC;
							goto case " Common Boolean Test Code";

						case "POOLING":
							strKey = DrvConst.DRV_PROP_CLIENTPOOL;
							goto case " Common Boolean Test Code";

						case "PERSISTSECURITYINFO":
							strKey = DrvConst.DRV_PROP_PERSISTSEC;
							goto case " Common Boolean Test Code";

						case "SENDINGRESDATES":
						case "SEND_INGRES_DATES":
							strKey = DrvConst.DRV_PROP_SENDINGRESDATES;
							goto case " Common Boolean Test Code";

						case " Common Boolean Test Code":
							if (strValLower == "true"  ||  strValLower == "yes")
								strVal = "true";
							else
								if  (strValLower == "false"  ||  strValLower == "no")
								strVal = "false";
							else
								throw new ArgumentException(
									String.Format(
									GcfErr.ERR_GC4202_CONNECTION_STRING_BAD_VALUE.Msg,
									strKey, strVal));
							//							"Connection string keyword '{0}' has invalid value: '{1}'"
							break;
						default:
							throw new ArgumentException(
								String.Format(
								GcfErr.ERR_GC4201_CONNECTION_STRING_BAD_KEY.Msg, strKey));
							//								Unknown connection string keyword: '{0}'
					}

					nv.Set(strKey, strVal);
					nv.Add(strKey, strKeyOriginal);
					sbKey.Length = 0;
					sbVal.Length = 0;
					trailingSpaceCount = 0;
					sanitizedCount = sbSanitized.Length;
					continue;
				}

				if (Char.IsWhiteSpace(c)) // track trailing spaces (outside of quotes)
					trailingSpaceCount++;
				else
					trailingSpaceCount = 0;

				if (scanningKey  &&  Char.IsWhiteSpace(c))
					continue;  // blank chars inside a key are ignored
				sbToken.Append(c);   // append to sbKey or sbVal

			} // end foreach(char cCurrent in connString)

			if (insideQuoteString)  // if unmatched quote. throw exception
				throw new ArgumentException(
					GcfErr.ERR_GC4206_CONNECTION_STRING_MISSING_QUOTE.Msg);
			//				"Invalid connection string has unmatched quote character."

			if (insideParenString)  // if unmatched parentheses. throw exception
				throw new ArgumentException(
					GcfErr.ERR_GC4207_CONNECTION_STRING_MISSING_PAREN.Msg);
			//				"Invalid connection string has unmatched paren character."

			if (sbSanitized.Length > 0)
				sbSanitized.Length--;    // strip the extra semicolon we added
			sanitizedString = sbSanitized.ToString();  // return the sanitzied string

			return nv;  // return the NameValueCollection
		}  // ParseConnectionString



		/*
		** Name: ToConnectionString
		**
		** Description:
		**	Build the connection string fron a NameValueCollection.
		**
		** Input:
		**	NameValueCollection (a collection of key/values pairs).
		**
		** Output:
		**	Connection string (stripped of passwords)
		**
		** Returns:
		**	NameValueCollection (a collection of key/values pairs).
		**
		** History:
		**	18-Apr-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Return the connection string configuration as a connection string.
		/// </summary>
		/// <param name="nv"></param>
		/// <returns></returns>
		static public string ToConnectionString(
			ConnectStringConfig nv)
		{
			if (nv == null  ||  nv.Count == 0)
				return String.Empty;

			System.Text.StringBuilder sb = new System.Text.StringBuilder(100);

			string [] specialsKeys =
				{
					DrvConst.DRV_PROP_HOST,      // Host
					DrvConst.DRV_PROP_PORT,      // Port
					DrvConst.DRV_PROP_DB,        // Database
					DrvConst.DRV_PROP_USR,       // User ID
					DrvConst.DRV_PROP_PWD        // Password
				};

			foreach (string strKey in specialsKeys) // emit special keys
			{	// put the special keywords first
				ToConnectionStringNameValue(sb, nv, strKey, false);
			}  // end foreach

			foreach (string strKey in nv)           // emit the rest of the keys
			{	// skip over the special keywords already put first or ignored
				if (Array.IndexOf(specialsKeys, strKey) != -1)
					continue;
				ToConnectionStringNameValue(sb, nv, strKey, false);
			}  // end foreach

			return sb.ToString();
		}  // ToConnectionString


		/*
		** Name: ToNormalizedConnectionString
		**
		** Description:
		**	Build the connection string fron a NameValueCollection with 
		 *  standard ConnectionStringBuilder keywords.
		**
		** Input:
		**	NameValueCollection (a collection of key/values pairs).
		**
		** Returns:
		**	Connection string including passwords
		**
		** History:
		**	12-Aug-05 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Return the connection string configuration as a connection string
		/// using ConnectionStringBuilder normalized keywords.
		/// </summary>
		/// <param name="nv"></param>
		/// <returns></returns>
		static internal string ToNormalizedConnectionString(
			ConnectStringConfig nv)
		{
			if (nv == null || nv.Count == 0)
				return String.Empty;

			System.Text.StringBuilder sb = new System.Text.StringBuilder(100);

			string[] specialsKeys =
				{
					DrvConst.DRV_PROP_HOST,      // Host
					DrvConst.DRV_PROP_PORT,      // Port
					DrvConst.DRV_PROP_DB,        // Database
					DrvConst.DRV_PROP_USR,       // User ID
					DrvConst.DRV_PROP_PWD        // Password
				};

			foreach (string strKey in specialsKeys) // emit special keys
			{	// put the special keywords first
				ToConnectionStringNameValue(sb, nv, strKey, true);
			}  // end foreach

			foreach (string strKey in nv)           // emit the rest of the keys
			{	// skip over the special keywords already put first or ignored
				if (Array.IndexOf(specialsKeys, strKey) != -1)
					continue;
				ToConnectionStringNameValue(sb, nv, strKey, true);
			}  // end foreach

			return sb.ToString();
		}  // ToNormalizedConnectionString


		/*
		** Name: ToConnectionStringNameValue
		**
		** Description:
		**	Build the connection string fron a NameValueCollection.
		**
		** Input:
		**	NameValueCollection (a collection of key/values pairs).
		**
		** Output:
		**	Connection string (stripped of passwords)
		**
		** Returns:
		**	NameValueCollection (a collection of key/values pairs).
		**
		** History:
		**	18-Apr-03 (thoda04)
		**	    Created.
		*/
	
		static private void ToConnectionStringNameValue(
			System.Text.StringBuilder sb,
			ConnectStringConfig nv,
			string strKey,
			bool normalize)
		{
			string strValue       = nv.Get(   strKey);  // value
			string strKeyOriginal = nv.GetKey(strKey);  // user-specified key synonym
			if (strValue == null  ||  strValue.Length == 0)
				return;

			if (sb.Length !=0)  // if 2nd or subsequent item, insert ';'
				sb.Append(';');
			if (normalize)  // set key if normalize to standard keywords
				strKeyOriginal = (String)NormalizedUserKeyword[strKey];
			sb.Append(strKeyOriginal);
			sb.Append('=');
			sb.Append(strValue);
		}


		/*
		** Name: General String Helper Routines
		**
		** Description:
		**	General internal string routines for lower, upper, compare, etc.
		**
		** History:
		**	24-Sep-02 (thoda04)
		**	    Created.
		*/
	
		private static string ToInvariantLower(string str)
		{
			return str.ToLower(System.Globalization.CultureInfo.InvariantCulture);
		}

		private static string ToInvariantUpper(string str)
		{
			return str.ToUpper(System.Globalization.CultureInfo.InvariantCulture);
		}

	}  // ConnectStringConfig
}