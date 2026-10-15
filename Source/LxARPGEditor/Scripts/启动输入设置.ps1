# 隐藏启动独立的虚幻官方 MCP 制作会话，验收使用随机设置与进度存档前缀。
$ErrorActionPreference = 'Stop'
$inputRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$inputPrefix = 'LxInputSmoke_' + [Guid]::NewGuid().ToString('N')
$inputArgs = @(
    ('"' + $inputRoot + '/LxARPG.uproject"'), '/Engine/Maps/Entry',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry', '-ModelContextProtocolPort=8058',
    '-AllowCommandletRendering', '-RenderOffscreen', '-LxMenuUIRenderPreviews',
    ('-LxMenuSavePrefix=' + $inputPrefix), '-LxMenuIgnoreLegacy', ('-LxPlayerSettingsSlot=' + $inputPrefix + '_Settings'),
    ('-ExecutePythonScript="' + $PSScriptRoot + '/输入设置工具.py"'),
    ('-abslog="' + $inputRoot + '/Saved/Logs/输入设置验收.log"'), '-stdout', '-FullStdOutLogOutput'
)
$inputProcess = Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $inputArgs -WorkingDirectory $inputRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput ($inputRoot + '/Saved/Logs/输入设置启动.txt') -RedirectStandardError ($inputRoot + '/Saved/Logs/输入设置错误.txt')
Write-Output ('ProcessId=' + $inputProcess.Id)
Write-Output ('IsolatedSavePrefix=' + $inputPrefix)
