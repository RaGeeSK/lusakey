import QtQuick

// Minimalist shield-check glyph — sidebar icon for "Коды авторизации"
// (authenticator/TOTP codes).
Canvas {
    id: canvas

    property color strokeColor: "black"

    implicitWidth: 20
    implicitHeight: 20

    onStrokeColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = Math.max(1.4, width * 0.08);
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        const w = width;
        const h = height;

        // Shield outline.
        ctx.beginPath();
        ctx.moveTo(w * 0.5, h * 0.1);
        ctx.lineTo(w * 0.84, h * 0.24);
        ctx.lineTo(w * 0.84, h * 0.5);
        ctx.bezierCurveTo(w * 0.84, h * 0.76, w * 0.68, h * 0.88, w * 0.5, h * 0.94);
        ctx.bezierCurveTo(w * 0.32, h * 0.88, w * 0.16, h * 0.76, w * 0.16, h * 0.5);
        ctx.lineTo(w * 0.16, h * 0.24);
        ctx.closePath();
        ctx.stroke();

        // Checkmark.
        ctx.beginPath();
        ctx.moveTo(w * 0.34, h * 0.5);
        ctx.lineTo(w * 0.45, h * 0.61);
        ctx.lineTo(w * 0.68, h * 0.37);
        ctx.stroke();
    }
}
