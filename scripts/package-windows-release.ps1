param(
    [string]$Version = "1.0.0",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$repoRoot = (& git rev-parse --show-toplevel 2>$null)
if (-not $repoRoot) {
    throw "Run this script from inside the xlxd Git repository."
}
$repoRoot = $repoRoot.Trim()

$exe = Join-Path $repoRoot "build\windows-x64\$Configuration\ambed.exe"
$license = Join-Path $repoRoot "license.txt"
$readme = Join-Path $repoRoot "README-WINDOWS.md"
$notices = Join-Path $repoRoot "THIRD-PARTY-NOTICES.txt"
$releaseNotes = Join-Path $repoRoot "RELEASE-NOTES-WINDOWS-1.0.0.md"

foreach ($required in @($exe, $license, $readme, $notices, $releaseNotes)) {
    if (-not (Test-Path $required)) {
        throw "Required file not found: $required"
    }
}

$releaseRoot = Join-Path $repoRoot "release"
$packageName = "AMBEd-Windows-x64-$Version"
$stage = Join-Path $releaseRoot $packageName
$zipPath = Join-Path $releaseRoot "$packageName.zip"
$zipHashPath = Join-Path $releaseRoot "$packageName.zip.sha256"

if (Test-Path $stage) {
    Remove-Item $stage -Recurse -Force
}
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
if (Test-Path $zipHashPath) {
    Remove-Item $zipHashPath -Force
}

New-Item -ItemType Directory -Path $stage -Force | Out-Null

Copy-Item $exe (Join-Path $stage "ambed.exe")
Copy-Item $license (Join-Path $stage "license.txt")
Copy-Item $readme (Join-Path $stage "README-WINDOWS.md")
Copy-Item $notices (Join-Path $stage "THIRD-PARTY-NOTICES.txt")
Copy-Item $releaseNotes (Join-Path $stage "RELEASE-NOTES.md")

$commit = (& git -C $repoRoot rev-parse HEAD).Trim()
$branch = (& git -C $repoRoot branch --show-current).Trim()
$built = Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"

@"
AMBEd Windows x64 release package
Package version: $Version
AMBEd version: 1.3.5
Git branch: $branch
Git commit: $commit
Packaged: $built

FTDI D2XX runtime is not included.
See README-WINDOWS.md and THIRD-PARTY-NOTICES.txt.
"@ | Set-Content -Path (Join-Path $stage "BUILD-INFO.txt") -Encoding UTF8

$filesToHash = Get-ChildItem $stage -File |
    Where-Object { $_.Name -ne "SHA256SUMS.txt" } |
    Sort-Object Name

$hashLines = foreach ($file in $filesToHash) {
    $hash = (Get-FileHash $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($file.Name)"
}

$hashLines | Set-Content -Path (Join-Path $stage "SHA256SUMS.txt") -Encoding ASCII

Compress-Archive -Path $stage -DestinationPath $zipPath -CompressionLevel Optimal

$zipHash = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
"$zipHash  $packageName.zip" |
    Set-Content -Path $zipHashPath -Encoding ASCII

Write-Host ""
Write-Host "Release package created:"
Write-Host "  $zipPath"
Write-Host ""
Write-Host "ZIP SHA-256:"
Write-Host "  $zipHash"
Write-Host ""
Write-Host "Package contents:"
Get-ChildItem $stage | Format-Table Name, Length
