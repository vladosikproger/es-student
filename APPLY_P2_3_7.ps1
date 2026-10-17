$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$project = Join-Path $root '231-firmware-template'

if (-not (Test-Path (Join-Path $project 'CMakeLists.txt'))) {
    throw "Не найден $project. Распакуй содержимое архива в корень es-student рядом с папкой libs, затем запусти этот скрипт ещё раз."
}

# Удаляем только устаревшие файлы, заменённые текущим заданием.
$obsolete = @(
    (Join-Path $project 'command.h'),
    (Join-Path $project 'logic/api/api-callbacks.h'),
    (Join-Path $project 'logic/CMakeLists.txt'),
    (Join-Path $project 'platform/CMakeLists.txt')
)
foreach ($path in $obsolete) {
    if (Test-Path $path -PathType Leaf) {
        Remove-Item -LiteralPath $path
        Write-Host "Удалён устаревший файл: $path"
    }
}

$nested = Join-Path $project 'logic/logic'
if (Test-Path $nested -PathType Container) {
    if (-not (Get-ChildItem -LiteralPath $nested -Force)) {
        Remove-Item -LiteralPath $nested
        Write-Host "Удалена пустая папка: $nested"
    } else {
        Write-Host "Папка $nested не пуста — оставлена без изменений."
    }
}
Write-Host 'Обновление файлов п2.3.7 завершено. Перед сборкой проверь check-2-3-7.yml.'
