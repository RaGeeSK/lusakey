# AGENTS.md — lusakey

Инструкции для агентов (и людей), работающих в этом репозитории. Держите этот файл в актуальном состоянии по мере продвижения — особенно секции «Статус» и «Известные пробелы».

## Что это за проект

Кроссплатформенный (Windows/macOS/Linux) локальный менеджер паролей со встроенным TOTP-генератором (аналог Google Authenticator). Одна кодовая база на C++/Qt6. Хранилище — единственный зашифрованный файл на устройстве, без облака, без аккаунтов, без серверов. Между устройствами — только ручной экспорт/импорт зашифрованного файла. Дизайн — в стиле Claude.ai (тёплая палитра, терракотовый акцент).

## Зафиксированные архитектурные решения (не пересматривать без явного запроса пользователя)

- **Стек:** нативный C++20/Qt6, не Electron/Tauri/webview.
- **UI-слой: Qt Quick/QML**, не Qt Widgets. Выбрано пользователем осознанно ради скорости итерации по кастомному дизайну — несмотря на то что Telegram Desktop (эталон «одна кодовая база под 3 ОС») исторически рисует UI вручную на Widgets. Не переключать на Widgets без явного запроса.
- **`libs/core` не имеет ни одной зависимости от Qt.** Жёсткое правило: весь код в `libs/core` компилируется и тестируется без Qt. Это то, что позволяет `nmhost` (будущему native-messaging-host для браузерного расширения) переиспользовать `VaultService` без переписывания.
- **Синхронизация — только локально + ручной экспорт/импорт.** Никакого облака, никаких аккаунтов, никаких серверов.
- **Браузерное расширение не реализуется**, но шов (`VaultService` + `libs/ipc`) существует и компилируется уже сейчас (`nmhost/`, флаг `LUSAKEY_BUILD_NMHOST`, выключен по умолчанию).
- **Криптография:** libsodium — Argon2id (KDF) + XChaCha20-Poly1305 (AEAD). Не менять без явного запроса — обоснование в `docs/VAULT_FORMAT.md`.
- **TOTP:** RFC 4226/6238, совместимость с `otpauth://`. HMAC-SHA1 — вендоренная реализация (её нет в libsodium), HMAC-SHA256/512 — через libsodium напрямую.
- **Отложенный компромисс:** поля `Entry` (`title`/`username`/`password`/`url`/`notes`) — обычный `std::string`, не `crypto::SecureBuffer`. `SecureBuffer` зарезервирован для ключа хранилища (единственный секрет, расшифровывающий всё сразу). Полностью SecureBuffer-based модель записи была признана слишком рискованной без компилятора под рукой в сессии, где это писалось — см. комментарий в `entry.h`.

## Структура репозитория

```
lusakey/
├── libs/core/     # крипто, формат хранилища, TOTP, VaultService — БЕЗ Qt
│   ├── include/lusakey/core/{crypto,totp,vault,util}/*.h   # публичный API
│   └── src/{crypto,totp,vault,util}/*.cpp                   # реализация + внутренние заголовки (sha1.h, hmac_sha1.h, base32.h — не публичные)
├── libs/qr/       # обёртка над zxing-cpp (импорт TOTP QR из файла изображения); собирается только вместе с app
├── libs/ipc/      # фрейминг native-messaging (Chrome/Firefox), без Qt, собирается всегда
├── app/           # Qt Quick GUI-приложение (LUSAKEY_BUILD_APP=ON)
│   ├── bridge/    # AppController, VaultListModel, ActivityEventFilter — C++/QML мост
│   └── ui/        # Theme.qml + components/ + screens/ + Main.qml
├── nmhost/        # заглушка native-messaging host (LUSAKEY_BUILD_NMHOST=ON, выкл. по умолчанию)
├── tests/core/    # Catch2-тесты для libs/core (без Qt)
├── tests/ipc/     # Catch2-тесты для libs/ipc
├── resources/fonts/   # Inter/JetBrains Mono — файлы шрифтов сюда нужно скачать вручную, см. README там
├── packaging/{windows,macos,linux}/   # Inno Setup / Info.plist+entitlements+notarize.sh / .desktop+AppImage
└── docs/VAULT_FORMAT.md   # точный байтовый формат файла .lusakey
```

## Статус по вехам

