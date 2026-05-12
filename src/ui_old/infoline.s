use m txt
export infoline

type infoline.widget: info_text!txt(small '')
infoline.render =
| Txt gui.mice_widget.infoline
| $info_text.value = if Txt.is_fn: Txt() else Txt
| $info_text.render
