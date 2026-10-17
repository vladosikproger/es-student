Как применить патч

1. Распакуй архив в C:\Repositories\pico\es-student — рядом должны оказаться папки 231-firmware-template и libs, а также APPLY_P2_3_7.ps1. Подтверди замену только тех файлов, которые спросит Windows.
2. Открой PowerShell в C:\Repositories\pico\es-student и выполни: .\APPLY_P2_3_7.ps1
3. Скрипт удаляет только перечисленные устаревшие файлы и только пустую папку logic\logic. Исходники он не удаляет.
4. Workflow check-2-3-7.yml в присланном архиве отсутствовал; скопируй оригинальный файл из материалов AntLab в 231-firmware-template\.github\workflows\.
5. Перейди в C:\Repositories\pico\es-student\231-firmware-template и выполни cmake --build build.

Патч рассчитан на корень репозитория es-student, в котором 231-firmware-template и libs расположены рядом. Не распаковывай его внутрь самой папки 231-firmware-template.