- [x] **M0** — крипто-ядро: `SecureBuffer`, KDF (Argon2id), AEAD (XChaCha20-Poly1305), формат файла `.lusakey`, тесты round-trip/tamper-detection.
- [x] **M1** — HOTP/TOTP (RFC 4226/6238), Base32, `otpauth://`, тесты по официальным тестовым векторам.
- [x] **M2** — `VaultService`: CRUD записей, поиск/фильтр, генератор паролей, save/export/import, onChanged.
- [x] **M3/M4** — QML UI: разблокировка/создание хранилища, список с поиском и сайдбаром, детальная карточка с TOTP-блоком, отдельный экран кодов, генератор паролей, настройки.
- [x] **M5** — авто-блокировка (таймер простоя + app-wide event filter + lock на suspend/hidden), автоочистка буфера обмена, `prctl(PR_SET_DUMPABLE)` на Linux, смена мастер-пароля (бэкенд есть, диалог в UI — нет, см. пробелы).
- [x] **M6** — экспорт/импорт реализованы в `VaultService`/`AppController`; UI не подключён к реальному выбору файла (см. пробелы).
- [x] **M7** — дизайн-система: `Theme.qml` (светлая/тёмная палитра), переиспользуемые компоненты.
- [x] **M8** — упаковка: Inno Setup (`packaging/windows`), Info.plist+entitlements+notarize.sh (`packaging/macos`), .desktop+CPack DEB+AppImage-инструкция (`packaging/linux`).
- [x] **M9 (заготовка)** — `libs/ipc` (фрейминг) + `nmhost` (стаб с игрушечным `ping`) компилируются и линкуются с `lusakey-core` без Qt.

**Проверено реальной сборкой (Windows, MSVC 2022 + Ninja + vcpkg + Qt 6.7.3, msvc2019_64 kit):**
- Ядро (`libs/core`, `libs/ipc`, тесты) собирается чисто; `ctest` — 100% тестов проходят (крипто round-trip/tamper-detection, официальные векторы RFC 4226/6238, `VaultService`, ipc-фрейминг).
- GUI (`app`, `-DLUSAKEY_BUILD_APP=ON`) собирается чисто, включая `libs/qr`/zxing-cpp (см. апдейт по риску №1 ниже — опасения не подтвердились, собралось без правок).
- Дымовой запуск `lusakey.exe`: приложение стартует без краша, цепочка QML → `AppController` → `VaultService` → сигнал `errorOccurred` → `console.warn` отработала целиком (проверено на кейсе пустого пароля). **Не проверено визуально/интерактивно** — реальный внешний вид (цвета, шрифты, раскладка, анимации, диалоги) никто не смотрел глазами; смок-тест был безголовым.
- При первой настройке потребовалось поправить только сам `vcpkg.json` (не C++ код): нужен `builtin-baseline`, и реальное имя vcpkg-порта — `nu-book-zxing-cpp`, а не `zxing-cpp`. Плюс один C++ фикс — `main.cpp` не инклюдил `vault_list_model.h` (был только forward declare через `app_controller.h`), из-за чего `setContextProperty` резолвился в неверный (удалённый) `QVariant`-оверлоад вместо `QObject*`.
- **Не проверено:** сборка на macOS/Linux (все presets кроме `windows-x64`), упаковка (`packaging/*`, CPack DEB, AppImage), `nmhost` (`LUSAKEY_BUILD_NMHOST=ON` ни разу не собирался), реальное распознавание QR-кода (компилируется, но не тестировалось на настоящем изображении).

Для установки тулчейна на Windows см. `scripts/windows/setup-toolchain.ps1`.

## Сборка и тесты

