Param(
    [string]$Configuration = "Release",
    [string]$Runtime = "win-x64",
    [switch]$VersionStamp
)

$ErrorActionPreference = 'Stop'

$project = Join-Path $PSScriptRoot 'WpsStageGen.UI.csproj'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outScRoot = Join-Path $PSScriptRoot 'publish'
$outFdRoot = Join-Path $PSScriptRoot 'publish-fd'
$outSc = Join-Path $outScRoot $stamp
$outFd = Join-Path $outFdRoot $stamp

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

Write-Host "Done." -ForegroundColor Green

Write-Host "\nSelf-contained output:" -ForegroundColor Yellow
Get-ChildItem -File $outSc | Select-Object Name,Length | Format-Table -AutoSize
Write-Host "\nFramework-dependent output:" -ForegroundColor Yellow
Get-ChildItem -File $outFd | Select-Object Name,Length | Format-Table -AutoSize
