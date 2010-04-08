<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/* Name:	profile.php	
 *
 * Description: - HTML Template code for profile View. This contains calls to xajax functions and other 
 *		  GUI related code
 *		- evaluation of form data after submit. Needs to be done here because xajax 0.24 can't handle
 *		  file uploads properly. If future versions of xajax can handle this, this code could be moved to
 *		  profile.ajax.php
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created.
 *              26-nov-2007 (peeje01)
 *                  Bug: 117733
 *                  Change $PHP_SELF to $_SERVER['PHP_SELF'] following the
 *                  note at:
 *                      http://us3.php.net/manual/en/security.globals.php
 */

	require_once("models/ProfileModel.php");
	global $errorMsg;
	foreach($_POST as $key => $value)
	{
		$_POST[$key] = htmlentities(strip_tags($value),ENT_QUOTES,"UTF-8");
	}
	if(isset($_POST['addProfile']) || isset($_POST['changeProfile']))
	{
		//check mandatory input
		if(	trim($_POST['lastname']) 	  	== "" 		||
			trim($_POST['firstname']) 	  	== "" 		||
			trim($_POST['mailaddressText']) 	== ""		||
			$_POST['home_country_select']	 	== "invalid" 	||
			$_POST['home_region_select']	  	== "invalid" 	||
			$_POST['home_apcode_select']	  	== "invalid" 
		  )
		{	
			echo "Please complete your credentials";
		} else {
		//Insert Data
			
			//create array with data	
			$credits = array(	'up_first' => $_POST['firstname'], 
						'up_last' => $_POST['lastname'],
						'up_email' => $_POST['mailaddressText'],
						'up_airport'  => $_POST['home_apcode_select']);
			
			//save name of uploaded file in array	
			if(isset($_FILES['picture']) && $_FILES['picture']['error'] != 4)
			{
				$credits['up_image'] = $_FILES['picture']['tmp_name'];
			}
			//if no picture was uploaded set filename in array to empty string
			else
			{	
				if(isset($_POST['addProfile']))
					$credits['up_image'] = "";
			}
			//Variable to check wheter Profile has to be default profile	
			$setDefault = false;	
			if(isset($_POST['setAsDefault']))
					$setDefault = true;
			
			//Create new instance of the ProfileModel
			$pm = new ProfileModel();
			//Insert or update?
			if(isset($_POST['addProfile']))
			{
				try{
					//call addNew method of ProfileModel and pass the credits of the profile to create
					$pm->addNew($credits);
				}catch(Exception $e) {
					//if exception of type 2, 5 or six is catched, print the error message in 
					//contentView div in index.php else re-throw exception
					if ($e->getCode() == 2 || $e->getCode() == 5 || $e->getCode() == 6) {
						$errorMsg = "<br/>".$e->getMessage()."<br/>";	
					}
					else throw $e;
				}
			}
			else
			{	
				try
				{
					//same as for addNew but only for Exeptions of type 5
					$pm->update($credits);
				} catch (Exception $e) {
					if($e->getCode() == 5 || $e->getCode() == 6) {
						$errorMsg = "<br/>".$e->getMessage()."<br/>";
					}
					else throw $e;
				}
			}
			//if profile is wished to be default set Session and cookie variables appropriate
			if($setDefault)
			{
				@session_start();
				$_SESSION['defaultProfile'] = $credits['up_email'];
				setcookie("defaultProfile",$credits['up_email'],time()+60*60*24*7*2,
					   dirname($_SERVER['PHP_SELF']),$_SERVER['SERVER_NAME']);
			}
		}
			
	}
?>
<!--
	rest ist HTML Template for profile View
//-->
<script type="text/javascript">
<!--
	function setNewProfileMask()
	{
		document.getElementById('mailaddressText').disabled=false;
		document.getElementById('lastname').disabled=false;
		document.getElementById('firstname').disabled=false;
		document.getElementById('picture').disabled=false;
		document.getElementById('setAsDefault').disabled=false;
		document.getElementById('addProfile').disabled=false;
		document.getElementById('lastname').value="";
		document.getElementById('firstname').value="";
		document.getElementById('home_region_select').disabled=true;
		document.getElementById('home_apcode_select').disabled=true;
		xajax_loadValuesForNewProfile();
		xajax_showNextHelp('profile_add.htm');
	}
	function setModifyMask()
	{
		document.getElementById('lastname').disabled=false;
		document.getElementById('lastname').value="";
		document.getElementById('firstname').disabled=false;
		document.getElementById('firstname').value="";
		document.getElementById('picture').disabled=false;
		document.getElementById('picture').value="";
		document.getElementById('removeProfile').disabled=false;
		document.getElementById('changeProfile').disabled=false;
		document.getElementById('setAsDefault').disabled=false;
		xajax_loadEmail();
		xajax_showNextHelp('email_load.htm');
	}
	function deleteProfile()
	{
		check=confirm("Are you sure you want to delete the selected profile?");
		if(check == true)
		{
			xajax_deleteProfile(xajax.getFormValues('profile_form'));
		}
		xajax_showNextHelp('profile_remove.htm');
	}
