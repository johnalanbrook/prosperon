/*
  TYPES OF PROFILING

  cpu gathering - gets stack frames randomly for a few seconds

  frames - user defined to see how long engine takes

  cache - can see specific events that happened

  memory - can see how much memory is allocated and from where
*/

var t_units = ["ns", "us", "ms", "s", "ks", "Ms"];

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

profile.cpu = function profile_cpu(fn, times = 1, q = "unnamed") {
  var retgather = gathering_cpu;
  profile.gather_stop();
  var empty = calc_cpu(empty_fn, 100000);
  var mean = Math.mean(empty);
  var series = calc_cpu(fn, times, mean);

  var elapsed = Math.sum(series);
  var avgt = profile.best_t(elapsed / series.length);
  var totalt = profile.best_t(elapsed);

  say(`profile [${q}]: ${avgt} ± ${profile.best_t(Math.ci(series))} [${totalt} for ${times} loops]`);
  say(`result of function is ${fn()}`);

  if (retgather) profile.start_prof_gather();
};

profile.ms = function (t) {
  return profile.secs(t) * 1000;
};

var callgraph = {};
profile.rawstacks = {};

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
var start_gather = profile.now();

profile.cpu_start = undefined;

profile.clear_cpu = function () {
  callgraph = {};
  profile.cpu_instr = undefined;
};

profile.start_cpu_gather = function (gathertime = 5) {
  profile.clear_cpu();
  // gather cpu frames for 'time' seconds
  if (profile.cpu_start) return;
  profile.cpu_start = profile.now();
  var st = profile.cpu_start;

  profile.gather(hittar, function () {
    var time = profile.now() - st;

    var err = new Error();
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

    st = profile.now();
    if (profile.secs(st - profile.cpu_start) < gathertime) profile.gather_rate(Math.variate(hittar, hitpct));
    else {
      profile.gather_stop();
      profile.cpu_start = undefined;
      var e = Object.values(callgraph);
      e = e.filter(x => x.line);

      for (var x of e) {
        var ffs = x.line.split(":");

        x.timestr = profile.best_t(x.time);
        x.timeper = x.time / x.hits;
        x.timeperstr = profile.best_t(x.timeper);
        x.pct = (profile.secs(x.time) / gathertime) * 100;
        x.alone.timestr = profile.best_t(x.alone.time);
        x.alone.timeper = x.alone.time / x.alone.hits;
        x.alone.timeperstr = profile.best_t(x.alone.timeper);
        x.alone.pct = (profile.secs(x.alone.time) / gathertime) * 100;
        x.fncall = get_line(ffs[0], ffs[1]);
        x.log = x.line + " " + x.fn + " " + x.fncall;
        x.incl = {
          time: x.time,
          timestr: x.timestr,
          timeper: x.timeper,
          timeperstr: x.timeperstr,
          hits: x.hits,
          pct: x.pct,
        };
      }
      profile.cpu_instr = e;
    }
  });
};

function push_time(arr, ob, max) {
  arr.push({
    time: profile.now(),
    ob,
  });
}

profile.cpu_frames = [];
profile.last_cpu_frame = undefined;
profile.cpu_frame = function () {
  profile.gather(Math.random_range(300, 600), function () {
    var err = new Error();
    profile.last_cpu_frame = err.stack; //.split('\n').slicconsole.stack(2);
    profile.gather_stop();
  });
};

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

profile.best_t = function (t) {
  var qq = 0;

  while (t > 1000 && qq < t_units.length - 1) {
    t /= 1000;
    qq++;
  }

  return `${t.toPrecision(4)} ${t_units[qq]}`;
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

/*
  Cache reporting is to measure how long specific events take, that are NOT every frame
  Useful to measure things like how long it takes to make a specific creature
*/

var cache_reporting = false;

var report_cache = {};

var cachest = 0;
var cachegroup;
var cachetitle;

profile.cache_reporting = function () {
  return cache_reporting;
};
profile.cache_toggle = function () {
  cache_reporting = !cache_reporting;
};
profile.cache_dump = function () {
  report_cache = {};
};

profile.cache = function profile_cache(group, title) {
  if (!cache_reporting) return;
  cachest = profile.now();
  cachegroup = group;
  cachetitle = title;
};

profile.endcache = function profile_endcache(tag = "") {
  return;
  addreport(cachegroup, cachetitle + tag, cachest);
};

function addreport(group, line, start) {
  return;
  if (typeof group !== "string") group = "UNGROUPED";
  report_cache[group] ??= {};
  var cache = report_cache[group];
  cache[line] ??= [];
  var t = profile.now();
  cache[line].push(t - start);
  return t;
}

function printreport(cache, name) {
  var report = `==${name}==` + "\n";

  var reports = [];
  for (var i in cache) {
    var time = cache[i].reduce((a, b) => a + b);
    reports.push({
      time: time,
      name: i,
      hits: cache[i].length,
      avg: time / cache[i].length,
    });
  }
  reports = reports.sort((a, b) => {
    if (a.avg < b.avg) return 1;
    return -1;
  });

  for (var rep of reports) report += `${rep.name}    ${profile.best_t(rep.avg)} (${rep.hits} hits) (total ${profile.best_t(rep.time)})\n`;

  return report;
}

profile.data = {};
profile.curframe = 0;

profile.snapshot = {};


var classes = ["gameobject", "transform", "dsp_node", "texture", "font", "warp_gravity", "warp_damp", "sg_buffer", "datastream", "cpShape", "cpConstraint", "timer", "skin"];
var get_snapshot = function()
{
 var snap = profile.snapshot;
 snap.actors ??= {};
 Object.assign(snap.actors, actor.__stats());
 snap.memory ??= {};
 Object.assign(snap.memory, os.mem());
 snap.memory.textures = game.texture.total_size();
 snap.memory.texture_vram = game.texture.total_vram();

 snap.rusage ??= {};
 var rusage = os.rusage();
 rusage.ru_maxrss *= 1024; // delivered in KB; convert here to B
 Object.assign(snap.rusage, rusage);

 snap.mallinfo ??= {};
 Object.assign(snap.mallinfo, os.mallinfo());

 snap.particles ??= {};
 Object.assign(snap.particles, stat_emitters());

 snap.obj ??= {};
 for (var i of classes) {
   var proto = globalThis[`${i}_proto`];
   if (!proto) continue;
   snap.obj[i] = proto._count();
 }
}

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
};

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
