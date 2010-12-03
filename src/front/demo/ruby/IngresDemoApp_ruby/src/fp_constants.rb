class FPConstants

  def FPConstants.emptyString
    ""
  end

# General widget and window geometry constants
  def FPConstants.borderColor
    "black"
  end
  
  def FPConstants.defaultFont
    "Tahoma 8 normal"
  end

  def FPConstants.defaultBoldFont
    "Tahoma 8 bold"
  end

  def FPConstants.widgetActiveState
    "normal"
  end

  def FPConstants.widgetInactiveState
    "disabled"
  end

  def FPConstants.widgetActiveBackground
    "white"
  end

  def FPConstants.widgetInactiveBackground
    "light grey"
  end

  def FPConstants.defaultLabelHeight
    1
  end

  def FPConstants.defaultLabelAnchor
    "w"
  end

  def FPConstants.defaultButtonWidth
    15
  end

  def FPConstants.buttonHeight
    25
  end

  def FPConstants.buttonWidth
    102
  end

  def FPConstants.menuButtonHeight
    27
  end

  def FPConstants.textEntryHeight
    22
  end

  def FPConstants.pixelsToCharWidth
    18.0 / 109.0
  end

  def FPConstants.charToPixelsHeight
    18
  end

  def FPConstants.border
    4
  end

# Main window geometry
  def FPConstants.flightPlannerWindowWidth
    880
  end
  
  def FPConstants.flightPlannerWindowTitle
    "Ingres Frequent Flyer"
  end

  def FPConstants.flightPlannerWindowHeight
    FPConstants.statusBarBottomEdge
  end

# Airport selector geometry
  def FPConstants.airportFrameHeight
    2 * FPConstants.charToPixelsHeight + 2 * FPConstants.menuButtonHeight + 3 * FPConstants.border
  end

  def FPConstants.airportFrameWidth
    270
  end

# Airport selector criteria defaults
  def FPConstants.defaultCountryOption
    "Select value..."
  end

  def FPConstants.defaultRegionOption
    "Select value..."
  end

  def FPConstants.defaultAirportOption
    FPConstants.emptyString
  end
  
# Connection details geometry
  def FPConstants.connectionElementFrameHeight
    2 * FPConstants.textEntryHeight + FPConstants.charToPixelsHeight + 3 * FPConstants.border
  end

  def FPConstants.connectionDetailsFrameHeight
    FPConstants.connectionElementFrameHeight + FPConstants.charToPixelsHeight + 4 * FPConstants.border
  end

  def FPConstants.connectionDataSourceFrameHeight
    FPConstants.connectionDetailsFrameHeight + FPConstants.buttonHeight + 3 * FPConstants.charToPixelsHeight + 4 * FPConstants.border
  end
  
  def FPConstants.connectionFrameHeight
    2 * FPConstants.border + FPConstants.connectionDataSourceFrameHeight + 2
  end

# Main window large icon button bar geometry
  def FPConstants.buttonBarImageSize
    130
  end

  def FPConstants.buttonBarImageBorder
    FPConstants.border
  end

  def FPConstants.buttonBarBottomEdge
    FPConstants.ingresBarBottomEdge + 4 * (FPConstants.buttonBarImageSize + FPConstants.buttonBarImageBorder + FPConstants.border)
  end

  def FPConstants.buttonBarSideEdge
    FPConstants.border + FPConstants.buttonBarImageSize + FPConstants.buttonBarImageBorder + FPConstants.border
  end

# Help about window Ingres graphic bar geometry
  def FPConstants.ingresLogoImageHeight
    308
  end

  def FPConstants.ingresLogoImageWidth
    154
  end

  def FPConstants.helpAboutHeight
    2 * FPConstants.helpAboutBorder + FPConstants.ingresLogoImageHeight + FPConstants.border
  end

  def FPConstants.helpAboutWidth
    500
  end

  def FPConstants.helpAboutBorder
    10
  end

# Main window Ingres graphic bar geometry
  def FPConstants.ingresBarImageHeight
    70
  end

  def FPConstants.ingresBarImageWidth
    873
  end

  def FPConstants.ingresBarBottomEdge
    FPConstants.ingresBarImageHeight + FPConstants.border + 2 # adding two pixels for image boarder
  end

# Main window status bar geometry
  def FPConstants.statusBarHeight
    1 * FPConstants.charToPixelsHeight
  end

  def FPConstants.statusBarWidth
    FPConstants.flightPlannerWindowWidth - (2 * FPConstants.border)
  end

  def FPConstants.statusBarBottomEdge
    FPConstants.buttonBarBottomEdge + FPConstants.charToPixelsHeight + FPConstants.border
  end

# Icon location and names
  def FPConstants.imagesFolder
    "../images"
  end

  def FPConstants.ingresBannerImage
    FPConstants.imagesFolder + "/IngresBanner.gif"
  end

  def FPConstants.routesButtonImage
    FPConstants.imagesFolder + "/RoutesButton.gif"
  end

  def FPConstants.profileButtonImage
    FPConstants.imagesFolder + "/ProfileButton.gif"
  end

  def FPConstants.connectButtonImage
    FPConstants.imagesFolder + "/ConnectButton.gif"
  end

  def FPConstants.exitButtonImage
    FPConstants.imagesFolder + "/ExitButton.gif"
  end

  def FPConstants.ingresLogoImage
    FPConstants.imagesFolder + "/IngresLogo.gif"
  end

# Database connection defaults
  def FPConstants.dbConnectionDefaultHost
    "127.0.0.1"
  end

  def FPConstants.dbConnectionDefaultInstance
    "II"
  end

  def FPConstants.dbConnectionDefaultUser
    FPConstants.emptyString
  end

  def FPConstants.dbConnectionDefaultPassword
    FPConstants.emptyString
  end

  def FPConstants.dbConnectionDefaultDatabase
    "demodb"
  end

  def FPConstants.dbConnectionDefaultProfileEmailAddress
    FPConstants.emptyString
  end

  def FPConstants.defaultEmailAddressOption
    "Select value..."
  end

# Database connection configuration file
  def FPConstants.defaultDBConfigPath
    defaultDBConfigPath = ENV['TMP']
    defaultDBConfigPath = ENV['TEMP'] if !defaultDBConfigPath
    defaultDBConfigPath
  end

  def FPConstants.defaultDBConfigFilename
    "dbconn.xml"
  end

  def FPConstants.dbConfigFileTypes
    types = [ [ "XML Files", ".xml" ], ["All Files", "*"]]
  end

  def FPConstants.photographFileTypes
    types = [ [ "JPEG Files", ".jpg" ], ["All Files", "*"]]
  end

# Database date as string format
  def FPConstants.timeFmtStr
    '%d-%b-%G %H:%M:%S'
  end

  def FPConstants.daysOfWeek
    [ "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" ]
  end

end
