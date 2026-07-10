import QtQuick

// Minimalist warning-triangle glyph — replaces the ⚠️ emoji on the
// destructive "delete everything" confirmation step.
Canvas {
    id: canvas

    property color strokeColor: "black"

    implicitWidth: 22
    implicitHeight: 22

    onStrokeColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        ctx.strokeStyle = strokeColor;
        ctx.fillStyle = strokeColor;
        ctx.lineWidth = Math.max(1.4, width * 0.07);
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        const w = width;
        const h = height;

        ctx.beginPath();
        ctx.moveTo(w * 0.5, h * 0.08);
        ctx.lineTo(w * 0.94, h * 0.88);
        ctx.lineTo(w * 0.06, h * 0.88);
        ctx.closePath();
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(w * 0.5, h * 0.36);
        ctx.lineTo(w * 0.5, h * 0.62);
        ctx.lineWidth = w * 0.08;
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(w * 0.5, h * 0.74, w * 0.045, 0, Math.PI * 2);
        ctx.fill();
    }
}
