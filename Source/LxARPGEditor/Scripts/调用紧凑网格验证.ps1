param([ValidateSet('cache','ocean','benchmark','quit')][string]$Stage='cache')
# 使用虚幻 5.8 官方 MCP 接口，验证入口只会保存 Saved 下的关卡副本。
$ErrorActionPreference='Stop'
$meshUri='http://127.0.0.1:8034/mcp'
$meshHeaders=@{Accept='application/json, text/event-stream'}
$meshInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='compact-mesh-verification';version='1'}}} | ConvertTo-Json -Depth 12
$meshResponse=Invoke-WebRequest -Uri $meshUri -Method Post -ContentType 'application/json' -Headers $meshHeaders -Body $meshInit -TimeoutSec 60
$meshHeaders['Mcp-Session-Id']=$meshResponse.Headers['Mcp-Session-Id'][0]
$meshArgs=@{toolset_name='Game.Source.LxARPGEditor.Scripts.验证紧凑网格保存.CompactMeshVerificationTools';tool_name='verify_compact_mesh';arguments=@{stage=$Stage}}
$meshBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=$meshArgs}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $meshUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $meshHeaders -Body ([Text.Encoding]::UTF8.GetBytes($meshBody)) -TimeoutSec 60 | ConvertTo-Json -Depth 20
