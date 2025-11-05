$vcxprojPath = "Engine\Engine.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Replace hardcoded paths with MSBuild macros
$content = $content -replace [regex]::Escape('C:\Users\Jungle\Desktop\Projects\Jungle\Week09\FutureEngine\Build\Debug\Intermediate\'), '$(IntDir)'
$content = $content -replace [regex]::Escape('C:\Users\Jungle\Desktop\Projects\Jungle\Week09\FutureEngine\Engine\Source'), '$(ProjectDir)Source'
$content = $content -replace [regex]::Escape('C:\Users\Jungle\Desktop\Projects\Jungle\Week09\FutureEngine\External\Include'), '$(SolutionDir)External\Include'

# Save the modified content
Set-Content $vcxprojPath -Value $content -NoNewline

Write-Host "Paths fixed successfully!"
