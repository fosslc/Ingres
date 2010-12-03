require "fp_constants"

class FlightPlannerFrames

  def create(parent)
    height = baseFrameHeight
    width = baseFrameWidth
    @baseFrame = TkFrame.new(parent) { width width ; height height }
    @baseFrameHeight = height
    @baseFrameWidth = width
    createTopFrame(@baseFrame, width)
    createBottomFrame(@baseFrame, width)
  end

  def baseFrameWidth
    FPConstants.flightPlannerWindowWidth - FPConstants.buttonBarSideEdge - FPConstants.border
  end

  def baseFrameHeight
    FPConstants.buttonBarBottomEdge - FPConstants.ingresBarBottomEdge
  end

  def show
    @baseFrame.place('x' => FPConstants.buttonBarSideEdge, 'y' => FPConstants.ingresBarBottomEdge)
    @baseFrame.raise
    showTopFrame
    showBottomFrame
  end

  protected

  def createBorderedFrame(parent, width, height, x, y)
    borderedFrame = BorderedFrame.new(parent, width, height, x, y)
    return borderedFrame
  end

  def createLabelFrame(parent, width, height, text)
    TkLabelFrame.new(parent) { width width; height height; text text ; font FPConstants.defaultFont }
  end

  def createCheckBox(parent, state, text)
    checkBox = TkCheckButton.new(parent){state state; text text ; font FPConstants.defaultFont }
    return checkBox
  end

  def createLabelEntryAndTextVar(parent, labelText, entryWidth, height = FPConstants.defaultLabelHeight, anchor = FPConstants.defaultLabelAnchor)
    label = TkLabel.new(parent){ text labelText ; font FPConstants.defaultFont }
    text = TkVariable.new
    entry = TkEntry.new(parent, 'textvariable' => text){ width entryWidth ; font FPConstants.defaultFont }
    return label, entry, text
  end

  def createRadioButton(parent, state, text, value = nil, variable = nil)
    radioButton = TkRadioButton.new(parent){ text text ; font FPConstants.defaultFont }
    radioButton.configure('value' => value) if value != nil
    radioButton.configure('variable' => variable) if variable != nil
    return radioButton
  end

  def createButton(parent, text, width = FPConstants.defaultButtonWidth)
    TkButton.new(parent){ width width; text text ; font FPConstants.defaultFont }
  end

  def createMenuButton(parent, width, height = FPConstants.defaultLabelHeight, anchor = FPConstants.defaultLabelAnchor)
    menuButton = TkOptionMenuButton.new(parent) do 
      anchor anchor 
      width width 
      height height 
      font FPConstants.defaultFont
      background FPConstants.widgetActiveBackground
      activebackground FPConstants.widgetActiveBackground 
    end
    var = menuButton.cget('textvariable')
    return menuButton, var
  end

  def createLabelAndMenuButton(parent, text, width, height = FPConstants.defaultLabelHeight, anchor = FPConstants.defaultLabelAnchor)
    menuButton, var = createMenuButton(parent, width, height, anchor)
    label = TkLabel.new(parent){ text text ; font FPConstants.defaultFont }
    return label, var, menuButton
  end

  def showMenuButton(button, x, y, state = FPConstants.widgetActiveState, background = FPConstants.widgetActiveBackground)
    button.place('x' => x, 'y' => y)
    button.configure('state' => state, 'background' => background)
  end

  def showLabelAndMenuButton(label, button, labelX, labelY, buttonX, buttonY, state = FPConstants.widgetActiveState, background = FPConstants.widgetActiveBackground)
    label.place('x' => labelX, 'y' => labelY)
    showMenuButton(button, buttonX, buttonY, state, background)
  end
  
  def showLabelEntryAndTextVar(label, entry, text, labelX, labelY, entryText, entryX, entryY = nil)
    entryY = labelY if entryY == nil
    label.place('x' => labelX, 'y' => labelY)
    entry.place('x' => entryX, 'y' => entryY)
    text.value = entryText
  end

  def showCheckBox(checkBox, x, y)
    checkBox.deselect
    checkBox.place('x' => x, 'y' => y)
  end

  def showRadioButton(radioButton, x, y)
    radioButton.deselect
    radioButton.place('x' => x, 'y' => y)
  end

end