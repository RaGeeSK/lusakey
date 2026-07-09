# Сборка AppImage (Linux, основной артефакт v1)

Инструмент: [`linuxdeploy`](https://github.com/linuxdeploy/linuxdeploy) + плагин `linuxdeploy-plugin-qt`.

```sh
cmake --preset linux-x64 -DLUSAKEY_BUILD_APP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build --preset linux-x64
cmake --install build/linux-x64 --prefix AppDir/usr

# linuxdeploy + Qt-плагин подтягивают все .so/QML-модули автоматически
linuxdeploy --appdir AppDir \
  --executable AppDir/usr/bin/lusakey \
  --desktop-file packaging/linux/lusakey.desktop \
  --plugin qt \
  --output appimage
```

Результат — `lusakey-x86_64.AppImage` (однофайловый, не требует установки/root). Значок приложения (`lusakey.svg`/`.png`) пока не создан — `linuxdeploy` потребует его явно указать (`--icon-file`) либо появится предупреждение.

`.deb`-пакет собирается через CPack (уже настроено в `CMakeLists.txt`, срабатывает автоматически при `LUSAKEY_BUILD_APP=ON` на Linux):

```sh
cmake --build build/linux-x64 --target package
```

Flatpak — сознательно отложен (fast-follow, не блокирует v1): нужно продумать доступ к произвольному пути файла хранилища через `xdg-desktop-portal` file chooser в сэндбоксе, это отдельная задача.
