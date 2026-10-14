param([ValidateSet('inspect','plan','apply','verify','tests','quit')][string]$Stage = 'inspect')
# 使用虚幻官方 MCP 维护词条数据，中文请求以 UTF-8 发送。
$ErrorActionPreference = 'Stop'
$entryHeaders = @{Accept = 'application/json, text/event-stream'}
$entryUri = 'http://127.0.0.1:8051/mcp'
$entryInit = @{jsonrpc='2.0'; id=1; method='initialize'; params=@{protocolVersion='2024-11-05'; capabilities=@{}; clientInfo=@{name='codex-entry-data';version='1'}}} | ConvertTo-Json -Depth 12
$entryResponse = Invoke-WebRequest -Uri $entryUri -Method Post -ContentType 'application/json' -Headers $entryHeaders -Body $entryInit -TimeoutSec 20
$entryHeaders['Mcp-Session-Id'] = $entryResponse.Headers['Mcp-Session-Id'][0]
$entryBody = @{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.词条数据工具.EntryDataTools';tool_name='process_entries';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
$entryReply = Invoke-RestMethod -Uri $entryUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $entryHeaders -Body ([Text.Encoding]::UTF8.GetBytes($entryBody)) -TimeoutSec 60
if ($entryReply.error -or $entryReply.result.isError) { throw ($entryReply | ConvertTo-Json -Depth 20) }
foreach ($entryContent in $entryReply.result.content) {
    if ($entryContent.type -ne 'text') { continue }
    $entryPayload = $entryContent.text | ConvertFrom-Json
    if ($entryPayload.returnValue) {
        try { $entryPayload.returnValue | ConvertFrom-Json | ConvertTo-Json -Depth 20 }
        catch { $entryPayload.returnValue }
    }
    else { $entryContent.text }
}
