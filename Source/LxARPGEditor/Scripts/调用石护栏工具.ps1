param([ValidateSet('build','verify','preview','tests','refresh','reload','corners','corners_before','corner_verify')][string]$Stage='verify')
# 调用 UE 5.8 官方 MCP；应先以 -ExecutePythonScript 加载同目录的石护栏资产工具.py。
$ErrorActionPreference='Stop'
$railHeaders=@{Accept='application/json, text/event-stream'}
$railUri='http://127.0.0.1:8031/mcp'
$railInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='stone-railing';version='1'}}}|ConvertTo-Json -Depth 8
$railResponse=Invoke-WebRequest -Uri $railUri -Method Post -ContentType 'application/json' -Headers $railHeaders -Body $railInit -TimeoutSec 15
$railHeaders['Mcp-Session-Id']=$railResponse.Headers['Mcp-Session-Id'][0]
$railBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.石护栏资产工具.StoneRailingTools';tool_name='process_railing';arguments=@{stage=$Stage}}}}|ConvertTo-Json -Depth 12
$railResult=Invoke-RestMethod -Uri $railUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $railHeaders -Body ([Text.Encoding]::UTF8.GetBytes($railBody)) -TimeoutSec 240
$railResult|ConvertTo-Json -Depth 20
