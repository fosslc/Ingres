require 'tk'
require "fp_constants"
require "flight_planner_frames"

class HelpAboutDialog < FlightPlannerFrames

	def initialize(parent)
		helpAboutDialog = TkToplevel.new(parent) do
			title "About Ingres Frequent Flyer"
			height FPConstants.helpAboutHeight
			width FPConstants.helpAboutWidth
		end
    helpAboutDialog.wait_visibility
    helpAboutDialog.grab_set
    helpAboutDialog.transient(parent)
    helpAboutDialog.withdraw
    helpAboutDialog.deiconify
    ingresLogo = TkFrame.new(helpAboutDialog)
    ingresLogoImage = TkPhotoImage.new('file' => FPConstants.ingresLogoImage)
    TkLabel.new(ingresLogo) { image ingresLogoImage; width FPConstants.ingresLogoImageWidth; height FPConstants.ingresLogoImageHeight }.pack
    ingresLogo.place('x' => FPConstants.helpAboutBorder, 'y' => FPConstants.helpAboutBorder)
    xOffset = 2 * FPConstants.helpAboutBorder + FPConstants.ingresLogoImageWidth
    yOffset = FPConstants.helpAboutBorder
    TkLabel.new(helpAboutDialog) { text "IngresDemoApp" }.place('x' => xOffset, 'y' => yOffset)
    yOffset += (1.5 * FPConstants.charToPixelsHeight).to_i
    TkLabel.new(helpAboutDialog) { text "Version 1.0.0.0" }.place('x' => xOffset, 'y' => yOffset)
    yOffset += (1.5 * FPConstants.charToPixelsHeight).to_i
    TkLabel.new(helpAboutDialog) { text "Copyright © 2006" }.place('x' => xOffset, 'y' => yOffset)
    yOffset += (1.5 * FPConstants.charToPixelsHeight).to_i
    TkLabel.new(helpAboutDialog) { text "Ingres Corporation" }.place('x' => xOffset, 'y' => yOffset)
    yOffset += (1.5 * FPConstants.charToPixelsHeight).to_i
    borderedFrameWidth = FPConstants.helpAboutWidth - (3 * FPConstants.helpAboutBorder + FPConstants.ingresLogoImageWidth)
    borderedFrameHeight = FPConstants.helpAboutHeight - (yOffset + FPConstants.buttonHeight + 2 * FPConstants.helpAboutBorder)
    borderedFrame = createBorderedFrame(helpAboutDialog, borderedFrameWidth, borderedFrameHeight, xOffset, yOffset)
    TkLabel.new(borderedFrame.innerFrame) { text "Ingres Frequent Flyer Demonstration Application" }.place('x' => 1, 'y' => 1)
    TkScrollbar.new(borderedFrame.innerFrame).place('relheight' => 1, 'relx' => 1, 'rely' => 0,'anchor' => "ne")
    borderedFrame.show
    okButton = createButton(helpAboutDialog, "OK")
    xOffset = FPConstants.helpAboutWidth - (FPConstants.buttonWidth + FPConstants.helpAboutBorder)
    yOffset = FPConstants.helpAboutHeight - (FPConstants.buttonHeight + FPConstants.helpAboutBorder)
    okButton.place('x' => xOffset, 'y' =>yOffset)
    okButton.command() { helpAboutDialog.destroy }
  end

end
