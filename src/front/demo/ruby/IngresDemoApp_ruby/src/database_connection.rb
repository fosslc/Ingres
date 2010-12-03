require 'singleton'
require 'Ingres'
require 'rexml/document'
require "fp_constants"
require "routes"
require "profiles"

class DatabaseConnection
  include Singleton
 
  def disconnect
    if (@connected) then 
      begin
        @connection.disconnect if (@connection)
      rescue Exception
      end
      @connection = nil
    end
    @countryCodes = nil
    @connected = false
  end

  def execute(*args)
    pexecute(*args)
  end

  def countries
    if (!@countryCodes) then
      countryResults = execute("SELECT ct_name, ct_code FROM country ORDER BY ct_name")
      countryCodes = Hash.new
      countries = Array.new
      countryResults.each { | countryDetails | countryCodes[countryDetails[0]]= countryDetails[1] ; countries << countryDetails[0] }
      @countries = countries
      @countryCodes = countryCodes
      @codeCountries = countryCodes.invert
    end
    @countries
  end

  def regions(country)
    regionResults = execute("SELECT DISTINCT ap_place, ap_ccode FROM airport WHERE ap_ccode = '#{@countryCodes[country]}' ORDER BY ap_place")
    regionCodes = Hash.new
    regions = Array.new
    regionResults.each { | regionDetails | regionCodes[regionDetails[0]]= regionDetails[1] ; regions << regionDetails[0] }
    @regionCodes = regionCodes
    regions
  end

  def getAirportDetails(airport)
    airportDetailResults = execute("SELECT DISTINCT ap_place, ap_ccode FROM airport WHERE ap_iatacode = '#{airport}' ORDER BY ap_place")
    countryCode = String.new
    region = String.new
    airportDetailResults.each { | airportDetails | region = airportDetails[0] ; countryCode = airportDetails[1] }
    return @codeCountries[countryCode], region
  end

  def airports(country, region)
    airportResults = execute("{call get_my_airports ( ccode = ?, area = ? )}",  "ccode", "n", @countryCodes[country] , "area", "n", region)
    airportCodes = Hash.new
    airports = Array.new
    airportResults.each { | airportDetails | airportCodes[airportDetails[0]]= airportDetails[2] ; airports << airportDetails[0] }
    @airportCodes = airportCodes
    airports
  end

  def getRoutes(departFrom, arriveTo, days, includeReturn)
    flightDay = getFlightDays(days)
    query = "SELECT rt_airline, al_iatacode, al_name, rt_flight_num, rt_depart_from, rt_arrive_to, rt_depart_at, rt_arrive_at, rt_arrive_offset, rt_flight_day " + 
            "FROM route r, airline a " +
            "WHERE ((rt_depart_from = '#{departFrom}' and rt_arrive_to = '#{arriveTo}') "
    query +=       " OR   (rt_depart_from = '#{arriveTo}' and rt_arrive_to = '#{departFrom}')" if (includeReturn)
    query += ") and rt_flight_day LIKE '#{flightDay}' and rt_airline = al_icaocode ORDER BY rt_airline, rt_flight_num"
    resultset = execute(query)
    routes = Array.new
    resultset.each { | route | routes << Route.new(route) }
    routes
  end

  def getEmailAddresses
    emailAddressResults = execute("SELECT up_email FROM user_profile ORDER BY up_email")
    emailAddresses = Array.new
    emailAddressResults.each { | emailAddressDetails | emailAddresses << emailAddressDetails[0] }
    emailAddresses
  end

  def insertProfile(lastname, firstname, iataCode, emailAddress, isDefault = false, profileImage = nil)
    profileImage = "" if (profileImage == nil)
#    setDebugFlag("GLOBAL_DEBUG", "TRUE")
    execute("INSERT INTO user_profile (up_id, up_last, up_first, up_airport, up_email, up_image) VALUES (next value FOR up_id, ?, ?, ?, ?, ?)", 
            "N", lastname, "N", firstname, "v", iataCode, "v", emailAddress, "B", profileImage)
#    setDebugFlag("GLOBAL_DEBUG", "TRUE")
    if (isDefault) then
      @defaultProfileEmailAddress = String.new(emailAddress)
      saveConnectionInfo
      getDefaultProfile(true)
    end
  end

  def updateProfile(lastname, firstname, iataCode, emailAddress, isDefault = false, profileImage = nil)
    profileImage = "" if (profileImage == nil)
#    setDebugFlag("GLOBAL_DEBUG", "TRUE")
    execute("UPDATE user_profile SET up_last = ?, up_first = ?, up_airport = ?, up_image = ? WHERE up_email = ?", 
            "N", lastname, "N", firstname, "v", iataCode, "B", profileImage, "v", emailAddress)
#    setDebugFlag("GLOBAL_DEBUG", "TRUE")
    if (isDefault || @defaultProfileEmailAddress == emailAddress) then
      @defaultProfileEmailAddress = FPConstants.dbConnectionDefaultProfileEmailAddress
      @defaultProfileEmailAddress = String.new(emailAddress) if (isDefault)
      saveConnectionInfo
      getDefaultProfile(true)
    end
  end

  def getProfile(emailAddress)
    profile = nil

    # to save going to the database when it's not necessary
    if (emailAddress && emailAddress.length > 0) then
      resultset = execute("SELECT up_last, up_first, up_email, up_airport, up_image FROM user_profile WHERE up_email = '#{emailAddress}'")
      resultset.each { | result | profile = Profile.new(result) }
    end
    profile
  end

  def getDefaultProfile(forceGet = false)
    if (!@defaultProfileEmailAddress) then
      database()
    end
    if (!@defaultProfile || forceGet) then
      @defaultProfile = getProfile(@defaultProfileEmailAddress)
    end
    @defaultProfile
  end

  def deleteProfile(emailAddress)
