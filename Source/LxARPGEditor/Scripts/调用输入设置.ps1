param([ValidateSet('inspect', 'migrate', 'tests', 'quit')][string]$Stage = 'inspect')
# 通过本机虚幻官方 MCP 调用明确的输入设置制作阶段。
$ErrorActionPreference = 'Stop'
$inputHeaders = @{ Accept = 'application/json, text/event-stream' }
$inputUri = 'http://127.0.0.1:8058/mcp'
$inputInit = @{ jsonrpc = '2.0'; id = 1; method = 'initialize'; params = @{ protocolVersion = '2024-11-05'; capabilities = @{}; clientInfo = @{ name = 'codex-input-settings'; version = '1' } } } | ConvertTo-Json -Depth 12
$inputResponse = Invoke-WebRequest -Uri $inputUri -Method Post -ContentType 'application/json' -Headers $inputHeaders -Body $inputInit -TimeoutSec 20
$inputHeaders['Mcp-Session-Id'] = $inputResponse.Headers['Mcp-Session-Id'][0]
$inputBody = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'call_tool'; arguments = @{ toolset_name = 'Game.Source.LxARPGEditor.Scripts.输入设置工具.InputSettingsTools'; tool_name = 'process_input_settings'; arguments = @{ stage = $Stage } } } } | ConvertTo-Json -Depth 15
$inputResult = Invoke-RestMethod -Uri $inputUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $inputHeaders -Body ([Text.Encoding]::UTF8.GetBytes($inputBody)) -TimeoutSec 180
foreach ($inputContent in $inputResult.result.content) {
    if ($inputContent.type -ne 'text') { continue }
    $inputPayload = $inputContent.text | ConvertFrom-Json
    if ($inputPayload.returnValue) {
        Write-Output $inputPayload.returnValue
        if ($inputPayload.returnValue.StartsWith('ERROR:')) { throw $inputPayload.returnValue }
    }
    else { Write-Output $inputContent.text }
}
if ($inputResult.error -or $inputResult.result.isError) { throw ($inputResult | ConvertTo-Json -Depth 24) }
