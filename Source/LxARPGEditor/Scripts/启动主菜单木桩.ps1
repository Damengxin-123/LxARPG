param([switch]$NoRender, [switch]$Regression, [string]$Level = '/Engine/Maps/Entry')
# 隐藏启动独立的官方 MCP 制作会话，保留项目正在使用的主菜单与关卡配置。
$ErrorActionPreference = 'Stop'
$menuDummyRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$menuDummyArgs = @(
    ('"' + $menuDummyRoot + '/LxARPG.uproject"'), $Level,
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',
    ('-ExecutePythonScript="' + $PSScriptRoot + '/主菜单木桩工具.py"'),
    ('-abslog="' + $menuDummyRoot + '/Saved/Logs/主菜单木桩验收.log"'), '-stdout', '-FullStdOutLogOutput'
)
if ($NoRender) { $menuDummyArgs += '-NullRHI' }
else { $menuDummyArgs += @('-AllowCommandletRendering', '-RenderOffscreen') }
if ($Regression) {
    # 真实 PIE 测试只写本次专用目录，禁止读取或覆盖玩家的游戏进度。
    $menuDummyTestPrefix = 'LxMenuSmoke_Dummy_' + [Guid]::NewGuid().ToString('N')
    $menuDummyArgs += @(('-LxMenuSavePrefix=' + $menuDummyTestPrefix), '-LxMenuIgnoreLegacy')
    Write-Output ('RegressionPrefix=' + $menuDummyTestPrefix)
}
(Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $menuDummyArgs -WorkingDirectory $menuDummyRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput ($menuDummyRoot + '/Saved/Logs/主菜单木桩启动.txt') -RedirectStandardError ($menuDummyRoot + '/Saved/Logs/主菜单木桩错误.txt')).Id
