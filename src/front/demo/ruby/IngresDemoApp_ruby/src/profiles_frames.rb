require 'base64'
require 'stringio'
require 'singleton'
require 'tk'
require 'tkextlib/tkimg'
require "fp_constants"
require "bordered_frame"
require "airport_frame"
require "flight_planner_frames"
require "connect_frames"
require "profiles"

class ProfilesFrames < FlightPlannerFrames

  include Singleton

  private

  def createTopFrame(parent, width)
    x = 0
    y = 0
    height = topFrameHeight
    @topFrame = createBorderedFrame(parent, width, height, x, y)
    createAboutMe(@topFrame.innerFrame)
  end

  def createBottomFrame(parent, width)
    x = 0
    y = @topFrame.outerHeight + FPConstants.border
    height = @baseFrameHeight - y - 2 # for pane outline
    @bottomFrame = createBorderedFrame(parent, width, height, x, y)
  end

  def createAboutMe(parent)
    height = parent.height - 2 * FPConstants.border
    width = parent.width - 2 * FPConstants.border
    @aboutMeFrame = createLabelFrame(parent, width, height, " About Me ")
    @personalDetailsFrame = createPersonalDetailsFrame(@aboutMeFrame, personalDetailsFrameWidth, personalDetailsFrameHeight)
    @preferencesFrame = createPreferencesFrame(@aboutMeFrame, preferencesFrameWidth, preferencesFrameHeight)
    @newButton, @modifyButton, @addButton, @removeButton, @changeButton = createButtons(@aboutMeFrame)
    @defaultProfileCheckBox = createCheckBox(@aboutMeFrame, FPConstants.widgetActiveState, "Default profile")
  end

  def createButtons(parent)
    new = createButton(parent, "New")
    modify = createButton(parent, "Modify")
    add = createButton(parent, "Add")
    remove = createButton(parent, "Remove")
    change = createButton(parent, "Change")
    new.command() { newButtonHandler }
    modify.command() { modifyButtonHandler }
    add.command() { addButtonHandler }
    remove.command() { removeButtonHandler }
    change.command() { changeButtonHandler }
    return new, modify, add, remove, change
  end

  def createPersonalDetailsFrame(parent, width, height)
    frame = createLabelFrame(parent, width, height, "Personal Details")
    @emailAddressLabel, @emailAddressEntry, @emailAddressText = createLabelEntryAndTextVar(frame, "Email Address",  detailsEntryWidth)
    @emailAddressOptionButton, @emailAddressVar = createMenuButton(parent, detailsEntryWidth - 6)
    @lastNameLabel, @lastNameEntry, @lastNameText = createLabelEntryAndTextVar(frame, "Last Name",  detailsEntryWidth)
    @firstNameLabel, @firstNameEntry, @firstNameText = createLabelEntryAndTextVar(frame, "First Name",  detailsEntryWidth)
    @photographLabel, @photographEntry, @photographText = createLabelEntryAndTextVar(frame, "Photograph",  photoEntryWidth)
    @browseButton = createButton(frame, "Browse...", 10)
    @browseButton.command() { browseButtonHandler }
    frame
  end

  def createPreferencesFrame(parent, width, height)
    frame = createLabelFrame(parent, width, height, "Preferences")
    @homeAirportFrame = AirportFrame.new(frame, "Home Airport")
    frame
  end

  def showTopFrame
    @topFrame.show
    showAboutMeFrame
    setDefaultStates
    @personalDetailsFrame.raise
    defaultProfile = DatabaseConnection.instance.getDefaultProfile(true)              #ST070523
    displayProfileDetails(defaultProfile.emailAddress)                                  #ST070523
  end

  def setDefaultStates(state = FPConstants.widgetInactiveState)
    @emailAddressLabel.configure('state' => state)
    @emailAddressEntry.configure('state' => state)
    @lastNameLabel.configure('state' => state)
    @lastNameEntry.configure('state' => state)
    @firstNameLabel.configure('state' => state)
    @firstNameEntry.configure('state' => state)
    @photographLabel.configure('state' => state)
    @photographEntry.configure('state' => state)
    @homeAirportFrame.configure(state)
    @defaultProfileCheckBox.configure('state' => state)
    @browseButton.configure('state' => state)
    @addButton.configure('state' => state)
    @removeButton.configure('state' => state)
    @changeButton.configure('state' => state)
    @lastNameText.value = FPConstants.emptyString
    @firstNameText.value = FPConstants.emptyString
    @emailAddressText.value = FPConstants.emptyString
    @emailAddressVar.value = FPConstants.defaultEmailAddressOption
  end

  def showAboutMeFrame
    @aboutMeFrame.place('x' => FPConstants.border, 'y' => FPConstants.border)
    x = FPConstants.border
    y = FPConstants.border
    showPersonalDetailsFrame(x, y)
    x += FPConstants.border + personalDetailsFrameWidth
    showPreferencesFrame(x, y)
    x += FPConstants.border + preferencesFrameWidth
    showButtons(@newButton, @modifyButton, @addButton, @removeButton, @changeButton)
    y = personalDetailsFrameHeight + 2 * FPConstants.border
    x = 0
    showCheckBox(@defaultProfileCheckBox, x, y)
  end

  def showPersonalDetailsFrame(x, y)
    @personalDetailsFrame.place('x' => x, 'y' => y)
    text = nil
    entryXOffset = detailsEntryOffset
    showMenuButton(@emailAddressOptionButton, x + entryXOffset, y + (3 * FPConstants.border) + 2)
    x = FPConstants.border
    y = FPConstants.border
    showLabelEntryAndTextVar(@emailAddressLabel, @emailAddressEntry, @emailAddressText, x, y, text, entryXOffset)
    @emailAddressOptionButton.lower
    y += FPConstants.border + FPConstants.textEntryHeight
    showLabelEntryAndTextVar(@lastNameLabel, @lastNameEntry, @lastNameText, x, y, text, entryXOffset)
    y += FPConstants.border + FPConstants.textEntryHeight
    showLabelEntryAndTextVar(@firstNameLabel, @firstNameEntry, @firstNameText, x, y, text, entryXOffset)
    y += FPConstants.border + FPConstants.textEntryHeight + 2
    showLabelEntryAndTextVar(@photographLabel, @photographEntry, @photographText, x, y, text, entryXOffset)
    y -= 2
    browseXOffset = entryXOffset + (@photographEntry.width / FPConstants.pixelsToCharWidth).to_i + FPConstants.border + 6
    @browseButton.place('x' => browseXOffset, 'y' => y)
  end
  
  def showButtons(new, modify, add, remove, change)
    y = 2 * FPConstants.border
    x = personalDetailsFrameWidth + preferencesFrameWidth + 3 * FPConstants.border
    new.place('x' => x, 'y' => y)
    y += FPConstants.buttonHeight + FPConstants.border
    modify.place('x' => x, 'y' => y)
    x =@aboutMeFrame.width - 3 * FPConstants.buttonWidth - 4 * FPConstants.border
    y = personalDetailsFrameHeight + 3 * FPConstants.border + FPConstants.charToPixelsHeight
    add.place('x' => x, 'y' => y)
    x += FPConstants.buttonWidth + FPConstants.border
    remove.place('x' => x, 'y' => y)
    x += FPConstants.buttonWidth + FPConstants.border
    change.place('x' => x, 'y' => y)
  end

  def showPreferencesFrame(x, y)
    @preferencesFrame.place('x' => x, 'y' => y)
    x = FPConstants.border
    @homeAirportFrame.show(x)
  end

  def showBottomFrame
