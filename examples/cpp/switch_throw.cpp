defmacten_proc switch {
  case_name { ident }
  body { ident }
  branch { case "case_name": { body } }
  branches { branches branch } | { branch }
  target { ident }
  switch_str { switch target { branches } }
}