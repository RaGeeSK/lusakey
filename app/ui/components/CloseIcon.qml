import QtQuick

// Minimalist "X" glyph — replaces the "✕" character used for remove/close
// buttons (e.g. removing a recovery-question row).
Canvas {
    id: canvas

    property color strokeColor: "black"

    implicitWidth: 14
    implicitHeight: 14

    onStrokeColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = Math.max(1.4, width * 0.14);
        ctx.lineCap = "round";

        const w = width;
        const h = height;

        ctx.beginPath();
        ctx.moveTo(w * 0.18, h * 0.18);
        ctx.lineTo(w * 0.82, h * 0.82);
        ctx.moveTo(w * 0.82, h * 0.18);
        ctx.lineTo(w * 0.18, h * 0.82);
        ctx.stroke();
    }
}
