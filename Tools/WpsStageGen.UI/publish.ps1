Param(
    [string]$Configuration = "Release",
    [string]$Runtime = "win-x64",
    [switch]$VersionStamp
)

$ErrorActionPreference = 'Stop'

$project = Join-Path $PSScriptRoot 'WpsStageGen.UI.csproj'
$generatorProject = Join-Path (Split-Path $PSScriptRoot -Parent) 'WpsStageGen\WpsStageGen.csproj'
$templatesSrc = Join-Path (Split-Path $PSScriptRoot -Parent) 'WpsStageGen\templates'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outScRoot = Join-Path $PSScriptRoot 'publish'
$outFdRoot = Join-Path $PSScriptRoot 'publish-fd'
$outFdMultiRoot = Join-Path $PSScriptRoot 'publish-fd-multi'
$outSc = Join-Path $outScRoot $stamp
$outFd = Join-Path $outFdRoot $stamp
$outFdMulti = Join-Path $outFdMultiRoot $stamp

if ($VersionStamp) {
    $date = Get-Date -Format 'yyyy.MM.dd.HHmm'
    $git = $null
    try { $git = (git rev-parse --short HEAD) 2>$null } catch {}
    $ver = if ($git) { "$date-$git" } else { $date }
    Write-Host "Version stamp: $ver" -ForegroundColor Cyan
    $pVersion = @("-p:Version=$ver","-p:AssemblyVersion=1.0.0.0","-p:FileVersion=1.0.0.0","-p:InformationalVersion=$ver")
} else {
    $pVersion = @()
}

Write-Host "Publishing self-contained (single-file) to $outSc" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $outSc | Out-Null
dotnet publish $project -c $Configuration -r $Runtime -p:PublishSingleFile=true -p:SelfContained=true -p:IncludeNativeLibrariesForSelfExtract=true -p:PublishTrimmed=false -o $outSc @pVersion

Write-Host "Publishing framework-dependent (single-file) to $outFd" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $outFd | Out-Null
dotnet publish $project -c $Configuration -r $Runtime -p:PublishSingleFile=true -p:SelfContained=false -p:PublishTrimmed=false -o $outFd @pVersion

# Extra: Publish framework-dependent (multi-file) to include WpsStageGen.UI.dll explicitly
Write-Host "Publishing framework-dependent (multi-file) to $outFdMulti" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $outFdMulti | Out-Null
dotnet publish $project -c $Configuration -r $Runtime -p:PublishSingleFile=false -p:SelfContained=false -p:PublishTrimmed=false -o $outFdMulti @pVersion

# Copy templates folder next to all UI publish outputs
function Copy-Templates($dest) {
    if (-not (Test-Path $dest)) { return }
    if (Test-Path $templatesSrc) {
        $templatesDest = Join-Path $dest 'templates'
        Write-Host "Copying templates -> $templatesDest" -ForegroundColor Cyan
        New-Item -ItemType Directory -Force -Path $templatesDest | Out-Null
        Copy-Item -Path (Join-Path $templatesSrc '*') -Destination $templatesDest -Recurse -Force -ErrorAction SilentlyContinue
    } else {
        Write-Host "Templates source not found: $templatesSrc" -ForegroundColor DarkYellow
    }
}

# Publish the generator and place artifacts next to UI outputs so the UI can run it directly
function Publish-Generator($dest, [bool]$SelfContained) {
    if (-not (Test-Path $dest)) { return }
    if (-not (Test-Path $generatorProject)) { Write-Host "Generator project missing: $generatorProject" -ForegroundColor Yellow; return }

    $genOut = Join-Path $dest 'generator'
    New-Item -ItemType Directory -Force -Path $genOut | Out-Null
    $scFlag = if ($SelfContained) { 'true' } else { 'false' }
    $singleFile = if ($SelfContained) { 'true' } else { 'true' } # keep single-file for FD to simplify
    Write-Host "Publishing WpsStageGen (SelfContained=$SelfContained) -> $genOut" -ForegroundColor Cyan
    dotnet publish $generatorProject -c $Configuration -r $Runtime -p:PublishSingleFile=$singleFile -p:SelfContained=$scFlag -p:PublishTrimmed=false -o $genOut @pVersion | Out-Null

    # For convenience, also copy the main exe/dll to the parent folder so ResolveGeneratorInvocation finds it
    $exe = Get-ChildItem -Path $genOut -Filter WpsStageGen.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($exe) { Copy-Item $exe.FullName (Join-Path $dest 'WpsStageGen.exe') -Force }
    $dll = Get-ChildItem -Path $genOut -Filter WpsStageGen.dll -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($dll) { Copy-Item $dll.FullName (Join-Path $dest 'WpsStageGen.dll') -Force }
}

# Post-steps for each publish output
Copy-Templates -dest $outSc
Copy-Templates -dest $outFd
Copy-Templates -dest $outFdMulti

Publish-Generator -dest $outSc -SelfContained $true
Publish-Generator -dest $outFd -SelfContained $false
Publish-Generator -dest $outFdMulti -SelfContained $false

Write-Host "Done." -ForegroundColor Green

Write-Host "\nSelf-contained output:" -ForegroundColor Yellow
Get-ChildItem -File $outSc | Select-Object Name,Length | Format-Table -AutoSize
Write-Host "\nFramework-dependent (single-file) output:" -ForegroundColor Yellow
Get-ChildItem -File $outFd | Select-Object Name,Length | Format-Table -AutoSize
Write-Host "\nFramework-dependent (multi-file) output (dll present):" -ForegroundColor Yellow
Get-ChildItem -File $outFdMulti | Select-Object Name,Length | Format-Table -AutoSize
