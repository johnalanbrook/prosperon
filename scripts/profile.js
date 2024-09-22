var t_units = ["ns", "us", "ms", "s", "ks", "Ms"];

function calc_cpu(fn, times, diff=0)
{
  var series = []; 
  
  for (var i = 0; i < times; i++) {
    var st = profile.now();
    fn(i);
    series.push(profile.now()-st-diff);
  }
  
  return series;
}

function empty_fn() {}

profile.cpu = function profile_cpu(fn, times = 1, q = "unnamed") {
  var retgather = gathering_cpu;
  profile.gather_stop();
  var empty = calc_cpu(empty_fn, 100000);
  var mean = Math.mean(empty);
  var series = calc_cpu(fn,times, mean);
    
  var elapsed = Math.sum(series);
  var avgt = profile.best_t(elapsed/series.length);
  var totalt = profile.best_t(elapsed);
  
  say(`profile [${q}]: ${avgt} ± ${profile.best_t(Math.ci(series))} [${totalt} for ${times} loops]`);
  say(`result of function is ${fn()}`);
  
  if (retgather)
    profile.start_prof_gather();
}

profile.ms = function(t) { return profile.secs(t)*1000; }

var callgraph = {};
profile.cpu_cg = callgraph;
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

var hittar = 500; // number of call instructions before getting a new frame
var hitpct = 0.2; // amount to randomize it
var start_gather = profile.now();

var cpu_start;

profile.start_cpu_gather = function(gathertime = 5) // gather cpu frames for 'time' seconds
{
  if (cpu_start) return;
  cpu_start = profile.now();
  
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
    if (profile.secs(st-cpu_start) < gathertime)
      profile.gather_rate(Math.variate(hittar,hitpct));
    else {
      profile.gather_stop();
      cpu_start = undefined;
      var e = Object.values(callgraph);
      e = e.sort((a,b) => {
        if (a.time > b.time) return -1;
	return 1;
     });

     for (var x of e) {
       var ffs = x.line.split(':');
       var time = profile.best_t(x.time);
       var pct = profile.secs(x.time)/gathertime*100;
       x.log =`${x.line}::${x.fn}:: ${time} (${pct.toPrecision(3)}%) (${x.hits} hits) --> ${get_line(ffs[0], ffs[1])}`;
     }

     profile.cpu_instr = e;
    }
  });
}

function push_time(arr, ob, max)
{
  arr.push({
    time:profile.now(),
    ob
  });
}

profile.cpu_frames = [];
profile.cpu_frame = function()
{
  if (gathering_cpu) return;
  
  profile.gather(Math.random_range(300,600), function() {
    push_time(profile.cpu_frames, console.stack(2));
    profile.gather_stop();
  });
}

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

profile.stop_cpu_instr = function()
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


/*
  Frame averages are an instrumented profiling technique. Place frame() calls in your code to get a call graph for things you are interested in.
*/

var frame_avg = false;
profile.frame_avg_t = 72000;

profile.start_frame_avg = function()
{
  if (frame_avg) return;
  profile_frames = {};
  profile_frame_ts = [];
  profile_cframe = profile_frames;
  pframe = 0;
  frame_avg = true;
}

profile.stop_frame_avg = function()
{
  if (!frame_avg) return;
  
  frame_avg = false;
  profile.print_frame_avg();
}

profile.toggle_frame_avg = function()
{
  if (frame_avg) profile.stop_frame_avg();
  else profile.start_frame_avg();
}

var profile_framer = {
  series: [],
  avg: {},
  frame: 72000,
};
var profile_cframe = undefined;
var pframe = 0;
var profile_stack = [];

profile.frame = function profile_frame(title)
{
  if (!frame_avg) return;
  
  if (!profile_cframe) {
    profile_cframe = {};
    profile_framer.series.push({
      time:profile.now(),
      data:profile_cframe
    });
  } else
    profile_stack.push(profile_cframe);
  
  profile_cframe[title] ??= {};
  profile_cframe = profile_cframe[title];
  profile_cframe.time = profile.now();
}

profile.endframe = function profile_endframe()
{
  if (!frame_avg) return;
  profile_cframe.time = profile.now() - profile_cframe.time;
  profile_cframe = profile_frame_ts.pop();
}

var print_frame = function(frame, indent, title)
{
  say(indent + `${title} ::::: ${profile.best_t(Math.mean(frame._times))} ± ${profile.best_t(Math.ci(frame._times))} (${frame._times.length} hits)`);
  
  for (var i in frame) {
    if (i === '_times') continue;
    print_frame(frame[i], indent + "  ", i);
  }
}

profile.print_frame_avg = function()
{
  say("===FRAME AVERAGES===\n");
  
  var indent = "";
  for (var i in profile_frames)
    print_frame(profile_frames[i], "", 'frame');
    
  say("\n");
}


/*
  Cache reporting is to measure how long specific events take, that are NOT every frame
  Useful to measure things like how long it takes to make a specific creature
*/

var cache_reporting = false;

var report_cache = {};

var cachest = 0;
var cachegroup;
var cachetitle;

profile.cache_reporting = function() { return cache_reporting; }
profile.cache_toggle = function() { cache_reporting = !cache_reporting; }
profile.cache_dump = function() {
  report_cache = {};
}

profile.cache = function profile_cache(group, title)
{
  if (!cache_reporting) return;
  cachest = profile.now();
  cachegroup = group;
  cachetitle = title;
}

profile.endcache = function profile_endcache(tag = "")
{
  addreport(cachegroup, cachetitle + tag, cachest);
}

profile.print_cache_report = function()
{
  var str = "===START CACHE REPORTS===\n";
  for (var i in report_cache)
    str += printreport(report_cache[i], i) + "\n";

  say(str);
}

function addreport(group, line, start) {
  if (typeof group !== 'string') group = 'UNGROUPED';
  report_cache[group] ??= {};
  var cache = report_cache[group];
  cache[line] ??= [];
  var t = profile.now();
  cache[line].push(t - start);
  return t;
};

function printreport(cache, name) {
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

profile.best_mem = function(bytes)
{
  var sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
  if (bytes == 0) return '0 Bytes';
  var i = parseInt(Math.floor(Math.log(bytes) / Math.log(1024)));
  return (bytes / Math.pow(1024, i)).toPrecision(3) + ' ' + sizes[i];
}

profile.print_mem = function()
{
  var mem = os.mem();
  say('total memory used: ' + profile.best_mem(mem.memory_used_size));
  say('total malloced: ' + profile.best_mem(mem.malloc_size));
  delete mem.memory_used_size;
  delete mem.malloc_size;
  for (var i in mem) {
    if (i. includes("size"))
      say("  " + i + " :: " + profile.best_mem(mem[i]));
  }
}

profile.last_mem = undefined;
profile.mems = [];
profile.gcs = [];
profile.print_gc = function()
{
  var gc = os.check_gc();
  if (!gc) return;
  profile.gcs.push(gc);
  profile.mems.push(os.mem());
  say("GC Hit");
  say (`time: ${profile.best_t(gc.time)}`);
  say(`new threshold: ${profile.best_mem(profile.mems.last().gc_threshold)}`);
  say(`memory checked: ${profile.best_mem(profile.gcs.last().mem)}`);
  say(`memory freed: ${profile.best_mem(profile.gcs.last().startmem - profile.gcs.last().mem)}`);
}

return {profile};
