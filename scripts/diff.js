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

function ediff(from,to)
{
  var ret = {};
  
  if (!to)
    return ediff(from,{});
  
  Object.entries(from).forEach(function([key,v]) {
    if (typeof v === 'function') return;
    if (typeof v === 'undefined') return;

    if (Array.isArray(v)) {
      if (!Array.isArray(to[key]) || v.length !== to[key].length)
	ret[key] = Object.values(ediff(v, []));

      var diff = ediff(from[key], to[key]);
      if (diff && !diff.empty)
	ret[key] = Object.values(ediff(v,[]));

      return;
    }

    if (typeof v === 'object') {
      var diff = ediff(v, to[key]);
      if (diff && !diff.empty)
	  ret[key] = diff;	
      return;
    }

    if (typeof v === 'number') {
      var a = Number.prec(v);
      if (!to || a !== to[key])
	ret[key] = a;
      return;
    }

    if (!to || v !== to[key])
      ret[key] = v;
  });
  
  return ret;
}
