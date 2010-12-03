require 'singleton'
require 'tk'
require "fp_constants"

class StatusBar

  include Singleton

  def setStatusBarText(host = FPConstants.emptyString, instance = FPConstants.emptyString, database = FPConstants.emptyString, user = FPConstants.emptyString)
    @statusBar.destroy if @statusBar
    @statusBar = TkFrame.new(@root) { width FPConstants.statusBarWidth; height FPConstants.statusBarHeight }
    @statusBar.place('x' => FPConstants.border, 'y' => FPConstants.buttonBarBottomEdge)
    setStatusBarItem(@statusBar, "    User:", user)
    setStatusBarItem(@statusBar, "    Database:", database)
    setStatusBarItem(@statusBar, "    Instance:", instance)
    setStatusBarItem(@statusBar, "Host:", host)
  end

  def show(root)
    @root = root
    setStatusBarText()
  end
  
  private

  def setStatusBarItem(parent, labelText, detailText)
    detailLabel = TkLabel.new(parent) { text detailText ; font FPConstants.defaultBoldFont }.pack('side' => "right", 'anchor' => "w")
    labelLabel = TkLabel.new(parent) { text labelText ; font FPConstants.defaultFont }.pack('side' => "right", 'anchor' => "e")
    return labelLabel, detailLabel
  end

end
