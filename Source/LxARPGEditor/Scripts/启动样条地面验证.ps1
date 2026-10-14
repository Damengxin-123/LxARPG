param([switch]$Render)
# 隐藏启动专用验证会话；通过官方MCP控制，结果只保存到专用资产和Saved。
$groundRoot=[System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$groundArgs=@(('"'+$groundRoot+'/LxARPG.uproject"'),'/Engine/Maps/Entry','-unattended','-nop4','-nosplash','-nosound','-NoLiveCoding',
 '-EnablePlugins=ModelContextProtocol,ToolsetRegistry',('-ExecutePythonScript="'+$PSScriptRoot+'/样条地面资产工具.py"'),
 ('-abslog="'+$groundRoot+'/Saved/Logs/样条地面验证.log"'),('-ReportExportPath="'+$groundRoot+'/Saved/Automation/SplineGround"'))
if (-not $Render) { $groundArgs+='-NullRHI' }
(Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $groundArgs -WindowStyle Hidden -PassThru).Id
