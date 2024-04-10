param (
    [Parameter(Mandatory=$true)][string]$version
 )

$curDir = Get-Location

Write-Output "Setting up development environment with Visuals Studio 2022"
Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\2022\Community\"

$Env:RELEASE_VERSION = "$version"
$Env:PATH_TO_JUCE = "$curDir\JUCE"

Write-Output "Changing location to $curDir"
Set-Location $curDir

$signId = "Open Source Developer, Geert Bevin"
$buildLocation = "Builds\VisualStudio2017\x64"

Write-Output "Deleting previous build from $buildLocation"
Remove-Item -LiteralPath $buildLocation -Force -Recurse

Write-Output "Building project"
MSBuild.exe .\Builds\VisualStudio2017\sendmidi.sln /p:Configuration=Release /p:PreferredToolArchitecture=x64 /p:Platform=x64 /clp:ErrorsOnly

Write-Output "Codesigning all artifacts"

& signtool sign /n "$signId" /t http://time.certum.pl/ /fd sha1 /v "$buildLocation\Release\ConsoleApp\sendmidi.exe"
