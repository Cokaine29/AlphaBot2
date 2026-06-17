param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet("compile", "upload", "upload-ota", "monitor", "detect")]
    [string]$Action,

    [Parameter(Position=1)]
    [string]$SketchDir
)

$configFile = Join-Path $PSScriptRoot ".arduino-config.json"

function Get-Config {
    if (Test-Path $configFile) {
        return Get-Content -Raw $configFile | ConvertFrom-Json
    } else {
        # Default config
        $default = [PSCustomObject]@{
            fqbn = "arduino:avr:uno"
            port = ""
            baud = 115200
        }
        $default | ConvertTo-Json | Out-File $configFile -Encoding utf8
        return $default
    }
}

function Save-Config($config) {
    $config | ConvertTo-Json | Out-File $configFile -Encoding utf8
}

function Auto-Detect-Port {
    Write-Host "Scanning COM ports..." -ForegroundColor Cyan
    $cliOutput = arduino-cli board list --format json | ConvertFrom-Json
    
    if ($cliOutput.detected_ports -and $cliOutput.detected_ports.Count -gt 0) {
        # Select the first detected port
        $detected = $cliOutput.detected_ports[0]
        $portAddress = $detected.port.address
        $boardName = "Generic Serial Device"
        if ($detected.matching_boards -and $detected.matching_boards.Count -gt 0) {
            $boardName = $detected.matching_boards[0].name
        }
        
        Write-Host "Found board: $boardName on port $portAddress" -ForegroundColor Green
        return $portAddress
    }
    
    # Fallback: Check for generic serial ports via WMIC if arduino-cli didn't identify a board
    $serialPorts = [System.IO.Ports.SerialPort]::GetPortNames()
    if ($serialPorts.Count -gt 0) {
        Write-Host "No recognized Arduino board matched, but serial ports found: $($serialPorts -join ', ')" -ForegroundColor Yellow
        Write-Host "Selecting first available port: $($serialPorts[0])" -ForegroundColor Yellow
        return $serialPorts[0]
    }
    
    Write-Host "Error: No connected serial devices detected." -ForegroundColor Red
    return $null
}

$config = Get-Config

