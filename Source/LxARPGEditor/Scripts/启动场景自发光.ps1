# 隐藏启动独立的 UE 官方 MCP 离屏维护会话，使用隔离存档前缀。
$ErrorActionPreference = 'Stop'
$exposureRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$exposureArgs = @(
    ('"' + $exposureRoot + '/LxARPG.uproject"'), '/Engine/Maps/Entry',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-RenderOffscreen', '-AllowCommandletRendering',
    '-LxMenuSavePrefix=LxMenuSmoke_ExposureFix_20261015', '-LxMenuIgnoreLegacy',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',
    ('-ExecutePythonScript="' + $PSScriptRoot + '/修复场景自发光.py"'),
    ('-abslog="' + $exposureRoot + '/Saved/黑色显示修复/最终验收会话.log"')
)
(Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $exposureArgs -WorkingDirectory $exposureRoot -WindowStyle Hidden -PassThru).Id