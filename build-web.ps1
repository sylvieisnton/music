$ErrorActionPreference = "Stop"

Push-Location -LiteralPath $PSScriptRoot
try {
    $compiler = (Get-Command "em++" -ErrorAction Stop).Source

    $flags = @(
        "-std=c++17"
        "-O3"
        "-fexceptions"
        "--no-entry"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sENVIRONMENT=worker"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sFORCE_FILESYSTEM=1"
        "-sEXPORTED_FUNCTIONS=_run_music"
        "-sEXPORTED_RUNTIME_METHODS=ccall,FS"
    )

    & $compiler "web/bridge.cpp" "-DMUSIC_ENCODE=1" @flags `
        "-o" "web/encode.mjs"

    if ($LASTEXITCODE -ne 0) {
        throw "Encoder build failed."
    }

    & $compiler "web/bridge.cpp" @flags `
        "-o" "web/decode.mjs"

    if ($LASTEXITCODE -ne 0) {
        throw "Decoder build failed."
    }

    Write-Host "Built web/encode.mjs and web/decode.mjs with WASM."
}
finally {
    Pop-Location
}