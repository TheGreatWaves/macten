defmacten_proc switch {
  case_name { ident }
  body { ident }
  branch { case "case_name": { body } }
  branches { branches branch } | { branch }
  target { ident }
  switch_str { switch target { branches } }
}

defmacten_proc declaration {
 typename { ident }
 varname { ident }
 declaration { varname: typename; }
}

defmacten_dec foo {
 () => {hello}
}


foo![]
