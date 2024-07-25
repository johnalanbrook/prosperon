profile.cpu = function(fn, times = 1, q = "unnamed") {
  var start = profile.now();
  for (var i = 0; i < times; i++)
    fn();
    
  var elapsed = profile.now() - start;
  var avgt = profile.best_t(elapsed/times);
  var totalt = profile.best_t(elapsed);

  say(`profile [${q}]: ${profile.best_t(avgt)} average [${profile.best_t(totalt)} for ${times} loops]`);
}

profile.ms = function(t) { return t/1000000; }
profile.secs = function(t) { return t/1000000000; }

var callgraph = {};
var st = profile.now();

function add_callgraph(fn, line, time) {
  var cc = callgraph[line];
  if (!cc) {
    var cc = {};
    callgraph[line] = cc;
    cc.time = 0;
    cc.hits = 0;
    cc.fn = fn;
    cc.line = line;
  }
  cc.time += time;
  cc.hits++;
}

var hittar = 500;
var hitpct = 0.2;
var start_gather = profile.now();
profile.gather(hittar, function() {
  var time = profile.now()-st;
  
  var err = new Error();
  var stack = err.stack.split("\n");

  stack = stack.slice(1);
  stack = stack.map(x => x.slice(7).split(' '));

  var fns = stack.map(x => x[0]).filter(x=>x);
  var lines = stack.map(x => x[1]).filter(x => x);
  lines = lines.map(x => x.slice(1,x.length-1));

  for (var i = 0; i < fns.length; i++)
    add_callgraph(fns[i], lines[i], time);

  st = profile.now();
  
  profile.gather_rate(Math.variate(hittar,hitpct));
});

var filecache = {};
function get_line(file, line) {
  var text = filecache[file];
  if (!text) {
    var f = io.slurp(file);
    if (!f) {
      filecache[file] = "undefined";
      return filecache[file];
    }
    filecache[file] = io.slurp(file).split('\n');
    text = filecache[file];
  }
  
  if (typeof text === 'string') return text;
  text = text[Number(line)-1];
  if (!text) return "NULL";
  return text.trim();
}

profile.print_cpu_instr = function()
{
  say("===CPU INSTRUMENTATION===\n");
  var gather_time = profile.now()-start_gather;
  var e = Object.values(callgraph);
  e = e.sort((a,b) => {
    if (a.time > b.time) return -1;
    return 1;
  });

  e.forEach(x => {
    var ffs = x.line.split(':');
    var time = profile.best_t(x.time);
    var pct = x.time/gather_time*100;
    say(`${x.line}::${x.fn}:: ${time} (${pct.toPrecision(3)}%) (${x.hits} hits) --> ${get_line(ffs[0], ffs[1])}`);
  });
}

profile.best_t = function (t) {
  var qq = 0;
  while (t > 1000 && qq < t_units.length-1) {
    t /= 1000;
    qq++;
  }
  return `${t.toPrecision(4)} ${t_units[qq]}`;
};

profile.report = function (start, msg = "[undefined report]") { console.info(`${msg} in ${profile.best_t(profile.now() - start)}`); };

var profile_frames = {};
var profile_frame_ts = [];
var profile_cframe = profile_frames;
var profile_frame = 0;
profile.frame = function(title)
{
  profile_frame_ts.push(profile_cframe);
  profile_cframe[title] ??= {};
  profile_cframe = profile_cframe[title];
  profile_cframe._times ??= [];
  profile_cframe._times[profile_frame] = profile.now();
}

profile.endframe = function()
{
  if (profile_cframe === profile_frames) return;
  profile_cframe._times[profile_frame] = profile.now() - profile_cframe._times[profile_frame];
  profile_cframe = profile_frame_ts.pop();
  if (profile_cframe === profile_frames) profile_frame++;
}

var print_frame = function(frame, indent, title)
{
  var avg = frame._times.reduce((sum, e) => sum += e)/frame._times.length;
  say(indent + `${title} ::::: ${profile.best_t(avg)}`);
  
  for (var i in frame) {
    if (i === '_times') continue;
    print_frame(frame[i], indent + "  ", i);
  }
}

profile.print_frame_avg = function()
{
  var indent = "";
  for (var i in profile_frames)
    print_frame(profile_frames[i], "", 'frame');
}

profile.report_cache = {};

profile.addreport = function (group, line, start) {
  if (typeof group !== 'string') group = 'UNGROUPED';
  profile.report_cache[group] ??= {};
  var cache = profile.report_cache[group];
  cache[line] ??= [];
  var t = profile.now();
  cache[line].push(t - start);
  return t;
};

profile.printreport = function (cache, name) {
  var report = `==${name}==` + "\n";

  var reports = [];
  for (var i in cache) {
    var time = cache[i].reduce((a,b)=>a+b);
    reports.push({
      time:time,
      name:i,
      hits:cache[i].length,
      avg:time/cache[i].length
    });
  }
  reports = reports.sort((a,b) => {
    if (a.avg<b.avg) return 1;
    return -1;
  });
  
  for (var rep of reports)
    report += `${rep.name}    ${profile.best_t(rep.avg)} (${rep.hits} hits) (total ${profile.best_t(rep.time)})\n`;

  return report;
};

var null_fn = function(){};
profile.disable = function()
{
  profile.gather_stop();
  profile.frame = null_fn;
  profile.endframe = null_fn;
  profile.disabled = true;
}