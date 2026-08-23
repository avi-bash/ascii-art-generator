#how to

ascii-translation.exe [video-path] [red] [green] [blue] [-t|-o] [-tt value] [-g] [-gs value] [-gf value] [-gr value] [-grd value]
ascii-translation-gpu.exe [video-path] [red] [green] [blue] [-t|-o] [-tt value] [-g] [-gs value] [-gf value] [-gr value] [-grd value]

The transparent overlay is enabled by default. Use "-o" ("--opaque") or
"--no-transparent" to render the black background. Use "-t" ("--transparent")
to explicitly enable the overlay.

Use "--glow-strength" or "-gs" with any non-negative number to customize the
glow brightness. "--glow" and "-g" enable the effect. For example:

ascii-translation.exe c:\...\downloads\bird.mp4 -t -g -gs 3

The default strength is
"5.0" for the CPU renderer and "5" for the GPU renderer.

Use "--glow-falloff" or "-gf" with any non-negative number to customize the
glow spread. Lower values create a shorter, tighter glow, while higher values
create a longer, softer falloff. The default is "5.0" for both renderers. For
example:

ascii-translation-gpu.exe c:\...\downloads\bird.mp4 -g -gf 3

Use "--glow-resolution" or "-gr" with a value from 0.1 to 1.0 to control the
resolution used while rendering the glow. Higher values produce smoother edges
but use more processing time. The default is "0.5". For example:

ascii-translation.exe c:\...\downloads\bird.mp4 -g -gf 8 -gr 0.75

Use "--glow-radius" or "-grd" with a non-negative integer to limit the glow
radius in final output pixels. A value of 0 uses the automatic blur kernel.
For example:

ascii-translation-gpu.exe c:\...\downloads\bird.mp4 -g -gf 8 -grd 32

Use "--transparent-threshold" or "-tt" with an integer from 0 to 255 to
control which dark pixels are omitted in transparent mode. The default is 16.