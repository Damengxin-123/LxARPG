param([string]$Stage = 'inspect')
# 通过虚幻官方 MCP 执行技能美术工具，中文 JSON 使用 UTF-8 传输。
$ErrorActionPreference = 'Stop'
$skillArtHeaders = @{ Accept = 'application/json, text/event-stream' }
$skillArtUri = 'http://127.0.0.1:8051/mcp'
$skillArtInit = @{ jsonrpc = '2.0'; id = 1; method = 'initialize'; params = @{ protocolVersion = '2024-11-05'; capabilities = @{}; clientInfo = @{ name = 'codex-skill-art'; version = '1' } } } | ConvertTo-Json -Depth 12
$skillArtResponse = Invoke-WebRequest -Uri $skillArtUri -Method Post -ContentType 'application/json' -Headers $skillArtHeaders -Body $skillArtInit -TimeoutSec 20
$skillArtHeaders['Mcp-Session-Id'] = $skillArtResponse.Headers['Mcp-Session-Id'][0]
$skillArtBody = @{ jsonrpc = '2.0'; id = 2; method = 'tools/call'; params = @{ name = 'call_tool'; arguments = @{ toolset_name = 'Game.Source.LxARPGEditor.Scripts.技能美术工具.SkillArtTools'; tool_name = 'process_skill_art'; arguments = @{ stage = $Stage } } } } | ConvertTo-Json -Depth 15
$skillArtResult = Invoke-RestMethod -Uri $skillArtUri -Method Post -ContentType 'application/json; charset=utf-8' -Headers $skillArtHeaders -Body ([Text.Encoding]::UTF8.GetBytes($skillArtBody)) -TimeoutSec 600
$skillArtReportRoot = Join-Path $PSScriptRoot '../../../Saved/技能美术'
[IO.Directory]::CreateDirectory($skillArtReportRoot) | Out-Null
$skillArtResult | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath (Join-Path $skillArtReportRoot ('MCP_' + $Stage + '.json')) -Encoding utf8
foreach ($skillArtContent in $skillArtResult.result.content) {
    if ($skillArtContent.type -eq 'text') {
        $skillArtPayload = $skillArtContent.text | ConvertFrom-Json
        if ($skillArtPayload.returnValue) {
            Write-Output $skillArtPayload.returnValue
            if ($skillArtPayload.returnValue.StartsWith('ERROR:')) { throw $skillArtPayload.returnValue }
        }
        else { Write-Output $skillArtContent.text }
    }
}
if ($skillArtResult.error -or $skillArtResult.result.isError) { throw ($skillArtResult | ConvertTo-Json -Depth 24) }
