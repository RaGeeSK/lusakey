import QtQuick

// Minimalist checkmark glyph — replaces the "✓" character used inside
// AppCheckBox's indicator.
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
        ctx.lineWidth = Math.max(1.4, width * 0.16);
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        const w = width;
        const h = height;

        ctx.beginPath();
        ctx.moveTo(w * 0.18, h * 0.52);
        ctx.lineTo(w * 0.42, h * 0.74);
        ctx.lineTo(w * 0.84, h * 0.26);
        ctx.stroke();
    }
}
