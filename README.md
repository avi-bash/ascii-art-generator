## ASCII Art Generator

Turn a video into a live ASCII-art display with the CPU renderer.

## Installation

### Requirements

- 64-bit Windows
- A video file in a format supported by the bundled OpenCV video backends
The installer includes the application executable, OpenCV, the FFmpeg video
backend, and the required Microsoft Visual C++ runtime DLLs.

### Install from the setup executable

1. Obtain `ascii-translation-setup.exe` from the project's release files.
2. Double-click the setup executable. If Windows shows a security prompt,
	verify that the file came from the expected source before allowing it to run.
3. Accept the license and choose an installation folder, or keep the default
	`C:\Program Files\ASCII Art Generator` folder.
4. Finish the installation. The installer creates a Start Menu shortcut and a
	desktop shortcut.

To remove the application later, use **Installed apps** in Windows Settings or
the **Uninstall** entry in the Start Menu. The installer does not copy or
modify your video files.

## Running the application

### Using the shortcuts

1. Open **ASCII Art Generator** from the Start Menu or desktop.
2. The `ASCII Settings` window opens. Drag a video file onto the bordered drop
	box.
3. Playback starts in the output window. The settings window stays available
	while the video plays, so another video can be dropped at any time.
4. Use **QUIT** in the settings window, or close the output window, to exit.

### Running from a terminal

Open PowerShell or Command Prompt and change to the installation folder:

```powershell
cd "C:\Program Files\ASCII Art Generator"
```

Run the executable without arguments to open the settings window:

```powershell
.\ascii-translation.exe
```

To start directly with a video, pass its full path. Quoting is required when
the path contains spaces:

```powershell
.\ascii-translation.exe "C:\Users\you\Videos\sample video.mp4"
```

The command-line options are:

```text
ascii-translation.exe [video-path] [red] [green] [blue] [options]
```

The optional color values are integers from 0 to 255 in red, green, blue
order. For example, this starts a transparent cyan display with glow:

```powershell
.\ascii-translation.exe "C:\Videos\bird.mp4" 0 220 255 -t -g -gs 3
```

### Build the installer

On a Windows machine with Visual Studio Build Tools, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-installer.ps1
```

The script builds the CPU renderer, stages the OpenCV, FFmpeg, and Microsoft
Visual C++ runtime DLLs,
downloads Inno Setup 6 if it is not installed, and creates
`build\installer\ascii-translation-setup.exe`. It targets 64-bit Windows
because the bundled OpenCV binaries are 64-bit.

The transparent overlay is enabled by default. Use "-o" ("--opaque") or
"--no-transparent" to render the black background. Use "-t" ("--transparent")
to explicitly enable the overlay.

Use "--glow-strength" or "-gs" with any non-negative number to customize the
glow brightness. "--glow" and "-g" enable the effect. For example:

ascii-translation.exe c:\...\downloads\bird.mp4 -t -g -gs 3

The default glow strength is "5.0".

## Live settings

The renderer opens an `ASCII Settings` window before and while the video is playing.
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

The command-line options are also supported for scripting and for setting the
initial values.

Use "--glow-falloff" or "-gf" with any non-negative number to customize the
glow spread. Lower values create a shorter, tighter glow, while higher values
create a longer, softer falloff. The default is "5.0". For
example:

ascii-translation.exe c:\...\downloads\bird.mp4 -g -gf 3

Use "--glow-resolution" or "-gr" with a value from 0.1 to 1.0 to control the
resolution used while rendering the glow. Higher values produce smoother edges
but use more processing time. The default is "0.5". For example:

ascii-translation.exe c:\...\downloads\bird.mp4 -g -gf 8 -gr 0.75

Use "--glow-radius" or "-grd" with a non-negative integer to limit the glow
radius in final output pixels. A value of 0 uses the automatic blur kernel.
For example:

ascii-translation.exe c:\...\downloads\bird.mp4 -g -gf 8 -grd 32

Use "--transparent-threshold" or "-tt" with an integer from 0 to 255 to
control which dark pixels are omitted in transparent mode. The default is 16.
