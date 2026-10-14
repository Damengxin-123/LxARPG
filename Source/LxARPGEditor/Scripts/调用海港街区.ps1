param([string]$Stage='inspect')
# 使用官方 MCP 接口执行街区脚本，中文参数按 UTF-8 传输。
$ErrorActionPreference='Stop'
$districtHeaders=@{Accept='application/json, text/event-stream'}
$districtUri='http://127.0.0.1:8045/mcp'
$districtInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-harbor-district';version='1'}}} | ConvertTo-Json -Depth 12
$districtResponse=Invoke-WebRequest -Uri $districtUri -Method Post -ContentType 'application/json' -Headers $districtHeaders -Body $districtInit -TimeoutSec 20
$districtHeaders['Mcp-Session-Id']=$districtResponse.Headers['Mcp-Session-Id'][0]
$districtBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.海港街区工具.HarborDistrictTools';tool_name='process_district';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $districtUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $districtHeaders -Body ([Text.Encoding]::UTF8.GetBytes($districtBody)) -TimeoutSec 900 | ConvertTo-Json -Depth 30
