## How to use

Run `ascii-translation.exe` or `ascii-translation-gpu.exe` without arguments
to open the settings menu. Drag any video file onto the drop box to start
playback. The same menu remains available while the video is playing, so you
can drop another video there to switch it.

For scripting, a video path can still be supplied as the first argument:

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

## Live settings

Both renderers open an `ASCII Settings` window before and while the video is playing.
Use its checkboxes and sliders to change transparency, text color, threshold,
ASCII resolution, and glow settings without restarting the video. `ASCII
resolution (columns)` controls the rendered character detail from 40 to 300
columns. The output window can also be resized; the rendered image scales to
fit it while preserving the video aspect ratio. The `Glow strength x10` and
`Glow falloff x10` values are displayed ten times larger than their actual
decimal values; for example, `50` means `5.0`.

Use `Video display width` and `Video display height` to change the output
window size independently while the video is playing.

Use `Video window scale (%)` to resize both dimensions together while keeping
the current video aspect ratio.

Right-click any slider to reset that option to its default value.

Drop a video file onto the bordered drop box in the settings window to switch
the current preview. Playback restarts from the beginning of the new file.

The command-line options below are still supported for scripting and for
setting the initial values.

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