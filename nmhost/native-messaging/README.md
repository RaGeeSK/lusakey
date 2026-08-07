# Регистрация native-messaging host (Windows)

Реального браузерного расширения ещё нет (см. корневой `AGENTS.md`) — этот
каталог только заводит шов, которым оно позже воспользуется.

## Установка

```powershell
cmake --preset windows-x64 -DLUSAKEY_BUILD_NMHOST=ON
cmake --build --preset windows-x64
.\register-windows.ps1
```

Скрипт идемпотентен: рендерит `com.lusakey.nmhost.json` с реальным путём к
`lusakey-nmhost.exe`, кладёт его в `%LOCALAPPDATA%\lusakey\native-messaging\`
и прописывает его в реестре для Chrome
(`HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.lusakey.nmhost`) и
Firefox (`HKCU:\Software\Mozilla\NativeMessagingHosts\com.lusakey.nmhost`).

`allowed_origins` в манифесте — плейсхолдер: реального id расширения ещё
нет. Когда оно появится — `.\register-windows.ps1 -ExtensionId "<реальный id>"`.

## Проверка протокола без браузера

`lusakey-nmhost.exe` читает/пишет обычный stdin/stdout — расширение не
нужно, чтобы убедиться, что протокол работает. Формат сообщения — 4 байта
little-endian длины + UTF-8 JSON (см. `lusakey/core/ipc/native_message_channel.h`).

Пример на PowerShell — собрать один framed-запрос и прогнать через exe:

```powershell
function Build-FramedMessage([string]$json) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
    $lenBytes = [BitConverter]::GetBytes([uint32]$bytes.Length)
    return [byte[]]($lenBytes + $bytes)
}

$requests = @(
    '{"action":"ping"}',
    '{"action":"unlock","password":"ваш мастер-пароль"}',
    '{"action":"listEntries"}'
)

$allBytes = New-Object System.Collections.Generic.List[byte]
foreach ($r in $requests) { $allBytes.AddRange([byte[]](Build-FramedMessage $r)) }
[System.IO.File]::WriteAllBytes("requests.bin", $allBytes.ToArray())

cmd /c '"..\..\build\windows-x64\nmhost\lusakey-nmhost.exe" < requests.bin > responses.bin'

$bytes = [System.IO.File]::ReadAllBytes("responses.bin")
$pos = 0
while ($pos -lt $bytes.Length) {
    $len = [BitConverter]::ToUInt32($bytes, $pos); $pos += 4
    [System.Text.Encoding]::UTF8.GetString($bytes, $pos, $len)
    $pos += $len
}
```

Обязательно закройте `lusakey.exe`, если хотите указать nmhost на **тот же**
файл хранилища, что открыт в GUI — два процесса, одновременно пишущих в один
файл, могут повести себя непредсказуемо. Для полностью изолированного теста
(без риска для реального хранилища) — задайте `LUSAKEY_TEST_VAULT_DIR`
одинаково и GUI, и `lusakey-nmhost.exe`: тогда оба процесса откроют один
и тот же одноразовый тестовый vault (см. `AGENTS.md`, «Визуальная проверка
UI без риска для реального хранилища»).

Действия и их параметры/ответы — см. doc-комментарий в
`nmhost/include/lusakey/nmhost/request_dispatcher.h`.
