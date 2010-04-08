<!--
	Copright (c) 2007 Ingres Corporation
-->

<!--
	Name:		routesCritera.php
	Description:	HTML Template for routesView. Contains calls of xajax function and other 
			GUI related Stuff
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
	function changeSelectState()
	{
	   if(document.getElementById('flyAnyDay').checked) {
		  document.getElementById('weekDay[]').disabled=true;
	   } else {
		  document.getElementById('weekDay[]').disabled=false;
	   }
	}
	function newSearch()
	{
		document.getElementById('routes_form').reset();
		document.getElementById('flyAnyDay').checked = true;
		document.getElementById('weekDay[]').disabled = true;
		xajax_loadInitialData();
	}
	
	//call the xajax function which fills the Country selects (if $_SESSION['defaultProfile'] is set
	//region and apcode of departure airport will be set too
	xajax_loadInitialData();
//-->
</script>

<form method="post" action="<?php echo htmlentities($_SERVER['PHP_SELF'])?>" name="routes_form" id="routes_form" 
		    onSubmit="false;xajax_doSearch(xajax.getFormValues('routes_form'))">
<table style="text-align: left; width: 100%;" border="0" cellpadding="0" cellspacing="0">
	<tbody>
	<tr>
		<td colspan="7" rowspan="1">
			<h3>Route Criteria</h3>
		</td>
	</tr>
	<tr>
		<td class="topField" style="width: 225px;" colspan="2" rowspan="1">
			<h4>Departing</h4>
		</td>
		<td>&nbsp;</td>
		<td class="topField" colspan="2" rowspan="1" style="width: 225px;">
			<h4>Arriving</h4>
		</td>	
		<td>&nbsp;</td>
		<td class="topField" style="width: 104px;">
			<h4>Flying On</h4>
		</td>
	</tr>
	<tr>
		<td class="rowField" style="width: 275px;" colspan="2" rowspan="1">
			Country&nbsp;
			<div id="dep_country">
			<select tabindex="1" name="dep_country_select" id="dep_country_select"
				onChange="xajax_fillRegion('dep_region',this.options[this.selectedIndex].value)">
			</select>
			</div>
		</td>
		<td>&nbsp;</td>
		<td class="rowField" colspan="2" rowspan="1" style="width: 225px;">
			Country&nbsp;
			<div id="arv_country">
			<select tabindex="4" name="arv_country_select" id="arv_country_select" 
				onChange="xajax_fillRegion('arv_region',this.options[this.selectedIndex].value)">
			</select>
			</div>
		</td>
		<td>&nbsp;</td>
		<td class="rowField" style="width: 104px;">
			<input tabindex="7" name="flyOnAnyDay" type="checkbox" id="flyAnyDay" onClick="changeSelectState()" checked>
				Any Day
		</td>
	</tr>
	<tr>
		<td class="leftField" style="width: 115px;">
			Region
		</td>
		<td class="rightField" style="width: 110px;">
			Airport Code
		</td>
		<td>&nbsp;</td>
		<td class="leftField" style="width: 115px;">
			Region
		</td>
		<td class="rightField" style="width: 110px;">
			Airport Code
		</td>
		<td>&nbsp;</td>
		<td class="bottomField" colspan="1" rowspan="4" style="width: 104px;">
			<select tabindex="8" disabled="disabled" multiple="multiple" size="5" name="weekDay[]" id="weekDay[]">
				<option value="1">Monday</option>
				<option value="2">Tuesday</option>
				<option value="3">Wednesday</option>
				<option value="4">Thursday</option>
				<option value="5">Friday</option>
				<option value="6">Saturday</option>
				<option value="7">Sunday</option>
				</select>
		</td>
	</tr>
	<tr>
		<td class="leftField" style="width: 175px;">
			<div id="dep_region">
			<select tabindex="2" name="dep_region_select" disabled="disabled">
			<option value="invalid">Select Country</option>
			</select>
			</div>
		</td>
		<td class="rightField" style="width: 110px;">
			<div id="dep_apcode">
			<select tabindex="5" name="dep_apcode_select" disabled="disabled">
			<option value="invalid">Select Region</option>
			</select>
			</div>
		</td>
		<td>&nbsp;</td>
		<td class="leftField" style="width: 115px;">
			<div id="arv_region">
			<select tabindex="3" name="arv_region_select" disabled="disabled">
			<option value="invalid">Select Country</option>
			</select>
			</div>
		</td>
		<td class="rightField" style="width: 110px;">
			<div id="arv_apcode">
			<select tabindex="6" name="arv_apcode_select" disabled="disabled">
			<option value="invalid">Select Region</option>
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
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td colspan="7" rowspan="1" >
			<input name="return_route" type="checkbox">
				Include return journey
		</td>
	</tr>
	<tr>
		<td colspan="6" rowspan="1">
			&nbsp;
		</td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td colspan="6" rowspan="1" align="right">
			<input type="button" name="Go" value="Go" onClick="xajax_doSearch(xajax.getFormValues('routes_form'))">
			<!-- after Klick on New Serach all Input fields must be disablen, except of the Country select//-->
			<input type="button" name="NewSearch" value="New Search" onClick="newSearch()">
		</td>
		<td>&nbsp;</td>
	</tr>
</tbody>
</table>
</form>
