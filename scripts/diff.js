function deep_copy(from) { return json.decode(json.encode(from)); }

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
      if (!isFinite(v)) v = null; // Squash infinity to null
      if (v !== to[key])
	ret[key] = v;
      return;
    }

    if (!to || v !== to[key])
      ret[key] = v;
  });
  if (Object.empty(ret)) return undefined;
  
  return ret;
}

ediff.doc = "Given a from and to object, returns an object that, if applied to from, will make it the same as to. Does not include deletion; it is only additive. If one element in an array is different, the entire array is copied. Squashes infinite numbers to null for use in JSON.";

function samediff(from, to)
{
  var same = [];
  if (!to) return same;
  if (typeof to !== 'object') {
    console.warn("'To' must be an object. Got " + to);
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

return {
  deep_copy,
  ediff,
  samediff,
}
