var panel;

self.screengui = function()
{
  if (panel) panel.gui();
}

self.prompt = function(msg = "prompt", value = "", list = [], cb = function() {})
{
  console.info(`creating popup`);
  panel = Object.create(listpanel);
  panel.title = msg;
  panel.value = value;
  panel.allassets = list;
  panel.action = function() {
    cb(panel.value);
    panel = undefined;
  }
  panel.start();
  player[0].control(panel);
}