//-->
</script>

<form enctype="multipart/form-data" method="post" action="<?php echo htmlentities($_SERVER['PHP_SELF'])?>?feature=profile" 
      accept-charset="UTF-8,ISO-8859-1" name="profile_form" id="profile_form">
	<table style="text-align: left; width: 100%;" border="0" cellpadding="0" cellspacing="0">
	<tbody>
		<tr>
			<td class="topField" colspan="2" rowspan="1">
				<h4>Personal Details</h4>
			</td>
			<td>&nbsp;</td>
			<td class="topField" colspan="2" rowspan="1">
				<h4>Preferences</h4>
			</td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td class="leftField">
				Email Address
			</td>
			<td class="rightField">
				<div id="mailaddress">
					<!--TODO: 	text field input must be replaced by an select-filed after click on "Modify". 
							Needs to be done in PHP after AJAX Call that means the innerHTML property 
							would be set to "<select ..." 
					//-->
					<input tabindex="1" name='mailaddressText' 
						id='mailaddressText' type='text' size='40' maxlength='40' disabled="disabled">
				</div>
			</td>
			<td>&nbsp;</td>
			<td class="rowField"colspan="2" rowspan="1">
				Home Airport
			</td>
			<td style="padding-left:10px" colspan="2" rowspan="1">
				<input tabindex="100" type="button" name="New" id="new" value="New" 
					onClick="setNewProfileMask();">
			</td>
		</tr>
		<tr>
			<td class="leftField">
				Last Name
			</td>
			<td class="rightField">
				<input tabindex="2" id="lastname" name="lastname" size="30" maxlength="30" type="text" disabled="disabled">
			</td>
			<td>&nbsp;</td>
			<td class="rowField" colspan="2" rowspan="1" style="width: 117px;">
				Country;
				<div id="home_country">
				<select tabindex="5" id="home_country_select" name="home_country_select" disabled="disabled">
				</select>
				</div>
			</td>
			<td style="padding-left:10px" colspan="2" rowspan="1">
				<input tabindex="101" type="button" value="Modify" name="modifyProfile" onClick="setModifyMask();">
			</td>
		</tr>
		<tr>
			<td class="leftField">
				First Name
			</td>
			<td class="rightField">
				<input tabindex="3" id="firstname" name="firstname" size="30" maxlength="30" type="text" disabled="disabled">
			</td>
			<td>&nbsp;</td>
			<td class="leftField">
				Region
			</td>
			<td class="rightField">
				Airport Code
			</td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td class="leftField">
				Photograph
			</td>
			<td class="rightField">
				<input tabindex="4" id="picture" name="picture" accept="image/*" size="30" type="file" disabled="disabled">
			</td>
			<td>&nbsp;</td>
			<td class="leftField">
				<div id="home_region">
				<select tabindex="6" id="home_region_select" name="home_region_select" disabled="disabled">
				</select>
				</div>
			</td>
			<td class="rightField">
				<div id="home_apcode">
				<select tabindex="7" id="home_apcode_select" name="home_ap_code_select" disabled="disabled">
				</select>
				</div>
			</td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td class="bottomField" colspan="2">&nbsp;</td>
			<td>&nbsp;</td>
			<td class="bottomField" colspan="2">&nbsp;</td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td colspan="2">
				<input tabindex="8" id="setAsDefault" name="setAsDefault" type="checkbox" disabled="disabled">
					Default Profile
				</td>
			<td>&nbsp;</td>
			<td>&nbsp;</td>
			<td>&nbsp;</td>
			<td>&nbsp;</td>
		</tr>
		<tr>
			<td>&nbsp;</td>
			<td style="text-align:right;" colspan="4">
				<button tabindex="9" id="addProfile" type="submit" value="Add" name="addProfile" 
				       disabled="disabled">Add</button>
				<button tabindex="10" id="removeProfile" type="button" value="Remove" name="removeProfile" 
				       disabled="disabled" onClick="deleteProfile();">Remove</button>
				<button tabindex="11" id="changeProfile" type="submit" value="Change" name="changeProfile" 
			               disabled="disabled" >Change</button>
			</td>
			<td>&nbsp;</td>
		</tr>
	</tbody>
	</table>
</form>
