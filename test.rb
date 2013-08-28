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

q = Point.new(10, -10)
# q = Point.new()

bb = AABB.new(p, q)

puts bb.inspect
puts "The size of the bounding-box is: (#{bb.half.x}, #{bb.half.y})"

bb.half = Point.new(-5, 5)

puts bb.inspect
puts "The size of the bounding-box is now: (#{bb.half.x}, #{bb.half.y})"
