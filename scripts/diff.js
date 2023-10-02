function deep_copy(from) {
  if (typeof from !== 'object')
    return from;

  if (Array.isArray(from)) {
    var c = [];
    from.forEach(function(x,i) { c[i] = deep_copy(x); });
    return c;
  }
    
  var obj = {};
  for (var key in from)
    obj[key] = deep_copy(from[key]);

  return obj;
};


var walk_up_get_prop = function(obj, prop, endobj) {
  var props = [];
  var cur = obj;
  while (cur !== Object.prototype) {
    if (cur.hasOwn(prop))
      props.push(cur[prop]);

    cur = cur.__proto__;
  }

  return props;
};

/* Deeply remove source keys from target, not removing objects */
function unmerge(target, source) {
  for (var key in source) {
    if (typeof source[key] === 'object' && !Array.isArray(source[key]))
      unmerge(target[key], source[key]);
    else
      delete target[key];
  }
};

/* Deeply merge two objects, not clobbering objects on target with objects on source */
function deep_merge(target, source)
{
  Log.warn("Doing a deep merge ...");
  for (var key in source) {
    if (typeof source[key] === 'object' && !Array.isArray(source[key])) {
            Log.warn(`Deeper merge on ${key}`);
      deep_merge(target[key], source[key]);
    }
    else {
      Log.warn(`Setting key ${key}`);    
      target[key] = source[key];
    }
  }
};


function equal(x,y) {
  if (typeof x === 'object')
    for (var key in x)
      return equal(x[key],y[key]);

  return x === y;
};

function diffassign(target, from) {
  if (from.empty) return;

  for (var e in from) {
    if (typeof from[e] === 'object') {
      if (!target.hasOwnProperty(e))
        target[e] = from[e];
      else 
        diffassign(target[e], from[e]);
    } else {
      if (from[e] === "DELETE") {
        delete target[e];
      } else {
        target[e] = from[e];
      }
    }
  }
};

function positive_diff(from, to)
{
  var diff = {};
}

function diff(from, to) {
  var obj = {};

  for (var e in to) {
    if (typeof to[e] === 'object' && from.hasOwnProperty(e)) {
      obj[e] = diff(from[e], to[e]);
      if (obj[e].empty)
        delete obj[e];
    } else {
      if (from[e] !== to[e])
        obj[e] = to[e];
    }
  }

  for (var e in from) {
    if (!to.hasOwnProperty(e))
      obj[e] = "DELETE";
  }

  return obj;
};
