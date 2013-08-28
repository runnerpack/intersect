# TODO: Add real unit tests
require './intersect'

include Intersect

p = Point.new

puts p.inspect

p.x = 123
p.y = 456

puts p.inspect
puts p.length
puts p.inspect
