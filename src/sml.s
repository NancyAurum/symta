//Symta Markup Language

sml_parse_tag Tag = Tag.parse.0{['!' X] = X}

SMLSpec: '#'  ' '  '\n'  '<'  '>'
SMLSpec = SMLSpec.bag

text.smle = @text: map C Me: if SMLSpec.has C: "#[C]" else C //escape SML
  

//FIXME: check tag validity
text.sml =
  Depth 0
  C 0
  Cs:
  Items:
  Esc 0
  Prev 0
  TagHead 0
  SubTags 0
  L $l
  till L.end:
    Prev = C
    C = pop L
    if Esc:
      if Depth: push C Cs
      else
        if C >< '\n': C = [n]
        push C Items
      Esc = 0
      pass
    if Depth: case C:
      '#' = push C Cs
            Esc = 1
      '|' = if Depth><1 and not TagHead:
              push Cs.f.text SubTags
              Cs =:
            else push C Cs
      ':' =
        if Depth><1 and not TagHead:
          TagHead = Cs.f.text
          Cs =:
        else push C Cs
      '<' =
        Depth+
        push C Cs
      '>' =
        Depth-
        if Depth:
          push C Cs
          pass
        for Tag SubTags: push Tag^sml_parse_tag Items
        TagBody:
        NeedsClose 1
        if TagHead:
          TagBody = Cs.f.text.sml
        else
          TagHead = Cs.f.text
          NeedsClose = 0
        Cs =:
        push TagHead^sml_parse_tag Items
        for Item TagBody: push Item Items
        if NeedsClose: push No Items
        for Tag SubTags: push No Items
        TagHead = 0
      _ = push C Cs
    else case C:
      '#' = Esc = 1
      ' '+'\n' =
        case Items:
          []+[' '@_] =
          Else = push ' ' Items
      '<' = Depth+
            SubTags =:
      '>' = bad 'unclosed tag'
      _ = push C Items
  Items.f

list.cunsml = //clean unsml
  Items Me
  Item 0
  PrevItem 0
  Out:
  till Items.end:
    PrevItem = Item
    Item = pop Items
    if Item.is_text: push Item Out
    else case Item:
      [s] = push " " Out
      [n] = push "\n" Out
      [C<1.is_int] =
      [['%' C]] =
      No =
      [pic Name x!0 y!0 w!No h!No] =
      [Tag] =
  Out.f.text

list.unsml =
  Items Me
  Item 0
  PrevItem 0
  Out:
  emit_tag Head =
    case Items:
      [No@] = pop Items
      Else = push Head Out
  till Items.end:
    PrevItem = Item
    Item = pop Items
    if Item.is_text:
      if SMLSpec.has Item: Item = "#[Item]" //spec symbol like `<` and `>`
      push Item Out
    else case Item:
      [s] = push "<s>" Out
      [n] = push "<n>" Out
      [C<1.is_int] = emit_tag "<[C]:"
      [['%' C]] = emit_tag "<%[C]:"
      No = push ">" Out
      [pic Name x!0 y!0 w!No h!No] =
        push "<pic `[Name]` x![X] y![Y] w![W] h![H]>" Out
      [['/' Id] x!0 y!0 w!0 h!0] =
        push "</`[Id]` x![X] y![Y] w![W] h![H]>" Out
      [Tag] = emit_tag "<[Tag]:"
  Out.f.text

//T "Abc <pic tiger w=256 h=128> Def <d e f> "
//say T.sml

