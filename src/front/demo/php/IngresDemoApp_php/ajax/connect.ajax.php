<?
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/*
 * Name:	connect.ajax.php	
 *
 * Description: contains xajax function changeConnectionProfile which is called after click on "Change" button
 *		in connect.php. The function reads the form data and set's the Session variables appropriate.
 *		Those session variables are composed to an dynanic vnode in DBConnection.php.
 *		The Function also sends cookies to the client, if the user has checked "Make Connection default"
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 *		19-Feb-2007 (michael.lueck@ingres.com)
 *		Changed logic for mandatory value check
 *
 */
	
	global $xajax;
	$xajax->registerFunction("changeConnectionProfile");

	function changeConnectionProfile($aFormValues)
	{
		foreach( $aFormValues as $key => $value)
			$aFormValues[$key] = htmlentities(strip_tags($value),ENT_QUOTES,"UTF-8");
		$res = new xajaxResponse();
		$invalidData = false;
		$reason ="";
		if(isset($aFormValues['hostname'])) {
			if($aFormValues['hostname'] != "(local)") {
				if(!isset($aFormValues['userID']) || $aFormValues['userID'] == "") $invalidData = true;
				elseif(!isset($aFormValues['password']) || $aFormValues['password'] == "") {$invalidData = true;}
			}	
			if(!isset($aFormValues['instance']) || $aFormValues['instance'] == "") $invalidData = true; 
			elseif(!isset($aFormValues['dbname']) || $aFormValues['dbname'] == "") {$invalidData = true;}
		}
			
		if($invalidData)
		{
			$res->addAssign("contentView","innerHTML","Missing connection information. Please provide values for all inputs");
			return $res;
		}
		
		@session_start();
		$_SESSION['host'] = $aFormValues['hostname'];
		$_SESSION['instance'] = $aFormValues['instance'];
		$_SESSION['user'] = $aFormValues['userID'];
		$_SESSION['password'] = $aFormValues['password'];
		$_SESSION['database'] = $aFormValues['dbname'];
		
		//after setting the connection to a new host, we need to delete the default profile (SESSION and COOKIE)
		//because it possibly won't be exist on the new database.
		unset($_SESSION['defaultProfile']);
		if(isset($_COOKIE['defaultProfile']))
			$ret = setcookie("defaultProfile","",time(),dirname($_SERVER['PHP_SELF']),$_SERVER['SERVER_NAME']);
		$res->addAssign("contentView","innerHTML","Changed Connection information");

		return $res;

			
	}
?>
