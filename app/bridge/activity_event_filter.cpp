#include "activity_event_filter.h"

#include <QEvent>

#include "app_controller.h"

ActivityEventFilter::ActivityEventFilter(AppController* controller, QObject* parent)
    : QObject(parent), controller_(controller) {}

bool ActivityEventFilter::eventFilter(QObject* watched, QEvent* event) {
    switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::KeyPress:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
            if (controller_) {
                controller_->resetAutoLockTimer();
            }
            break;
        default:
            break;
    }
    return QObject::eventFilter(watched, event);
}
