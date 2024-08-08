var repl = {};

var file = "repl.jj";
var last = 0;

repl.hotreload = function() {
  if (io.mod(file) > last) {
    say("REPL:::");
    last = io.mod(file);
    var script = io.slurp(file);
    eval(script);
  }
}

return {repl:repl};
