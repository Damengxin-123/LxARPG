param([string]$Stage='inspect')
# 通过虚幻5.8官方MCP执行样条地面制作与验证，中文请求采用UTF-8。
$ErrorActionPreference='Stop'
$groundHeaders=@{Accept='application/json, text/event-stream'}
$groundUri='http://127.0.0.1:8035/mcp'
$groundInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-spline-ground';version='1'}}}|ConvertTo-Json -Depth 12
$groundResponse=Invoke-WebRequest -Uri $groundUri -Method Post -ContentType 'application/json' -Headers $groundHeaders -Body $groundInit -TimeoutSec 20
$groundHeaders['Mcp-Session-Id']=$groundResponse.Headers['Mcp-Session-Id'][0]
$groundBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.样条地面资产工具.SplineGroundTools';tool_name='process_ground';arguments=@{stage=$Stage}}}}|ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $groundUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $groundHeaders -Body ([Text.Encoding]::UTF8.GetBytes($groundBody)) -TimeoutSec 60|ConvertTo-Json -Depth 20
