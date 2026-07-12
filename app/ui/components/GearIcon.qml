import QtQuick

// Minimalist gear glyph — sidebar icon for "Настройки" (settings).
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
        const cx = w * 0.5;
        const cy = h * 0.5;
        const outerRadius = w * 0.38;
        const innerRadius = w * 0.28;
        const toothCount = 8;

        ctx.beginPath();
        for (let i = 0; i < toothCount * 2; i++) {
            const angle = (Math.PI * 2 * i) / (toothCount * 2);
            const r = i % 2 === 0 ? outerRadius : innerRadius;
            const x = cx + r * Math.cos(angle);
            const y = cy + r * Math.sin(angle);
            if (i === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
        }
        ctx.closePath();
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(cx, cy, w * 0.14, 0, Math.PI * 2);
        ctx.stroke();
    }
}
