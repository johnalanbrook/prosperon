/* Tests for prosperon */

var test = {};

var tests = [];
var pass = 0;
var fail = 0;
var failed = [];

test.run_suite = function (file) {
  test = [];
  pass = 0;
  fail = 0;
  failed = [];
};

test.run = function (name, fn) {
  var func = function () {
    print(`${pass + fail + 1}/${tests.length}: ${name} ... `);
    var p = profile.now();
    var b = fn();
    p = profile.lap(p);
    print(`${b ? "pass" : "fail"} [${p}]`);
    return b;
  };
  func.testname = name;
  tests.push(func);
};

say(`Testing ${tests.length} tests.`);
for (var t of tests) {
  if (t()) pass++;
  else {
    fail++;
    failed.push(t.testname);
  }
  print("\n");
}

say(`Passed ${pass} tests and failed ${fail} [${((pass * 100) / (pass + fail)).toPrecision(4)}%].`);
say(`Failed tests are:`);
for (var f of failed) say(f);

os.exit(0);

return { test };
