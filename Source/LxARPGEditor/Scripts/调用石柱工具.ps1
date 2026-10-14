param([ValidateSet('refresh','build','verify','demo','tests','capture','static','replace','reload','quit')][string]$Stage='verify')
# 通过虚幻5.8官方MCP串行调用石柱工具，中文参数使用UTF-8。
$ErrorActionPreference='Stop'
$wallHeaders=@{Accept='application/json, text/event-stream'}
$wallUri='http://127.0.0.1:8008/mcp'
$wallInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-stone-pillar';version='1'}}} | ConvertTo-Json -Depth 12
$wallResponse=Invoke-WebRequest -Uri $wallUri -Method Post -ContentType 'application/json' -Headers $wallHeaders -Body $wallInit -TimeoutSec 20
$wallHeaders['Mcp-Session-Id']=$wallResponse.Headers['Mcp-Session-Id'][0]
$wallBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.石柱资产工具.StonePillarTools';tool_name='process_pillar';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $wallUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $wallHeaders -Body ([Text.Encoding]::UTF8.GetBytes($wallBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 20
