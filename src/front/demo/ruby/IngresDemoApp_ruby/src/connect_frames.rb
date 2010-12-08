require 'singleton'
require 'tk'
require "bordered_frame"

class ConnectFrames < FlightPlannerFrames

  include Singleton

  private

  def createTopFrame(parent, width)
    x = 0
    y = 0
    height = FPConstants.connectionFrameHeight
    @topFrame = createBorderedFrame(parent, width, height, x, y)
    createDataSourceSelector(@topFrame.innerFrame, @topFrame.innerWidth)
  end

  def createBottomFrame(parent, width)
    x = 0
    y = @topFrame.outerHeight + FPConstants.border
    height = @baseFrameHeight - y - 2 # for pane outline
    @bottomFrame = createBorderedFrame(parent, width, height, x, y)
  end

  def createDataSourceSelector(parent, width)
    height = FPConstants.connectionDataSourceFrameHeight
    width -= 2 * FPConstants.border
    @dataSourceFrame = createLabelFrame(parent, width, height, " Ingres Instance Data Source ")
    @sourceSelectedVar = TkVariable.new
    @defaultRadio = createRadioButton(@dataSourceFrame, FPConstants.widgetActiveState, "Default source", defaultSource, @sourceSelectedVar)
    @definedRadio = createRadioButton(@dataSourceFrame, FPConstants.widgetActiveState, "Defined source", definedSource, @sourceSelectedVar)
    height = FPConstants.connectionDetailsFrameHeight
    width -= 3 * FPConstants.border
    @connectionDetailsFrame = createLabelFrame(@dataSourceFrame, width, height, nil)
    @connectionDetailFrameWidth = (width - 5 * FPConstants.border) / 3
    @connectionDetailFrameWidth = @connectionDetailFrameWidth.to_i
    height = FPConstants.connectionElementFrameHeight
    @serverFrame = createServerFrame(@connectionDetailsFrame, @connectionDetailFrameWidth, height)
    @credentialsFrame = createCredentialsFrame(@connectionDetailsFrame, @connectionDetailFrameWidth, height)
    @databaseFrame = createDatabaseFrame(@connectionDetailsFrame, @connectionDetailFrameWidth, height)
    @makeDefinedBox = createCheckBox(@connectionDetailsFrame, FPConstants.widgetActiveState, "Make defined source default")
    @changeButton, @resetButton = createButtons(@dataSourceFrame)
    @defaultRadio.configure('command' => proc { defaultSourceHandler } )
    @definedRadio.configure('command' => proc { definedSourceHandler } )
    @changeButton.configure('command' => proc { changeSourceHandler } )
    @resetButton.configure('command' => proc { resetSourceHandler } )
  end

  def createButtons(parent)
    change = createButton(parent, "Change")
    reset = createButton(parent, "Reset")
    return change, reset
  end

  def createServerFrame(parent, width, height)
    frame = createLabelFrame(parent, width, height, "Server")
    @hostLabel, @hostEntry, @hostText = createLabelEntryAndTextVar(frame, "Host name",  20)
    @instanceLabel = TkLabel.new(frame) { text "Instance name" ; font FPConstants.defaultFont }
    @ingresLabel, @ingresEntry, @ingresText = createLabelEntryAndTextVar(frame, "Ingres",  6)
    frame
  end

  def createCredentialsFrame(parent, width, height)
    frame = createLabelFrame(parent, width, height, "Server")
    @userLabel, @userEntry, @userText = createLabelEntryAndTextVar(frame, "User ID",  20) ;
    @passwordLabel, @passwordEntry, @passwordText = createLabelEntryAndTextVar(frame, "Password",  20) ;
    frame
  end
  
  def createDatabaseFrame(parent, width, height)
    frame = createLabelFrame(parent, width, height, "Database")
    entryWidth = (width - 4 * FPConstants.border) * FPConstants.pixelsToCharWidth
    entryWidth = entryWidth.to_i
    @databaseText = TkVariable.new
    @databaseEntry = TkEntry.new(frame, 'textvariable' => @databaseText){ width entryWidth ; font FPConstants.defaultFont }
    frame
  end 

  def showTopFrame
    @topFrame.show
    @dbconn = DatabaseConnection.instance
    showDataSourceSelector
    resetSourceHandler
    if (@dbconn.isConnected)
      defaultSourceHandler
    else
      definedSourceHandler
    end
  end

  def showDataSourceSelector
    @dataSourceFrame.place('x' => FPConstants.border, 'y' => FPConstants.border)
    showRadioButton(@defaultRadio, FPConstants.border, FPConstants.border)
    showRadioButton(@definedRadio, FPConstants.border, FPConstants.charToPixelsHeight + FPConstants.border)
    @connectionDetailsFrame.place('x' => FPConstants.border, 'y' => 2 * (FPConstants.charToPixelsHeight + FPConstants.border))
    x = FPConstants.border
    y = FPConstants.border
    showServerFrame(x, y)
    x += FPConstants.border + @connectionDetailFrameWidth
    showCredentialsFrame(x, y)
    x += FPConstants.border + @connectionDetailFrameWidth
    showDatabaseFrame(x, y)
    x = FPConstants.border
    y = FPConstants.connectionElementFrameHeight + 2 * FPConstants.border
    showCheckBox(@makeDefinedBox, x, y)
    showButtons(@dataSourceFrame, @changeButton, @resetButton)
  end

  def showButtons(parent, change, reset)
    buttonWidth = 102
    y = FPConstants.connectionDetailsFrameHeight + FPConstants.border + 2 * (FPConstants.border + FPConstants.charToPixelsHeight) + 2
    x = parent.width - 3 * FPConstants.border - 2 * buttonWidth
    change.place('x' => x, 'y' => y)
    x += FPConstants.border + buttonWidth
    reset.place('x' => x, 'y' => y)
  end

  def showServerFrame(x, y)
    @serverFrame.place('x' => x, 'y' => y)
    text = FPConstants.emptyString
    entryXOffset = hostEntryOffset
    x = FPConstants.border
    y = FPConstants.border
    showLabelEntryAndTextVar(@hostLabel, @hostEntry, @hostText, x, y, text, entryXOffset)
    y += FPConstants.border + FPConstants.textEntryHeight
    @instanceLabel.place('x' => x, 'y' => y)
    entryXOffset = hostEntryOffset + ((@ingresLabel.cget("text").length / FPConstants.pixelsToCharWidth)).to_i
    text = FPConstants.emptyString
    showLabelEntryAndTextVar(@ingresLabel, @ingresEntry, @ingresText, hostEntryOffset, y, text, entryXOffset)
  end
  
  def showCredentialsFrame(x, y)
    @credentialsFrame.place('x' => x, 'y' => y)
    text = FPConstants.emptyString
    entryXOffset = hostEntryOffset
    x = FPConstants.border
    y = FPConstants.border
    showLabelEntryAndTextVar(@userLabel, @userEntry, @userText, x, y, text, entryXOffset)
    y += FPConstants.border + FPConstants.textEntryHeight
    text = FPConstants.emptyString
    showLabelEntryAndTextVar(@passwordLabel, @passwordEntry, @passwordText, x, y, text, entryXOffset)
    @passwordEntry.show("*")
  end

  def showDatabaseFrame(x, y)
    @databaseFrame.place('x' => x, 'y' => y)
    x = FPConstants.border
    y = FPConstants.border
    @databaseEntry.place('x' => x, 'y' => y)
  end

  def showBottomFrame
    @bottomFrame.show
  end
  
  private 

  def reconfigureElements(state)
    @hostLabel.configure('state' => state)
    @hostEntry.configure('state' => state)
    @instanceLabel.configure('state' => state)
    @ingresLabel.configure('state' => state)
    @ingresEntry.configure('state' => state)
    @userLabel.configure('state' => state)
    @userEntry.configure('state' => state)
    @passwordLabel.configure('state' => state)
    @passwordEntry.configure('state' => state)
    @databaseEntry.configure('state' => state)
    @changeButton.configure('state' => state)
    @resetButton.configure('state' => state)
    @makeDefinedBox.configure('state' => state)
  end

  def defaultSourceHandler
    reconfigureElements(FPConstants.widgetInactiveState)
    @defaultRadio.select
  end

  def definedSourceHandler
    reconfigureElements(FPConstants.widgetActiveState)
    @definedRadio.select
  end

  def changeSourceHandler
    var = @makeDefinedBox.cget('variable')
    @dbconn.setConnectionInfo(@hostText.value, @databaseText.value, @ingresText.value, @userText.value, @passwordText.value)
    if (var.value == "1") then
      @dbconn.saveConnectionInfo()
      defaultSourceHandler
      var.value = "0"
    end
    resetSourceHandler
  end

  def resetSourceHandler
    @passwordText.value = String.new(@dbconn.password)
    @userText.value = String.new(@dbconn.username)
    @hostText.value = String.new(@dbconn.hostname)
    @ingresText.value = String.new(@dbconn.instance)
    @databaseText.value = String.new(@dbconn.database)
  end

  def hostEntryOffset
    94
  end

  def defaultSource
    "defaultSource"
  end

  def definedSource
    "definedSource"
  end

end