Требования: CMake ≥3.25, компилятор с C++20, [vcpkg](https://github.com/microsoft/vcpkg) (`VCPKG_ROOT` в переменных окружения), Ninja, Qt 6.5+ (для сборки GUI).

```sh
# Только ядро (без Qt) — крипто/TOTP/VaultService/ipc и их тесты:
cmake --preset windows-x64   # или linux-x64 / macos-arm64 / macos-x64
cmake --build --preset windows-x64
ctest --preset windows-x64

# Полностью, включая GUI (нужен установленный Qt6):
cmake --preset windows-x64 -DLUSAKEY_BUILD_APP=ON -DCMAKE_PREFIX_PATH="C:\Qt\6.7.3\msvc2019_64"
cmake --build --preset windows-x64
```

**Windows-специфичная ловушка:** нужен активный окружение MSVC (`vcvars64.bat`) для компилятора/Ninja, но `vcvars64.bat` сам выставляет свой `VCPKG_ROOT` (на vcpkg, встроенный в VS Build Tools) — переустанавливайте `$env:VCPKG_ROOT` на свой vcpkg **после** импорта vcvars, иначе получите «this vcpkg instance requires a manifest with a specified baseline» от чужого, не настроенного под этот проект vcpkg.

## Формат хранилища

Полная спецификация байт-в-байт — в `docs/VAULT_FORMAT.md`. Кратко: один файл `.lusakey`, открытый заголовок (magic/версия/параметры Argon2id/соль/nonce) целиком передаётся как AAD, тело — один непрозрачный зашифрованный блоб, плюс BLAKE2b-чек-сумма для дешёвой детекции порчи файла до попытки расшифровки. KDF выполняется один раз при разблокировке/создании/смене пароля (`vault::writeNew`); рутинное сохранение (`vault::writeWithKey`, из `VaultService::save()`) переиспользует уже выведенный ключ и лишь генерирует новый nonce — иначе Argon2id на каждое сохранение убил бы отзывчивость UI.

## Дизайн-система (стиль Claude)

Реализовано в `app/ui/Theme.qml` (singleton). Светлая тема: фон `#F5F2E9`, поверхность `#FDFBF6`, акцент `#DA7756`. Тёмная: фон `#1F1B16`, акцент `#E08663`. Полный список токенов — читайте сам `Theme.qml`, это единственный источник правды (не дублировать значения здесь и там разойдутся).

Шрифты — **Inter** (UI) и **JetBrains Mono** (пароли/TOTP-коды, обязателен для разборчивости) как открытые аналоги проприетарных Styrene/Tiempos. Файлы шрифтов не входят в репозиторий — см. `resources/fonts/README.md`.

## Известные пробелы и риски (проверить в первую очередь при первой сборке)

1. ~~`libs/qr/src/qr_decoder.cpp` — риск несовпадения API zxing-cpp~~ — **подтверждено сборкой**: с портом `nu-book-zxing-cpp@2.3.0` (vcpkg) собирается как есть, `ReaderOptions`/`setFormats`/`ImageView`/`ReadBarcode` — все актуальны. Не проверено: реальное распознавание QR на настоящем изображении (только компиляция).
2. **Шрифты отсутствуют физически** — нужно скачать Inter и JetBrains Mono (OFL) и положить в `resources/fonts/`, см. README там. Без них приложение просто использует системный шрифт (не падает).
3. **QML file-dialogs не подключены**: экспорт/импорт хранилища и импорт TOTP QR из файла — в `SettingsScreen`/`Main.qml` только `TODO`/`console.warn`-заглушки. `AppController::exportVault/importVault` и `VaultService`/`libs/qr` уже готовы принять путь — не хватает только `QtQuick.Dialogs.FileDialog` в QML.
4. **Экран «Authenticator Codes» (`TotpView.qml`)** не подключён к реальной модели — нужен `TotpListModel` (аналог `VaultListModel`, но пересчитывающий коды по таймеру), которого пока нет.
5. **Редактирование существующей записи** открывает панель только с `entryId`, без предзаполнения полей — на `AppController` нет ещё `Q_INVOKABLE`, возвращающего полную `Entry` в QML-совместимом виде (`VaultService::getEntry` в C++-ядре уже есть).
6. **Диалог смены мастер-пароля** не построен в UI (`VaultService::changeMasterPassword`/`AppController::changeMasterPassword` уже есть, вызов — просто `console.warn`).
7. **Авто-блокировка** реагирует только на активность внутри окна приложения (через `QObject`-фильтр событий) и на `applicationStateChanged` (suspend/hidden) — настоящая общесистемная проверка простоя (`GetLastInputInfo`/`CGEventSourceSecondsSinceLastEventType`/`XScreenSaverQueryInfo`) не реализована (слишком платформо-специфично, чтобы писать вслепую).
8. **Иконки приложения** (`.ico`/`.icns`/`.svg` для трёх ОС) не созданы — упаковочные скрипты (`packaging/`) ссылаются на них по имени, но файлов нет.

## Конвенции для агентов, работающих в этом репозитории

- Не добавлять зависимость от Qt в `libs/core` ни при каких обстоятельствах.
- Публичный API `libs/core` — обычный C++ (`std::` типы), никаких Qt-типов в сигнатурах.
- Внутренние детали реализации (SHA-1, HMAC-SHA1, Base32, zxing-cpp) — не выносить в публичные заголовки `include/lusakey/core/...`.
- Новый код в `libs/core` — покрывать тестами Catch2 в `tests/core`; для TOTP/крипто сверяться с официальными тестовыми векторами, а не только ручным round-trip.
- Секреты (мастер-пароль, производный ключ) — только через `crypto::SecureBuffer`; поля `Entry` — сознательное исключение, см. «Отложенный компромисс» выше.
- В QML — всегда через `Theme.*` (цвета/шрифты/отступы/радиусы), никогда не хардкодить hex-цвет или пиксельный размер напрямую в экране/компоненте.
- При закрытии любого пункта из «Известные пробелы» — вычеркните его из списка и обновите статус вехи, если это её разблокирует.
