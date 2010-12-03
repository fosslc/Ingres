require 'tk'
require "fp_constants"

class BorderedFrame

  def initialize(parent, width, height, x, y)
    @outerWidth = width
    @outerHeight = height
    @outerParent = parent
    @outerX = x
    @outerY = y
    @innerX = 1
    @innerY = 1
    @outerFrame = TkFrame.new(parent) { width width; height height }
    width = innerWidth
    height = innerHeight
    @innerFrame = TkFrame.new(@outerFrame) { width width; height height }
    @innerFrame
  end
  
  def show
    @outerFrame.place('x' => @outerX, 'y' => @outerY)
    @outerFrame.configure('background' => FPConstants.borderColor)
    @innerFrame.place('x' => @innerX, 'y' => @innerY)
    @outerFrame.raise
  end

  def innerWidth
    @outerWidth - 2
  end

  def innerHeight
    @outerHeight - 2
  end

  attr_reader :innerFrame, :outerHeight

end
