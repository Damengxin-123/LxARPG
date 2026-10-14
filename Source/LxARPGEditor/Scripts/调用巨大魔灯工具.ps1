param([ValidateSet('refresh','build','verify','preview','tests','quit')][string]$Stage='verify')
# 使用 UE 5.8 官方 MCP 创建及验收巨大灯具，中文参数使用 UTF-8。
$ErrorActionPreference='Stop'
$giantHeaders=@{Accept='application/json, text/event-stream'}
$giantUri='http://127.0.0.1:8027/mcp'
$giantInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-giant-lamp';version='1'}}} | ConvertTo-Json -Depth 12
$giantResponse=Invoke-WebRequest -Uri $giantUri -Method Post -ContentType 'application/json' -Headers $giantHeaders -Body $giantInit -TimeoutSec 20
$giantHeaders['Mcp-Session-Id']=$giantResponse.Headers['Mcp-Session-Id'][0]
$giantBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.巨大魔灯资产工具.GiantMagicLampTools';tool_name='process_giant_lamp';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $giantUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $giantHeaders -Body ([Text.Encoding]::UTF8.GetBytes($giantBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 20
