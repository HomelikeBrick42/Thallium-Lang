$srcDir =    [String]$pwd + "\src\"        # Directory for source files
$buildDir =    [String]$pwd + "\build\"    # Ouput directory
$outFile =    "Thallium.exe"               # Executable name

$compilerFlags =
    "-g",
    "-std=c17",
    "-Wall",
    "-Werror",
    "-Wno-unused-parameter",
    "-Wno-unused-variable"
$compilerDefines =
    "-D_CRT_SECURE_NO_WARNINGS",
    "-D_DEBUG"

$files = [System.Collections.ArrayList]@() # Create empty array
foreach ($file in Get-ChildItem $srcDir -Recurse -Include "*.c" -Force) { # For each file in the source directory
    $unused = $files.Add($file.FullName)                                  # Add it to the list
}

Write-Output Compiling: @files # Output the files that we are compiling to the console

if (!(Test-Path buildDir)) {                                # If the build directory does not exist
    $unused = New-Item $buildDir -ItemType Directory -Force # Create it
}

Push-Location $buildDir                                     # Go into the build directory
clang @compilerDefines @compilerFlags -o $outFile @files    # Build the project
Pop-Location                                                # Exit the build directory
