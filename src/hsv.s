use rgb
export rgb2hsv hsv2rgb

rgb2hsv RGB =
| unrgb RGB R G B
| R R.float/255.0
| G G.float/255.0
| B B.float/255.0
| V (R+G+B)/3.0
| K 0.0
| when G < B:
  | swap G B
  | K = -1.0
| MinGB B
| when R < G:
  | swap R G
  | K = -2.0/6.0 - K
  | MinGB = if G < B then G else B
| Chroma R - MinGB
| H K + (G-B)/(6.0*Chroma + 0.0000000001)
| when H < 0.0: H = -H
| S Chroma / (R + 0.0000000001)
| H,S,R
//| H,S,V

hsv2rgb H S V =
//| V V/(2.0 - S)*2.0 //correct V for low saturation to avoid loss of details
| HH H*360.0
| when HH > 360.0: HH = 0.0
| HH /= 60.0
| I HH.int
| FF HH - I.float
| P ((V * (1.0 - S))*255.0).int
| Q ((V * (1.0 - S*FF))*255.0).int
| T ((V * (1.0 - S*(1.0 - FF)))*255.0).int
| V = (V*255.0).int.clip(0 255)
| when P > 255: P = 255
| when Q > 255: Q = 255
| when T > 255: T = 255
| when V > 255: V = 255
| if I >< 0 then rgb V T P
  else if I >< 1 then rgb Q V P
  else if I >< 2 then rgb P V T
  else if I >< 3 then rgb P Q V
  else if I >< 4 then rgb T P V
  else rgb V P Q
