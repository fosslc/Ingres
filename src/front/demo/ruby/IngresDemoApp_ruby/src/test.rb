unicodeTest = true
if (unicodeTest)
  require "database_connection"
  $KCODE="UTF8"
  $-K = "u"
  require 'jcode'
  require 'tcltklib'

  def processLast(dbconnection, emailAddress)
  sql = "SELECT up_last FROM user_profile WHERE up_email = ?"
  results = dbconnection.execute(sql, "v", emailAddress)
#  puts results
  lastname = ""
  results.each { | result | lastname = result[0] }
  lastname.each_byte { | c | print c, ' ' }
  puts "last name is '" + lastname + "}'"
  return lastname
  end

  dbconnection = DatabaseConnection.instance
  dbconnection.connect
  dbconnection.setDebugFlag("DEBUG_UNICODE", "TRUE")
  emailAddress = "ac.fan@cool-hvac.net"
  lastname = processLast(dbconnection, emailAddress)
  emailAddress = "b.k@i"
  sql = "UPDATE user_profile SET up_last = ? WHERE up_email = ?"
  results = dbconnection.execute(sql, "N", lastname, "v", emailAddress)
  lastname = processLast(dbconnection, emailAddress)
  dbconnection.disconnect
end


unicodeTest1 = false
if (unicodeTest1)
  require "database_connection"
  $KCODE="UTF8"
  $-K = "u"
  require 'jcode'
  require 'tcltklib'

  dbconnection = DatabaseConnection.instance
  dbconnection.connect
  dbconnection.setDebugFlag("GLOBAL_DEBUG", "TRUE")
  emailAddress = "ac.fan@cool-hvac.net"
  sql = "SELECT up_last, up_first FROM user_profile WHERE up_email = ?"
  results = dbconnection.execute(sql, "v", emailAddress)
  dbconnection.setDebugFlag("GLOBAL_DEBUG", "FALSE")
  puts results
  lastname = ""
  firstname = ""
  results.each { | result | lastname = result[0] ; firstname = result[1] }
  lastname.each_byte { | c | print c, ' ' }
  TclTkLib.encoding_system='utf-8'
  puts "last name is '" + lastname + "}'"
  firstname.each_byte { | c | print c, ' ' }
  puts "first name is '" + firstname + "'"
  defaultEncoding = TclTkLib.encoding_system
  TclTkLib.encoding_system = 'utf-8'
  utfLast = TclTkLib._toUTF8(lastname)
  TclTkLib.encoding_system = defaultEncoding
  defLast = TclTkLib._fromUTF8(utfLast)
  puts utfLast.unpack('U')
  puts defLast.unpack('CCC')
  dbconnection.disconnect
end


profileTest = false
if (profileTest)
  require "profiles"
  require "database_connection"
  dbconnection = DatabaseConnection.instance
  dbconnection.connect
#  dbconnection.setDebugFlag("GLOBAL_DEBUG", "TRUE")
#  results = dbconnection.execute("SELECT up_last, up_first, up_email, up_airport FROM user_profile WHERE up_email = ?", "n", "bobby.kent@ingres.com")
#  profile = dbconnection.getProfile("bobby.kent@ingres.com")
  photoFile = File.open("C:/Bobby.jpg", "rb")
  photoData = photoFile.read
  photoFile.close
  dbconnection.insertProfile("last", "first", "JFK", "email@test", false, photoData)
  profile = dbconnection.getProfile("email@test")
#  dbconnection.deleteProfile("email@test")
  puts photoData.length
  puts profile.profileImage.length
  dbconnection.disconnect
#  puts results
end

routeTest = false
if (routeTest)
  require "routes"
  route = Route.new("wibble")
  puts route.airline
  puts route.iataCode
end

driverTest = false
if (driverTest)
  require "Ingres"
  $KCODE="UTF8"
  $-K = "u"
  require 'jcode'
  
  def runtest(ing, globalDebug, database = "demodb")
    ing.set_debug_flag("GLOBAL_DEBUG", globalDebug)
    begin
    ing.connect_with_credentials("@demodata,tcp_ip,II[ingres,ingres]::" + database,"ingres","ingres")
    result_set = ing.execute("SELECT ct_name, ct_code FROM country ORDER BY ct_name")
    puts result_set
  
#    ing.set_debug_flag("GLOBAL_DEBUG", "FALSE")
    rescue Exception
    end
  
    ing.disconnect
  end
  
  ing = Ingres.new
  #runtest(ing, "FALSE")
  runtest(ing, "TRUE", "demo")
end

dbTest = false
if (dbTest)
  require "database_connection"
  $KCODE="UTF8"
  $-K = "u"
  require 'jcode'
  
  def dtToS(dateTime)
    result = String.new
  #  result += dateTime
  end
  
  dbconnection = DatabaseConnection.instance
  #puts dbconnection.countries()
  #puts dbconnection.regions("AUSTRALIA")
  #dbconnection.setDebugFlag("GLOBAL_DEBUG", "TRUE")
  #puts dbconnection.airports("AUSTRALIA", "Sydney")
  #dbconnection.setDebugFlag("GLOBAL_DEBUG", "FALSE")
  #dbconnection.disconnect()
  #routes = dbconnection.getRoutes("JFK", "LHR", [], false)
  #routes.each { | route | puts route.airline + ", " + route.iataCode + ", " + route.departAt.asctime + ", " + route.arriveAt.asctime }
  puts dbconnection.database
  dbconnection.setDefaults
  dbconnection.getDefaults
  puts dbconnection.database
end

