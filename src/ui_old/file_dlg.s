use m txt button folder_view txt_input

type file_dlg.$base ui defaultFolder:
  base picked title filename
  pick_button cancel_button
  widget click![0 No] cancelCB
  filename_widget
  save
  paused!0
  allow_picking_folders!0
  =
| $pick_button = button (=> if $save then 'Save' else 'Load')
                   skin!medium_small: =>
  | $show =  0
  | $ui.block_input--
  | when $paused: $ui.unpause
  | Picked   if $save then "[$folder][$filename.value]" else $picked
  | if no $click.1 then ($click.0)(Picked)
    else ($click.0)($click.1 Picked)
| $pick_button.state =  'disabled'
| $cancel_button =  button 'Cancel' skin!medium_small: =>
  | $show =  0
  | when $paused: $ui.unpause
  | $ui.block_input--
  | when got $cancelCB: ($cancelCB)(Me)
| $title = txt header: => if $save then 'Save' else 'Load'
| $filename = txt_input ''
| $widget = folder_view 320 240: File =>
  | if $allow_picking_folders and File.last >< '/': File = File.lead
  | $picked = File
  | if $save then
    | FN  File.url.1
    | when FN<>'': $filename.value =  FN
    else //file must exist
    | Valid File.exists 
    | if Valid: less $allow_picking_folders: Valid = not File.folder
    | $pick_button.state =  if Valid: 'normal' else 'disabled'
| $set_filter(N => no N.locate('.')) //only files without externsion
| $widget.folder =  $defaultFolder
| MW   44
| MH   86
| DlgA   hidden: dlg: mtx
  |  15  12 | txt mediumb "Name:" bold!1
  |  15+55  10 | $filename
| $filename_widget =  DlgA
| DlgB   dlg: mtx
  |   0   0 | spacer 600 600 //FIXME: hack
  |  15  10 | $widget
| LayAB   layV DlgA,DlgB
| Dlg   dlg: mtx
  |   0   0 | $ui.main.img(ui_box)
  | 240  12 | $title
  |  MW  MH | LayAB
  |  15+MW 305+MH | $pick_button
  | 220+MW 305+MH | $cancel_button

| $base =  hidden: Dlg


file_dlg.set_filter FilterFn =
| $widget.filter =  FilterFn

file_dlg.render =
| when $save:
  | $pick_button.state =  if $filename.value<>''
                          then \normal
                          else \disabled
| $base.render

file_dlg.folder = $widget.folder

file_dlg.=folder Folder =
| when no Folder: Folder =  $defaultFolder
| $widget.folder =  Folder

file_dlg.run Click Folder cancelCB!No saveAs!No AllowPickingFolders!0 =
| $allow_picking_folders = AllowPickingFolders
| $click <= if Click.is_list then Click else [Click No]
| $cancelCB =  CancelCB
| $folder =  Folder
| $save =  0
| when got SaveAs:
  | $save =  1
  | $filename.value =  SaveAs
| $filename_widget.show =  $save
| $paused =  not $ui.paused
| $ui.block_input++
| when $paused: $ui.pause
| $show =  1

ui.new_file_dlg Folder!No =
| when no Folder: Folder =  $data
| file_dlg Me Folder