#    @tmpFrame.destroy if (@tmpFrame) #BPK070523
    @bottomFrame.show
  end
  
  private 

  def browseButtonHandler
    fileToOpen = Tk.getOpenFile('defaultextension' => "jpeg", 'filetypes' => FPConstants.photographFileTypes)
    if (fileToOpen.length > 0)
      @photographText.value = fileToOpen
    end
  end

  def newButtonHandler
    setDefaultStates(FPConstants.widgetActiveState)
    @personalDetailsFrame.raise
    @removeButton.configure('state' => FPConstants.widgetInactiveState)
    @changeButton.configure('state' => FPConstants.widgetInactiveState)
    @photoData = nil
  end

  def modifyButtonHandler
    setDefaultStates(FPConstants.widgetActiveState)
    @emailAddressOptionButton.raise
    @addButton.configure('state' => FPConstants.widgetInactiveState)
    dbconnection = DatabaseConnection.instance
    @emailAddressVar.value = FPConstants.defaultEmailAddressOption
    emailAddresses = dbconnection.getEmailAddresses()
    menu = TkMenu.new(@emailAddressOptionButton, 'tearoff' => false)
    menu.add('command', 'label' => FPConstants.defaultEmailAddressOption, 'command' => proc { displayProfileDetails })
    emailAddresses.each { |emailAddress| menu.add('command', 'label' => emailAddress.to_s, 'command' => proc { displayProfileDetails(emailAddress.to_s) }) }
    @emailAddressOptionButton.configure('menu' => menu)
  end

  def profileOK(emailAddress)
    returnValue = true
    msg = "First name" if (@firstNameText.value == FPConstants.emptyString)
    msg = "Last name" if (@lastNameText.value == FPConstants.emptyString)
    msg = "Email address" if (emailAddress == FPConstants.emptyString || emailAddress == FPConstants.defaultEmailAddressOption)
    msg += " is a required entry" if (msg)
    msg = "Home airport missing" if (@homeAirportFrame.getAirport == FPConstants.emptyString && !msg)
    msg = "Invalid picture file" if (!msg && !testPhoto(@photographText.value))
    if (msg) then
      buttons = [ "     OK     " ]
      TkDialog.new('title' => "Profiles", 'message' => msg, 'buttons' => buttons)
      returnValue = false
    end
    return returnValue
  end

  def testPhoto(photoFileName)
    return true if (photoFileName.length == 0)
    return false if (!File.file?(photoFileName))
    photoFile = File.open(photoFileName, "rb")
    @photoData = photoFile.read
    photoFile.close
    photoImage = TkPhotoImage.new('data' => Base64.encode64(@photoData))
    return false if (photoImage.height == 0 && photoImage.width == 0)
    return true
  end

  def addButtonHandler
    if (profileOK(@emailAddressText.value)) then
      isDefault = (@defaultProfileCheckBox.cget('variable').value == "0") ? false : true
      DatabaseConnection.instance.insertProfile(@lastNameText.value, @firstNameText.value, @homeAirportFrame.getAirport, @emailAddressText.value, isDefault, @photoData)
      #  displayProfileDetails(@emailAddressVar.value)                                                  #ST070523
      displayProfileDetails(@emailAddressText.value)                                 #ST070523
    end
  end

  def removeButtonHandler
    deleteOk = false
    if (@emailAddressVar.value != FPConstants.defaultEmailAddressOption) then
      buttons = [ "     Yes     ", "     No     " ]
      msg = "Profile " + @emailAddressVar.value
    dialogResult = TkDialog.new('title' => "Remove Profile", 'message' => msg, 'buttons' => buttons)
      deleteOk = (dialogResult.value == 0) ? true : false
    end
    if (deleteOk) then
      DatabaseConnection.instance.deleteProfile(@emailAddressVar.value)
    end
  end

  def changeButtonHandler
    if (profileOK(@emailAddressVar.value)) then
      isDefault = (@defaultProfileCheckBox.cget('variable').value == "0") ? false : true
      DatabaseConnection.instance.updateProfile(@lastNameText.value, @firstNameText.value, @homeAirportFrame.getAirport, @emailAddressVar.value, isDefault, @photoData)
      displayProfileDetails(@emailAddressVar.value)
    end
  end

  def displayProfileDetails(emailAddress = nil)
    @tmpFrame.destroy if @tmpFrame
    if (emailAddress) then
      @emailAddressVar.value = emailAddress
      @emailAddressText.value = emailAddress                                                              #ST070523
      @profile = DatabaseConnection.instance.getProfile(emailAddress)
      @lastNameText.value = String.new(@profile.lastname)
      @firstNameText.value = String.new(@profile.firstname)
      @homeAirportFrame.configure(FPConstants.widgetActiveState, String.new(@profile.iataCode))
      parent = @bottomFrame.innerFrame
      height = parent.cget('height') - 2 * FPConstants.border
      width = parent.cget('width') - 2 * FPConstants.border
      @tmpFrame = createLabelFrame(parent, width, height, "Profile Result")
      @tmpFrame.place('x' => FPConstants.border, 'y' => FPConstants.border)
      parent = @tmpFrame
      labels = [ [ "Welcome " + @firstNameText.value, "bold", 40, 40 ],
                 [ "Email Address", "normal", 200, 75 ],
                 [ emailAddress, "bold", 375, 75 ],
                 [ "Airport Preference", "normal", 200, 100 ],
                 [ String.new(@profile.iataCode), "bold", 375, 100 ] ]
      labels.each { | text, style, x, y | TkLabel.new(parent){ text text ; font "Tahoma 14 " + style }.place('x' => x, 'y' => y) }
      @photoData = @profile.profileImage
      photoImage = TkPhotoImage.new('data' => Base64.encode64(@profile.profileImage))
      TkLabel.new(parent){ image photoImage ; height 160 ; width 130 }.place('x' => 40, 'y' => 75)
      photoImage = ""
    else
      @emailAddressVar.value = FPConstants.defaultEmailAddressOption
      @lastNameText.value = FPConstants.emptyString
      @firstNameText.value = FPConstants.emptyString
      @homeAirportFrame.configure()
    end
  end

  def personalDetailsFrameWidth
    width = 3 * FPConstants.border + detailsEntryOffset + (detailsEntryWidth / FPConstants.pixelsToCharWidth) + 2
    width = width.to_i
    width
  end

  def personalDetailsFrameHeight
    height = 5 * (FPConstants.charToPixelsHeight + FPConstants.border) + FPConstants.charToPixelsHeight
    height = height.to_i
    height
  end

  def preferencesFrameWidth
    FPConstants.airportFrameWidth + 3 * FPConstants.border
  end

  def preferencesFrameHeight
    FPConstants.airportFrameHeight + FPConstants.charToPixelsHeight + 2 * FPConstants.border
  end

  def topFrameHeight
    if (preferencesFrameHeight > personalDetailsFrameHeight) then
      height = preferencesFrameHeight
    else
      height = personalDetailsFrameHeight
    end
    height += 6 * FPConstants.border + 2 * FPConstants.charToPixelsHeight + FPConstants.buttonHeight
    height
  end
  
  def detailsEntryWidth
    36
  end
  
  def photoEntryWidth
    detailsEntryWidth - 13
  end

  def detailsEntryOffset
    90
  end

end
