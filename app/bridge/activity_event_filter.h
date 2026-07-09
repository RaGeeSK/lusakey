#pragma once

#include <QObject>

class AppController;

// Application-wide event filter (installed on QGuiApplication in main.cpp)
// that resets the auto-lock idle timer on any mouse/keyboard activity,
// anywhere in the app — not just on explicit CRUD actions. This only covers
// "the app has focus and the OS is delivering it input events"; true
// OS-wide idle detection (GetLastInputInfo / CGEventSourceSecondsSinceLastEventType /
// XScreenSaverQueryInfo) is a documented follow-up, see AGENTS.md — those
// APIs are platform-specific enough that getting them right without a
// compiler to test against each OS was judged too risky for this pass.
class ActivityEventFilter : public QObject {
    Q_OBJECT

public:
    explicit ActivityEventFilter(AppController* controller, QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    AppController* controller_;
};
