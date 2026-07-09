# Шрифты

Файлы шрифтов сюда не входят (не могут быть добавлены как бинарные файлы в рамках этой сессии) — их нужно скачать и положить в эту папку (`app` ищет их во время выполнения по относительному пути `fonts/...`, см. `app/main.cpp: loadBundledFonts()`), либо (лучше для релизной сборки) встроить через Qt resource system в `app/CMakeLists.txt`.

Обе гарнитуры — OFL (SIL Open Font License), можно свободно встраивать в приложение:

- **Inter** — https://github.com/rsms/inter/releases (нужны как минимум Regular/Medium/SemiBold/Bold, статические `.ttf`, не variable-шрифт — см. обоснование в `AGENTS.md`)
- **JetBrains Mono** — https://github.com/JetBrains/JetBrainsMono/releases (Regular/Medium)

Ожидаемые имена файлов (см. `loadBundledFonts()` в `app/main.cpp`):

```
resources/fonts/Inter-Regular.ttf
resources/fonts/Inter-Medium.ttf
resources/fonts/Inter-SemiBold.ttf
resources/fonts/Inter-Bold.ttf
resources/fonts/JetBrainsMono-Regular.ttf
resources/fonts/JetBrainsMono-Medium.ttf
```

Если файлов нет — приложение всё равно запустится (используется системный шрифт), в консоль просто выводится предупреждение.
