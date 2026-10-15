param([switch]$NoRender, [switch]$Runtime)
# 在独立隐藏会话启动官方 MCP，使用专用存档验证角色保存与主菜单。
$ErrorActionPreference = 'Stop'
$characterSaveRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$characterSavePrefix = 'LxMenuSmoke_Character_' + [Guid]::NewGuid().ToString('N')
$characterSaveLogName = if ($Runtime) { '角色存档真实流程' } else { '角色存档验收' }
$characterSaveArgs = @(
    ('"' + $characterSaveRoot + '/LxARPG.uproject"'), '/Engine/Maps/Entry',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoLiveCoding',
    ('-LxMenuSavePrefix=' + $characterSavePrefix), '-LxMenuIgnoreLegacy',
    ('-abslog="' + $characterSaveRoot + '/Saved/Logs/' + $characterSaveLogName + '.log"'), '-stdout', '-FullStdOutLogOutput'
)
if ($Runtime) {
    # 沿用配置中的旧关卡软路径启动，覆盖实际资产重定向后的保存及菜单预览。
    $characterSaveArgs[1] = '/Game/项目内容/关卡/角色测试'
    $characterSaveArgs += @('-game', '-Windowed', '-ResX=1600', '-ResY=900', '-ExecCmds="Automation RunTests LxARPG.Menu.RuntimeFlow"', '-TestExit="Automation Test Queue Empty"')
}
else {
    $characterSaveArgs += @('-EnablePlugins=ModelContextProtocol,ToolsetRegistry', ('-ExecutePythonScript="' + $PSScriptRoot + '/角色存档验收.py"'))
}
if ($NoRender) { $characterSaveArgs += '-NullRHI' }
else { $characterSaveArgs += @('-AllowCommandletRendering', '-RenderOffscreen') }
Write-Output ('RegressionPrefix=' + $characterSavePrefix)
(Start-Process -FilePath 'I:/TOOL/UE5/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $characterSaveArgs -WorkingDirectory $characterSaveRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput ($characterSaveRoot + '/Saved/Logs/' + $characterSaveLogName + '输出.txt') -RedirectStandardError ($characterSaveRoot + '/Saved/Logs/' + $characterSaveLogName + '错误.txt')).Id
