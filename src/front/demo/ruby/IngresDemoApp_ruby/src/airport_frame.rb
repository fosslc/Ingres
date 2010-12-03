require 'tk'
require "flight_planner_frames"
require "database_connection"
require "fp_constants"


class AirportFrame < FlightPlannerFrames

  def initialize(parent, text)
    width = FPConstants.airportFrameWidth
    height = FPConstants.airportFrameHeight
    @parentFrame = TkLabelFrame.new(parent){ width width; height height; text text ; font FPConstants.defaultFont }
    @countryLabel, @countryVar, @countryOptionButton = createLabelAndMenuButton(@parentFrame, "Country", 28)
    @regionLabel, @regionVar, @regionOptionButton = createLabelAndMenuButton(@parentFrame, "Region", 20)
    @airportLabel, @airportVar, @airportOptionButton = createLabelAndMenuButton(@parentFrame, "Airport Code", 5)
  end

  def configure(state = FPConstants.widgetActiveState, airport = nil)
    @countryLabel.configure('state' => state)
    @regionLabel.configure('state' => state)
    @airportLabel.configure('state' => state)
    if (state == FPConstants.widgetInactiveState) then
      showElements(state, FPConstants.widgetInactiveBackground)
    elsif (airport) then
      dbconnection = DatabaseConnection.instance
      country, region = dbconnection.getAirportDetails(airport)
      showElements(FPConstants.widgetActiveState, FPConstants.widgetActiveBackground, FPConstants.widgetActiveState, FPConstants.widgetActiveBackground, FPConstants.widgetActiveState, FPConstants.widgetActiveBackground, country, region, airport)
    else
      showElements
    end
  end

  def show(x)
    @parentFrame.place('x' => x, 'y' => FPConstants.border)
    showElements
  end
  
  def showElements(countryState = FPConstants.widgetActiveState, countryBackground = FPConstants.widgetActiveBackground, regionState = FPConstants.widgetInactiveState, regionBackground = FPConstants.widgetInactiveBackground, airportState = FPConstants.widgetInactiveState, airportBackground = FPConstants.widgetInactiveBackground, country = FPConstants.defaultCountryOption, region = FPConstants.defaultRegionOption, airport = FPConstants.defaultAirportOption)
    secondRowYOffset = FPConstants.menuButtonHeight + 2 * FPConstants.border
    thirdRowYOffset = secondRowYOffset + FPConstants.charToPixelsHeight
    showLabelAndMenuButton(@countryLabel, @countryOptionButton, FPConstants.border, 8, 49, FPConstants.border, countryState, countryBackground)
    @countryVar.value = country
    showLabelAndMenuButton(@regionLabel, @regionOptionButton, FPConstants.border, secondRowYOffset, FPConstants.border, thirdRowYOffset, regionState, regionBackground)
    @regionVar.value = region
    showLabelAndMenuButton(@airportLabel, @airportOptionButton, 187, secondRowYOffset, 187, thirdRowYOffset, airportState, airportBackground)
    @airportVar.value = airport

    dbconnection = DatabaseConnection.instance
    if (dbconnection.connect) then
      countries = dbconnection.countries()
      menu = TkMenu.new(@countryOptionButton, 'tearoff' => false)
      menu.add('command', 'label' => FPConstants.defaultCountryOption, 'command' => proc { listRegions })
      countries.each { | dbcountry | menu.add('command', 'label' => dbcountry, 'command' => proc { listRegions(dbcountry) }) }
      @countryOptionButton.configure('menu' => menu)
    end
  end

  def getCountry
    return (@countryVar.value == FPConstants.defaultCountryOption) ? FPConstants.emptyString : @countryVar.value
  end

  def getRegion
    return (@regionVar.value == FPConstants.defaultRegionOption) ? FPConstants.emptyString : @regionVar.value
  end

  def getAirport
    @airportVar.value
  end

  private

  def listRegions(country = nil)
    if (country) then
      @countryVar.value = country
      @regionOptionButton.configure('state' => FPConstants.widgetActiveState, 'background' => FPConstants.widgetActiveBackground)
      dbconnection = DatabaseConnection.instance
      regions = dbconnection.regions(country)
      menu = TkMenu.new(@regionOptionButton, 'tearoff' => false)
      menu.add('command', 'label' => FPConstants.defaultRegionOption, 'command' => proc {  listAirports })
      regions.each { |region| menu.add('command', 'label' => region.to_s, 'command' => proc {  listAirports(country, region.to_s) }) }
      @regionOptionButton.configure('menu' => menu)
    else
      @countryVar.value = FPConstants.defaultCountryOption
      @regionOptionButton.configure('state' => FPConstants.widgetInactiveState, 'background' => FPConstants.widgetInactiveBackground)
      listAirports
    end
    @regionVar.value = FPConstants.defaultRegionOption
    listAirports
  end

  def listAirports(country = nil, region = nil)
    if (region) then
      @regionVar.value = region
      @airportOptionButton.configure('state' => FPConstants.widgetActiveState, 'background' => FPConstants.widgetActiveBackground)
      dbconnection = DatabaseConnection.instance
      airports = dbconnection.airports(country, region)
        menu = TkMenu.new(@airportOptionButton, 'tearoff' => false)
        airports.each { |airport| menu.add('command', 'label' => airport.to_s, 'command' => proc {  setAirport(airport.to_s) }) }
        @airportOptionButton.configure('menu' => menu)
      if (airports.size == 1) then
        @airportVar.value = airports[0]
      else
        @airportVar.value = FPConstants.defaultAirportOption
      end
    else
      @regionVar.value = FPConstants.defaultRegionOption
      @airportVar.value = FPConstants.defaultAirportOption
      @airportOptionButton.configure('state' => FPConstants.widgetInactiveState, 'background' => FPConstants.widgetInactiveBackground)
    end
  end

  def setAirport(airport)
    @airportVar.value = airport
  end

end
