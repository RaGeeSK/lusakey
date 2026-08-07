import QtQuick

// Minimalist downward chevron — dropdown indicator for AppComboBox, instead
// of a bare unicode arrow character.
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
        ctx.lineWidth = Math.max(1.4, width * 0.12);
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        const w = width;
        const h = height;

        ctx.beginPath();
        ctx.moveTo(w * 0.2, h * 0.35);
        ctx.lineTo(w * 0.5, h * 0.65);
        ctx.lineTo(w * 0.8, h * 0.35);
        ctx.stroke();
    }
}
