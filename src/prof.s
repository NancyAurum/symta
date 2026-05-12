// UI profiler — counters + per-widget-class draw timing.
//
// Cost when disabled is one compare per instrumented site. Enabled cost
// is small but non-trivial — fine for diagnostic runs, do not leave on.
//
// Public API:
//   prof_enable               turn on instrumentation
//   prof_disable              turn off (counters keep their values)
//   prof_enabled              query
//   prof_reset                zero all counters + widget table
//   prof_incr Name            bump named counter (Name is a symbol)
//   prof_get Name             read counter
//   prof_record_widget_draw W DurationSec
//                             aggregate by widget cls + record
//                             instance peak (single-frame max draw ms)
//   prof_snapshot Tag         capture a deep copy of the counter +
//                             widget tables under Tag, then reset
//   prof_snapshots            list of [tag stats widgets] tuples
//   prof_clear_snapshots      drop accumulated snapshots
//   prof_format_log Header    produce a multi-line ascii report
//   prof_write_log Path Header
//                             write the report to disk
export prof_enable prof_disable prof_enabled prof_reset
       prof_incr prof_get
       prof_record_widget_draw
       prof_snapshot prof_snapshots prof_clear_snapshots
       prof_format_log prof_write_log
       prof_register_tick_fn prof_pending_tick_fn

ProfEnabled 0
ProfStats!     // counter name -> int
ProfWidgets!   // widget cls -> {count, sum_ms, max_ms, peak_gid}
ProfSnaps []   // accumulated [Tag StatsCopy WidgetsCopy]
ProfPendingTickFn 0   // set by harness; copied into uim.tick_fn at uim construct

prof_register_tick_fn Fn = ProfPendingTickFn = Fn
prof_pending_tick_fn = ProfPendingTickFn

prof_enable  = ProfEnabled = 1
prof_disable = ProfEnabled = 0
prof_enabled = ProfEnabled

prof_reset =
  ProfStats = !
  ProfWidgets = !

prof_incr Name =
  when ProfEnabled:
    V ProfStats.Name(No=0)
    ProfStats.Name = V + 1

prof_get Name = ProfStats.Name(No=0)

prof_record_widget_draw W Duration =
  less ProfEnabled: ret
  ClsName W.ub.'cls'
  less ClsName: ret
  MS Duration * 1000.0
  Entry have ProfWidgets.ClsName: count!0 sum_ms!0.0 max_ms!0.0 peak_gid!No
  Entry.count = Entry.count + 1
  Entry.sum_ms = Entry.sum_ms + MS
  when MS > Entry.max_ms:
    Entry.max_ms = MS
    Entry.peak_gid = W.gid(No=W.id)

deep_copy_table T =
  R!
  for K,V T:
    if V.is_table: R.K = deep_copy_table V
    else R.K = V
  R

prof_snapshot Tag =
  Stats deep_copy_table ProfStats
  Widgets deep_copy_table ProfWidgets
  ProfSnaps =: @ProfSnaps [Tag Stats Widgets]
  prof_reset

prof_snapshots = ProfSnaps

prof_clear_snapshots = ProfSnaps = []

prof_format_log Header =
  Ls:
  push "===========================================" Ls
  push Header Ls
  push "===========================================" Ls
  push "" Ls
  for [Tag Stats Widgets] ProfSnaps:
    push "--- snapshot: [Tag] ---" Ls
    Keys Stats.l{?0}.s
    for K Keys:
      push "  [K] = [Stats.K]" Ls
    push "  widget cls totals (count, sum_ms, max_ms, peak_widget):" Ls
    WClassesSorted Widgets.l.sortBy(?1.sum_ms.@0.float*-1.0)
    for ClsName,E WClassesSorted:
      push "    [ClsName.@0]  count=[E.count] sum_ms=[E.sum_ms.@0.format_ms] max_ms=[E.max_ms.@0.format_ms] peak=[E.peak_gid]" Ls
    push "" Ls
  Ls.text^'\n'

float.format_ms = (Me*100.0).int.float/100.0  //2 decimal places

prof_write_log Path Header =
  Text prof_format_log Header
  Path.set Text
  Text.n
