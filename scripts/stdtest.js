test.run("set transform scale", _ => {
  var t = os.make_transform();
  var s1 = t.scale.slice();
  t.scale = s1;
  return (t.scale.equal(s1));
});




