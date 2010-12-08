require 'parsedate'
require 'date'
require "fp_constants"

class Route
  
  attr_reader :airline, :iataCode, :name, :flightNum, :departFrom, :arriveTo, :departAt, :arriveAt, :arriveOffset, :flightDays

  def initialize(ary)
    @airline = ary[0] if ary.length > 0
    @iataCode = ary[1] if ary.length > 1
    @name = ary[2] if ary.length > 2
    @flightNum = (ary[3]).to_i if ary.length > 3
    @departFrom = ary[4] if ary.length > 4
    @arriveTo = ary[5] if ary.length > 5
    departAtSrc = ary[6] if ary.length > 6
    arriveAtSrc = ary[7] if ary.length > 7
    @arriveOffset = (ary[8]).to_i if ary.length > 8
    @flightDays = ary[9] if ary.length > 9
    @departAt = DateTime.strptime(departAtSrc, FPConstants.timeFmtStr) if (departAtSrc)
    @arriveAt = DateTime.strptime(arriveAtSrc, FPConstants.timeFmtStr) if (arriveAtSrc)
  end

end