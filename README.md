sane template to develop, build and deploy UWP C++ DirectX11 apps from the
command line without ever having to open visual studio

note that I don't recommend making your software UWP, especially games.
you're just throwing away backwards compatibility for a bloated
overengineered platform. I just made this so I can create sample apps to
study and reverse engineer the UWP platform

sorry if the directx code is crappy, I've never used it before

# Requirements
Visual Studio Community 2017 with the UWP components for C++ installed and
the DirectX11 SDK. not sure if you can get away with installing build tools
and individual SDK's.

# Usage
first of all set up your environment with

```
Set-ExecutionPolicy Bypass -Scope Process -Force
.\vcvarsall.ps1
```

now you can build with

```
.\build.ps1
```

it should produce a signed .appx package, install it and run it

on the first run, windows will complain about the package's untrusted
signature

right click the appx -> properties -> digital signatures, select the
one signature, click details, view certificate, install certificate

![](https://i.imgur.com/Y27r65n.png)

# Adding files and assets
open build.ps1, find the part where it creates the mapping.txt filelist:

```ps1
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
```

you can edit this to add extra assets or other files.

above this code you will also find the compiler call which you can edit
should you want to split your project into multiple compilation units

# Distributing
this mostly meant to build and deploy for local debugging or to share it
with friends who trust you, but if you wish so you can easily modify the
build script for proper distribution

you can set up a signing certificate by running these commands as admin

```
New-SelfSignedCertificate -Type Custom -Subject "CN=debug, O=debug, C=US" `
  -KeyUsage DigitalSignature -FriendlyName uwpdebug `
  -CertStoreLocation "Cert:\LocalMachine\My"
$pwd = ConvertTo-SecureString -String <Your Password> -Force -AsPlainText
Export-PfxCertificate -cert "Cert:\LocalMachine\My\<Thumbprint>" `
  -FilePath \path\to\project\cert.pfx
```

where thumbrint is from the output of New-SelfSignedCertificate

you'll also have to modify build.ps1 to
* remove the hardcoded debug password for the certificate
* change the Publisher and PublisherDisplayName fields in the manifest
* introduce an environment variable for versioning or hardcode it in
  build.ps1
