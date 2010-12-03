require 'tk'

class IngresBar

  def initialize(root)
    height = FPConstants.ingresBarImageHeight
    width = FPConstants.ingresBarImageWidth
    ingresBar = TkFrame.new(root) { width width; height height }.place('x' => 1, 'y' => 1)
    ingresImage = TkPhotoImage.new('file' => FPConstants.ingresBannerImage)
    TkLabel.new(ingresBar) { image ingresImage; width width; height height }.pack
  end  
  
end
