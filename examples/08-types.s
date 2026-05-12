// 08-types.s -- types, methods, and inheritance
//
// Run:  symta -f examples/08-types.s

// Declare a type with two fields. `type point x y` creates the
// `point` constructor and registers `is_point`.
type point x y
@as_text = "([$x], [$y])"     // method on point: how to print

// Methods written as `@name = ...` attach to the most recent type.
// `$x` is sugar for `Me.x` -- the field of the receiving object.
@distance_from_origin = ($x*$x + $y*$y).float.sqrt

P point 3 4
say P
say "[P] is [P.distance_from_origin] units from origin"
say "is_point? [P.is_point]"

// Mutate a field.
P.x = 6
P.y = 8
say "moved to [P], distance now [P.distance_from_origin]"

// A second type with explicit constructor body and a derived
// method. `type circle X Y R: x!X y!Y r!R` takes 3 args.
type circle X Y R: x!X y!Y r!R
@as_text = "circle@([$x],[$y]) r=[$r]"
@area = $r * $r * 3.14159265

C circle 0 0 5
say C
say "area: [C.area]"

// Inheritance via prototype: `type tagged.$base ...` makes the
// new type inherit methods from whatever value `$base` holds.
type tagged.$base X Y T: base!point(X Y) tag!T
@tag_text = "<[$tag]>"

T tagged 7 8 origin
say "T.tag      = [T.tag]"
say "T.tag_text = [T.tag_text]"
say "is_point?    [T.is_point]"
