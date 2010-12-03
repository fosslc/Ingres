require 'singleton'
require 'tk'
require "fp_constants"
require "bordered_frame"
require "airport_frame"
require "flight_planner_frames"
require "connect_frames"

class RoutesFrames < FlightPlannerFrames

  include Singleton

  private

  def createTopFrame(parent, width)
    x = 0
    y = 0
    height = topFrameHeight
    @topFrame = createBorderedFrame(parent, width, height, x, y)
    createRoutesSelector(@topFrame.innerFrame, @topFrame.innerWidth)
  end

  def createBottomFrame(parent, width)
    x = 0
    y = @topFrame.outerHeight + FPConstants.border
    height = @baseFrameHeight - y - 2 # for pane outline
    @bottomFrame = createBorderedFrame(parent, width, height, x, y)
  end

  def createListDaysFrame(parent, width, height)
    listDaysFrame = TkFrame.new(parent){ width width; height height }
    listDaysBox = TkListbox.new(listDaysFrame){ width 16 ; height RoutesFrames.linesInListDaysBox ; font FPConstants.defaultFont }
    listDaysBox.configure('selectmode' => "multiple")
    FPConstants.daysOfWeek.each { | day | listDaysBox.insert('end', day) }
    listDaysScrollBar = TkScrollbar.new(listDaysFrame)
    listDaysScrollBar.command() { | *args | listDaysBox.yview *args }
    listDaysBox.yscrollcommand() { | first, last | listDaysScrollBar.set(first, last) }
    return listDaysFrame, listDaysBox, listDaysScrollBar
  end

  def createButtons(parent)
    go = createButton(parent, "Go")
    go.configure('command' => proc { goHandler } )
    newSearch = createButton(parent, "New search")
    newSearch.configure('command' => proc { show } )
    return go, newSearch
  end

  def createRoutesSelector(parent, width)
    height = routeFrameHeight
    width -= 2 * FPConstants.border
    @routeFrame = createLabelFrame(parent, width, height, " Routes ")
    width -= (3 * FPConstants.border)
    height = routeCriteriaHeight
    @criteriaFrame = createLabelFrame(@routeFrame, width, height, "Route Criteria")
    @departingFrame = AirportFrame.new(@criteriaFrame, "Departing")
    @arrivingFrame = AirportFrame.new(@criteriaFrame, "Arriving")
    width -= ((3 * FPConstants.border) + 2 * (FPConstants.airportFrameWidth + FPConstants.border))
    height = daySelectorFrameHeight
    @flyingOnFrame = createLabelFrame(@criteriaFrame, width,  height, "Flying On")
    @anyDayBox = createCheckBox(@flyingOnFrame, FPConstants.widgetActiveState, "Any day")
    @anyDayBox.configure('command' => proc { anyDayHandler() })
    width -= 24 + FPConstants.border
    @listDaysFrame, @listDaysBox, @listDaysScrollBar = createListDaysFrame(@flyingOnFrame, width, listDaysBoxHeight)
    @returnJourneyBox = createCheckBox(@criteriaFrame, FPConstants.widgetActiveState, "Include return journey")
    @goButton, @newSearchButton = createButtons(@routeFrame)
  end

  def showListDaysFrame(frame, listBox, scrollBar, x, y)
    frame.place('x' => x, 'y' => y)
    listBox.pack('side' => "left")
    scrollBar.pack('side' => "right", 'fill' => "y")
    #listBox.deselect
  end

  def showButtons(parent,go, newSearch)
    buttonWidth = 102
    y = routeCriteriaHeight + 2 * FPConstants.border + 2
    x = parent.width - 3 * FPConstants.border - 2 * buttonWidth
    go.place('x' => x, 'y' => y)
    x += FPConstants.border + buttonWidth
    newSearch.place('x' => x, 'y' => y)
  end

  def showTopFrame
    @topFrame.show
    @routeFrame.place('x' => FPConstants.border, 'y' => FPConstants.border)
    @criteriaFrame.place('x' => FPConstants.border, 'y' => FPConstants.border)
    @departingFrame.show(FPConstants.border)
    @arrivingFrame.show(2 * FPConstants.border + FPConstants.airportFrameWidth)
    @flyingOnFrame.place('x' => (3 * FPConstants.border + 2 * FPConstants.airportFrameWidth), 'y' => FPConstants.border)
    showCheckBox(@anyDayBox, 0, FPConstants.border)
    @anyDayBox.select
    showListDaysFrame(@listDaysFrame, @listDaysBox, @listDaysScrollBar, 24, 2 * FPConstants.border + FPConstants.charToPixelsHeight)
    @listDaysBox.configure('state' => FPConstants.widgetInactiveState)
    showCheckBox(@returnJourneyBox, 0, FPConstants.airportFrameHeight + FPConstants.border + 2)
    showButtons(@routeFrame, @goButton, @newSearchButton)
    defaultProfile = DatabaseConnection.instance.getDefaultProfile
    @departingFrame.configure(FPConstants.widgetActiveState, defaultProfile.iataCode) if (defaultProfile)
  end
  
  def anyDayHandler
    var = @anyDayBox.cget('variable')
    state = (var.value == "0") ? FPConstants.widgetActiveState : FPConstants.widgetInactiveState
    @listDaysBox.configure('state' => state)
  end
  
  def goHandler
