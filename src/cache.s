// The purpose of this file is maintaining a simple global cache,
// Each item has a lifetime. Each time it gets accessed, its life extended

export cache_get cache_clean

CacheClock clock
CacheStore!
CacheStamps!

cache_get Life Key RegenF =
  CacheStamps.Key = CacheClock+Life
  have CacheStore.Key: RegenF()

cache_clean =
  Now clock //new clock
  if Now-CacheClock < 0.1: ret
  CacheClock = Now
  for Key,Stamp CacheStamps:
    if Stamp < Now:
      //say free Key CacheStore.Key
      CacheStore.Key.free
      CacheStore.del Key
      CacheStamps.del Key
