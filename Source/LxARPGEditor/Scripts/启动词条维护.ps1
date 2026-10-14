param(
    [switch]$NoRender,
    [string]$EditorPath = 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
)
# 后台启动独立数据维护会话，通过官方 MCP 读取与修改词条；渲染只用于离屏验收。
$ErrorActionPreference = 'Stop'
$entryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$entryProject = Join-Path $entryRoot 'LxARPG.uproject'
$entryScript = Join-Path $PSScriptRoot '词条数据工具.py'
foreach ($entryRequired in @($EditorPath, $entryProject, $entryScript)) {
    if (-not (Test-Path -LiteralPath $entryRequired -PathType Leaf)) { throw "找不到词条维护所需文件：$entryRequired" }
}
$entryLogDirectory = Join-Path $entryRoot 'Saved/Logs'
$entryReportDirectory = Join-Path $entryRoot 'Saved/词条数据维护/自动化报告'
[IO.Directory]::CreateDirectory($entryLogDirectory) | Out-Null
[IO.Directory]::CreateDirectory($entryReportDirectory) | Out-Null
$entryArguments = @(
    ('"' + $entryProject + '"'), '/Engine/Maps/Entry',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',
    ('-ExecutePythonScript="' + $entryScript + '"'),
    ('-abslog="' + (Join-Path $entryLogDirectory '词条数据维护.log') + '"'),
    ('-ReportExportPath="' + $entryReportDirectory + '"'), '-stdout', '-FullStdOutLogOutput'
)
if ($NoRender) { $entryArguments += '-NullRHI' }
else { $entryArguments += '-AllowCommandletRendering' }
$entryStart = @{
    FilePath = $EditorPath; ArgumentList = $entryArguments; WindowStyle = 'Hidden'; PassThru = $true
    RedirectStandardOutput = (Join-Path $entryLogDirectory '词条维护启动.txt')
    RedirectStandardError = (Join-Path $entryLogDirectory '词条维护启动错误.txt')
}
(Start-Process @entryStart).Id
