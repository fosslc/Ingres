require 'tk'
require "fp_constants"
require "routes_frames"
require "connect_frames"
require "profiles_frames"

class ButtonBar

  def createNewButton(root, imageFile, y)
    button = TkButton.new(root) do
      image TkPhotoImage.new('file' => imageFile)
      width FPConstants.buttonBarImageSize
      height FPConstants.buttonBarImageSize 
    end

    button.place('x' => FPConstants.border, 'y' => y)
    nextButtonY = y + FPConstants.buttonBarImageSize + FPConstants.buttonBarImageBorder + FPConstants.border
    return button, nextButtonY
  end

  def initialize(root)
    yOffset = FPConstants.ingresBarBottomEdge

    routesButton, yOffset = createNewButton(root, FPConstants.routesButtonImage, yOffset)
    profileButton, yOffset = createNewButton(root, FPConstants.profileButtonImage, yOffset)
    connectButton, yOffset = createNewButton(root, FPConstants.connectButtonImage, yOffset)
    exitButton, yOffset = createNewButton(root, FPConstants.exitButtonImage, yOffset)

    routesButton.command() { RoutesFrames.instance.show }
    profileButton.command() { ProfilesFrames.instance.show }
    connectButton.command() { ConnectFrames.instance.show }
    exitButton.command() { exit }
  end

end
