param([ValidateSet('build','style','inspect','materials','demo','capture','verify','tests','refresh','quit')][string]$Stage='build')
# 通过 UE 5.8 官方 MCP 制作和验收台地；中文 JSON 以 UTF-8 发送。
$ErrorActionPreference='Stop'
$terraceHeaders=@{Accept='application/json, text/event-stream'}
$terraceUri='http://127.0.0.1:8011/mcp'
$terraceInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-stone-terrace';version='1'}}} | ConvertTo-Json -Depth 12
$terraceResponse=Invoke-WebRequest -Uri $terraceUri -Method Post -ContentType 'application/json' -Headers $terraceHeaders -Body $terraceInit -TimeoutSec 20
$terraceHeaders['Mcp-Session-Id']=$terraceResponse.Headers['Mcp-Session-Id'][0]
$terraceBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.台地资产工具.StoneTerraceTools';tool_name='process_terrace';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $terraceUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $terraceHeaders -Body ([Text.Encoding]::UTF8.GetBytes($terraceBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 20
