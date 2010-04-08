<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/*
 * Name:	ProfileModel.php
 *
 * Description: contains class of profileModel. All profile relevant Database querys are made in here
 *
 * Private:	$DBConn
 *
 * Public:	None
 *
 * Methods:	__construct
 *		__destruct
 *		checkInputs
 *		getMailAdresses
 *		addNew
 *		update
 *		delete
 *		getProfileByEmail
 *		getPicture
 *		getAirportInfoByProfile
 *	
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 *
 */
	require_once("database/DBConnection.php");

	class ProfileModel
	{
		private $DBConn;
	
		/*
		 * Name:		__construct
		 *	
		 * Description:		constructor. creates new instance fo DBConnection and saves it in the $DBConn member
		 *
		 * Inputs:		None.
		 *
		 * Outputs:		None.
		 *
		 * Returns:		instance of ProfileModel
		 *
		 * Exceptions:		None.
		 */
		function __construct()
		{
			$this->DBConn = new DBConnection();
		}
		
		/*
		 * Name:		checkInputs
		 *
		 * Description:		method to checkt whether user inputs are valid. checks on email addresses and 
		 *			text input values. Input values are not allowed to contain select,insert,delete or 
		 *			update
		 *
		 * Inputs:		$aInputs	-	array containing values to check. Only values for the keys 
		 *						up_email, up_first, up_last and up_image are checked.
		 *
		 * Outputs:		None.
		 *
		 * Returns:		String 		-	contains messages which provide information for the user
		 *						which values should be changed to make them valid. If all inputs are
		 *						valid method returns an empty string 
		 *
		 * Exceptions:		None.
		 */
		private function checkInputs($aInputs)
		{
			if(!is_array($aInputs))
			{
				return "";
			}
			$invalidValues="";
			foreach($aInputs as $key => $value)
			{
				if($key == "up_email")
				{
					if(!eregi("^[a-z0-9._-]+@[a-z0-9._-]+\.[a-z]{2,4}$",$value))
						$invalidValues .= "Email address is invalid<br/>";
				}	
				if($key == "up_first")
				{
					if(eregi("(select|insert|delete|update)", $value))
						$invalidValues .= "Firstname is invalid<br/>";
				}
				if($key == "up_last")
				{
					if(eregi("(select|insert|delete|update)", $value))
						$invalidValues .= "Lastname is invalid<br/>";
				}
				if($key == "up_image" && $value != "")
				{
					$type = exif_imagetype($value);
					switch($type)
					{
						case IMAGETYPE_JPEG:
							break;
						default:
							$invalidValues .="Image type not supported or no Image file given";
					}
				}
			}
			return $invalidValues;
		}
	
		/*
		 * Name:		getMailAddresses
		 *
		 * Description:		gets all MailAddresses from the table user_profile;
		 *
		 * Inputs:		None
		 *
		 * Outputs:		None
		 *
		 * Returns:		two dimensional array - each row contains a array that only consists of the element with 
		 *			key 'up_mail'. all values are converted to UTF-8
		 *
		 * Exceptions:		None.
		 */
		function getMailAddresses()
		{
			$this->DBConn->query("SELECT up_email from user_profile");					
			$resultArray = $this->DBConn->getResultArray();
			$mailaddresses = array();
			foreach($resultArray as $key => $emailaddress)
			{
				$mailaddresses[$key]['up_email'] = iconv("UTF-16LE","UTF-8",$emailaddress['up_email']);
			}
			return $mailaddresses;
		}
		
		/*
		 * Name:		addNew
		 *
		 * Description:		inserts new user into user_profile. if credits contain the file name of 
		 *			an uploaded file, the file is inserted as BLOB
		 *
		 * Inputs:		$credits	-	array, composed in profile.php. consists of following elements:
		 *						up_airport,up_email,up_first,up_last and up_image. If no image has been
		 *						uploaded by the user than $credits[up_image] is an empty string, otherwise
		 *						it saves the file name of the temporary file.
		 * Outputs:		None.
		 *
		 * Returns:		None.
		 *	
		 * Exceptions:		Exception with code 6 -	invalid inputs. Some of the inputs in the credits array are not valid
		 *			Exception with code 2 - duplicate key on insert. if there is already an entry with the specified
		 *						mailaddress in the database
		 *			Exception with code 3 - Exception from within DBConnection. Query could not be executed.
		 */
		function addNew($credits)
		{
			//We only check for semantic errors on the values in credits. HTML tags are converted to special 
			//chars or striped in profile.php
			$invalidValues = $this->checkInputs($credits);
			if($invalidValues != "")
				throw new Exception($invalidValues,6);
			
			//look whether profile already exist
			$this->DBConn->query("SELECT count(*) from user_profile where up_email='".$credits['up_email']."'");
			$result = $this->DBConn->getResultArray();
			
			//if result is 0 than anything is ok. If not throw Exception
			if($result[0][0] != 0)
				throw new Exception("There is already an entry with the specified Email-Address in the database",2);
			
			$first = iconv("UTF-8","UTF-16LE",addslashes($credits["up_first"]));
			$last  = iconv("UTF-8","UTF-16LE",addslashes($credits["up_last"]));
				
			$params = array ("up_first" => $first, "up_last" => $last);
			$sqltypes ="NN";

			//if up_image is not empty string, user has uploaded a fil
			if($credits['up_image']!= "")
			{
				//open the file
				$handle = fopen($credits['up_image'],"rB");
				//write contents into variable
				$image = stream_get_contents($handle);	
				//close file handle
				fclose($handle);
				
				//SQL type for binary files is B for LONG BYTE
				$sqltypes .= "B";

				//add to parameter array
				$params["up_image"] = $image;
				//and make sql statement; add the placeholder "?" where the value for the BLOB column stands
				$sql="INSERT INTO user_profile("
							."up_id,"
							."up_airport,"
							."up_email,"
							."up_first,"
							."up_last,"
							."up_image)"
						      ."VALUES ("
							."next value for UP_ID,"
							."'".$credits['up_airport']."',"
							."'".$credits['up_email']."',"
							."?,"
							."?,"
							."?)";
				try
				{	
					//we could do a commit here, so that prior changes are not rolled
					//back. but every change has to decide itself to commit or not.
					//so if we not commited prior they will be lost and it's our own fault
					//execute query
					$this->DBConn->query($sql,$params,$sqltypes);
				} catch (Exception $e) {
					//something bad happened. Ok than, let's rollback our work.
					$this->DBConn->rollback();
					throw $e;
				}
			} else {
				// no picture was uploaded so it's quite simple to insert the new profile.
				// it's only a prepared sql statement
				$sql = "INSERT INTO user_profile(
						up_id,
						up_airport,
						up_email,
						up_first,
						up_last,
						up_image)
					VALUES (
						next value for UP_ID,"
						."'".$credits['up_airport']."',"
						."'".$credits['up_email']."',"
						."?,"
						."?,"
						."NULL)";
				try 
				{
					//execute the query
					$this->DBConn->query($sql,$params,$sqltypes);	

				} catch (Exception $e) {
					//something bad happened. so let's rollback our work
					$this->DBConn->rollback();
					throw $e;
				}
			}
			//Everything was ok (all Exceptions were thrown again); so we will commit our changes	
			$this->DBConn->commit();
			
		}
		
		/*
		 * Name:		update
		 *
		 * Description:		updates the row with the specified mailaddress in the table user_profile. If an picture 
		 *			has been uploaded by the user it is inserted as BLOB in the database. If not, the up_image
		 *			field of the updated row is not changed.
		 *
		 * Inputs:		$credits	-	array. Contains the values of the form in profile view
		 *						(up_airport,up_email,up_first,up_last and up_image where up_image's
		 *						value is an empty string if no picture has been uploaded. otherwise 
		 *						its value is the filename of the temporary file
		 *
		 * Outputs:		None
		 *
		 * Returns:		None
		 *
		 * Exceptions:		Exception with code 6	- invalid Inputs. some of the form values have invalid value
		 *			Exception with code 3	- query could not be executed. before further throwing the exeception 
		 *						  changes are rolled back.
		 */
		function update($credits)
		{
			$invalidInputs = $this->checkInputs($credits);
			if($invalidInputs != "")
				throw new Exception($invalidInputs,6);
			//that's the beginning of our update statement
			$sql = "UPDATE user_profile SET ";

			//check what's the lase key in the array
			$lastCreditKey = end(array_keys($credits));
			//and reset array pointer to the start of the array
			reset($credits);
			$params = array();
			$sqltypes = "";
			//now go through the credits array
			foreach($credits as $key => $value)
			{	
				//we addslashes here to escape ' and " because they would change the semantic of the sql statement
				//if not escaped. 
				$value = addslashes($value);
				//if the key is not up_image that we append $key = $value to the sql string
				if($key != "up_image")
				{
					if($key == "up_first" || $key == "up_last"){
					 $sql .= $key."= ? ";
					 $params[$key] = iconv("UTF-8","UTF-16LE",$value);
					 $sqltypes .= "N";
					}
					else
					 $sql .= $key."='".$value."' ";
				}
				//but if the key is up_image we append an placeholder for the value
				else
					$sql .= "up_image = ? ";
				//if the actual key is not the last we put a "," behind our $key = $value statement
				if($key != $lastCreditKey)
					$sql .=", ";
			}
			// Now we add the WHERE clause
			$sql .= " WHERE up_email='".$credits['up_email']."'";
			if(isset($credits['up_image']))
			{
				//if a picture has been uploaded we will get the binary contents into an php variable
				$handle = fopen($credits['up_image'],"rB");
				$image = stream_get_contents($handle);
				fclose($handle);

				//now we need to say that the variable $image has  binary content
				$sqltypes .= "B"; // B stands for LONG BINARY

				//here we created our parameter array to pass to the query method
				$params["up_image"] = $image;
			}	
				//execute the query
			try
			{
				$this->DBConn->query($sql,$params,$sqltypes);
			} catch (Exception $e){
				//could query not be executed?
				if($e->getCode() == 3 )
				{
					//then rollback and throw Exception
					$this->DBConn->rollback();
					throw $e;
				}else{
					throw $e;
				} 
			}
			// if no Exception was catched, we commit our work
			$this->DBConn->commit();
		
		}
		
		/*
		 * Name:		delete
		 *
		 * Description:		delete the row with the given mailaddress from table user_profile
		 *
		 * Inputs:		$email		-	String containing the profile's mailaddress
		 *
		 * Outputs:		None
		 *
		 * Returns:		None
		 *
		 * Exceptions:		Exception with code 6 -	invalid Inputs. the email address doesn't have the
		 *						required format
		 */
		function delete($email)
		{
			$invalidInputs = $this->checkInputs(array("up_email" => $email));
			if($invalidInputs != "")
				throw new Exception($invalidInputs,6);
			$sql = "DELETE FROM user_profile WHERE up_email='".$email."'";
			try
			{
			$this->DBConn->query($sql);
			} catch (exception $e) {
				$this->DBConn->rollback();
				throw $e;
			}
			$this->DBConn->commit();
		}

		/*
		 * Name:		getProfileByEmail
		 *
		 * Description:		selects the all the attributes of the entry with the given mailaddress from table user_profile
		 *			Reads BLOB data for user image
		 *
		 * Inputs:		$email		-	String containing the profile's mailaddress
		 *
		 * Outputs:		None
		 *
		 * Returns:		object		-	object with attributes, each attribute is named after the selected 
		 *						columnnames. ALL String values are converted to UTF-8
		 *
		 * Exceptions:		Exception with code 1 -	No profile found. If one users deletes an profile, which another user 
		 *						currently want's to be displayed.
		 */
		function getProfileByEmail($email)
		{
			$invalidInputs = $this->checkInputs(array("up_email" => $email));
			if($invalidInputs != "")
				throw new Exception($invalidInputs,6);
			$sql ="SELECT 
				up_airport,
				up_email,
				up_first,
				up_last,
				up_image,
				ap_name,
				ap_place,
				ct_name,
				ct_code
			       FROM
				user_profile,
				airport,
				country
			       WHERE
				up_airport = ap_iatacode AND
				ap_ccode = ct_code AND
				up_email ='".$email."'";
			
			$this->DBConn->query($sql);
			
			//let's get the profile as an object
			$resObjArray = $this->DBConn->getResultObjects();
			//but $resObjArray is an array cointaining objects. one object for each row in the result
			if(count($resObjArray) == 0)
			{
				//seems that an other user deleted the entry
				//so we need to reload the mail-adress List
				throw new Exception("No profile found for the given Mailadress",1);
			} elseif (count($resObjArray) != 1) 
				die("Got wrong count of results. Expected 1 got ".count($resObjArray)."\n Sent query:".$sql);

			//Ok now we convert all the string values to UTF-8
			$retObj->first = iconv("UTF-16LE","UTF-8",stripslashes($resObjArray[0]->up_first));	
			$retObj->last = iconv("UTF-16LE","UTF-8",stripslashes($resObjArray[0]->up_last));		
			$retObj->airport = iconv("UTF-16LE","UTF-8",$resObjArray[0]->up_airport);		
			$retObj->ap_name = iconv("UTF-16LE","UTF-8",$resObjArray[0]->ap_name);		
			$retObj->ap_place = iconv("UTF-16LE","UTF-8",$resObjArray[0]->ap_place);		
			$retObj->country = iconv("UTF-16LE","UTF-8",$resObjArray[0]->ct_name);		
			$retObj->ccode = iconv("UTF-16LE","UTF-8",$resObjArray[0]->ct_code);	
			$retObj->email = iconv("UTF-16LE","UTF-8",$resObjArray[0]->up_email);	
			$retObj->image = $resObjArray[0]->up_image;	
				
			return $retObj;
		}
		
		function getPicture($email) {
			$email = iconv("UTF-8","UTF-16LE",$email);
			$sql = "SELECT up_image FROM user_profile WHERE up_email=?";
			$params = array ("up_image" => $email);
			$sqltypes = "N";
			$this->DBConn->query($sql,$params,$sqltypes);
			$resObjArr = $this->DBConn->getResultObjects();
			return $resObjArr[0]->up_image;
		}
		/*
		 * Name:		getAirportInfoByEmail
		 *
		 * Description:		uses SUB SELECT  to join user_profile values for given mailaddress with country and airport.
		 *			so all required aiport information for the homeairport of the profile with specified mailaddress
		 *			are returned. 
		 *
		 * Inputs:		$email 		-	String containing the relevant mailaddress
		 *
		 * Outputs:		None.	
		 *
		 * Returns:		two dimensional Array	-	each row consists of an array with following keys
		 *							country, ccode, region, iatacode. ALL String values are in UTF-8
		 *
		 * Exceptions:
		 */
		function getAirportInfoByEmail($email)
		{
			$invalidInputs = $this->checkInputs(array("up_email" => $email));
			if($invalidInputs != "")
				throw new Exception($invalidInputs,6);
			$sql = "SELECT
				   ct_name,
				   ct_code,
				   ap_place,
				   ap_iatacode
				FROM 
				   country c,
				   airport a
				WHERE
	   		           c.ct_code = a.ap_ccode
				   AND a.ap_iatacode = 
					(SELECT
						up_airport 
					 FROM 
						user_profile
					 WHERE
						up_email='".$email."')";
	
			$this->DBConn->query($sql);
			$resultArray =$this->DBConn->getResultArray();
			
			if(count($resultArray) != 1)
				die("Got wrong count of results. Expected 1 got ".count($resultArray));  
			
			$airportInfos = array();
			foreach($resultArray as $key => $result)
			{	
				$airportInfos[$key]['country'] = iconv("UTF-16LE","UTF-8",$resultArray[$key]['ct_name']);
				$airportInfos[$key]['ccode'] = iconv("UTF-16LE","UTF-8",$resultArray[$key]['ct_code']);
				$airportInfos[$key]['region'] = iconv("UTF-16LE","UTF-8",$resultArray[$key]['ap_place']);
				$airportInfos[$key]['iatacode'] = iconv("UTF-16LE","UTF-8",$resultArray[$key]['ap_iatacode']);
			}	
			return $airportInfos;
		}	
		
		/*
		 * Name:		__destruct
		 *
		 * Description:		destructor
		 *
		 * Inputs:		None
		 *
		 * Outputs:		None
		 *
		 * Returns:		None
		 *
		 * Exceptions:		None
		 */
		function __destruct()
		{
	
		}
	}
?>
