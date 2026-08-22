## Usage

ascii-translation.exe [video-path] [red] [green] [blue] [-g] [-gs value]
ascii-translation-gpu.exe [video-path] [red] [green] [blue] [-g] [-gs value]

Use `--glow-strength` or `-gs` with any non-negative number to customize the
glow brightness. `--glow` and `-g` enable the effect. For example:

ascii-translation.exe bird.mp4 -g -gs 3

The default strength is
`5.0` for the CPU renderer and `2.5` for the GPU renderer.