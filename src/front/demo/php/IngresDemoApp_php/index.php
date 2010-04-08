<?php	
/*
 * Copright (c) 2007 Ingres Corporation
 */

/* Name: index.php
 *
 * Description: index page of "Ingres Frequent Flyer", the Ingres Demo
 *		application.
 *		- loads values from cookies into session variable
 *		- sets up the xajax environment which is used for GUI of the application
 *		- include further php files based on GET Parametersi
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 */
	//error_reporting(0);
  	//define variable for errorMsg which will be printed within the table if not empty
	require("LocalSettings.php");
	$old = ini_set("ingres.array_index_start","0");
/*	if($old === false)
		die("can't set ingres.array_index_start to value 0. Please adjust your php.ini by hand");*/
	global $errorMsg;
	$errorMsg = "";

	session_start();
	
	//Calculate timestamp in two weeks
	$twoWeeks = time() + (60*60*24*7*2);
	
	//put Cookie values for default profile into session variables
	if(isset($_COOKIE['defaultProfile']) && !isset($_SESSION['defaultProfile']))
		$_SESSION['defaultProfile'] = $_COOKIE['defaultProfile'];

	//if the birth cookie (which is set with the defaultProfile cookie
	//expires the next day than we set it's expire date to two weeks later
	if(isset($_COOKIE['birth'])) 
	{
		if(time() - $_COOKIE['birth'] < (60*60*24))
		{
			foreach($_COOKIE as $cookie => $value)
			{	
				setcookie($cookie,$value,$twoWeeks,dirname($_SERVER['PHP_SELF']),$_SERVER['SERVER_NAME']);
			}
		}
	}
	
	//XAJAX related code
	//See www.xajaxproject.org for more info on xajax and it's features
	require_once("xajax.inc.php");
	global $xajax;
	//create new xajax object und set RequestURI to exactly this site. 
	//This is very important to prevent XSS atacks via GET parameters that contain javascript code
	$xajax = new xajax("./index.php");
	//$xajax->debugOn();
	//include code for ajax functions on all country, region and Airport Code selects
	require_once("ajax/airport.ajax.php");

	//include ajax code required for routes,profile and connect GUI
	include "ajax/routes.ajax.php";
	include "ajax/profile.ajax.php";
	include "ajax/connect.ajax.php";
	include "ajax/help.ajax.php";
	
	//all ajax calls process the code until this point.  processRequests calls the appropriate functions
	//(included from routes.ajax.php,...) 
	$xajax->processRequests();
?>
<!--
	Now the HTML template for index.php
//-->
<html>
<head>
<title>Ingres Frequent Flyer</title>
<link rel="stylesheet" type="text/css" href="iff.css">
<?php
	//let xajax print it's javascript code in the index.php
	$xajax->printJavascript();
?>
</head>
<body>
<center>
<table id="mainTable" style="text-align: left; width:732px;" border="0" cellpadding="0" cellspacing="0">
<tbody>
	<tr>
		<td class="topField" colspan="2" rowspan="1">
			<img alt="" src="pictures/IngresBanner.jpg"></img> 
		</td>
	</tr>
	<tr>
		<td  class="leftField" valign="top" width="100"> 
			<div class="navi">
			<a href="index.php?feature=routes"><img alt="" src="pictures/RoutesButton.jpg" border="0"/></a>
			<a href="index.php?feature=profile"><img alt="" src="pictures/ProfileButton.jpg" border="0"></a>
			<a href="index.php?feature=connect"><img alt="" src="pictures/ConnectButton.jpg" border="0"></a>
			<a href="" onClick="self.close();" ><img  alt="" src="pictures/ExitButton.jpg" border="0"></a>
			</div>
		</td>
		<td class="rightField"  style="text-align: left; vertical-align: top;">
			<table width="100%" style="text-align: left;" border="0" cellpadding="0" cellspacing="0">
			<tbody>
			<tr class="forms">
				<td id="tab" width="10%">
					<?php
						if(isset($_GET['feature']))
						{
							switch($_GET['feature']) {
								case "profile":
									echo "Profile";
									break;
								case "routes":
									echo "Routes";
									break;
								case "connect":
									echo "Connect";
									break;
							}
						} else {
							echo "Routes";
						}
					?>
				</td>
				<td style="border-bottom-width:1px;border-bottom-style:solid;border-color:black;
					   background-color:#FFFDDB" width="90%">&nbsp;</td>
			</tr>
			<tr class="forms">
				<td class="bottomField" colspan="2" style="vertical-align: top; padding-top:20px; 
						padding-left:20px; padding-right:20px; padding-bottom:20px;">	
				<?php 
					if(isset($_GET['feature'])) 
					{ 
						switch($_GET['feature']) 
						{ 
							case "profile": 
								include "profile.php";
								break; 
							case "connect":
								include "connect.php"; 
								break; 
							case "routes": 
								include "routesCriteria.php"; 
								break; 
						} 
					} else { 
						include "routesCriteria.php"; 
					}
				?>&nbsp;
				</td>
			</tr>
			<tr>
				<td>&nbsp;</td>	
				<td>&nbsp;</td>	
			</tr>
			<tr>
				<td colspan="2">
				<div id="contentView">
					<?php
					if(isset($_GET['feature']) && $errorMsg == "") 
					{ 
						switch($_GET['feature']) 
						{ 
						case "profile": 
							echo "<iframe src=\"help/dev_profile.htm\" width=\"100%\"></iframe>";
							break; 
						case "connect":
							echo "<iframe src=\"help/dev_connect.htm\"></iframe>"; 
							break; 
						case "routes": 
							echo "<iframe src=\"help/dev_routes.htm\"></iframe>"; 
							break; 
						} 
					} elseif($errorMsg == "") { 
						echo  "<iframe src=\"help/welcome.htm\"></iframe>"; 
					} else {
						echo $errorMsg;
					}
					?>
				</div>
				</td>
			</tr>
			</tbody>
			</table>
		</td> 
	</tr>
	<tr>
		<td class="bottomField" colspan="2">&nbsp;</td>
	</tr>
</tbody>
</table>
</center>
</body>
</html>
