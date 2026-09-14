param(
  [string]$BuildRoot = "./build",
  [string]$OutputPath = "./build/SBOM/zero-virtual-console.spdx.json"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$dependencyPath = Join-Path $BuildRoot "DependencyReports/dependency-surface.json"
if (-not (Test-Path $dependencyPath -PathType Leaf)) { throw "Missing dependency evidence: $dependencyPath" }
$dependency = Get-Content $dependencyPath -Raw | ConvertFrom-Json
if ($dependency.result -ne "PASS") { throw "SBOM generation requires a passing dependency-surface report." }

$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { (& git -C $repoRoot rev-parse HEAD).Trim() }
if (-not $commit -or $commit -eq "unknown") { throw "Unable to resolve exact source commit for SBOM." }

$files = New-Object System.Collections.Generic.List[object]
$releaseRoot = Join-Path $BuildRoot "Release"
if (-not (Test-Path $releaseRoot -PathType Container)) { throw "Missing Release output: $releaseRoot" }
Get-ChildItem $releaseRoot -File | Sort-Object Name | ForEach-Object {
  $hash = (Get-FileHash -Algorithm SHA256 $_.FullName).Hash.ToLowerInvariant()
  $files.Add([ordered]@{
    SPDXID = "SPDXRef-File-$($_.Name -replace '[^A-Za-z0-9.-]','-')"
    fileName = "Release/$($_.Name)"
    checksums = @([ordered]@{ algorithm = "SHA256"; checksumValue = $hash })
    fileTypes = @("BINARY")
  })
}

$namespace = "https://zero.local/spdx/$commit"
$document = [ordered]@{
  spdxVersion = "SPDX-2.3"
  dataLicense = "CC0-1.0"
  SPDXID = "SPDXRef-DOCUMENT"
  name = "ZERO-Virtual-Console-$commit"
  documentNamespace = $namespace
  creationInfo = [ordered]@{
    created = [DateTime]::UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ")
    creators = @("Tool: ZERO-generate-sbom.ps1")
  }
  packages = @([ordered]@{
    name = "ZERO Virtual Console"
    SPDXID = "SPDXRef-Package-ZERO-Virtual-Console"
    versionInfo = "Runtime V4.1"
    downloadLocation = "NOASSERTION"
    filesAnalyzed = $true
    licenseConcluded = "NOASSERTION"
    licenseDeclared = "NOASSERTION"
    copyrightText = "NOASSERTION"
    externalRefs = @([ordered]@{
      referenceCategory = "OTHER"
      referenceType = "zero-source-commit"
      referenceLocator = $commit
    })
  })
  files = $files.ToArray()
  relationships = @($files | ForEach-Object { [ordered]@{
    spdxElementId = "SPDXRef-Package-ZERO-Virtual-Console"
    relationshipType = "CONTAINS"
    relatedSpdxElement = $_.SPDXID
  }})
  annotations = @([ordered]@{
    annotationDate = [DateTime]::UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ")
    annotationType = "OTHER"
    annotator = "Tool: ZERO-generate-sbom.ps1"
    comment = "Dependency model: $($dependency.dependency_model); dependency gate: $($dependency.result)."
  })
}

$dir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$document | ConvertTo-Json -Depth 16 | Set-Content -Encoding UTF8 -Path $OutputPath
if ((Get-Item $OutputPath).Length -le 0) { throw "Generated SBOM is empty." }
Write-Host "ZERO SPDX SBOM: $OutputPath"
