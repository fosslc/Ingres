require 'tk'
require "help_about"

class MenuBar
  def createFileMenu(menuBar)
    fileMenu = TkMenu.new(menuBar, 'tearoff' => false)
    fileMenu.add('command',
                 'label' => 'Open',
                 'command' => proc { openFileHandler },
                 'underline' => 0)
    fileMenu.add('command',
                 'label' => 'Save',
                 'command' => proc { saveFileHandler },
                 'underline' => 0)
    fileMenu.add('command',
                 'label' => 'Save As',
                 'command' => proc { saveAsFileHandler },
                 'underline' => 5)
    fileMenu.add('separator')
    fileMenu.add('command',
                 'label' => 'Exit',
                 'command' => proc { exit },
                 'underline' => 1)
    menuBar.add('cascade', 
                'menu' => fileMenu, 
                'label' => "File")
  end

  def createToolsMenu(menuBar)
    toolsMenu = TkMenu.new(menuBar, 'tearoff' => false)
    toolsMenu.add('command',
                 'label' => 'Route search',
                 'command' => proc { routeSearchHandler },
                 'underline' => 0)
    toolsMenu.add('command',
                 'label' => 'My profile',
                 'command' => proc { myProfileHandler },
                 'underline' => 0)
    toolsMenu.add('separator')
    optionsMenu = TkMenu.new(toolsMenu, 'tearoff' => false)
    optionsMenu.add('command',
                    'label' => 'Connect',
                    'command' => proc { connectHandler },
                    'underline' => 1)
    toolsMenu.add('cascade', 
                'menu' => optionsMenu, 
                'label' => "Options")
    menuBar.add('cascade', 
                'menu' => toolsMenu, 
                'label' => "Tools")
  end

  def createHelpMenu(menuBar, root)
    helpMenu = TkMenu.new(menuBar, 'tearoff' => false)
    helpMenu.add('command',
                 'label' => 'Contents',
                 'command' => proc { helpContentsHandler },
                 'underline' => 0)
    helpMenu.add('separator')
    helpMenu.add('command',
                 'label' => 'About',
                 'command' => proc { helpAboutHandler(root) },
                 'underline' => 0)
    menuBar.add('cascade', 
                'menu' => helpMenu, 
                'label' => "Help")
  end

  def initialize(root)
    bar = TkMenu.new()
    createFileMenu(bar)
    createToolsMenu(bar)
    createHelpMenu(bar, root)
    root.menu(bar)
  end

  def openFileHandler
    fileToOpen = Tk.getOpenFile('defaultextension' => "xml", 'initialdir' => FPConstants.defaultDBConfigPath, 'initialfile' => FPConstants.defaultDBConfigFilename, 'filetypes' => FPConstants.dbConfigFileTypes)
    if (fileToOpen.length > 0)
      DatabaseConnection.instance.getConnectionInfo(fileToOpen)
      ConnectFrames.instance.show
    end
  end

  def saveAsFileHandler
    saveAsFile = Tk.getSaveFile('defaultextension' => "xml", 'initialdir' => FPConstants.defaultDBConfigPath, 'initialfile' => FPConstants.defaultDBConfigFilename, 'filetypes' => FPConstants.dbConfigFileTypes)
    if (saveAsFile.length > 0)
      DatabaseConnection.instance.saveConnectionInfo(saveAsFile)
    end
  end

  def saveFileHandler
    DatabaseConnection.instance.saveConnectionInfo()
  end

  def routeSearchHandler
    RoutesFrames.instance.show
  end

  def myProfileHandler
    ProfilesFrames.instance.show
  end

  def connectHandler
    ConnectFrames.instance.show
    DatabaseConnection.instance.connect
  end

  def helpContentsHandler
    buttons = [ "     OK     " ]
    msg = "Help pages not yet implemented"
    TkDialog.new('title' => "Help - Ingres Frequent Flyer", 'message' => msg, 'buttons' => buttons)
  end

  def helpAboutHandler(parent)
    HelpAboutDialog.new(parent)
  end

end
