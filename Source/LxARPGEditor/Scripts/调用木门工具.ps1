param([ValidateSet('build','style','verify','demo','reload','capture','detail','tests','quit')][string]$Stage='verify')
# 通过虚幻5.8官方MCP串行调用木门工具，中文参数使用UTF-8。
$ErrorActionPreference='Stop'
$doorHeaders=@{Accept='application/json, text/event-stream'}
$doorUri='http://127.0.0.1:8007/mcp'
$doorInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-wooden-door';version='1'}}} | ConvertTo-Json -Depth 12
$doorResponse=Invoke-WebRequest -Uri $doorUri -Method Post -ContentType 'application/json' -Headers $doorHeaders -Body $doorInit -TimeoutSec 20
$doorHeaders['Mcp-Session-Id']=$doorResponse.Headers['Mcp-Session-Id'][0]
$doorBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.木门资产工具.WoodenDoorTools';tool_name='process_door';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $doorUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $doorHeaders -Body ([Text.Encoding]::UTF8.GetBytes($doorBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 20
