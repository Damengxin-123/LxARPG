param([string]$Stage = 'inspect')
# 通过虚幻官方 MCP 执行主菜单与木桩制作工具，中文参数以 UTF-8 传输。
$ErrorActionPreference = 'Stop'
$menuDummyHeaders = @{ Accept = 'application/json, text/event-stream' }
$menuDummyUri = 'http://127.0.0.1:8054/mcp'
$menuDummyInit = @{ jsonrpc = '2.0'; id = 1; method = 'initialize'; params = @{ protocolVersion = '2024-11-05'; capabilities = @{}; clientInfo = @{ name = 'codex-main-menu-dummy'; version = '1' } } } | ConvertTo-Json -Depth 12
$menuDummyResponse = Invoke-WebRequest -Uri $menuDummyUri -Method Post -ContentType 'application/json' -Headers $menuDummyHeaders -Body $menuDummyInit -TimeoutSec 20
$menuDummyHeaders['Mcp-Session-Id'] = $menuDummyResponse.Headers['Mcp-Session-Id'][0]
$menuDummyBody = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'call_tool'; arguments = @{ toolset_name = 'Game.Source.LxARPGEditor.Scripts.主菜单木桩工具.MainMenuDummyTools'; tool_name = 'process_main_menu_dummy'; arguments = @{ stage = $Stage } } } } | ConvertTo-Json -Depth 15
$menuDummyResult = Invoke-RestMethod -Uri $menuDummyUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $menuDummyHeaders -Body ([Text.Encoding]::UTF8.GetBytes($menuDummyBody)) -TimeoutSec 600
$menuDummyReportRoot = Join-Path $PSScriptRoot '../../../Saved/主菜单木桩'
[IO.Directory]::CreateDirectory($menuDummyReportRoot) | Out-Null
$menuDummyResult | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath (Join-Path $menuDummyReportRoot ('MCP_' + $Stage + '.json')) -Encoding utf8
foreach ($menuDummyContent in $menuDummyResult.result.content) {
    if ($menuDummyContent.type -eq 'text') {
        $menuDummyPayload = $menuDummyContent.text | ConvertFrom-Json
        if ($menuDummyPayload.returnValue) {
            Write-Output $menuDummyPayload.returnValue
            if ($menuDummyPayload.returnValue.StartsWith('ERROR:')) { throw $menuDummyPayload.returnValue }
        }
        elseif ($menuDummyPayload.PSObject.Properties.Name -contains 'returnValue') {
            throw '官方 MCP 工具返回空结果，请检查主菜单木桩验收日志中的脚本异常。'
        }
        else { Write-Output $menuDummyContent.text }
    }
}
if ($menuDummyResult.error -or $menuDummyResult.result.isError) { throw ($menuDummyResult | ConvertTo-Json -Depth 24) }
