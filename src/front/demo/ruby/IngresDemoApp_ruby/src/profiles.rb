class Profile
  
  attr :lastname, 'writable'
  attr :firstname, 'writable'
  attr :iataCode, 'writable'
  attr :profileImage, 'writable'
  attr :emailAddress, 'writable'

  def initialize(ary)
    self.lastname = ary[0] if ary.length > 0
    self.firstname = ary[1] if ary.length > 1
    self.emailAddress = ary[2] if ary.length > 2
    self.iataCode = ary[3] if ary.length > 3
    self.profileImage = ary[4] if ary.length > 4
  end

end