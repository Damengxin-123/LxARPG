param([string]$Stage = 'inspect')
# 使用 UE 5.8 官方 MCP 串行处理曝光维护操作，中文 JSON 按 UTF-8 发送。
$ErrorActionPreference = 'Stop'
$exposureUri = 'http://127.0.0.1:8061/mcp'
$exposureHeaders = @{ Accept = 'application/json, text/event-stream' }
$exposureInit = @{ jsonrpc = '2.0'; id = 1; method = 'initialize'; params = @{ protocolVersion = '2024-11-05'; capabilities = @{}; clientInfo = @{ name = 'codex-world-emission'; version = '1' } } } | ConvertTo-Json -Depth 12
$exposureResponse = Invoke-WebRequest -Uri $exposureUri -Method Post -ContentType 'application/json' -Headers $exposureHeaders -Body $exposureInit -TimeoutSec 20
$exposureHeaders['Mcp-Session-Id'] = $exposureResponse.Headers['Mcp-Session-Id'][0]
$exposureBody = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'call_tool'; arguments = @{ toolset_name = 'Game.Source.LxARPGEditor.Scripts.修复场景自发光.WorldEmissionTools'; tool_name = 'process'; arguments = @{ stage = $Stage } } } } | ConvertTo-Json -Depth 15
$exposureResult = Invoke-RestMethod -Uri $exposureUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $exposureHeaders -Body ([Text.Encoding]::UTF8.GetBytes($exposureBody)) -TimeoutSec 360
$exposureResult | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath (Join-Path $PSScriptRoot ('../../../Saved/黑色显示修复/MCP_' + $Stage + '.json')) -Encoding utf8
foreach ($item in $exposureResult.result.content) {
    if ($item.type -ne 'text') { continue }
    $payload = $item.text | ConvertFrom-Json
    if ($payload.returnValue) {
        Write-Output $payload.returnValue
        $operationResult = $payload.returnValue | ConvertFrom-Json
        if ($operationResult -is [pscustomobject] -and $operationResult.PSObject.Properties.Name -contains 'error' -and $operationResult.error) { throw $operationResult.error }
    }
    else { Write-Output $item.text }
}
if ($exposureResult.error -or $exposureResult.result.isError) { throw ($exposureResult | ConvertTo-Json -Depth 30) }