#    Not handling up_image just yet as LOBs are not fully implemented in the Ingres Ruby driver
#    resultset = execute("SELECT up_last, up_first, up_email, up_airport, up_image FROM user_profile WHERE up_email = '#{emailAddress}'")
    resultset = execute("DELETE FROM user_profile WHERE up_email = '#{emailAddress}'")
  end

  def setDebugFlag(flag, value)
    @connection.set_debug_flag(flag, value)
  end

  def hostname
    if (!@hostname) then
      database()
    end
    @hostname
  end

  def instance
    if (!@instance) then
      database()
    end
    @instance
  end

  def username
    if (!@username) then
      database()
    end
    @username = "" if !@username #BPK070523
    @username
  end

  def password
    if (!@database) then
      database()
    end
    @password = "" if !@password #BPK070523
    @password
  end

  def database
    if (!@database) then
      getConnectionInfo
    end
    @database
  end

  def getConnectionInfo(xmlFileName = FPConstants.emptyString)
    if (@connecting != true) then
      disconnect 
    end
    if (xmlFileName == FPConstants.emptyString) then
      xmlFileName = FPConstants.defaultDBConfigPath + "/" + FPConstants.defaultDBConfigFilename
    end
    if (File.exist?(xmlFileName)) then
      xml = REXML::Document.new(File.open(xmlFileName, "r"))
      @database = getDetail(xml, "database")
      @password = getDetail(xml, "password")
      @username = getDetail(xml, "username")
      @instance = getDetail(xml, "instance")
      @hostname = getDetail(xml, "hostname")
      @defaultProfileEmailAddress = getDetail(xml, "defaultProfileEmailAddress")
    else
      @database = FPConstants.dbConnectionDefaultDatabase
      @password = FPConstants.dbConnectionDefaultPassword
      @username = FPConstants.dbConnectionDefaultUser
      @instance = FPConstants.dbConnectionDefaultInstance
      @hostname = FPConstants.dbConnectionDefaultHost
      @defaultProfileEmailAddress = FPConstants.dbConnectionDefaultProfileEmailAddress
    end
    if (@connecting != true) then
      connect
    end
  end
  
  def setConnectionInfo(hostname, database, instance, username, password)
    disconnect
    @hostname = String.new(hostname)
    @database = String.new(database)
    @instance = String.new(instance)
    @username = String.new(username)
    @password = String.new(password)
    connect
  end
  
  def saveConnectionInfo(xmlFileName = FPConstants.emptyString)
    if (xmlFileName == FPConstants.emptyString) then
      xmlFileName = FPConstants.defaultDBConfigPath + "/" + FPConstants.defaultDBConfigFilename
    end
    xmlFile = File.open(xmlFileName, "w+")
    xml = REXML::Document.new
    dbC = REXML::Element.new("databaseConnection")
    addDetail(dbC, "database", @database)
    addDetail(dbC, "password", @password)
    addDetail(dbC, "username", @username)
    addDetail(dbC, "instance", @instance)
    addDetail(dbC, "hostname", @hostname)
    addDetail(dbC, "defaultProfileEmailAddress", @defaultProfileEmailAddress)
    xml << REXML::XMLDecl.new
    xml.add_element(dbC)
    xml.write(xmlFile, 0)
    xmlFile.close
  end
  
  def connect()
    begin
      if (!@connected) then
        @connecting = true
        @connection = Ingres.new() if (!@connection)
        if (hostname().length > 0 && instance().length > 0 && username().length > 0) then
          @connection.connect_with_credentials("@" + hostname() + ",tcp_ip," + instance() + "[" + username() + "," + password() + "]::" + database(), username(), password())
        elsif (username().length > 0) then
          @connection.connect_with_credentials(database(), username(), password())
        else
          @connection.connect(database())
        end
        @countryCodes = nil
        begin
          StatusBar.instance.setStatusBarText(hostname, instance, database, username)
        rescue Exception
        end
        @connecting = false
      end
      @connected = true
    rescue Exception
      buttons = [ "     OK     " ]
      msg = "Connection to database failed due to " + $!
      begin
        TkDialog.new('title' => "Database Connection Error", 'message' => msg, 'buttons' => buttons)
      rescue
        puts msg
      end
      disconnect
      @connected = false
    end
    return @connected
  end

  def isConnected
    @connected
  end

  private

  def getDetail(parent, element)
    retval = String.new
    parent.elements.each("//"+element) { | ele | retval = ele.text }
    retval
  end
  
  def addDetail(parent, child, value)
    element = parent.add_element(child)
    element.text = value
    element
  end
  
  def getFlightDays(days)
    if (!days || days.empty?)
      result = "%"
    else
      result = String.new
      first = (days.shift).to_i + 1
      result = "%" if first > 1
      result += first.to_s
      days.each do | day|
        second = day.to_i + 1
        result += "%" if (second - first) > 1
        result += second.to_s
        first = second
      end
      result += "%" if first < 7
    end
    result
  end

  def pexecute(*args)
    connect()
#    setDebugFlag("GLOBAL_DEBUG", "TRUE")
    results = @connection.pexecute(*args)
#    setDebugFlag("GLOBAL_DEBUG", "FALSE")
    results
  end

end
