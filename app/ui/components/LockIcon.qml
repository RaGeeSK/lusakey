import QtQuick

// Minimalist padlock glyph — replaces the 🔒 emoji on the unlock screen.
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
        ctx.fillStyle = strokeColor;
        ctx.lineWidth = Math.max(1.4, width * 0.08);
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        const w = width;
        const h = height;

        // Shackle.
        ctx.beginPath();
        ctx.arc(w * 0.5, h * 0.4, w * 0.22, Math.PI, 0, false);
        ctx.stroke();

        // Body.
        ctx.beginPath();
        ctx.rect(w * 0.2, h * 0.4, w * 0.6, h * 0.42);
        ctx.stroke();

        // Keyhole.
        ctx.beginPath();
        ctx.arc(w * 0.5, h * 0.56, w * 0.05, 0, Math.PI * 2);
        ctx.fill();
        ctx.beginPath();
        ctx.moveTo(w * 0.5, h * 0.58);
        ctx.lineTo(w * 0.5, h * 0.7);
        ctx.lineWidth = w * 0.07;
        ctx.stroke();
    }
}
