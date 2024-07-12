defmacten_dec assignment {
  ($varname := $value $($vname := $lol)) => {assignment![$varname := $value] assignment![$lol]}
  ($varname := $value) => {
    auto $varname = $value;
  }
}

assignment![
  foo := 5 
  bar := 5 
  lol := ('hi')
 ]
