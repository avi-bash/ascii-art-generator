#how to

ascii-translation.exe [video-path] [red] [green] [blue] [-t|-o] [-tt value] [-g] [-gs value]
ascii-translation-gpu.exe [video-path] [red] [green] [blue] [-t|-o] [-tt value] [-g] [-gs value]

The transparent overlay is enabled by default. Use "-o" ("--opaque") or
"--no-transparent" to render the black background. Use "-t" ("--transparent")
to explicitly enable the overlay.

Use "--glow-strength" or "-gs" with any non-negative number to customize the
glow brightness. "--glow" and "-g" enable the effect. For example:

ascii-translation.exe c:\...\downloads\bird.mp4 -t -g -gs 3

The default strength is
"5.0" for the CPU renderer and "5" for the GPU renderer.

Use "--transparent-threshold" or "-tt" with an integer from 0 to 255 to
control which dark pixels are omitted in transparent mode. The default is 16.