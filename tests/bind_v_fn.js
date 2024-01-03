var binds = [];
var a = {
  n: 1,
  test() { this.n++; }
};
var count = 10000000;

var start = Date.now();
for (var i = 0; i < count; i++)
  binds.push(a.test.bind(a));

console.log(`Make bind time: ${Date.now()-start} ms`);

start = Date.now();
for (var i = 0; i < count; i++)
  binds[i]();

console.log(`Bind time: ${Date.now()-start} ms`);

start = Date.now();
for (var i = 0; i < count; i++)
  a['test']();

console.log(`Call time: ${Date.now()-start} ms`);
