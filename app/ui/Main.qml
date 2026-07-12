import QtQuick
import QtQuick.Controls.Basic
import Lusakey

// App shell: a single StackView switching between the unlock screen and the
// main vault UI. `appController` and `vaultListModel` are context properties
// set from app/main.cpp.
//
// Known gaps (see AGENTS.md): export/import aren't wired to a real file
// picker yet (no QtQuick.Dialogs FileDialog hookup), editing an *existing*
// entry only carries its id (no getEntry()-shaped call on AppController yet
// to pre-fill the form), and the Authenticator Codes view (inside
// VaultListScreen, alongside the entries list) has no real per-entry-ticking
// model wired up yet.
ApplicationWindow {
    id: window
    width: 1024
    height: 720
    visible: true
    title: qsTr("lusakey")
    color: Theme.bgCanvas

    Component.onCompleted: {
        Theme.darkMode = Qt.styleHints.colorScheme === Qt.Dark;

        // Never let the window shrink below the unlock screen's own card —
        // read once here (not bound) since the card's height changes with
        // UnlockScreen's `mode` (recovery/delete flows), and the window
        // should keep this floor even after unlocking into other screens.
        if (stack.currentItem) {
            window.minimumWidth = stack.currentItem.cardWidth + Theme.space7 * 2;
            window.minimumHeight = stack.currentItem.cardHeight + Theme.space7 * 2;
        }

        window.requestActivate();
        startupFocusTimer.start();
    }

    // Grabbing focus for the password field right in Component.onCompleted
    // doesn't reliably stick — the platform window itself often hasn't
    // finished becoming the OS-active/foreground window yet at that point,
    // so the keystrokes go nowhere until the user clicks the field once.
    // A short deferred retry (after requestActivate()) fixes it in practice.
    Timer {
        id: startupFocusTimer
        interval: 50
        onTriggered: {
            window.requestActivate();
            if (stack.currentItem && stack.currentItem.focusPasswordField) {
                stack.currentItem.focusPasswordField();
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: unlockScreenComponent
    }

    Component {
        id: unlockScreenComponent
        UnlockScreen {
            errorVisible: appController.lastUnlockFailed
            vaultExists: appController.vaultExists
            onUnlockRequested: function (password) {
                appController.unlockOrCreate(password);
            }
        }
    }

    Component {
        id: vaultListComponent
        VaultListScreen {
            onEntrySelected: function (entryId) {
                const entry = appController.getEntry(entryId);
                stack.push(entryDetailComponent, {
                    entryId: entryId,
                    entryTitle: entry.title || "",
                    username: entry.username || "",
                    password: entry.password || "",
                    url: entry.url || "",
                    notes: entry.notes || "",
                    hasTotp: entry.hasTotp || false
                });
            }
            onAddEntryRequested: {
                stack.push(entryDetailComponent, {entryId: null});
            }
            onSettingsRequested: {
                stack.push(settingsComponent);
            }
            onSearchTextChanged: function (text) {
                appController.setSearchText(text);
            }
        }
    }

    Component {
        id: entryDetailComponent
        EntryDetailPanel {
            // entryId === null means "new entry"; otherwise it's the
            // qulonglong id passed in via StackView.push()'s properties arg.
            totpSecondsRemaining: entryId !== null ? appController.currentTotpSecondsRemaining(entryId) : 30
            totpCode: entryId !== null ? appController.currentTotpCode(entryId) : ""

            onSaveRequested: function (title, username, password, url, notes) {
                if (entryId !== null) {
                    appController.updateEntry(entryId, title, username, password, url, notes);
                } else {
                    appController.addEntry(title, username, password, url, notes);
                }
                stack.pop();
            }
            onDeleteRequested: {
                if (entryId !== null) {
                    appController.removeEntry(entryId);
                }
                stack.pop();
            }
            onCloseRequested: stack.pop()
            onLinkTotpRequested: function (otpauthUri) {
                if (entryId !== null && appController.setEntryTotp(entryId, otpauthUri)) {
                    hasTotp = true;
                }
            }
            onUnlinkTotpRequested: {
                if (entryId !== null) {
                    appController.removeEntryTotp(entryId);
                    hasTotp = false;
                }
            }
            onCopyPasswordRequested: function (password) {
                appController.copyToClipboard(password);
            }
            onCopyTotpRequested: appController.copyToClipboard(totpCode)
        }
    }

    Component {
        id: settingsComponent
        SettingsScreen {
            onBackRequested: stack.pop()
            onExportRequested: {
                // TODO: replace with a QtQuick.Dialogs FileDialog for the destination path.
                console.warn("lusakey: export not wired to a file picker yet");
            }
            onImportRequested: {
                // TODO: replace with a QtQuick.Dialogs FileDialog + password prompt.
                console.warn("lusakey: import not wired to a file picker yet");
            }
            onChangeMasterPasswordRequested: {
                // TODO: a small dialog collecting old/new password, then
                // appController.changeMasterPassword(oldPw, newPw).
                console.warn("lusakey: change-master-password dialog not built yet");
            }
        }
    }

    Connections {
        target: appController
        function onUnlocked() {
            stack.replace(vaultListComponent);
        }
        function onLocked() {
            stack.replace(unlockScreenComponent);
        }
        function onErrorOccurred(message) {
            console.warn("lusakey:", message);
        }
    }
}
