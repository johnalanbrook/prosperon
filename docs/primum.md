# Primum

The core tenents of Primum are as follows. When choosing features for Primum, these are the guidelines we follow.
- Gameplay first. Visuals are important but should be chosen to be more simple if it makes implementing gameplay more difficult.
- Dynamic first. Video games are dynamic, so as much as possible should be dynamically generated. For example, signed distance fields for particle system collision will not be used, as it requires baking sometimes false geometry. We include midi playback, which can be changed in real time far easier than wavs or mp3s.
- Marriage of code and editor. Neither completely replaces the other. What is easier to do in code should be done in code, and what should be done in editor should be done in editor. No solutions try to step on the toes of the other solution.