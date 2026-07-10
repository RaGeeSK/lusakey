# lusakey GUI application: Qt Quick/QML shell (ui/) over a thin C++ bridge
# (bridge/) that adapts lusakey-core's VaultService to Qt/QML idioms.
# Requires Qt 6.5+ (for qt_add_qml_module / engine.loadFromModule).

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(lusakey
    main.cpp
    bridge/app_controller.h
    bridge/app_controller.cpp
    bridge/vault_list_model.h
    bridge/vault_list_model.cpp
    bridge/activity_event_filter.h
    bridge/activity_event_filter.cpp
)

# Marks Theme.qml as the module's singleton (accessed as `Theme` from any
# file that does `import Lusakey`), per Qt6's qt_add_qml_module convention.
set_source_files_properties(ui/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(lusakey
    URI Lusakey
    VERSION 1.0
    QML_FILES
        ui/Theme.qml
        ui/Main.qml
        ui/components/AppButton.qml
        ui/components/Card.qml
        ui/components/AppTextField.qml
        ui/components/AppTextArea.qml
        ui/components/Chip.qml
        ui/components/SidebarItem.qml
        ui/components/PasswordStrengthMeter.qml
        ui/components/TotpRing.qml
        ui/components/AppSpinBox.qml
        ui/components/AppSwitch.qml
        ui/components/AppCheckBox.qml
        ui/components/AppSlider.qml
        ui/components/EyeIcon.qml
        ui/components/LockIcon.qml
        ui/components/WarningIcon.qml
        ui/components/CloseIcon.qml
        ui/components/CheckIcon.qml
        ui/screens/UnlockScreen.qml
        ui/screens/VaultListScreen.qml
        ui/screens/EntryDetailPanel.qml
        ui/screens/TotpView.qml
        ui/screens/GeneratorDialog.qml
        ui/screens/SettingsScreen.qml
        ui/screens/RecoverySetupDialog.qml
)

target_link_libraries(lusakey PRIVATE
    lusakey-core
    lusakey-qr
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

target_compile_features(lusakey PRIVATE cxx_std_20)

# GUI app, not a console tool — without this, Windows launches lusakey.exe
# with a visible console window behind the QML window (stdout/stderr from
# Qt's logging still work, they just go nowhere visible, same as any other
# desktop app).
if(WIN32)
    set_target_properties(lusakey PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Copy any bundled font files (see resources/fonts/README.md — not committed
# to the repo, downloaded separately) next to the built executable, where
# main.cpp's loadBundledFonts() looks for them (applicationDirPath()+"/fonts").
# Tolerant of the files being absent (e.g. a fresh checkout before running
# the download step) — just copies whatever currently exists at configure time.
file(GLOB LUSAKEY_FONT_FILES "${CMAKE_SOURCE_DIR}/resources/fonts/*.ttf")
if(LUSAKEY_FONT_FILES)
    add_custom_command(TARGET lusakey POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:lusakey>/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LUSAKEY_FONT_FILES} "$<TARGET_FILE_DIR:lusakey>/fonts"
        COMMENT "Copying bundled fonts next to the lusakey executable"
        VERBATIM
    )
endif()

# --- Linux packaging metadata ---
# Windows/macOS packaging (windeployqt+Inno Setup, macdeployqt+notarize.sh)
# runs as a separate post-build step outside CMake's install() — see
# packaging/windows/ and packaging/macos/. CPack variables + include(CPack)
# itself live in the top-level CMakeLists.txt (after add_subdirectory(app)),
# so they see this target's install() rules regardless of subdirectory
# processing order.
if(UNIX AND NOT APPLE)
    install(TARGETS lusakey RUNTIME DESTINATION bin)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/linux/lusakey.desktop DESTINATION share/applications)
endif()
