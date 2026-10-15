param([switch]$NoRender)
# 隐藏启动独立的官方 MCP 会话，默认启用 GPU 离屏渲染；不控制计算机屏幕。
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot '检查技能资产占用.ps1')
$skillArtRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$skillArtArgs = @(
    ('"' + $skillArtRoot + '/LxARPG.uproject"'), '/Engine/Maps/Entry',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-ExecCmds="file.allowdeleteopenfiles 1"',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',
    ('-ExecutePythonScript="' + $PSScriptRoot + '/技能美术工具.py"'),
    ('-abslog="' + $skillArtRoot + '/Saved/Logs/技能美术验收.log"'), '-stdout', '-FullStdOutLogOutput'
)
if ($NoRender) { $skillArtArgs += '-NullRHI' }
else { $skillArtArgs += @('-AllowCommandletRendering', '-RenderOffscreen') }
(Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $skillArtArgs -WorkingDirectory $skillArtRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput ($skillArtRoot + '/Saved/Logs/技能美术验收启动.txt') -RedirectStandardError ($skillArtRoot + '/Saved/Logs/技能美术验收错误.txt')).Id
