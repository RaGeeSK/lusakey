import QtQuick

// Minimalist "eye" / "eye-slash" glyph drawn on a Canvas — no emoji, no
// bundled icon font. `crossed: true` draws the eye with a diagonal slash
// through it (conventionally: "content is currently visible, click to
// hide"); `crossed: false` draws a plain open eye ("content is hidden,
// click to reveal").
Canvas {
    id: canvas

    property bool crossed: false
    property color strokeColor: "black"

    implicitWidth: 18
    implicitHeight: 18

    onCrossedChanged: requestPaint()
    onStrokeColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        ctx.strokeStyle = strokeColor;
        ctx.lineWidth = 1.6;
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        const w = width;
        const h = height;
        const cx = w / 2;
        const cy = h / 2;

        // Almond-shaped eye outline, via two quadratic curves meeting at
        // the left/right corners.
        ctx.beginPath();
        ctx.moveTo(w * 0.06, cy);
        ctx.quadraticCurveTo(cx, h * 0.08, w * 0.94, cy);
        ctx.quadraticCurveTo(cx, h * 0.92, w * 0.06, cy);
        ctx.closePath();
        ctx.stroke();

        // Pupil.
        ctx.beginPath();
        ctx.arc(cx, cy, w * 0.14, 0, Math.PI * 2);
        ctx.stroke();

        if (crossed) {
            ctx.beginPath();
            ctx.moveTo(w * 0.1, h * 0.88);
            ctx.lineTo(w * 0.9, h * 0.12);
            ctx.stroke();
        }
    }
}
