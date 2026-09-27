$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Net.Http
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12
$env:GIT_TERMINAL_PROMPT='0'; $env:GCM_INTERACTIVE='never'
$raw = cmd /c 'git -c credential.helper=manager credential fill < _p5_cred.txt' 2>&1
$txt = ($raw | Out-String)
if ($txt -notmatch 'password=(\S+)') { Write-Output 'NO_TOKEN'; exit 2 }
$tok = $Matches[1]
$handler = New-Object System.Net.Http.HttpClientHandler
$handler.Proxy = New-Object System.Net.WebProxy('http://127.0.0.1:7892')
$handler.UseProxy = $true
$client = New-Object System.Net.Http.HttpClient($handler)
$client.DefaultRequestHeaders.Authorization = New-Object System.Net.Http.Headers.AuthenticationHeaderValue('Bearer', $tok)
$client.DefaultRequestHeaders.UserAgent.ParseAdd('bfc-release-patch')
$client.Timeout = [TimeSpan]::FromSeconds(40)
$base = 'https://api.github.com/repos/xiaoguo141106/bfc'
$ex = $client.GetAsync($base + '/releases/tags/beta-0.0.4').GetAwaiter().GetResult()
Write-Output ('GET release -> ' + [int]$ex.StatusCode)
if ([int]$ex.StatusCode -ne 200) { exit 2 }
$rel = ($ex.Content.ReadAsStringAsync().GetAwaiter().GetResult()) | ConvertFrom-Json
$notes = Get-Content -Raw -Encoding UTF8 '_p5_notes.md'
$esc = $notes
$esc = $esc -replace '\\', '\\'
$esc = $esc -replace '"', '\"'
$esc = $esc -replace ([string][char]13), ''
$esc = $esc -replace ([string][char]10), '\n'
$esc = $esc -replace ([string][char]9), '\t'
$payload = '{"body":"' + $esc + '"}'
$enc = New-Object System.Text.UTF8Encoding($false)
$content = New-Object System.Net.Http.StringContent($payload, $enc, 'application/json')
$req = New-Object System.Net.Http.HttpRequestMessage([System.Net.Http.HttpMethod]::Patch, $base + '/releases/' + $rel.id)
$req.Content = $content
$resp = $client.SendAsync($req).GetAwaiter().GetResult()
Write-Output ('PATCH release -> ' + [int]$resp.StatusCode)
if ([int]$resp.StatusCode -eq 200) { Write-Output ('updated: ' + $rel.html_url) } else { $b = $resp.Content.ReadAsStringAsync().GetAwaiter().GetResult(); Write-Output ($b.Substring(0, [Math]::Min(400, $b.Length))) }
