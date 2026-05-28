# SimpleDisplay (merged into PlotToSerial)

This example has been **merged into [PlotToSerial](../PlotToSerial)**.

`PlotToSerial` now produces serial output **and** draws the on-screen display when
an LCD is present. The presence of a display is detected at runtime
(`M5.Display.width() > 0`):

- **With an LCD** (Core, Core2, CoreS3, Tab5, …): the same rich display the old
  `SimpleDisplay` example provided (typed string, modifier indicators, key-state
  bits; a compact layout on small screens) is rendered, in addition to the serial
  output.
- **Without an LCD** (Atom, NanoC6, StampS3, …): serial output only; no drawing.

See [`../PlotToSerial/PlotToSerial.ino`](../PlotToSerial/PlotToSerial.ino).
