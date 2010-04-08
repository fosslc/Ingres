<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/*
 * Name:	DBConnection.php
 *
 * Description:	this class encapsulates the access to the Database. It provides several methods
 *		that just call the related Ingres PHP driver commands.
 *
 * Private:	$Connection	- 	class Variable for saving the link id which is the result of
 *					ingres_connect
 *
 * Methods:	__construct
 *		__destruct
 *		query
 *		getResultArray
 *		getResultObjects
 *		commit
 *		rollback
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 *		15-Nov-2007 (michael.lueck@ingres.com)
 *	        if user specifies "(local)" for the hostname he likes to connect to, the specified username and password
 *		will be used instead of the default values defined in LocalSettings.php
 *		if any of the values $_SESSION['username'], $_SESSION['password'] or $_SESSION['database'] is not set
 *		the default values of LocalSettings.php will be used to connect to the local Ingres instance	
 *		20-Mar-2009 (grant.croker@ingres.com)
 *		Bug 121831 -  Update for PECL ingres 2.x
 *			Add $Result attribute to manage result resource from ingres_query()
 *			Disable ingres.utf8, ingres.scrollable
 *			Add mandatory parameters
 *
 */
require_once("LocalSettings.php");
define('DBCONNECTION',1);
	class DBConnection
	{
		private $Connection;
		private $Result;

		/* Name: 	__construct
		 *
 		 * Description: Constructor. Composes dynamic vnode string from SESSION Variables if set.
		 *		Sets up connection to the server and saves link id in $Connection
		 * Inputs:	None
		 *	
		 * Outputs:	None
		 *
		 * returns: 	Instance of DBConnection
		 *
		 * Exception:	Exception with code 4 - message contains connection string and error message from server
		 */
		function __construct()
		{
			/* disable scrollable result sets since they are not compatible with lobs */
			ini_set("ingres.scrollable", false);
			/* the demoapp uses iconv to convert UTF-16<-->UTF-8
			 * TODO : remove this dependence 
			 */
			ini_set("ingres.utf8", false);
			$connectString ="";
			@session_start();
			if(isset($_SESSION['host']) && $_SESSION['host'] != "(local)")
			{
			//make connection string with dynamic vnode syntax:
			// @hostname,protocol,InstanceName[user,password]::database
			// for example @127.0.0.1,tcp_ip,II[apache,123]::demodb	
				$connectString = "@";
				$connectString .= $_SESSION['host'].",";
				$connectString .= "tcp_ip,";
				$connectString .= $_SESSION['instance'];
				$connectString .= "[".$_SESSION['user'].",".$_SESSION['password']."]";
				$connectString .= "::".$_SESSION['database'];

				$this->Connection = ingres_connect($connectString,"","");
			} else {
				//just connect to a local Ingres Instance
				if(isset($_SESSION['user']) && isset($_SESSION['password']) && isset($_SESSION['database']))
				{
					$this->Connection = ingres_connect($_SESSION['database'],$_SESSION['user'],$_SESSION['password']);
				} else {
					$this->Connection = ingres_connect(DATABASE,USER,PASSWORD);
				}
			}
			
			if(ingres_errno($this->Connection) != 0) 
			{
				throw new Exception("Error occured: Errno".ingres_errno()." Error: ".ingres_error().
							"connection-String:".$connectString,4);
			}
		}
	
		/*
 		 * Name:	query
		 *
		 * Description: takes sql string, parameter array and parameter type description
		 *		and sends query on link saved in $Connection
		 *
		 * Inputs:	$sql		  -	SQL-Query string. Could contain "?" if dynamic sql statement is used
		 *		$param (optional) -     if "?" are given in $sql, than params must contain the values for that "?"
		 *					order of the parameters is important because for the first "?" the first array
		 *					element is examined, for the secon "?" the second array element is examined and so on
		 *		$sqltypes	  - 	there are only a few php types that match Ingres types. So if the value of an 
		 *					PHP variable should be inserted in the database (for example) than $sqltypes must be
		 *					give the type of the column where the variable should be inserted				       *
		 * Returns: 	None	
		 *
		 * Exceptions:  Exception with code 3  - 	if query couldn't be executed.Message contains query and error messages
		 */
		function query($sql, $params = array(), $sqltypes = "")
		{
			/*After a query is send, its result must be fetched BEFORE another query is executed
			 *If not the result of the first query is destroyed by the second query
			 */

			$this->Result = ingres_query($this->Connection,$sql,$params,$sqltypes);
			if($this->Result) return;
			else 
			{ 
			   throw new Exception("Error while sending query:".
						$sql."<br/>Errno: ".ingres_errno()."<br/>Error: ".ingres_error(), 3);
			}
		}
		
		/*
		 * Name:	getResultArray
		 *
		 * Description:	calls ingres_fetch_array at gets ALL results for the last query. The results are appended to
		 *		the array returned by the method. So getResultArray returns a two dimensional array.
		 *
		 * Inputs:	$associative	-	if true the every result row is fetched only as an associative array
		 *					default is false, so with default every row can accessed through int index
		 *					or assciative index
		 * Output	None
		 *
		 * returns	two dimensional array	-	every element of that array is an result row of the last query
		 *
		 */

		function getResultArray($associative =false)
		{	
			$resultArray = array();
			if($associative)
			{
				while($row = ingres_fetch_array($this->Result, INGRES_ASSOC))
				{
					$resultArray[] = $row;
				}
			} else {
				while($row = ingres_fetch_array($this->Result))
				{
					$resultArray[] = $row;
				}
			}
			return $resultArray;
		}
		/*
		 * Name:	getResultObjects
		 *
		 * Description: fetches ALL results of the last query as php objects. For each result row an object is appended
		 *		to an array which is returned by the method
		 *
		 * Inputs:	None
		 *
		 * Returns:	array of objects - every element of this array is an object. The Attributes of this objects are named
		 *		after the selected columns. See ProfileModel::getProfileByEmail for example 
		 *		
		 */
		function getResultObjects()
		{
			$resultArray = array();
			while($obj = ingres_fetch_object($this->Result))
			{
				$resultArray[] = $obj;
			}
			return $resultArray;
		}
		
		/*
		 * Name:	commit
		 *
		 * Description:	just calls ingres_commit();
		 *
		 * inputs: 	None
		 *
		 * outputs: 	None
		 *
		 * returns:	None
		 */	
		function commit()
		{
			ingres_commit($this->Connection);
		}

		/*
		 * Name: 	rollback
		 *
		 * Description:	just calls ingres_rollback
		 *
		 * inputs:	None
		 *
		 * outputs:	None
		 *
		 * returns:	None
		 */
		function rollback()
		{
			ingres_rollback($this->Connection);
		}
	
		/*
		 * Name:	__destruct
		 *
		 * Description:	Destructor. Closes Connection to server
		 *
		 * inputs:	None
		 *
		 * outputs:	None
		 *
		 * returns: 	None
		 */	
		function __destruct()
		{
			ingres_close($this->Connection);
		}
	}
?>
