param(
  [switch]$SelfContained
)

$ErrorActionPreference = 'Stop'

$proj = Join-Path $PSScriptRoot 'CustomDropEventEditor.csproj'

$sc = if ($SelfContained) { 'true' } else { 'false' }

Write-Host "Publishing CustomDropEventEditor (win-x64, single-file)..."

dotnet publish $proj -c Release -r win-x64 `
  /p:SelfContained=$sc `
  /p:PublishSingleFile=true `
  /p:PublishTrimmed=false `
  /p:PublishReadyToRun=true `
  /p:IncludeNativeLibrariesForSelfExtract=true `
  --nologo

$publishDir = Join-Path $PSScriptRoot 'bin/Release/net8.0-windows/win-x64/publish'
if (Test-Path $publishDir) {
  Write-Host "Publish output: $publishDir" -ForegroundColor Green
  Get-ChildItem $publishDir | Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
} else {
  Write-Warning "Publish directory not found: $publishDir"
}
