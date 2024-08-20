var aa = [1,2,3,4,5,6,7,8,9,10];

/*
var ta = [1,2,3,4,5,6,7,8,9,10];

function tadd(a,b)
{
  var c = a.slice();
  for (var i = 0; i < a.length; i++)
    c[i] += b[i];
  return c;
}

say(vector.angle_between([0,1], [1,0]));

function test_arrays(ta, tb)
{
  profile.cpu(_=> ta.add(tb), 100000, "add two arrays");
  say(ta.add(tb));
}
*/
//for (var i = 2; i <= 10; i++) {
//  say(`TESTING FOR ${i} ARRAY`);
//  test_arrays(ta.slice(0,i), ta.slice(0,i));
//}
/*
say(prosperon.guid());
say(prosperon.guid());
var aa = [];
var ao = {};

var amt = 10000;

profile.cpu(_ => prosperon.guid(), 10000, "guid generation");

profile.cpu(_ => aa.push(1), amt, "add one to array");
profile.cpu(_ => ao[prosperon.guid()] = 1, amt, "add one via guid");

var aos = Object.keys(ao);

say(aa.length);

var useguid = aos[amt/2];

say(useguid);

var tries = [];
for (var i = 0; i < amt; i++)
  tries[i] = aa.slice();

var rmamt = amt/2;
profile.cpu(x => tries[x].remove(rmamt), amt, "remove from array");

for (var i = 0; i < amt; i++) {
  tries[i] = {};
  for (var j in ao) tries[i][j] = ao[j];
}


profile.cpu(x => delete tries[x][useguid], amt, "delete with guid");

//profile.cpu(_ => Object.values(ao), 10000, "make array from objects");
var dofn = function(x) { x += 1; }
profile.cpu(_ => aa.forEach(dofn), 1000, "iterate array");
profile.cpu(_ => ao.forEach(dofn), 1000, "iterate object foreach");
profile.cpu(_ => {
  for (var i in ao) ao[i] += 1;
}, 1000, "iterate in object values");
*/
