# Шрифты

Файлы шрифтов лежат здесь (Inter 4.1, JetBrains Mono 2.304 — оба OFL, свободно встраиваются в приложение):

```
resources/fonts/Inter-Regular.ttf
resources/fonts/Inter-Medium.ttf
resources/fonts/Inter-SemiBold.ttf
resources/fonts/Inter-Bold.ttf
resources/fonts/JetBrainsMono-Regular.ttf
resources/fonts/JetBrainsMono-Medium.ttf
```

`app/CMakeLists.txt` копирует их post-build в `<папка_с_exe>/fonts/`; `app/main.cpp` (`loadBundledFonts()`) резолвит путь от `QCoreApplication::applicationDirPath()`, а не от текущей рабочей директории — иначе поиск ломался бы в зависимости от того, как запущен exe (ярлык, другой рабочий каталог и т.п.).

Источники (на случай обновления версии):
- **Inter** — https://github.com/rsms/inter/releases (нужны как минимум Regular/Medium/SemiBold/Bold, статические `.ttf` из `extras/ttf/`, не variable-шрифт — см. обоснование в `AGENTS.md`)
- **JetBrains Mono** — https://github.com/JetBrains/JetBrainsMono/releases (Regular/Medium, статические `.ttf` из `fonts/ttf/`)

Если файлов нет (например, после `git clone` без LFS/бинарей) — приложение всё равно запустится, просто использует системный шрифт вместо Inter/JetBrains Mono и выводит предупреждение в консоль.
