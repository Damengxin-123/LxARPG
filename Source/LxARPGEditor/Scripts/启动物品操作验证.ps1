param(
    [switch]$Render,
    [string]$EditorPath = 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
)

# 隐藏启动独立 Entry 验证会话，通过官方 MCP 控制，不打开或保存用户关卡。
$ErrorActionPreference = 'Stop'
$itemOperationRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$itemOperationProject = Join-Path $itemOperationRoot 'LxARPG.uproject'
$itemOperationScript = Join-Path $PSScriptRoot '物品操作工具.py'
if (-not (Test-Path -LiteralPath $EditorPath -PathType Leaf)) { throw "未找到虚幻编辑器：$EditorPath" }
if (-not (Test-Path -LiteralPath $itemOperationProject -PathType Leaf)) { throw "未找到项目：$itemOperationProject" }
if (-not (Test-Path -LiteralPath $itemOperationScript -PathType Leaf)) { throw "未找到物品操作官方 MCP 脚本：$itemOperationScript" }

# 避免重复创建会话后端口冲突；不关闭用户已经运行的任何编辑器。
$itemOperationServer = Get-NetTCPConnection -LocalPort 8038 -State Listen -ErrorAction SilentlyContinue
if ($itemOperationServer) { throw '端口 8038 已有监听会话，请复用该会话或关闭专用验证会话后重试。' }
$itemOperationLogDirectory = Join-Path $itemOperationRoot 'Saved/Logs'
$itemOperationReportDirectory = Join-Path $itemOperationRoot 'Saved/Automation/ItemOperations'
[System.IO.Directory]::CreateDirectory($itemOperationLogDirectory) | Out-Null
[System.IO.Directory]::CreateDirectory($itemOperationReportDirectory) | Out-Null
$itemOperationArguments = @(
    ('"' + $itemOperationProject + '"'), '/Engine/Maps/Entry',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',
    ('-ExecutePythonScript="' + $itemOperationScript + '"'),
    ('-abslog="' + (Join-Path $itemOperationLogDirectory '物品操作验证.log') + '"'),
    ('-ReportExportPath="' + $itemOperationReportDirectory + '"')
)
if ($Render) { $itemOperationArguments += '-AllowCommandletRendering' }
else { $itemOperationArguments += '-NullRHI' }
(Start-Process -FilePath $EditorPath -ArgumentList $itemOperationArguments -WindowStyle Hidden -PassThru).Id
