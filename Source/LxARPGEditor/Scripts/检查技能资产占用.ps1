# 只读检查共享访问权限，不写入或截断资源；防止专用会话与当前编辑器争用技能资产。
$ErrorActionPreference = 'Stop'
# 请求删除访问权限但不执行删除，以检测会阻止 Unreal 替换包文件的共享锁。
if (-not ('SkillArtFileAccessProbe' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
// 技能资产共享访问探针；所有句柄仅用于检查，禁止写入或删除文件。
public static class SkillArtFileAccessProbe
{
    // 打开现有文件并请求删除访问权，用于判断其他进程是否允许替换文件。
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern SafeFileHandle CreateFileW(string fileName, uint desiredAccess, uint shareMode, IntPtr securityAttributes, uint creationDisposition, uint flags, IntPtr templateFile);
}
'@
}
$skillCheckRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$skillCheckFolders = @('Content/项目内容/美术资产/特效/技能表现', 'Content/项目内容/功能资产/技能资产')
$skillLockedFiles = [System.Collections.Generic.List[string]]::new()
foreach ($skillFolder in $skillCheckFolders) {
    $skillFolderPath = Join-Path $skillCheckRoot $skillFolder
    if (-not (Test-Path -LiteralPath $skillFolderPath)) { continue }
    foreach ($skillFile in Get-ChildItem -LiteralPath $skillFolderPath -Recurse -File -Filter '*.uasset') {
        $skillProbe = $null
        try {
            $skillProbe = [SkillArtFileAccessProbe]::CreateFileW($skillFile.FullName, 0x00010000, 0x00000007, [IntPtr]::Zero, 3, 0x00000080, [IntPtr]::Zero)
            if ($skillProbe.IsInvalid) {
                $skillAccessError = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
                if ($skillAccessError -eq 32 -or $skillAccessError -eq 33) { $skillLockedFiles.Add($skillFile.FullName) }
                else { throw [ComponentModel.Win32Exception]::new($skillAccessError, $skillFile.FullName) }
            }
        }
        catch [System.IO.IOException] { $skillLockedFiles.Add($skillFile.FullName) }
        finally { if ($null -ne $skillProbe) { $skillProbe.Dispose() } }
    }
}
if ($skillLockedFiles.Count -gt 0) {
    throw ('技能资产被其他会话占用，请先保存并关闭占用的编辑器。共 ' + $skillLockedFiles.Count + ' 个；首项：' + $skillLockedFiles[0])
}
