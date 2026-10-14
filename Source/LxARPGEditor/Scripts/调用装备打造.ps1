param([ValidateSet('inspect','build','npc','preview','tests','maptests','quit')][string]$Stage='inspect')
# 使用虚幻5.8官方MCP，中文参数以UTF-8发送。
$ErrorActionPreference='Stop'
$craftHeaders=@{Accept='application/json, text/event-stream'}
$craftUri='http://127.0.0.1:8041/mcp'
$craftInit=@{jsonrpc='2.0';id=1;method='initialize';params=@{protocolVersion='2024-11-05';capabilities=@{};clientInfo=@{name='codex-equipment-crafting';version='1'}}} | ConvertTo-Json -Depth 12
$craftResponse=Invoke-WebRequest -Uri $craftUri -Method Post -ContentType 'application/json' -Headers $craftHeaders -Body $craftInit -TimeoutSec 20
$craftHeaders['Mcp-Session-Id']=$craftResponse.Headers['Mcp-Session-Id'][0]
$craftBody=@{jsonrpc='2.0';id=2;method='tools/call';params=@{name='call_tool';arguments=@{toolset_name='Game.Source.LxARPGEditor.Scripts.装备打造工具.EquipmentCraftingTools';tool_name='process_equipment';arguments=@{stage=$Stage}}}} | ConvertTo-Json -Depth 15
Invoke-RestMethod -Uri $craftUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $craftHeaders -Body ([Text.Encoding]::UTF8.GetBytes($craftBody)) -TimeoutSec 240 | ConvertTo-Json -Depth 20
