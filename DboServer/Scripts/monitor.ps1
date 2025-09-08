# Monitor-Auth20300.ps1
# Monitorea conexiones al puerto 20300 y consumo del proceso que lo usa.
# Crea logs en C:\AuthMonitor\Logs\

param(
    [int]$Port = 20200,
    [int]$IntervalSeconds = 60,
    [int]$TopN = 10,
    [string]$LogDir = "C:\AuthMonitor\Logs"
)

# Preparar carpetas y archivos
New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
$summaryCsv = Join-Path $LogDir ("summary_{0}.csv" -f (Get-Date -Format "yyyyMMdd"))
$detailLog  = Join-Path $LogDir ("details_{0}.log"  -f (Get-Date -Format "yyyyMMdd"))

# Escribir encabezado CSV si no existe
if (-not (Test-Path $summaryCsv)) {
    "Timestamp,Total,Listen,Established,SynSent,SynReceived,TimeWait,CloseWait,FinWait1,FinWait2,Closing,LastAck,Bound,Other,ProcId,ProcName,CPU_s,WorkingSet_MB,Handles" | Out-File -FilePath $summaryCsv -Encoding UTF8
}

Write-Host "Monitoring TCP port $Port every $IntervalSeconds seconds. Logs in $LogDir`nPress Ctrl+C to stop." -ForegroundColor Cyan

function Get-PortProcessInfo {
    param([int]$Port)
    $listenConns = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if ($listenConns) {
        $procId = ($listenConns | Select-Object -First 1).OwningProcess
        try {
            $proc = Get-Process -Id $procId -ErrorAction Stop
            return [pscustomobject]@{
                PID   = $procId
                Name  = $proc.ProcessName
                CPU_s = [math]::Round($proc.CPU,2)              # tiempo acumulado de CPU en segundos
                WS_MB = [math]::Round($proc.WorkingSet64/1MB,1) # RAM en MB
                Handles = $proc.Handles
            }
        } catch {
            return [pscustomobject]@{ PID=$procId; Name="(exited)"; CPU_s=0; WS_MB=0; Handles=0 }
        }
    } else {
        return $null
    }
}

while ($true) {
    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

    # Conexiones al puerto
    $conns = Get-NetTCPConnection -LocalPort $Port -ErrorAction SilentlyContinue
    $total = ($conns | Measure-Object).Count

    # Conteo por estado
    $byState = $conns | Group-Object -Property State | Sort-Object Name
    $stateMap = @{
        Listen=0; Established=0; SynSent=0; SynReceived=0; TimeWait=0; CloseWait=0; FinWait1=0; FinWait2=0; Closing=0; LastAck=0; Bound=0; Other=0
    }
    foreach ($g in $byState) {
        if ($stateMap.ContainsKey($g.Name)) {
            $stateMap[$g.Name] = $g.Count
        } else {
            $stateMap["Other"] += $g.Count
        }
    }

    # Top IPs remotas por cantidad de conexiones
    $topIps = $conns | Where-Object { $_.RemoteAddress -and $_.State -ne "Listen" } |
        Group-Object -Property RemoteAddress |
        Sort-Object Count -Descending |
        Select-Object -First $TopN

    # Info del proceso que está en LISTEN (si existe)
    $pinfo = Get-PortProcessInfo -Port $Port

    # Guardar resumen CSV
    $csvLine = ('"{0}",{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},"{14}",{15},{16},{17}' -f
        $ts,
        $total,
        $stateMap.Listen,
        $stateMap.Established,
        $stateMap.SynSent,
        $stateMap.SynReceived,
        $stateMap.TimeWait,
        $stateMap.CloseWait,
        $stateMap.FinWait1,
        $stateMap.FinWait2,
        $stateMap.Closing,
        $stateMap.LastAck,
        $stateMap.Bound,
        $stateMap.Other,
        ($(if($pinfo){$pinfo.PID}else{"-"})),
        ($(if($pinfo){'"' + $pinfo.Name + '"'}else{'"-"'})),
        ($(if($pinfo){$pinfo.CPU_s}else{"-"})),
        ($(if($pinfo){$pinfo.WS_MB}else{"-"})),
        ($(if($pinfo){$pinfo.Handles}else{"-"}))
    )
    Add-Content -Path $summaryCsv -Value $csvLine

    # Guardar detalle con Top IPs
    Add-Content -Path $detailLog -Value ("`n[{0}] Port {1} - Total:{2}  ESTABLISHED:{3}  SYN-RX:{4}  TIME_WAIT:{5}" -f $ts,$Port,$total,$stateMap.Established,$stateMap.SynReceived,$stateMap.TimeWait)
    if ($topIps) {
        Add-Content -Path $detailLog -Value ("Top {0} remote IPs:" -f $TopN)
        foreach ($ip in $topIps) {
            Add-Content -Path $detailLog -Value ("  {0}  ->  {1} conns" -f $ip.Name, $ip.Count)
        }
    } else {
        Add-Content -Path $detailLog -Value ("(no remote connections)")
    }

    # Mensaje en consola útil
    Write-Host ("[{0}] Total:{1}  EST:{2}  SYN-RX:{3}  TIME_WAIT:{4}  PID:{5} ({6}) CPU:{7}s RAM:{8}MB" -f `
        $ts, $total, $stateMap.Established, $stateMap.SynReceived, $stateMap.TimeWait, `
        ($(if($pinfo){$pinfo.PID}else{"-"})), $(if($pinfo){$pinfo.Name}else{"-"}), $(if($pinfo){$pinfo.CPU_s}else{"-"}), $(if($pinfo){$pinfo.WS_MB}else{"-"})) `
        -ForegroundColor Green

    Start-Sleep -Seconds $IntervalSeconds
}
