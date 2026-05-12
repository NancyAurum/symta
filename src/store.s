use gfx svg cache prof
export get_pic get_pic_sub add_pic_path

PicPaths ["[main_path]pic/"]
Pics!

add_pic_path Path = PicPaths =: Path @PicPaths

locate_pic N =
  if N.exists: ret N
  for E png,svg:
    P PicPaths.find("[?][N].[E]".exists)
    if got P: ret "[P][N].[E]"
  No

get_pic_rescale G W H =
  GW G.w
  GH G.h
  if no W: W = (GW*H+GH-1)/GH
  if no H: H = (GH*W+GW-1)/GW
  if W <> GW or H <> GH: G.rescale W H type!GFX_SMOOTH //GFX_MEDIAN

#AdaptBaseW 1920.0
#AdaptBaseH 1080.0 
adapt_val RH Val = Val = @int :@ceil Val*RH/AdaptBaseH

get_pic_adaptive_rescale G W H =
  H = -H
  NW adapt_val H G.w
  NH adapt_val H G.h
  if got W:
    if NW > W:
      NNW NW
      NW = W
      //NH = H*W/AdaptBaseW.int*NNW/W //more aggressive aspect
      NH = G.h*W/AdaptBaseW.int*NNW/W
  get_pic_rescale G NW NH


CacheFolder "[main_path]cache/"

less CacheFolder.exists: CacheFolder.mkpath



// In-memory image cache only. We dropped the disk PNG round-trip
// (see ui_speedup.md S1): composing a 9-slice or sub-rect is cheaper
// than encoding+decoding a PNG to/from disk, and the on-disk cache
// folder grew unbounded across runs. The cache_get TTL still
// evicts cold entries; callers that want a permanent pin pass
// `Life!STORE_FOREVER`.
//
//FIXME: pic should allow `FilteredId,Filter` param
//       so user could apply and store arbitrary filtered pics
get_pic N W!No H!No Life!3.0 =
  prof_incr \get_pic
  cache_get Life "pic[[N W H]]": =>
    if W><0 or H><0: gfx 0 0
    else
      Clrz No
      if N.any ':':
        Args N.split^':'
        N = Args.~
        KVs Args.0.split^'!'.group^2
        for K,V KVs: case K:
          clrz = Clrz = V
      FP locate_pic N
      if no FP: bad "couldn't locate the `[N]` pictogram"
      G case FP.url.2:
        png =
          G gfx FP
          if got W or got H:
            if H < 0: get_pic_adaptive_rescale G W H
            else get_pic_rescale G W H
          G
        svg = gfx_svg W.@0 H.@0 FP //FIXME: support adaptive rescale
      if got Clrz: G.colorize Clrz.int^16
      G

get_pic_sub N W!No H!No SX SY SW SH Index!0 Life!3.0 =
  prof_incr \get_pic_sub
  cache_get Life "pic[[N W H SX SY SW SH]]": =>
    if W><0 or H><0:
      G gfx 0 0 //happens with pic9
      G.w = W
      G.h = H
      G
    else
      G get_pic(N).cut SX SY SW SH
      if got W or got H: get_pic_rescale G W H
      G

no.load_pic F = get_pic F



