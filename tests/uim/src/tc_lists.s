// Collection widgets: lbox, drpl (droplist), htabs.

use cls uim rgb

export tc_lists

tc_lists W H =
  TestUI bg 0x181818 Add!:rowz:
    ftx O!abs X!10 Y!10 color!RGB_WHITE "lbox (list box, 5 items, item 2 picked)"
    lbox O!abs X!10 Y!40 W!200 H!120 5 ['Apple' 'Banana' 'Cherry' 'Durian' 'Elderberry']
         Picked!2

    ftx O!abs X!230 Y!10 color!RGB_WHITE "drpl (droplist, closed)"
    drpl O!abs X!230 Y!40 W!200 ['Option A',1 'Option B',2 'Option C',3] Init!1

    ftx O!abs X!10 Y!180 color!RGB_WHITE "htabs (horizontal tabs)"
    htabs l0:
      'Tab Zero',l0 ! ftx "<%white:content of tab zero — `lorem ipsum`>"
      'Tab One',l1  ! ftx "<%white:content of tab one>"
      'Tab Two',l2  ! ftx "<%white:content of tab two>"

  uim W H Title!"UIM test: lists" TestUI
