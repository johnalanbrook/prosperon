/*
  TYPES OF PROFILING
  report - can see specific events that happened. Includes inclusive vs noninclusive times. When used on top of each other, also generates a callstack.
  snapshot - See the amount of something every amount of time
  memory - can see how much memory is allocated and from where [not implemented yet]
*/

function calc_cpu(fn, times, diff = 0) {
  var series = [];

  for (var i = 0; i < times; i++) {
    var st = profile.now();
    fn(i);
    series.push(profile.now() - st - diff);
  }

  return series;
}

function empty_fn() {}

// Measure how long a fn takes to run, ignoring the overhead of a function call
profile.cpu = function profile_cpu(fn, times = 1, q = fn) {
  var empty = calc_cpu(empty_fn, 100000);
  var mean = Math.mean(empty);
  var series = calc_cpu(fn, times, mean);

  var elapsed = Math.sum(series);
  var avgt = profile.best_t(elapsed / series.length);
  var totalt = profile.best_t(elapsed);

  say(`profile [${q}]: ${avgt} ± ${profile.best_t(Math.ci(series))} [${totalt} for ${times} loops]`);
  say(`result of function is ${fn()}`);
};

profile.ms = function (t) {
  return profile.secs(t) * 1000;
};

var callgraph = {};
profile.rawstacks = {};

function add_callgraph_from_stack(err, time)
{
  var stack = err.stack.split("\n").slice(1);
  var rawstack = stack.join("\n");
  profile.rawstacks[rawstack] ??= {
    time: 0,
    hits: 0,
  };
  profile.rawstacks[rawstack].hits++;
  profile.rawstacks[rawstack].time += time;

  stack = stack.map(x => x.slice(7).split(" "));

  var fns = stack.map(x => x[0]).filter(x => x);
  var lines = stack.map(x => x[1]).filter(x => x);
  lines = lines.map(x => x.slice(1, x.length - 1));

  add_callgraph(fns[0], lines[0], time, true);
  for (var i = 1; i < fns.length; i++) add_callgraph(fns[i], lines[i], time, false);
}

function add_callgraph(fn, line, time, alone) {
  var cc = callgraph[line];
  if (!cc) {
    var cc = {};
    callgraph[line] = cc;
    cc.time = 0;
    cc.hits = 0;
    cc.fn = fn;
    cc.line = line;
    cc.alone = {
      time: 0,
      hits: 0,
    };
  }
  cc.time += time;
  cc.hits++;

  if (alone) {
    cc.alone.time += time;
    cc.alone.hits++;
  }
}

var hittar = 500; // number of call instructions before getting a new frame
var hitpct = 0.2; // amount to randomize it

profile.cpu_start = undefined;

profile.clear_cpu = function () {
  callgraph = {};
  profile.cpu_instr = undefined;
  profile.gather_stop();
  profile.cpu_start = undefined;
};

function cpu_record_frame()
{

}

// These values are set to get the most amount of frames without causing a stack overflow
var hittar = 450; 
var hitpct = 0.2;

profile.start_cpu_gather_fn = function()
{
  if (profile.cpu_start) return;
  profile.clear_cpu();

  profile.cpu_start = profile.now();
  var st = profile.cpu_start;

  profile.gather(Math.variate(hittar,hitpct), function() {
    var time = profile.now() - st;
    var err = new Error();
    add_callgraph_from_stack(err, time);
    st = profile.now();
    profile.gather_rate(Math.variate(hittar,hitpct));
  });
}

profile.start_cpu_gather = function (gathertime = 5) {
  if (profile.cpu_start) return;
  profile.clear_cpu();
  
  // gather cpu frames for 'gathertime' seconds
  profile.cpu_start = profile.now();
  var st = profile.cpu_start;

  profile.gather(hittar, function () {
    var time = profile.now() - st;

    var err = new Error();
    add_callgraph_from_stack(err, time);
    st = profile.now();
    
    if (profile.secs(st - profile.cpu_start) < gathertime)
      profile.gather_rate(Math.variate(hittar, hitpct));
    else
      profile.stop_cpu_measure();
  });
};

profile.stop_cpu_measure = function()
{
  if (!profile.cpu_start) return;
  profile.gather_stop();
  var gathertime = profile.now()-profile.cpu_start;
  console.info(`gathered for ${profile.best_t(gathertime)}`);
  profile.cpu_start = undefined;
  var e = Object.values(callgraph);
  e = e.filter(x => x.line);

  for (var x of e) {
    var ffs = x.line.split(":");

    x.timeper = x.time / x.hits;
    x.pct = x.time/gathertime * 100;
    x.alone.timeper = x.alone.time / x.alone.hits;
    x.alone.pct = x.alone.time / gathertime * 100;
    x.fncall = get_line(ffs[0], ffs[1]);
    x.log = x.line + " " + x.fn + " " + x.fncall;
    x.incl = {
      time: x.time,
      timeper: x.timeper,
      hits: x.hits,
      pct: x.pct,
    };
  }
  profile.cpu_instr = e;
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
    filecache[file] = io.slurp(file).split("\n");
    text = filecache[file];
  }

  if (typeof text === "string") return text;
  text = text[Number(line) - 1];
  if (!text) return "NULL";
  return text.trim();
}

