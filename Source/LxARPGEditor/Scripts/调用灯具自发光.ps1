param([ValidateSet('inspect','capture_before','repair','verify','capture_after','quit')][string]$Stage='inspect')
# 通过官方 MCP 调用固定的灯具维护操作，保留中文参数。
$ErrorActionPreference='Stop'
$lampHeaders=@{Accept='application/json, text/event-stream'}
$lampUri='http://127.0.0.1:8008/mcp'
$lampInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-lamp-emission';version='1'}}} | ConvertTo-Json -Depth 12
$lampResponse=Invoke-WebRequest -Uri $lampUri -Method Post -ContentType 'application/json' -Headers $lampHeaders -Body $lampInit -TimeoutSec 20
$lampHeaders['Mcp-Session-Id']=$lampResponse.Headers['Mcp-Session-Id'][0]
$lampBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.修复灯具自发光.LampEmissionTools';tool_name='process_lamp_emission';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $lampUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $lampHeaders -Body ([Text.Encoding]::UTF8.GetBytes($lampBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 25
