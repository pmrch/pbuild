using namespace System.Collections.Generic

$PSStyle.OutputRendering = 'ANSI'

[string] $cStandard = "clatest"
[string] $cCompiler = "clang-cl.exe"
[string] $cLinker = "clang-cl.exe"
[string] $cTarget = "pbuild"

[string] $mimallocLib = "D:\devTools\vcpkg\installed\x64-windows-static\lib"
[string] $cIncludes = "/Iinclude"
[string] $cOptimization = "/Gy /fp:fast /clang:-O3 /clang:-march=native"
[string] $cWinVer = "/D_WIN32_WINNT=0x0A00"
[string] $cAvoidTrouble = "/clang:-Wno-keyword-macro"
#[string]$cDebug = "-fsanitize=address /O0"

[string]$cStrictFlags = @(
    "/clang:-Wall /clang:-Wextra /clang:-Wpedantic /clang:-Werror /clang:-Wuninitialized /clang:-Wold-style-definition "
    "/clang:-Wsign-conversion /clang:-Wcast-align /clang:-Wcast-qual /clang:-Wstrict-aliasing=2 /clang:-Wpointer-arith /clang:-Warray-bounds "
    "/clang:-Wnull-dereference /clang:-Wmissing-prototypes /clang:-Wstrict-prototypes /clang:-Wconversion /clang:-Wredundant-decls /clang:-Wvla "
    "/clang:-Wshadow /clang:-Wundef /clang:-Wformat=2 /clang:-Wformat-security /clang:-Wwrite-strings /clang:-Wdouble-promotion /clang:-Wfloat-equal "
    "/clang:-Wswitch-enum /clang:-Wswitch-default /clang:-Wunused /clang:-Wunused-function /clang:-Wunused-variable /clang:-Wunused-parameter "
    "/clang:-Wno-padded /clang:-Wno-declaration-after-statement /clang:-Weverything /clang:-Wno-unsafe-buffer-usage /clang:-Wno-disabled-macro-expansion"
) -join ""

[string] $cCflags = "/nologo /std:$cStandard -fcolor-diagnostics -fansi-escape-codes /MT $cOptimization $cIncludes $cStrictFlags $cAvoidTrouble $cWinVer"
[string] $cLdflags = "/nologo /link /LIBPATH:$mimallocLib advapi32.lib mimalloc.lib /SUBSYSTEM:CONSOLE"
[string] $ccJson = "compile_commands_test.json"

[string] $cCwd = $PWD.Path
[string] $cSrc = Join-Path $cCwd "src"

[System.IO.DirectoryInfo] $cBuild = [System.IO.Path]::Combine($cCwd, "build")
if (-not $cBuild.Exists) { New-Item -ItemType Directory -Path $cBuild | Out-Null }

class ObjectFile {
    [string] $fullPath
    [string] $fileName

    ObjectFile([string] $fPath, [string] $fName) {
        $this.fullPath = $fPath
        $this.fileName = $fName
    }
}


[List[ObjectFile]] $objFileList = [List[ObjectFile]]::new()

function Compile {
    param (
        [Parameter(Mandatory)]
        [int]$LogLevel
    )
    
    [List[string]] $compileDb = [List[string]]::new()
    [string[]] $baseArgs = @($cCompiler) + $cCflags.Split()
    if (-not (([System.IO.DirectoryInfo]$ccJson).Exists)) { Write-Output "" > $ccJson }

    Get-ChildItem -Path $cSrc -Filter "*.c" -Recurse -File | ForEach-Object {
        [string] $srcFileBase = $_.BaseName
        [string] $srcFileName = $_.Name

        [string] $objName = "$srcFileBase.obj"
        [string] $objPath = [System.IO.Path]::Combine($cBuild, $objName)
        [string] $srcFileFullPath = $_.FullName

        [string[]] $extraArgs = @("/clang:-DLOG_LEVEL=$LogLevel", "/c", "$srcFileFullPath", "/Fo:$objPath")
        [string[]] $cArgs = $baseArgs + $extraArgs

        [string] $dbEntry = [ordered]@{ 
            arguments = $cArgs; 
            directory = $cCwd; 
            file      = $srcFileFullPath; 
            output    = $objPath 
        } | ConvertTo-Json 
        
        $compileDb.Add($dbEntry)
        $objFileList.Add([ObjectFile]::new($objPath, $objName))

        [string] $extraArgsString = $extraArgs -join " "
        [ScriptBlock]$scriptBlock = [ScriptBlock]::Create("$cCompiler $cCflags $extraArgsString")

        Write-Host "src/$srcFileName => build/$objName"
        [string] $outString = (& $scriptBlock --color=always 2>&1) -join [System.Environment]::NewLine
        [int] $exitCode = $LASTEXITCODE

        if (-not ($exitCode -eq 0)) { 
            Write-Host "$outString" 
            Break
        }
    }

    $compileDbJson = foreach ($json in $compileDb) { $json | ConvertFrom-Json }
    $compileDbJson | ConvertTo-Json -Depth 10 | Set-Content $ccJson
    $LASTEXITCODE = 0
}

function LinkObjs {
    param ()
    
    if (-not ($objFileList.Count -eq 0)) {
        [string] $objFiles = foreach ($obj in $objFileList) { $obj.fullPath }
        [string] $lnkArgs = "$cLinker $objFiles /Fe:$cTarget.exe $cLdflags"
        [ScriptBlock]$scriptBlock = [ScriptBlock]::Create("$lnkArgs")

        [string] $outString = (& $scriptBlock --color=always 2>&1) -join [System.Environment]::NewLine
        [int] $exitCode = $LASTEXITCODE

        if (-not ($exitCode -eq 0)) {
            Write-Host "Failed to link final executable: $outString"
            Return
        }
    }
}

[int] $logLvl = if ($args.Count -gt 0) { $args[0] } else { 1 }
Compile($logLvl)
LinkObjs