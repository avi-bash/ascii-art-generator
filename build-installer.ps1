$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $project 'build\Debug'
$stage = Join-Path $project 'build\installer-stage'
$output = Join-Path $project 'build\installer'
$iscc = $null

function Find-InnoCompiler {
    $roots = @(
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6'),
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6')
    )
    foreach ($root in $roots) {
        $candidate = Join-Path $root 'ISCC.exe'
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

$iscc = Find-InnoCompiler

function Find-File([string]$Root, [string]$Name) {
    return Get-ChildItem -Path $Root -Filter $Name -File -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
}

Write-Host 'Building the CPU renderer...'
& (Join-Path $project 'setup.ps1')
if ($LASTEXITCODE -ne 0) {
    throw 'The renderer build failed.'
}

$opencvDll = Find-File (Join-Path $project 'opencv') 'opencv_world*.dll'
$ffmpegDll = Find-File (Join-Path $project 'opencv') 'opencv_videoio_ffmpeg*_64.dll'
if (-not $opencvDll -or -not $ffmpegDll) {
    throw 'Required OpenCV runtime DLLs were not found. Run setup.ps1 and try again.'
}
$redistRoot = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio'
$runtimeDlls = foreach ($runtimeName in @('msvcp140.dll', 'vcruntime140.dll',
        'vcruntime140_1.dll', 'concrt140.dll')) {
    $runtimeDll = Get-ChildItem -Path $redistRoot -Filter $runtimeName -File -Recurse |
        Where-Object FullName -match '\\x64\\' | Select-Object -First 1
    if (-not $runtimeDll) { throw "MSVC runtime DLL was not found: $runtimeName" }
    $runtimeDll
}

Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stage, $output | Out-Null
Copy-Item (Join-Path $build 'ascii-translation.exe') $stage
Copy-Item $opencvDll.FullName $stage
Copy-Item $ffmpegDll.FullName $stage
Copy-Item $runtimeDlls.FullName $stage
Copy-Item (Join-Path $project 'README.md') $stage

if (-not (Test-Path $iscc)) {
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
    if (-not $winget) {
        throw 'Inno Setup 6 is not installed and winget.exe is unavailable. Install Inno Setup 6, then run this script again.'
    }
    Write-Host 'Inno Setup 6 was not found. Installing it with winget...'
    & $winget.Source install --id JRSoftware.InnoSetup --exact --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw 'winget failed to install Inno Setup 6.' }
    $iscc = Find-InnoCompiler
}
if (-not (Test-Path $iscc)) {
    throw "Inno Setup compiler was not found at $iscc."
}

$defines = @(
    "/DStageDir=$stage"
    "/DOutputDir=$output"
)

Write-Host 'Building installer...'
& $iscc (Join-Path $project 'installer.iss') $defines
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup failed to build the installer.' }
Write-Host "Created $(Join-Path $output 'ascii-translation-setup.exe')"