#    msg = nil
    msg = "Arriving airport" if (@arrivingFrame.getAirport == FPConstants.emptyString)
    msg = "Region of arrival" if (@arrivingFrame.getRegion == FPConstants.emptyString)
    msg = "Country of arrival" if (@arrivingFrame.getCountry == FPConstants.emptyString)
    msg = "Departing airport" if (@departingFrame.getAirport == FPConstants.emptyString)
    msg = "Region of departure" if (@departingFrame.getRegion == FPConstants.emptyString)
    msg = "Country of departure" if (@departingFrame.getCountry == FPConstants.emptyString)
    msg += " is a required entry" if (msg)
    msg = "A selection of at least one day is required" if (!msg && @anyDayBox.cget("variable").value == "0" && @listDaysBox.curselection.empty?)
    if (!msg) then
      curSelection = @listDaysBox.curselection if (@anyDayBox.cget("variable").value == "0")
      includeReturn = (@returnJourneyBox.cget("variable").value == "0") ? false : true
      dbconnection = DatabaseConnection.instance
      routes = dbconnection.getRoutes(@departingFrame.getAirport, @arrivingFrame.getAirport, curSelection, includeReturn)
      if (routes.empty?) then
        msg = "No routes found. Try again"
      else
        showRoutesGrid(routes)
      end
    end
    if (msg) then
      buttons = [ "     OK     " ]
      TkDialog.new('title' => "Routes", 'message' => msg, 'buttons' => buttons)
    end
  end
  
  def addLabelToGrid(parent, text, row, column, background = FPConstants.widgetInactiveBackground, height = 1, sticky = "we")
    label = TkLabel.new(parent) { text text ; height height ; relief "groove" ; background background ; font FPConstants.defaultFont }
    label.grid('row' => row, 'column' => column, 'sticky' => sticky, 'ipadx' => 16)
    return label
  end

  def formatTime(dateTime)
    return (dateTime.hour < 13 ? dateTime.hour : dateTime.hour - 12).to_s + ":" + sprintf("%02d", dateTime.min) + " " + ((dateTime.hour < 12) ? "A" : "P" ) + "M"
  end

  def showRoutesGrid(routes)
    labelTexts = [ " ", "Airline", "IATA\nCode", "Flight\nNumber", "Depart\nFrom", "Arrive To", "Departure\nTime", "Arrival Time", "Days\nLater", " On Days " ]
    parent = @bottomFrame.innerFrame
    @tmpFrame.destroy if @tmpFrame
    height = parent.cget('height') - 2
    width = parent.cget('width') - 2
    @tmpFrame = TkFrame.new(parent) { height height ; width width ; background "dark grey"}
    tmpFrame = @tmpFrame
    height = (2 + routes.length) * (FPConstants.charToPixelsHeight + 2)
    width = tmpFrame.cget("width") - 2
    parent = TkFrame.new(tmpFrame) { height height ; width width }
    column = 0
    row = 0
    rows = Array.new
    columns = Array.new
    labelTexts.each_index do | index | 
      columns = addLabelToGrid(parent, labelTexts[index], row, column, FPConstants.widgetInactiveBackground, 2, "w") 
      column += 1
    end
    rows << columns
    routes.each do | route |
      column = 0
      row += 1
      columns = Array.new
      rows << columns
      columns << addLabelToGrid(parent, labelTexts[0], row, column)
      column += 1
      columns << addLabelToGrid(parent, route.airline, row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, route.iataCode, row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, route.flightNum, row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, route.departFrom, row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, route.arriveTo, row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, formatTime(route.departAt), row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, formatTime(route.arriveAt), row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, route.arriveOffset, row, column, FPConstants.widgetActiveBackground)
      column += 1
      columns << addLabelToGrid(parent, route.flightDays, row, column, FPConstants.widgetActiveBackground)
    end
    tmpFrame.place('x' => 1, 'y' => 1)
    parent.place('x' => 1, 'y' => 1)
  end

  def showBottomFrame
    @tmpFrame.destroy if @tmpFrame
    @bottomFrame.show
  end

  def listDaysBoxHeight
    RoutesFrames.linesInListDaysBox * FPConstants.charToPixelsHeight + 4 # for the day of week listbox border
  end

  def daySelectorFrameHeight
    FPConstants.charToPixelsHeight + listDaysBoxHeight + 3 * FPConstants.border
  end

  def routeCriteriaHeight
    if (FPConstants.airportFrameHeight + FPConstants.border + FPConstants.charToPixelsHeight) > daySelectorFrameHeight then
      height = (FPConstants.airportFrameHeight + FPConstants.border + FPConstants.charToPixelsHeight)
    else
      height = daySelectorFrameHeight
    end
    height += FPConstants.charToPixelsHeight + 2 * FPConstants.border
    height
  end

  def routeFrameHeight
    FPConstants.charToPixelsHeight + 3 * FPConstants.border + routeCriteriaHeight + FPConstants.buttonHeight
  end

  def topFrameHeight
    height = 2 * FPConstants.border + routeFrameHeight + 2
    height
  end

  def RoutesFrames.linesInListDaysBox
    5
  end

end
