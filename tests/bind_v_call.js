var binds = [];
var cbs = [];
var fats = [];
var count = 1000000;

var a = {
  n: 1
}

function test_a() {
  this.n++;
}

var start = Date.now();
for (var i = 0; i < count; i++) 
  binds.push(test_a.bind(a));
console.log(`Make bind time: ${Date.now()-start} ms`);

start = Date.now();
for (var i = 0; i < count; i++)
  fats.push(() => test_a.call(a));
console.log(`Make fat time: ${Date.now()-start} ms`);

start = Date.now();
for (var i = 0; i < count; i++) {
  binds[i]();
}
console.log(`Bind time: ${Date.now()-start} ms`);

start = Date.now();
for (var i = 0; i < count; i++) {
  fats[i]();
}
console.log(`Fat time: ${Date.now()-start} ms`);

start = Date.now();
for (var i = 0; i < count; i++)
  test_a.call(a);

console.log(`Call time: ${Date.now()-start} ms`);
