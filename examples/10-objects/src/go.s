// 10-objects -- type, cls, and the cph phase system
//
// Three flavors of object orientation in Symta:
//   * `type` -- classic single-inheritance OOP, like C++/Java
//   * `cls`  -- entity-component-system (Symta's name: id-part-system)
//   * `cph`  -- phased systems that run over entities by part
//
// Run:  symta examples/10-objects && examples/10-objects/go.exe

use cls

// ---------------------------------------------------------------
// Part 1: classic `type`. Single-inheritance, vtables under the hood.
// ---------------------------------------------------------------
type point x y
@as_text = "([$x],[$y])"
@distance_from_origin = ($x*$x + $y*$y).float.sqrt

type circle.point X Y R: x!X y!Y r!R     // inherits methods of point
@as_text = "circle@([$x],[$y]) r=[$r]"
@area = $r*$r * 3.14159265

P point 3 4
C circle 0 0 5
say "[P]  is_point=[P.is_point]  d=[P.distance_from_origin]"
say "[C]  is_point=[C.is_point]  area=[C.area]"
say ""


// ---------------------------------------------------------------
// Part 2: `cls`. Entity-Component-System. An "entity" is just an
// integer id; "parts" are columns in a separate per-part table.
// `cls foo a b c` says "anything with parts a,b,c is a foo".
// ---------------------------------------------------------------
cls person name age
@as_text = "[$name] ([$age])"
@is_minor = $age < 18

cls coords xc yc                    // a separate kind of part

P1 person \Nancy 37
P2 person \Bernd 26
P3 person \Junior 12

// rig adds a different part to an existing entity
P1.coords_.xc = 10
P1.coords_.yc = 20

say "all persons:"
for X each(person.name): say "  [X]"

say "minors:"
for X each(person.name):
  when X.is_minor: say "  [X]"

say "all entities with coords:"
for X each(coords.xc): say "  id [X]: ([X.coords_.xc],[X.coords_.yc])"

say ""


// ---------------------------------------------------------------
// Part 3: `cph` -- columnar phase. Register a body that runs
// once per entity carrying a named part, when `cls_run_phase` is
// called for that phase. This is the ECS systems pattern.
// ---------------------------------------------------------------
cph birthday person.age:            // matches person_age column
  $age = $age+1

say "after one birthday phase:"
cls_run_phase birthday
for X each(person.name): say "  [X]"

// phase that prints a debug line per entity in a part
cph greet person.name:
  say "  hi [$name]"

say "running greet phase:"
cls_run_phase greet
