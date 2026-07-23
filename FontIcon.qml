import QtQuick

Canvas {
    id: root
    width: 18
    height: 18

    property color strokeColor: Theme.textPrimary

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        ctx.lineWidth = 2
        ctx.strokeStyle = strokeColor
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        // Draw a capital "T" letter
        // Horizontal bar
        ctx.beginPath()
        ctx.moveTo(4, 5)
        ctx.lineTo(14, 5)
        ctx.stroke()

        // Vertical bar
        ctx.beginPath()
        ctx.moveTo(9, 5)
        ctx.lineTo(9, 14)
        ctx.stroke()
    }
}
