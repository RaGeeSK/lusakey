# lusakey

Кроссплатформенный (Windows/macOS/Linux) локальный менеджер паролей со встроенным TOTP-генератором (аналог Google Authenticator). Одна кодовая база на C++/Qt6, хранилище — единственный зашифрованный файл на устройстве, без облака и аккаунтов.

Полный план архитектуры: `docs/VAULT_FORMAT.md` (формат хранилища) и история планирования проекта.

## Статус

Все вехи M0–M9 закрыты кодом (крипто-ядро, TOTP, VaultService, QML GUI, ipc/qr, упаковка). **Сборка проверена на Windows (MSVC 2022 + Ninja + vcpkg + Qt 6.7.3):** ядро собирается, `ctest` — 100% тестов проходят; GUI-приложение собирается и запускается (дымовой прогон без визуальной проверки — см. `AGENTS.md`, «Известные пробелы и риски», там же список того, что ещё не проверялось на macOS/Linux и что не проверено визуально/интерактивно в GUI).

## Требования для сборки

- CMake ≥ 3.25
- Компилятор с поддержкой C++20 (MSVC 2022 / Clang / GCC ≥ 11)
- [vcpkg](https://github.com/microsoft/vcpkg) с установленной переменной окружения `VCPKG_ROOT`
- Ninja (используется генератором в `CMakePresets.json`)
- Qt 6.5+ (нужен только для сборки GUI, `-DLUSAKEY_BUILD_APP=ON`)

### Windows: установка тулчейна одним скриптом

`scripts/windows/setup-toolchain.ps1` ставит Git/CMake/Ninja/MSVC (Visual Studio Build Tools)/vcpkg/Qt6 через winget и aqtinstall. Идемпотентен (безопасно перезапускать), ничего не делает при простом чтении файла — только при explicit-запуске:

```powershell
.\scripts\windows\setup-toolchain.ps1
# или без Qt (если нужно собрать только ядро без GUI):
.\scripts\windows\setup-toolchain.ps1 -SkipQt
```

После него — перезапустить терминал (чтобы подхватились PATH/`VCPKG_ROOT`) и собирать как обычно (см. ниже).

## Сборка ядра и тестов

```sh
cmake --preset windows-x64   # или linux-x64 / macos-arm64 / macos-x64
cmake --build --preset windows-x64
ctest --preset windows-x64
```

При первой конфигурации vcpkg автоматически установит зависимости из `vcpkg.json` (libsodium, `nu-book-zxing-cpp` — vcpkg-имя порта zxing-cpp, nlohmann-json, Catch2).

Для сборки GUI дополнительно передайте `-DLUSAKEY_BUILD_APP=ON -DCMAKE_PREFIX_PATH="<путь к Qt6>\<версия>\<kit>"`, например `-DCMAKE_PREFIX_PATH="C:\Qt\6.7.3\msvc2019_64"`.
