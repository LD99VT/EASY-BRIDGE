import AppKit
import Foundation

guard CommandLine.arguments.count == 3 else {
    fputs("usage: generate_bridge_macos_icon.swift <src.png> <dst.png>\n", stderr)
    exit(1)
}

let srcPath = CommandLine.arguments[1]
let dstPath = CommandLine.arguments[2]

guard let source = NSImage(contentsOfFile: srcPath) else {
    fputs("failed to load source image: \(srcPath)\n", stderr)
    exit(1)
}

let canvasSize = NSSize(width: 1024, height: 1024)
let output = NSImage(size: canvasSize)

// Finder in recent macOS versions presents some legacy-style app icons on
// a system plate, which makes Bridge look visually smaller than Trigger.
// This variant slightly overscales the source art so the visible symbol fills
// more of the plate while keeping the original design unchanged.
let drawRect = NSRect(x: -84, y: -84, width: 1192, height: 1192)

output.lockFocus()
NSGraphicsContext.current?.imageInterpolation = .high
NSColor.clear.setFill()
NSRect(origin: .zero, size: canvasSize).fill()
source.draw(in: drawRect,
            from: NSRect(origin: .zero, size: source.size),
            operation: .sourceOver,
            fraction: 1.0)
output.unlockFocus()

guard
    let tiff = output.tiffRepresentation,
    let rep = NSBitmapImageRep(data: tiff),
    let png = rep.representation(using: .png, properties: [:])
else {
    fputs("failed to encode png\n", stderr)
    exit(1)
}

try png.write(to: URL(fileURLWithPath: dstPath))
