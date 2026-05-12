use ui/data m lbox slider
export folder_view

type folder_lbox.$lbox W H F:
   root!'/' f!F lbox filter refresh_folder!1
   =
| $lbox =  lbox W H [] | N => ($f)("[$root][N]")
folder_lbox.render =
| when $refresh_folder:
  | $refresh_folder =  0
  | $lbox.data =  $list_folder($root)
| $lbox.render
folder_lbox.cd NewRoot =
| $root =  NewRoot
| $refresh_folder =  1
folder_lbox.input In = case In
  [mice double_left 1 P] | R   if $lbox.value >< '../'
                               then "[$root.lead.url.0]"
                               else "[$root][$lbox.value]"
                         | when R.folder: $cd(R)
  Else | $lbox.input(In)
folder_lbox.itemAt Point XY WH = [Me XY WH]
folder_lbox.list_folder Path = 
| Xs   Path.items
| Folders   Xs.keep(is+"[_]/")
| Files   Xs.skip(is+"[_]/")
| Parent   if Path >< '/' or Path.last >< ':' then [] else ['../']
| when $filter: Files =  Files.keep($filter)
| [@Parent @Folders.sort @Files.sort]

type folder_view.$lay W H F: lay base =
| FL   folder_lbox W H F
| $base =  FL
| S   slider v H | N => FL.offset =  @int N*FL.data.n.float
| $lay =  layH FL,S
folder_view.folder = $base.root
folder_view.=folder NewFolder = $base.cd(NewFolder)
folder_view.=filter F = $base.filter =  F
