param([ValidateSet('benchmark','reload')][string]$Mode='benchmark')
# 隐藏启动独立验证会话，通过官方 MCP 控制；原关卡只读，输出位于项目 Saved。
$meshRoot=[System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$meshSuffix=if ($Mode -eq 'reload') { '重新加载' } else { '首次验证' }
$meshReport=if ($Mode -eq 'reload') { 'CompactMeshReload' } else { 'CompactMeshFirst' }
$meshArgs=@(
    ('"' + $meshRoot + '/LxARPG.uproject"'), '/Engine/Maps/Entry',
    '-unattended','-nop4','-nosplash','-nosound','-NullRHI','-NoLiveCoding',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',
    ('-ExecutePythonScript="' + $PSScriptRoot + '/验证紧凑网格保存.py"'),
    ('-abslog="' + $meshRoot + '/Saved/Logs/紧凑网格' + $meshSuffix + '.log"'),
    ('-ReportExportPath="' + $meshRoot + '/Saved/Automation/' + $meshReport + '"'),
    ('-LxMeshSaveMode=' + $Mode)
)
(Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $meshArgs -WindowStyle Hidden -PassThru).Id
