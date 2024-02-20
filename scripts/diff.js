function deep_copy(from) { return json.decode(json.encode(from)); }

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
    return json.encode(x) === json.encode(y);

  return x === y;
};

function diffassign(target, from) {
  if (Object.empty(from)) return;

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

function objdiff(from,to)
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
      if (diff && !Object.empty(diff))
	ret[key] = Object.values(ediff(v,[]));

      return;
    }

    if (typeof v === 'object') {
      var diff = ediff(v, to[key]);
      if (diff && !Object.empty(diff))
	  ret[key] = diff;	
      return;
    }

    if (typeof v === 'number') {
      if (!to || v !== to[key])
	ret[key] = v;
      return;
    }

    if (!to || v !== to[key])
      ret[key] = v;
  });
  if (Object.empty(ret)) return undefined;
  
  return ret;
}

function valdiff(from,to)
{
  if (typeof from !== typeof to) return from;
  if (typeof from === 'function') return undefined;
  if (typeof from === 'undefined') return undefined;

  if (typeof from === 'number') {
      return to;
      
    return undefined;
  }

  if (typeof from === 'object')
    return ediff(from,to);

  if (from !== to) return to;

  return undefined;
}

/* Returns the json encoded object, assuming it has an implementation it must check through */
function impl_json(obj)
{
  
}

function ndiff(from,to)
{
  
}

function ediff(from,to)
{
  var ret = {};
  
  if (!to)
//    return ediff(from, {});
    return deep_copy(from);
  
  Object.entries(from).forEach(function([key,v]) {
    if (typeof v === 'function') return;
    if (typeof v === 'undefined') return;

    if (Array.isArray(v)) {
      if (!Array.isArray(to[key]) || v.length !== to[key].length) {
        var r = ediff(v,[]);
	if (r) ret[key] = Object.values(r);
	return;
      }

      var diff = ediff(from[key], to[key]);
      if (diff && !Object.empty(diff))
	ret[key] = Object.values(ediff(v,[]));

      return;
    }

    if (typeof v === 'object') {
      var diff = ediff(v, to[key]);
      if (diff && !Object.empty(diff))
	  ret[key] = diff;	
      return;
    }

    if (typeof v === 'number') {
      if (!to || v !== to[key])
	ret[key] = v;
      return;
    }

    if (!to || v !== to[key])
      ret[key] = v;
  });
  if (Object.empty(ret)) return undefined;
  
  return ret;
}

ediff.doc = "Given a from and to object, returns an object that, if applied to from, will make it the same as to. Does not include deletion; it is only additive.";

function subdiff(from,to)
{

}

subdiff.doc = "Given a from and to object, returns a list of properties that must be deleted from the 'from' object to make it like the 'to' object.";

function samediff(from, to)
{
  var same = [];
  if (!to) return same;
  if (typeof to !== 'object') {
    Log.warn("'To' must be an object. Got " + to);
    return same;
  }
  Object.keys(from).forEach(function(k) {
    if (Object.isObject(from[k])) {
      samediff(from[k], to[k]);
      return;
    }

//    if (Array.isArray(from[k])) {
//      var d = valdiff(from[k], to[k]);
//      if (!d)
//    }

    var d = valdiff(from[k], to[k]);
    if (!d)
      delete from[k];
  });

  return same;
}

samediff.doc = "Given a from and to object, returns an array of keys that are the same on from as on to.";

function cleandiff(from, to)
{
}
