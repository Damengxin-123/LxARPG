param([ValidateSet('refresh','build','verify','demo','capture','tests','quit')][string]$Stage='verify')
# 使用 UE 5.8 官方 MCP 串行调用魔灯工具，中文参数使用 UTF-8。
$ErrorActionPreference='Stop'
$lampHeaders=@{Accept='application/json, text/event-stream'}
$lampUri='http://127.0.0.1:8008/mcp'
$lampInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-magic-lamp';version='1'}}} | ConvertTo-Json -Depth 12
$lampResponse=Invoke-WebRequest -Uri $lampUri -Method Post -ContentType 'application/json' -Headers $lampHeaders -Body $lampInit -TimeoutSec 20
$lampHeaders['Mcp-Session-Id']=$lampResponse.Headers['Mcp-Session-Id'][0]
$lampBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.魔灯资产工具.MagicLampTools';tool_name='process_lamp';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $lampUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $lampHeaders -Body ([Text.Encoding]::UTF8.GetBytes($lampBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 20
