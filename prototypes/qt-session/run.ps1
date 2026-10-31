param([switch]$BuildOnly,[switch]$Launch,[string]$Capture)
$ErrorActionPreference='Stop'
Set-Location -LiteralPath $PSScriptRoot
$qtRoot=if($env:OWL_QT_ROOT){$env:OWL_QT_ROOT}else{'C:/Qt/6.10.2/msvc2022_64'}
$env:PATH="$qtRoot/bin;$env:PATH"
New-Item -ItemType Directory -Force -Path artifacts | Out-Null
& cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$qtRoot"
if($LASTEXITCODE){throw 'Configuration failed.'}
& cmake --build build --config Release --parallel 8
if($LASTEXITCODE){throw 'Build failed.'}
if($BuildOnly){exit 0}
$debugPath=Join-Path $PSScriptRoot 'build/Release/owl-debug-spike.exe'
if($Capture){
    $captureProcess=Start-Process -FilePath $debugPath -ArgumentList @('--capture',('"'+$Capture+'"'),'-platform','offscreen') -WindowStyle Hidden -PassThru
    $captureProcess.WaitForExit()
    exit $captureProcess.ExitCode
}
if($Launch){Start-Process -FilePath $debugPath -WorkingDirectory $PSScriptRoot -WindowStyle Normal;exit 0}
& $debugPath
exit $LASTEXITCODE
