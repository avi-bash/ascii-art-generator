$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $MyInvocation.MyCommand.Path
$source = Join-Path $project 'ascii-translation.cpp'
$build = Join-Path $project 'build\Debug'
$opencvRoot = Join-Path $project 'opencv\build'

function Find-File([string]$Root, [string]$Name) {
    return Get-ChildItem -Path $Root -Filter $Name -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
}

if (-not (Test-Path $source)) {
    throw "ascii-translation.cpp was not found in $project."
}

$header = Join-Path $opencvRoot 'include\opencv2\opencv.hpp'
$library = Find-File $opencvRoot 'opencv_world*.lib'
$dll = $null
if ($library) {
    $dllName = [System.IO.Path]::ChangeExtension($library.Name, '.dll')
    $dll = Find-File (Join-Path $project 'opencv') $dllName
}

if (-not (Test-Path $header) -or -not $library -or -not $dll) {
    $download = Join-Path $project 'opencv-windows.exe'
    $temp = Join-Path $project '.opencv-download'
    $url = 'https://github.com/opencv/opencv/releases/download/4.10.0/opencv-4.10.0-windows.exe'
    Write-Host 'OpenCV is missing or incomplete. Downloading the official Windows package...'
    Invoke-WebRequest -Uri $url -OutFile $download
    Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $temp | Out-Null
    Write-Host 'Extracting OpenCV into a temporary folder...'
    $extract = Start-Process -FilePath $download -ArgumentList @('-y', "-o$temp") -Wait -PassThru
    if ($extract.ExitCode -ne 0) {
        throw "OpenCV extraction failed with exit code $($extract.ExitCode)."
    }
    Remove-Item $download -Force

    $downloaded = Get-ChildItem $temp -Directory -Recurse -Filter 'build' | Select-Object -First 1
    if (-not $downloaded) {
        throw 'OpenCV extraction did not produce opencv\build. Run setup again after checking the downloaded installer.'
    }
    Copy-Item $downloaded.FullName (Join-Path $project 'opencv') -Recurse -Force
    Remove-Item $temp -Recurse -Force
    $header = Join-Path (Join-Path $project 'opencv\build') 'include\opencv2\opencv.hpp'
    $library = Find-File (Join-Path $project 'opencv\build') 'opencv_world*.lib'
    $dll = Find-File (Join-Path $project 'opencv') 'opencv_world*.dll'
}

$vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$cl = $null
if (Test-Path $vsWhere) {
    $installation = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($installation) {
        $cl = Get-ChildItem (Join-Path $installation 'VC\Tools\MSVC') -Filter cl.exe -File -Recurse |
            Where-Object { $_.FullName -match '\\Hostx64\\x64\\cl\.exe$' } |
            Select-Object -First 1
    }
}
if (-not $cl) {
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
}
if (-not $cl) {
    throw 'MSVC was not found. Install Visual Studio Build Tools with the Desktop development with C++ workload, then run setup.bat again.'
}

New-Item -ItemType Directory -Force -Path $build | Out-Null
$exe = Join-Path $build 'ascii-translation.exe'
$libDir = Split-Path $library.FullName -Parent
$dllDir = Split-Path $dll.FullName -Parent
$env:Path = "$dllDir;$env:Path"

Write-Host "Compiling with $($cl.FullName)..."
& $cl.FullName /nologo /std:c++17 /EHsc "/I$($header | Split-Path -Parent | Split-Path -Parent)" $source "/Fe:$exe" /link "/LIBPATH:$libDir" $library.Name
if ($LASTEXITCODE -ne 0) {
    throw 'The C++ build failed.'
}

Copy-Item $dll.FullName $build -Force
Write-Host "Built $exe"
if (Test-Path (Join-Path $project 'bird.mp4')) {
    Write-Host 'Run build\Debug\ascii-translation.exe to play bird.mp4.'
} else {
    Write-Host 'Add a video file, then run: build\Debug\ascii-translation.exe path\to\video.mp4'
}