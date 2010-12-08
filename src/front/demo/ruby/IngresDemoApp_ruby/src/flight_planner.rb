require 'tk'
require "fp_constants"
require "menu_bar"
require "ingres_bar"
require "status_bar"
require "button_bar"
require "profiles_frames"
require "database_connection"
require "rubygems"
$KCODE="UTF8"
$-K = "u"
require 'jcode'


class FlightPlanner

  def run
    root = TkRoot.new do
      title FPConstants.flightPlannerWindowTitle
      width FPConstants.flightPlannerWindowWidth
      height FPConstants.flightPlannerWindowHeight
    end
    rootFrame = TkFrame.new(root) { width FPConstants.flightPlannerWindowWidth; height FPConstants.flightPlannerWindowHeight }
    rootFrame.pack
    MenuBar.new(root)
    IngresBar.new(rootFrame)
    ButtonBar.new(rootFrame)
    StatusBar.instance.show(rootFrame)
    RoutesFrames.instance.create(rootFrame)
    ConnectFrames.instance.create(rootFrame)
    ProfilesFrames.instance.create(rootFrame)
    if (DatabaseConnection.instance.connect)
      RoutesFrames.instance.show
    else
      ConnectFrames.instance.show
    end
    Tk.mainloop
  end

end

flightPlanner = FlightPlanner.new()
flightPlanner.run()
