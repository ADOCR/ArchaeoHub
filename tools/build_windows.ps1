param([string]$QtRoot=$env:QT_ROOT,[string]$VisualStudioRoot='', [switch]$SkipTests)
$ErrorActionPreference='Stop'
$workbenchRoot=Split-Path -Parent $PSScriptRoot
$localConfiguration=Join-Path $workbenchRoot '.local\build-environment.json'
if(Test-Path -LiteralPath $localConfiguration){
    $configuration=Get-Content -LiteralPath $localConfiguration -Raw | ConvertFrom-Json
    if(-not $QtRoot){$QtRoot=$configuration.qtRoot}
    if(-not $VisualStudioRoot){$VisualStudioRoot=$configuration.visualStudioRoot}
}
if(-not $QtRoot){throw 'Set QT_ROOT or pass -QtRoot pointing to the Qt SDK prefix.'}
if(-not $VisualStudioRoot){
    $locator=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if(Test-Path -LiteralPath $locator){$VisualStudioRoot=& $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath}
}
foreach($path in @($QtRoot,$VisualStudioRoot,$workbenchRoot)){if(-not $path -or $path.Contains('"') -or $path.Contains("`n")){throw 'Invalid SDK or project path.'}}
$developer=Join-Path $VisualStudioRoot 'Common7\Tools\VsDevCmd.bat'
$cmake=Join-Path $VisualStudioRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ninja=Join-Path $VisualStudioRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
$ctest=Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
foreach($path in @($developer,$cmake)){if(-not(Test-Path -LiteralPath $path)){throw "Build tool missing: $path"}}
$local=Join-Path $workbenchRoot '.local';New-Item -ItemType Directory -Path $local -Force | Out-Null
$projArgument=''
if($configuration.projIncludePath){$projArgument=' -DWORKBENCH_PROJ_INCLUDE_DIR="'+$configuration.projIncludePath.Replace('\','/')+'"'}
if($configuration.transcriptionSdkPath){$projArgument+=' -DWORKBENCH_TRANSCRIPTION_SDK="'+$configuration.transcriptionSdkPath.Replace('\','/')+'"'}
$commands=@(
    '@echo off','setlocal','set "VSLANG=1033"',('call "'+$developer+'" -arch=x64 -host_arch=x64'),
    'if errorlevel 1 exit /b 1',
    ('set "PATH='+$QtRoot+'\lib\qt6\bin;'+$QtRoot+'\bin;'+$ninja+';%PATH%"'),
    ('"'+$cmake+'" -S "'+$workbenchRoot+'" -B "'+(Join-Path $workbenchRoot 'build')+'" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="'+$QtRoot.Replace('\','/')+'"'+$projArgument),
    'if errorlevel 1 exit /b 1',
    ('"'+$cmake+'" --build "'+(Join-Path $workbenchRoot 'build')+'" --parallel 4'),
    'if errorlevel 1 exit /b 1'
)
if(-not $SkipTests){$commands+=@(
    ('set "QT_PLUGIN_PATH='+$QtRoot+'\lib\qt6\plugins"'), 'set "QT_QPA_PLATFORM=offscreen"',
    ('"'+$ctest+'" --test-dir "'+(Join-Path $workbenchRoot 'build')+'" --output-on-failure'),
    'if errorlevel 1 exit /b 1')}
$batch=Join-Path $local 'build.cmd';$commands | Set-Content -LiteralPath $batch -Encoding ascii
& $env:ComSpec /d /c $batch
if($LASTEXITCODE -ne 0){throw 'Build or contract tests failed.'}
