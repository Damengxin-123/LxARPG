param([ValidateSet('inspect', 'tests', 'pie', 'quit')][string]$Stage = 'inspect')
# 串行调用虚幻 5.8 官方 MCP，中文 JSON 使用 UTF-8。
$ErrorActionPreference = 'Stop'
$characterSaveHeaders = @{ Accept = 'application/json, text/event-stream' }
$characterSaveUri = 'http://127.0.0.1:8057/mcp'
$characterSaveInit = @{ jsonrpc = '2.0'; id = 1; method = 'initialize'; params = @{ protocolVersion = '2024-11-05'; capabilities = @{}; clientInfo = @{ name = 'codex-character-save-validation'; version = '1' } } } | ConvertTo-Json -Depth 12
$characterSaveResponse = Invoke-WebRequest -Uri $characterSaveUri -Method Post -ContentType 'application/json' -Headers $characterSaveHeaders -Body $characterSaveInit -TimeoutSec 20
$characterSaveHeaders['Mcp-Session-Id'] = $characterSaveResponse.Headers['Mcp-Session-Id'][0]
$characterSaveBody = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'call_tool'; arguments = @{ toolset_name = 'Game.Source.LxARPGEditor.Scripts.角色存档验收.CharacterSaveValidationTools'; tool_name = 'validate_character_save'; arguments = @{ stage = $Stage } } } } | ConvertTo-Json -Depth 15
$characterSaveResult = Invoke-RestMethod -Uri $characterSaveUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $characterSaveHeaders -Body ([Text.Encoding]::UTF8.GetBytes($characterSaveBody)) -TimeoutSec 60
$characterSaveResult | ConvertTo-Json -Depth 20
if ($characterSaveResult.error -or $characterSaveResult.result.isError) { throw '角色存档官方 MCP 验收请求失败。' }
