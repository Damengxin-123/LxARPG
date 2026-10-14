param([ValidateSet('build','verify','preview','tests','refresh','reload')][string]$Stage='verify')
# 调用引擎官方 MCP，先在独立编辑器中以 -ExecutePythonScript 加载海堤资产工具.py。
$ErrorActionPreference='Stop'
$seaHeaders=@{Accept='application/json, text/event-stream'}
$seaUri='http://127.0.0.1:8032/mcp'
$seaInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='seawall';version='1'}}}|ConvertTo-Json -Depth 8
$seaResponse=Invoke-WebRequest -Uri $seaUri -Method Post -ContentType 'application/json' -Headers $seaHeaders -Body $seaInit -TimeoutSec 15
$seaHeaders['Mcp-Session-Id']=$seaResponse.Headers['Mcp-Session-Id'][0]
$seaBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.海堤资产工具.SeawallTools';tool_name='process_seawall';arguments=@{stage=$Stage}}}}|ConvertTo-Json -Depth 12
$seaResult=Invoke-RestMethod -Uri $seaUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $seaHeaders -Body ([Text.Encoding]::UTF8.GetBytes($seaBody)) -TimeoutSec 240
$seaResult|ConvertTo-Json -Depth 20
