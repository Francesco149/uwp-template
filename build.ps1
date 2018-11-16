$invocation = (Get-Variable MyInvocation).Value
$dir = Split-Path $invocation.MyCommand.Path
$project = Split-Path $dir -Leaf
Write-Output "working dir: $dir"
Write-Output "project name: $project"

function Write-Header {
  param ([string]$Message)
  $oldcolor = $host.UI.RawUI.ForegroundColor
  $host.UI.RawUI.ForegroundColor = "Green"
  Write-Output ":: $Message"
  $host.UI.RawUI.ForegroundColor = $oldcolor
}

Push-Location $dir
$res = $null
try {
  Write-Header "cleaning"
  Remove-Item "$project.appx" -ErrorAction SilentlyContinue
  Remove-Item main.obj -ErrorAction SilentlyContinue
  Remove-Item main.exe -ErrorAction SilentlyContinue
  Remove-Item main.pdb -ErrorAction SilentlyContinue

  Write-Header "linting"
  $hasAnalyzer = Get-Command "Invoke-ScriptAnalyzer" `
    -ErrorAction SilentlyContinue
  if (-not $hasAnalyzer) {
    Write-Warning "you're missing PSScriptAnalyzer, skipping lint step"
    return
  }
  $results = Invoke-ScriptAnalyzer -Path . -Recurse
  foreach ($result in $results) {
    Write-Output ($result | Format-Table | Out-String)
  }
  if ($results.Count -ne 0) {
    Throw "did not pass ps1 linting"
  }

  Write-Header "compiling"
  cl /EHsc /ZW /Zi main.cpp /link /SUBSYSTEM:WINDOWS
  if (-not $?) {
    Throw "cl failed: $LastExitCode"
  }

  Write-Header "packaging"
  @"
[Files]
"AppxManifest.xml"             "AppxManifest.xml"
"main.exe"                     "$project.exe"
"main.winmd"                   "$project.winmd"
"assets/splashscreen.png"      "assets/splashscreen.png"
"assets/square150x150logo.png" "assets/square150x150logo.png"
"assets/square44x44logo.png"   "assets/square44x44logo.png"
"assets/storelogo.png"         "assets/storelogo.png"
"@ |
  Out-File -FilePath mapping.txt -Encoding utf8
  if (-not $?) {
    Throw "failed to create mapping.txt"
  }
  @"
<?xml version="1.0" encoding="utf-8"?>
<Package
  xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
  xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
  IgnorableNamespaces="uap">
  <Identity
    Name="$project" Publisher="CN=debug, O=debug, C=US"
    Version="1.0.0.0" />
  <Properties>
    <DisplayName>$project</DisplayName>
    <PublisherDisplayName>WANGBLOWS</PublisherDisplayName>
    <Logo>assets\storelogo.png</Logo>
  </Properties>
  <Dependencies>
    <TargetDeviceFamily Name="Windows.Universal" MinVersion="10.0.0.0"
      MaxVersionTested="10.0.0.0" />
  </Dependencies>
  <Resources>
    <Resource Language="EN-US" />
  </Resources>
  <Applications>
    <Application Id="App" Executable="$project.exe"
      EntryPoint="$project.App">
      <uap:VisualElements
        DisplayName="$project"
        Square150x150Logo="assets\square150x150logo.png"
        Square44x44Logo="assets\square44x44logo.png"
        Description="$project"
        BackgroundColor="transparent">
        <uap:DefaultTile>
          <uap:ShowNameOnTiles>
            <uap:ShowOn Tile="square150x150Logo" />
          </uap:ShowNameOnTiles>
        </uap:DefaultTile>
        <uap:SplashScreen Image="assets\splashscreen.png" />
      </uap:VisualElements>
    </Application>
  </Applications>
  <Capabilities>
    <Capability Name="internetClient" />
  </Capabilities>
</Package>
"@ |
  Out-File -FilePath AppxManifest.xml -Encoding utf8
  MakeAppx pack /v /f mapping.txt  /p "$project.appx"
  if (-not $?) {
    Throw "MakeAppx failed: $LastExitCode"
  }
  SignTool sign /fd SHA256 /a /f cert.pfx /p debug "$project.appx"
  if (-not $?) {
    Throw "SignTool failed: $LastExitCode"
  }

  Write-Header "deploying"
  Get-AppxPackage "$project" -ErrorAction SilentlyContinue |
    Remove-AppxPackage -ErrorAction SilentlyContinue
  Add-AppxPackage "$project.appx" -ErrorAction Stop
  explorer shell:AppsFolder\$(Get-AppxPackage "$project" |
    Select-Object -ExpandProperty PackageFamilyName)!App
} catch {
  $res = $_
}
Pop-Location
if ($null -ne $res) {
  throw $res
}
