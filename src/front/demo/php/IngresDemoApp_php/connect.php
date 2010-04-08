<!--
	Copyright (c) 2007 Ingres Corporation
//-->

<!--
	Name: 		profile.php
	Description: 	HTML Template for connect View. Contains calls to xajax functions
			and other GUI related code
	History:	14-Feb-2007 (michael.lueck@ingres.com)
			Created
                        26-nov-2007 (peeje01)
			  Bug: 117733
                          Change $PHP_SELF to $_SERVER['PHP_SELF'] following the
                          note at:
                            http://us3.php.net/manual/en/security.globals.php
//-->
<script type="text/javascript">
<!--
	function enableInputs()
	{
		document.getElementById('hostname').disabled=false;
		document.getElementById('userID').disabled=false;
		document.getElementById('dbname').disabled=false;
		document.getElementById('instance').disabled=false;
		document.getElementById('password').disabled=false;
		document.getElementById('change').disabled=false;
		document.getElementById('reset').disabled=false;
	}
	
	function disableInputs()
	{
		document.getElementById('hostname').disabled=true;
		document.getElementById('userID').disabled=true;
		document.getElementById('dbname').disabled=true;
		document.getElementById('instance').disabled=true;
		document.getElementById('password').disabled=true;
		document.getElementById('change').disabled=true;
		document.getElementById('reset').disabled=true;
	}
//-->
</script>

<form method="post" action="<?php echo htmlentities($_SERVER['PHP_SELF']) ?>?feature=connect" name="connect_form" id="connect_form">
<table style="text-align: left; width: 100%;" border="0" cellpadding="0" cellspacing="0">
<tbody>
	<tr>
		<td>
			<input tabindex="1" name="datasource" value="default" type="radio" checked="checked" onClick="disableInputs()"> 
				Default Source
		</td>
		<td>
			&nbsp;
		</td>
		<td>
			&nbsp;
		</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>
			&nbsp;
		</td>
		<td>
			&nbsp;
		</td>
	</tr>
	<tr>
		<td>
			<input name="datasource" value="custom" type="radio" onClick="enableInputs()">
				Defined Source
		</td>
		<td>
			&nbsp;
		</td>
		<td>
			&nbsp;
		</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>
			&nbsp;
		</td>
		<td>
			&nbsp;
		</td>
	</tr>
	<tr>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td class="topField" colspan="2"> <h4>Server</h4></td>
		<td>&nbsp;</td>
		<td class="topField" colspan="2"> <h4>Credentials</h4></td>
		<td>&nbsp;</td>
		<td class="topField"> <h4>Database</h4></td>
	</tr>
	<tr>
		<td class="leftField">Hostname</td>
		<td class="rightField">
			<input tabindex="2" name="hostname" id="hostname" size="20" maxlength="20" type="text" value="(local)" disabled="disabled">
		</td>
		<td>&nbsp;</td>
		<td class="leftField">User ID</td>
		<td class="rightField">
			<input tabindex="4" name="userID" id="userID" size="20" maxlength="20" type="text" disabled="disabled">
		</td>
		<td>&nbsp;</td>
		<td class="rowField">
			<input tabindex="6" name="dbname" id="dbname" size="20" maxlength="20" 
			       type="text" value="demodb" disabled="disabled">
		</td>
	</tr>
	<tr>
		<td class="leftField">Instance name</td>
		<td class="rightField">Ingres&nbsp;
			<input tabindex="3" name="instance" id="instance" size="2" maxlength="2" type="text" value="II" disabled="disabled">
		</td>
		<td>&nbsp;</td>
		<td class="leftField">Password</td>
		<td class="rightField">
			<input tabindex="5" name="password" id="password" size="20" maxlength="20" type="password" disabled="disabled">
		</td>
		<td>&nbsp;</td>
		<td class="rowField">&nbsp;</td>
	</tr>
	<tr>
		<td class="bottomField" colspan="2">&nbsp;</td>
		<td>&nbsp;</td>
		<td class="bottomField" colspan="2">&nbsp;</td>
		<td>&nbsp;</td>
		<td class="bottomField">&nbsp;</td>
	</tr>
	<tr>
		<td colspan="2" rowspan="1">
		&nbsp;
		</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td style="padding-top:10px;text-align:right;">
			<input tabindex="7" type="button" name="change" id="change" value="Change" disabled="disabled" 
				onClick="xajax_changeConnectionProfile(xajax.getFormValues('connect_form'))">
			<input tabindex="8" type="reset"  name="reset" id="reset" value="Reset" disabled="disabled">
		</td>
	</tr>
</tbody>
</table>
</form> 


