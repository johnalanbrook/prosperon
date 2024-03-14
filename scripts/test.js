/* Tests for prosperon */

var tests = [];
var pass = 0;
var fail = 0;

var test = function(name, fn)
{
  tests.push(function() {
    print(`${pass+fail}/${tests.length}: ${name} ... `);
    var result = fn();
    if (result) print(`pass`);
    else print(`fail`);
    return result;
  });
}

test("Pass test", _=>1);
test("Fail test", _=>0);

say(`Testing ${tests.length} tests.`);
for (var t of tests) {
  if (t())
    pass++;
  else
    fail++;
  print("\n");
}

say(`Passed ${pass} tests and failed ${fail}`);
Game.quit();