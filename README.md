dynamic-shadows
===============

course project: adding two types of dynamic shadows to quake3 engine

I wrote this way back in 2005/2006 for a graduate level graphics course
at the University of Alberta. Here's the abstract from my report:

We investigate two different techniques for the rendering of real-time
shadows in hardware: shadow mapping and stenciled shadow volumes.  A
renderer capable of viewing Quake3 map/model files is used as a base
from which both shadow algorithms are added.  Shadow mapping performs
well, but visual artifacts are difficult to completely eliminate.  The
shadows can also be blocky, even if the texture-size is quite
large. Shadow volumes give nice shadows, but the overhead of the
stencil write passes can severely affect the framerate. If optimized,
the shadow volume technique is probably more suitable for this
environment.

You'll need the binary files for Quake3 in order
to run this. 

TODO: Make it compile!