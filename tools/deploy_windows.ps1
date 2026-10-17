param([string]$QtRoot=$env:QT_ROOT,[string]$PythonExe='',[string]$StagingDirectory='')
$ErrorActionPreference='Stop'
$hubRoot=Split-Path -Parent $PSScriptRoot
$configPath=Join-Path $hubRoot '.local\build-environment.json'
if(Test-Path -LiteralPath $configPath){$config=Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json;if(-not $QtRoot){$QtRoot=$config.qtRoot};if(-not $PythonExe){$PythonExe=$config.python}}
if(-not $QtRoot -or -not $PythonExe){throw 'Pass QtRoot and a development Python with pefile.'}
if(-not $StagingDirectory){$StagingDirectory=Join-Path $hubRoot 'dist\ArchaeoHub'}
$qtToolBin=if(Test-Path -LiteralPath (Join-Path $QtRoot 'lib\qt6\bin\windeployqt.exe')){Join-Path $QtRoot 'lib\qt6\bin'}else{Join-Path $QtRoot 'bin'}
$qmlSource=if(Test-Path -LiteralPath (Join-Path $QtRoot 'lib\qt6\qml')){Join-Path $QtRoot 'lib\qt6\qml'}else{Join-Path $QtRoot 'qml'}
$pluginSource=if(Test-Path -LiteralPath (Join-Path $QtRoot 'lib\qt6\plugins')){Join-Path $QtRoot 'lib\qt6\plugins'}else{Join-Path $QtRoot 'plugins'}
$qtTools=Join-Path $hubRoot '.local\qt-tools'
New-Item -ItemType Directory -Path $qtTools,$StagingDirectory -Force | Out-Null
foreach($tool in @('windeployqt.exe','qtpaths.exe')){Copy-Item -LiteralPath (Join-Path $qtToolBin $tool) -Destination $qtTools -Force}
$relativeBinaries=[IO.Path]::GetRelativePath($QtRoot,$qtToolBin).Replace('\','/')
$relativeQml=[IO.Path]::GetRelativePath($QtRoot,$qmlSource).Replace('\','/')
$relativePlugins=[IO.Path]::GetRelativePath($QtRoot,$pluginSource).Replace('\','/')
@"
[Paths]
Prefix=$($QtRoot.Replace('\','/'))
HostPrefix=$($QtRoot.Replace('\','/'))
HostBinaries=$relativeBinaries
Binaries=$relativeBinaries
QmlImports=$relativeQml
Plugins=$relativePlugins
Libraries=lib
"@ | Set-Content -LiteralPath (Join-Path $qtTools 'qt.conf') -Encoding ascii
$env:PATH=$qtToolBin+';'+(Join-Path $QtRoot 'bin')+';'+$env:PATH
Copy-Item -LiteralPath (Join-Path $hubRoot 'build\ArchaeoHub.exe') -Destination $StagingDirectory -Force
Copy-Item -LiteralPath (Join-Path $hubRoot 'build\CoordinateWorker.exe') -Destination $StagingDirectory -Force
foreach($module in @('catalog','coordinates','transcription','documents','inventory')){
    $engine=Join-Path $hubRoot ('build\modules\'+$module+'\engine')
    if(-not(Test-Path -LiteralPath $engine -PathType Container)){throw "Build module engine first: $module"}
    $moduleTarget=Join-Path $StagingDirectory ('modules\'+$module)
    New-Item -ItemType Directory -Path $moduleTarget -Force | Out-Null
    Copy-Item -LiteralPath $engine -Destination $moduleTarget -Recurse -Force
}
$spreadsheetEngine=Join-Path $hubRoot 'build\modules\coordinates\spreadsheets'
if(-not(Test-Path -LiteralPath $spreadsheetEngine)){throw 'Build the spreadsheet engine first.'}
Copy-Item -LiteralPath $spreadsheetEngine -Destination (Join-Path $StagingDirectory 'modules\coordinates') -Recurse -Force
& (Join-Path $qtTools 'windeployqt.exe') --qtpaths (Join-Path $qtTools 'qtpaths.exe') --release --no-translations --compiler-runtime --no-quick-import --dir $StagingDirectory (Join-Path $StagingDirectory 'ArchaeoHub.exe')
if($LASTEXITCODE -ne 0){throw 'Qt deployment failed.'}
$qmlTarget=Join-Path $StagingDirectory 'qml'
function Copy-QmlModule([string]$RelativePath,[switch]$OnlyRootFiles){
    $source=Join-Path $qmlSource $RelativePath;$target=Join-Path $qmlTarget $RelativePath
    if(-not(Test-Path -LiteralPath $source)){return}
    New-Item -ItemType Directory -Path $target -Force | Out-Null
    if($OnlyRootFiles){Get-ChildItem -LiteralPath $source -File | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $target -Force}}
    else{Get-ChildItem -LiteralPath $source | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $target -Recurse -Force}}
}
Copy-QmlModule 'QtQuick' -OnlyRootFiles
Copy-QmlModule 'QtQuick\Controls' -OnlyRootFiles
foreach($module in @('QtQml','QtCore','QtQuick\Controls\Basic','QtQuick\Controls\impl','QtQuick\Templates','QtQuick\Layouts','QtQuick\Window','QtQuick\Dialogs','QtQuick\Effects','QtQuick\NativeStyle','Qt\labs\folderlistmodel','Qt\labs\settings')){Copy-QmlModule $module}
foreach($plugin in @('platforms\qwindows.dll','imageformats\qico.dll','imageformats\qjpeg.dll')){
    $target=Join-Path $StagingDirectory $plugin
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $pluginSource $plugin) -Destination $target -Force
}
$sqlTarget=Join-Path $StagingDirectory 'sqldrivers';New-Item -ItemType Directory -Path $sqlTarget -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $pluginSource 'sqldrivers\qsqlite.dll') -Destination $sqlTarget -Force
@'
[Paths]
Prefix=.
Plugins=.
QmlImports=qml
'@ | Set-Content -LiteralPath (Join-Path $StagingDirectory 'qt.conf') -Encoding ascii
& $PythonExe (Join-Path $PSScriptRoot 'copy_pe_dependencies.py') $StagingDirectory (Join-Path $QtRoot 'bin') $qtToolBin $config.projBinPath --exclude-tree modules
if($LASTEXITCODE -ne 0){throw 'Dependency resolution failed.'}
$docsTarget=Join-Path $StagingDirectory 'docs';New-Item -ItemType Directory -Path $docsTarget -Force | Out-Null
foreach($name in @('README.md','ARCHITECTURE.md','MODULES.md','BUILDING.md','DESIGN.md')){
    Copy-Item -LiteralPath (Join-Path $hubRoot $name) -Destination $StagingDirectory -Force
    $oldCopy=Join-Path $docsTarget $name
    if(Test-Path -LiteralPath $oldCopy -PathType Leaf){Remove-Item -LiteralPath $oldCopy}
}
foreach($name in @('MIGRATION_PLAN.md','AUDIT.md','DATABASE.md','COORDINATE_MODULE_PLAN.md','CRS_REVIEW.md','SOURCES.md','VALIDATION.md','CATALOG_PARITY_REVIEW.md','INVENTORY.md','DOCUMENT_LIBRARY.md')){
    $source=Join-Path (Join-Path $hubRoot 'docs') $name
    if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $docsTarget -Force}
}
$engineeringTarget=Join-Path $docsTarget 'engineering'
New-Item -ItemType Directory -Path (Join-Path $engineeringTarget 'references') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $hubRoot 'docs\engineering\SKILL.md') -Destination $engineeringTarget -Force
Copy-Item -LiteralPath (Join-Path $hubRoot 'docs\engineering\references\VERIFICATION.md') -Destination (Join-Path $engineeringTarget 'references') -Force
Copy-Item -LiteralPath (Join-Path $hubRoot 'THIRD_PARTY.md') -Destination $StagingDirectory -Force
$licenseSource=Join-Path $hubRoot 'resources\licenses';if(Test-Path -LiteralPath $licenseSource){Copy-Item -LiteralPath $licenseSource -Destination (Join-Path $StagingDirectory 'licenses') -Recurse -Force}
Write-Output "Staged Hub: $StagingDirectory"