profile.stop_cpu_instr = function () {
  return;
};

/*
  Frame averages are an instrumented profiling technique. Place frame() calls
  in your code to get a call graph for things you are interested in.
*/

var frame_avg = false;

profile.start_frame_avg = function () {
  if (frame_avg) return;
  profile_frames = {};
  profile_frame_ts = [];
  profile_cframe = profile_frames;
  pframe = 0;
  frame_avg = true;
};

profile.stop_frame_avg = function () {
  frame_avg = false;
};

profile.toggle_frame_avg = function () {
  if (frame_avg) profile.stop_frame_avg();
  else profile.start_frame_avg();
};

var profile_framer = {
  series: [],
  avg: {},
  frame: 72000,
};
var profile_cframe = undefined;
var pframe = 0;
var profile_stack = [];

profile.frame = function profile_frame(title) {
  return;
  if (profile.cpu_start) return;
  if (!frame_avg) return;

  if (!profile_cframe) {
    profile_cframe = {};
    profile_framer.series.push({
      time: profile.now(),
      data: profile_cframe,
    });
  } else profile_stack.push(profile_cframe);

  profile_cframe[title] ??= {};
  profile_cframe = profile_cframe[title];
  profile_cframe.time = profile.now();
};

profile.endframe = function profile_endframe() {
  return;
  if (!frame_avg) return;
  profile_cframe.time = profile.now() - profile_cframe.time;
  profile_cframe = profile_frame_ts.pop();
};

profile.data = {};
profile.curframe = 0;
profile.snapshot = {};

var get_snapshot = function()
{
 var snap = profile.snapshot;
 
 for (var monitor of monitors) {
   var stat = monitor.fn();
   monitor.hook?.(stat);
   snap[monitor.path] = stat;
 }
 
 snap.actors = actor.__stats();
 snap.memory.textures = game.texture.total_size();
 snap.memory.texture_vram = game.texture.total_vram();

 snap.particles = stat_emitters();
}

var monitors = [];

profile.add_custom_monitor = function(path, fn, hook)
{
  monitors.push({
    path:path,
    fn:fn,
    hook:hook
  });
}

profile.add_custom_monitor('rusage', os.rusage, stat => stat.ru_maxrss *= 1024);
profile.add_custom_monitor('mallinfo', os.mallinfo);
profile.add_custom_monitor('memory', os.mem);

var fps = [];
var frame_lead = 1;
var fps_t = 0;
profile.report_frame = function (t) {
  fps.push(t);
  if (profile.secs(profile.now() - fps_t) > frame_lead) {
    profile.snapshot.fps = Math.mean(fps);
    fps.length = 0;
    fps_t = profile.now();
    get_snapshot();
  }
}

profile.reports = {};

profile.report = function(path)
{
  profile.reports[path] ??= {
    report: path,
    time: 0,
    hits: 0,
    avg: 0
  };
  if (profile.reports[path].st) return;
  profile.reports[path].st = profile.now();
}

profile.endreport = function endreport(path)
{
  var rep = profile.reports[path];
  if (!rep || !rep.st) return;
  rep.hits++;
  rep.time += profile.now()-rep.st;
  delete rep.st;  
  rep.avg = rep.time/rep.hits;

  profile.report_cache = Object.values(profile.reports);
}

function prof_add_stats(obj, stat) {
  for (var i in stat) {
    obj[i] ??= [];
    if (obj[i].last() !== stat[i]) obj[i][profile.curframe] = stat[i];
  }
}

profile.pushdata = function (arr, val) {
  if (arr.last() !== val) arr[profile.curframe] = val;
};

profile.capturing = false;
profile.capture_data = function () {
  if (!profile.capturing && profile.data.memory.malloc_size) return;
  prof_add_stats(profile.data.memory, os.mem());
  prof_add_stats(profile.data.gfx, imgui.framestats());
  prof_add_stats(profile.data.actors, actor.__stats());
  profile.curframe++;
};

profile.best_mem = function (bytes) {
  var sizes = ["Bytes", "KB", "MB", "GB", "TB"];
  if (bytes == 0) return "0 Bytes";
  var i = parseInt(Math.floor(Math.log(bytes) / Math.log(1024)));
  return (bytes / Math.pow(1024, i)).toPrecision(3) + " " + sizes[i];
};

profile.cleardata = function () {
  profile.data.gpu = {};
  profile.data.physics = {};
  profile.data.script = {};
  profile.data.memory = {};
  profile.data.gfx = {};
  profile.data.actors = {};
  profile.data.cpu = {
    scripts: [],
    render: [],
    physics: [],
  };
};

profile.cleardata();

profile.last_mem = undefined;
profile.mems = [];
profile.gcs = [];
profile.print_gc = function () {
  var gc = os.check_gc();
  if (!gc) return;
  profile.data.gc ??= [];
  profile.data.gc[profile.curframe] = gc;
};

return { profile };
