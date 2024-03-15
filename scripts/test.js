/* Tests for prosperon */

var tests = [];
var pass = 0;
var fail = 0;
var failed = [];

var test = function(name, fn)
{
  var func = function() {
    print(`${pass+fail+1}/${tests.length}: ${name} ... `);
    var p = profile.now();
    var b = fn();
    p = profile.lap(p);
    print(`${b ? "pass" : "fail"} [${p}]`);
    return b;
  };
  func.testname = name;
  tests.push(func);
}

say(`Testing ${tests.length} tests.`);
for (var t of tests) {
  if (t())
    pass++;
  else {
    fail++;
    failed.push(t.testname);
  }
  print("\n");
}

say(`Passed ${pass} tests and failed ${fail} [${(pass*100/(pass+fail)).toPrecision(4)}%].`);
say(`Failed tests are:`);
for (var f of failed)
  say(f);
  
os.quit();