tkTest = false
if (tkTest)
  require 'tk'
  
  TkLabel.new(:text=>'Sample of TkMenubutton').pack(:side=>:top)
  
  TkFrame.new{|f|
    pack(:side=>:top) 
  
  
    TkMenubutton.new(:parent=>f, :text=>'Right', :underline=>0, 
         :direction=>:right, :relief=>:raised){|mb|
      menu TkMenu.new(:parent=>mb, :tearoff=>0){
        add(:command, :label=>'Right menu: first item', 
                      :command=>proc{print 'You have selected the first item' + 
                                           " from the Right menu.\n"})
        add(:command, :label=>'Right menu: second item', 
                      :command=>proc{print 'You have selected the second item' + 
                                           " from the Right menu.\n"})
      }
      pack(:side=>:left, :padx=>25, :pady=>25)
    }
  
    TkMenubutton.new(:parent=>f, :text=>'Below', :underline=>0, 
         :direction=>:below, :relief=>:raised){|mb|
      menu(TkMenu.new(:parent=>mb, :tearoff=>0){
        add(:command, :label=>'Below menu: first item', 
                      :command=>proc{print 'You have selected the first item' + 
                                           " from the Below menu.\n"})
        add(:command, :label=>'Below menu: second item', 
                      :command=>proc{print 'You have selected the second item' + 
                                           " from the Below menu.\n"})
      })
      pack(:side=>:left, :padx=>25, :pady=>25)
    }
  
    TkMenubutton.new(:parent=>f, :text=>'Above', :underline=>0, 
         :direction=>:above, :relief=>:raised){|mb|
      menu TkMenu.new(:parent=>mb, :tearoff=>0){
        add(:command, :label=>'Above menu: first item', 
                      :command=>proc{print 'You have selected the first item' + 
                                           " from the Above menu.\n"})
        add(:command, :label=>'Above menu: second item', 
                      :command=>proc{print 'You have selected the second item' + 
                                           " from the Above menu.\n"})
      }
      pack(:side=>:left, :padx=>25, :pady=>25)
    }
  
    TkMenubutton.new(:parent=>f, :text=>'Left', :underline=>0, 
         :direction=>:left, :relief=>:raised){|mb|
      menu(TkMenu.new(:parent=>mb, :tearoff=>0){
        add(:command, :label=>'Left menu: first item', 
                      :command=>proc{print 'You have selected the first item' + 
                                           " from the Left menu.\n"})
        add(:command, :label=>'Left menu: second item', 
                      :command=>proc{print 'You have selected the second item' + 
                                           " from the Left menu.\n"})
      })
      pack(:side=>:left, :padx=>25, :pady=>25)
    }
  }
  
  ############################
  TkFrame.new(:borderwidth=>2, :relief=>:sunken, 
        :height=>5).pack(:side=>:top, :fill=>:x, :padx=>20)
  ############################
  
  TkLabel.new(:text=>'Sample of TkOptionMenu').pack(:side=>:top)
  
  colors = %w(Black red4 DarkGreen NavyBlue gray75 Red Green Blue gray50 
              Yellow Cyan Magenta White Brown DarkSeaGreen DarkViolet)
  
  TkFrame.new{|f|
    pack(:side=>:top) 
  
    b1 = TkOptionMenubutton . 
      new(:parent=>f, :values=>%w(one two three)) . 
      pack(:side=>:left, :padx=>25, :pady=>25)
  
    b2 = TkOptionMenubutton.new(:parent=>f, :values=>colors) {|optMB|
      colors.each{|color|
        no_sel = TkPhotoImage.new(:height=>16, :width=>16){
    put 'gray50', *[ 0,  0, 16,  1]
    put 'gray50', *[ 0,  1,  1, 16]
    put 'gray75', *[ 0, 15, 16, 16]
    put 'gray75', *[15,  1, 16, 16]
    put color,    *[ 1,  1, 15, 15]
        }
        sel = TkPhotoImage.new(:height=>16, :width=>16){
    put 'Black',  *[ 0,  0, 16,  2]
    put 'Black',  *[ 0,  2,  2, 16]
    put 'Black',  *[ 2, 14, 16, 16]
    put 'Black',  *[14,  2, 16, 14]
    put color,    *[ 2,  2, 14, 14]
        }
        optMB.entryconfigure(color, :hidemargin=>1, 
           :image=>no_sel, :selectimage=>sel)
      }
      optMB.menuconfigure(:tearoff, 1)
      %w(Black gray75 gray50 White).each{|color|
        optMB.entryconfigure(color, :columnbreak=>true)
      }
      pack(:side=>:left, :padx=>25, :pady=>25)
    }
  
    TkButton.new(:parent=>f){
      text 'show values'
      command proc{p [b1.value, b2.value]}
      pack(:side=>:left, :padx=>25, :pady=>5, :anchor=>:s)
    }
  }
  
  
  ############################
  TkFrame.new(:borderwidth=>2, :relief=>:sunken, 
        :height=>5).pack(:side=>:top, :fill=>:x, :padx=>20)
  ############################
  
  root = TkRoot.new(:title=>'menubutton samples')
  
  TkButton.new(root, :text=>'exit', :command=>proc{exit}){
    pack(:side=>:top, :padx=>25, :pady=>5, :anchor=>:e)
  }
  
  # VirtualEvent <<MenuSelect>> on Tcl/Tk ==> '<MenuSelect>' on Ruby/Tk
  # ( remove the most external <, > for Ruby/Tk notation )
  TkMenu.bind('<MenuSelect>', proc{|widget|
          p widget.entrycget('active', :label)
        }, '%W')
  
  ############################
  
  Tk.mainloop
end