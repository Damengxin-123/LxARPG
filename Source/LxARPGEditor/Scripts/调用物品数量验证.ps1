param([ValidateSet('inspect','tests','equipment_tests','quit')][string]$Stage = 'inspect')
# 通过官方 MCP 调用只读数量检查与隔离回归测试，中文请求使用 UTF-8。
$ErrorActionPreference = 'Stop'
$countHeaders = @{Accept='application/json, text/event-stream'}
$countUri = 'http://127.0.0.1:8052/mcp'
$countInit = @{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-item-count';version='1'}}} | ConvertTo-Json -Depth 12
$countResponse = Invoke-WebRequest -Uri $countUri -Method Post -ContentType 'application/json' -Headers $countHeaders -Body $countInit -TimeoutSec 20
$countHeaders['Mcp-Session-Id'] = $countResponse.Headers['Mcp-Session-Id'][0]
$countBody = @{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.物品数量验证.ItemCountTools';tool_name='verify_counts';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
$countReply = Invoke-RestMethod -Uri $countUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $countHeaders -Body ([Text.Encoding]::UTF8.GetBytes($countBody)) -TimeoutSec 60
if ($countReply.error -or $countReply.result.isError) { throw ($countReply | ConvertTo-Json -Depth 20) }
foreach ($countContent in $countReply.result.content) {
    if ($countContent.type -ne 'text') { continue }
    $countPayload = $countContent.text | ConvertFrom-Json
    try { $countPayload.returnValue | ConvertFrom-Json | ConvertTo-Json -Depth 15 }
    catch { $countPayload.returnValue }
}