switch ($Action) {
    "detect" {
        $detectedPort = Auto-Detect-Port
        if ($detectedPort) {
            $config.port = $detectedPort
            Save-Config $config
            Write-Host "Configuration updated: Target port is set to $detectedPort" -ForegroundColor Green
        } else {
            Write-Host "Could not auto-detect port. Please check your connections." -ForegroundColor Yellow
        }
    }
    
    "compile" {
        if ([string]::IsNullOrEmpty($SketchDir)) {
            $SketchDir = $PSScriptRoot
        }
        
        Write-Host "==================================================" -ForegroundColor Blue
        Write-Host "Compiling Sketch: $SketchDir" -ForegroundColor Cyan
        Write-Host "Board FQBN: $($config.fqbn)" -ForegroundColor Cyan
        Write-Host "==================================================" -ForegroundColor Blue
        
        $libsPath = Join-Path $PSScriptRoot "Arduino/libraries"
        $libTRSensors = Join-Path $libsPath "TRSensors"
        
        $compileArgs = @("compile", "--fqbn", $config.fqbn, "--libraries", $libsPath, "--library", $libTRSensors)
        if ($config.fqbn -notmatch "renesas_uno") {
            $libNeoPixel = Join-Path $libsPath "Adafruit_NeoPixel"
            $compileArgs += @("--library", $libNeoPixel)
        }
        $compileArgs += $SketchDir
        
        arduino-cli @compileArgs
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`nCompilation Successful!" -ForegroundColor Green
        } else {
            Write-Host "`nCompilation Failed with Exit Code $LASTEXITCODE" -ForegroundColor Red
            exit $LASTEXITCODE
        }
    }
    
    "upload" {
        if ([string]::IsNullOrEmpty($SketchDir)) {
            $SketchDir = $PSScriptRoot
        }
        
        # Determine Port
        $port = $config.port
        if ([string]::IsNullOrEmpty($port)) {
            Write-Host "No port configured. Attempting auto-detection..." -ForegroundColor Yellow
            $port = Auto-Detect-Port
            if ([string]::IsNullOrEmpty($port)) {
                Write-Host "Error: Cannot upload without a specified port. Connect a board or configure '.arduino-config.json'." -ForegroundColor Red
                exit 1
            }
            # Save the detected port for subsequent tasks
            $config.port = $port
            Save-Config $config
        }
        
        Write-Host "==================================================" -ForegroundColor Blue
        Write-Host "Uploading Sketch to Arduino" -ForegroundColor Cyan
        Write-Host "Sketch: $SketchDir" -ForegroundColor Cyan
        Write-Host "Board FQBN: $($config.fqbn)" -ForegroundColor Cyan
        Write-Host "COM Port: $port" -ForegroundColor Cyan
        Write-Host "==================================================" -ForegroundColor Blue
        Write-Host "Note: If uploading fails, check if the Bluetooth module is plugged in." -ForegroundColor Yellow
        Write-Host "      If it is, unplug it and try again.`n" -ForegroundColor Yellow
        
        # First Compile
        $libsPath = Join-Path $PSScriptRoot "Arduino/libraries"
        $libTRSensors = Join-Path $libsPath "TRSensors"
        Write-Host "Re-verifying code..." -ForegroundColor Gray
        
        $compileArgs = @("compile", "--fqbn", $config.fqbn, "--libraries", $libsPath, "--library", $libTRSensors)
        if ($config.fqbn -notmatch "renesas_uno") {
            $libNeoPixel = Join-Path $libsPath "Adafruit_NeoPixel"
            $compileArgs += @("--library", $libNeoPixel)
        }
        $compileArgs += $SketchDir
        
        arduino-cli @compileArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "`nError: Compilation failed. Aborting upload." -ForegroundColor Red
            exit $LASTEXITCODE
        }
        
        # Then Upload
        arduino-cli upload -p $port --fqbn $config.fqbn $SketchDir
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`nFlashing Successful! Sketch is now running on the board." -ForegroundColor Green
        } else {
            Write-Host "`nFlashing Failed with Exit Code $LASTEXITCODE" -ForegroundColor Red
            exit $LASTEXITCODE
        }
    }
    
    "monitor" {
        $port = $config.port
        if ([string]::IsNullOrEmpty($port)) {
            Write-Host "No port configured. Attempting auto-detection..." -ForegroundColor Yellow
            $port = Auto-Detect-Port
            if ([string]::IsNullOrEmpty($port)) {
                Write-Host "Error: Cannot monitor serial port without a configured port." -ForegroundColor Red
                exit 1
            }
        }
        
        Write-Host "==================================================" -ForegroundColor Blue
        Write-Host "Opening Serial Monitor on $port @ $($config.baud) baud" -ForegroundColor Cyan
        Write-Host "Press Ctrl+C to close the monitor." -ForegroundColor Yellow
        Write-Host "==================================================" -ForegroundColor Blue
        
        arduino-cli monitor -p $port -c baudrate=$($config.baud)
    }

    "upload-ota" {
        if ([string]::IsNullOrEmpty($SketchDir)) {
            $SketchDir = $PSScriptRoot
        }
        
        # Determine IP
        $ip = $config.ip
        if ([string]::IsNullOrEmpty($ip)) {
            $ip = "10.49.181.159"
        }
        
        Write-Host "==================================================" -ForegroundColor Blue
        Write-Host "Uploading Sketch wirelessly via OTA" -ForegroundColor Cyan
        Write-Host "Sketch: $SketchDir" -ForegroundColor Cyan
        Write-Host "Target IP: $ip" -ForegroundColor Cyan
        Write-Host "==================================================" -ForegroundColor Blue
        
        # First Compile
        $libsPath = Join-Path $PSScriptRoot "Arduino/libraries"
        $libTRSensors = Join-Path $libsPath "TRSensors"
        Write-Host "Re-verifying code..." -ForegroundColor Gray
        
        $compileArgs = @("compile", "--fqbn", $config.fqbn, "--libraries", $libsPath, "--library", $libTRSensors)
        if ($config.fqbn -notmatch "renesas_uno") {
            $libNeoPixel = Join-Path $libsPath "Adafruit_NeoPixel"
            $compileArgs += @("--library", $libNeoPixel)
        }
        $compileArgs += $SketchDir
        
        arduino-cli @compileArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "`nError: Compilation failed. Aborting upload." -ForegroundColor Red
            exit $LASTEXITCODE
        }
        
        # Then Upload via arduinoOTA
        $binName = (Get-Item $SketchDir).Name + ".ino.bin"
        $binPath = Join-Path $SketchDir "build/$binName"
        
        if (-not (Test-Path $binPath)) {
            Write-Host "Error: Could not find compiled binary at $binPath" -ForegroundColor Red
            exit 1
        }
        
        $otaPath = "C:\Users\Admin\AppData\Local\Arduino15\packages\arduino\tools\arduinoOTA\1.3.0\bin\arduinoOTA.exe"
        
        Write-Host "Uploading $binName to $ip..." -ForegroundColor Gray
        & $otaPath -address $ip -port 65280 -username arduino -password admin -sketch $binPath -upload /sketch
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`nOTA Upload Successful!" -ForegroundColor Green
        } else {
            Write-Host "`nOTA Upload Failed with Exit Code $LASTEXITCODE" -ForegroundColor Red
            exit $LASTEXITCODE
        }
    }
}
