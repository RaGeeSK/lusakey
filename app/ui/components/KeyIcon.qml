import QtQuick

// Minimalist key glyph — sidebar icon for "Все записи" (all entries).
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

        // Head (ring).
        ctx.beginPath();
        ctx.arc(w * 0.32, h * 0.32, w * 0.18, 0, Math.PI * 2);
        ctx.stroke();

        // Shaft.
        ctx.beginPath();
        ctx.moveTo(w * 0.44, h * 0.44);
        ctx.lineTo(w * 0.8, h * 0.8);
        ctx.stroke();

        // Teeth.
        ctx.beginPath();
        ctx.moveTo(w * 0.6, h * 0.6);
        ctx.lineTo(w * 0.7, h * 0.5);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(w * 0.7, h * 0.7);
        ctx.lineTo(w * 0.8, h * 0.6);
        ctx.stroke();
    }
}
