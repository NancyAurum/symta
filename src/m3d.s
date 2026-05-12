//math 3d
use m
export m3d m3dIdentity v3min v3max

v3min A B = [(min A.0 B.0) (min A.1 B.1) (min A.2 B.2)]
v3max A B = [(max A.0 B.0) (max A.1 B.1) (max A.2 B.2)]

list.dot V = $0*V.0 + $1*V.1 + $2*V.2

list.cross V =: $1*V.2 - $2*V.1
                $2*V.0 - $0*V.2
                $0*V.1 - $1*V.0

//angle between two vectors (use acos to get the rads value)
list.angle V = $dot(V)/($abs*V.abs)


type m3d Xx Xy Xz Yx Yy Yz Zx Zy Zz
  :
  x![Xx Xy Xz]
  y![Yx Yy Yz]
  z![Zx Zy Zz]
  =


m3d.as_text = "$<m3d:[[$x$y$z].j{as_text}.text^' ']>"

m3d.l =: @$x @$y @$z
m3d.ll =: $x $y $z

m3d.zip =
  M $ll
  m3d M.0.0 M.1.0 M.2.0
      M.0.1 M.1.1 M.2.1
      M.0.2 M.1.2 M.2.2


list.mm M =: M.x.dot^Me M.y.dot^Me M.z.dot^Me

m3d.mm M = m3d @$x.mm^M @$y.mm^M @$z.mm^M

m3dIdentity = m3d 1.0 0.0 0.0
                  0.0 1.0 0.0
                  0.0 0.0 1.0

m3dRotX Angle =
  C Angle.cos
  S Angle.sin
  m3d 1.0 0.0 0.0
      0.0  C   S
      0.0 -S   C

m3dRotY Angle =
  C Angle.cos
  S Angle.sin
  m3d C   0.0  -S
      0.0 1.0 0.0
      S   0.0   C

m3dRotZ Angle =
  C Angle.cos
  S Angle.sin
  m3d  C   S  0.0
      -S   C  0.0
      0.0 0.0 1.0

m3d.rot Angle = $mm(Angle.0^m3dRotX).mm(Angle.1^m3dRotY).mm(Angle.2^m3dRotZ)

//reverse the rotation
m3d.rotr Angle = $mm(Angle.2^m3dRotZ).mm(Angle.1^m3dRotY).mm(Angle.0^m3dRotX)
