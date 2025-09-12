param(
    [string]$Runtime = 'win-x64',
    [string]$VersionStamp
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function New-Timestamp {
    param([string]$Stamp)
    if ([string]::IsNullOrWhiteSpace($Stamp)) {
        return (Get-Date -Format 'yyyyMMdd-HHmmss')
    }
    return ($Stamp -replace '[^0-9A-Za-z-_]', '_')
}

function Publish-Project {
    param(
        [Parameter(Mandatory=$true)][string]$ProjectPath,
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$OutRoot,
        [Parameter(Mandatory=$true)][string]$Runtime
    )

    Write-Host ":: Publishing $Name (self-contained)" -ForegroundColor Cyan
    $outSC = Join-Path $OutRoot (Join-Path $Name 'self-contained')
    dotnet publish "$ProjectPath" -c Release -r $Runtime --self-contained true -p:PublishSingleFile=true -p:IncludeAllContentForSelfExtract=true -o "$outSC" | Out-Null

    Write-Host ":: Publishing $Name (framework-dependent)" -ForegroundColor Cyan
    $outFD = Join-Path $OutRoot (Join-Path $Name 'framework-dependent')
    dotnet publish "$ProjectPath" -c Release -r $Runtime --self-contained false -p:PublishSingleFile=true -o "$outFD" | Out-Null

    return [PSCustomObject]@{ Name=$Name; SC=$outSC; FD=$outFD }
}

# Resolve repo structure
$root = Split-Path -Parent $PSCommandPath
$stamp = New-Timestamp -Stamp $VersionStamp
$outRoot = Join-Path $root (Join-Path 'publish' $stamp)
New-Item -ItemType Directory -Path $outRoot -Force | Out-Null

$projects = @(
    @{ Path = Join-Path $root 'WpsStageGen\WpsStageGen.csproj'; Name = 'WpsStageGen' },
    @{ Path = Join-Path $root 'WpsStageGen.UI\WpsStageGen.UI.csproj'; Name = 'WpsStageGen.UI' },
    @{ Path = Join-Path $root 'CustomDropEventEditor\CustomDropEventEditor.csproj'; Name = 'CustomDropEventEditor' }
)

$results = @()
foreach ($p in $projects) {
    if (-not (Test-Path $p.Path)) { Write-Warning "Missing project: $($p.Path)"; continue }
    $results += Publish-Project -ProjectPath $p.Path -Name $p.Name -OutRoot $outRoot -Runtime $Runtime
}

Write-Host "`nPublish complete → $outRoot" -ForegroundColor Green

# Print summary sizes
foreach ($r in $results) {
    $pairs = @(@{Kind='SC';Dir=$r.SC}, @{Kind='FD';Dir=$r.FD})
    foreach ($p in $pairs) {
        $dir = $p.Dir
        if (-not (Test-Path $dir)) { continue }
        $exe = Get-ChildItem -Path $dir -Filter *.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -ne $exe) {
            Write-Host ("  {0} [{1}] -> {2:N0} bytes" -f $r.Name, $p.Kind, $exe.Length)
        }
    }
}
