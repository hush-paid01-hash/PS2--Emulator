import SwiftUI
import UniformTypeIdentifiers
import UIKit
import CoreGraphics

@main
struct RetroShelfApp: App {
    var body: some Scene {
        WindowGroup { ConsoleHomeView() }
    }
}

struct ConsoleHomeView: View {
    @State private var selectedDisc: URL?
    @State private var selectedBIOS: URL?
    @State private var choosingDisc = false
    @State private var choosingBIOS = false
    @State private var message = "Engine foundation only — game execution is not implemented yet."
    @State private var graphicsPreview: UIImage?

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: 18) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text("RETROSHELF").font(.caption.weight(.bold)).tracking(3).foregroundStyle(.cyan)
                        Text("Your PS2.\nYour core.").font(.system(size: 36, weight: .bold, design: .rounded))
                        Text("An independent PlayStation 2 emulator project for iPhone.")
                            .foregroundStyle(.secondary)
                    }.frame(maxWidth: .infinity, alignment: .leading)
                    statusCard
                    graphicsPreviewCard
                    VStack(spacing: 12) {
                        fileButton(title: "Choose PS2 BIOS", subtitle: selectedBIOS?.lastPathComponent ?? "Select a BIOS file you legally obtained", icon: "cpu") { choosingBIOS = true }
                        fileButton(title: "Choose game image", subtitle: selectedDisc?.lastPathComponent ?? "ISO, BIN, IMG or CHD", icon: "opticaldisc") { choosingDisc = true }
                    }
                    Button {
                        let result = rs_run_ee_diagnostic(1000)
                        if let result { message = String(cString: result) }
                    } label: {
                        Label("Run EE CPU diagnostic (1,000 instructions max)", systemImage: "play.fill")
                            .frame(maxWidth: .infinity)
                            .padding(14)
                            .background(.cyan.opacity(0.16), in: RoundedRectangle(cornerRadius: 14))
                    }
                    .buttonStyle(.plain)
                    .disabled(selectedDisc == nil)

                    VStack(alignment: .leading, spacing: 12) {
                        Text("CONTROLS PREVIEW").font(.caption.weight(.bold)).tracking(2).foregroundStyle(.secondary)
                        HStack {
                            control("◀")
                            VStack(spacing: 8) { control("▲"); control("▼") }
                            control("▶")
                            Spacer()
                            VStack(spacing: 12) { HStack { control("△"); control("○") }; HStack { control("□"); control("×") } }
                        }
                        Text("Touch controls are a visual prototype; they do not control a running game yet.")
                            .font(.footnote).foregroundStyle(.secondary)
                    }.padding(16).frame(maxWidth: .infinity, alignment: .leading).background(.white.opacity(0.05), in: RoundedRectangle(cornerRadius: 18))
                    Text(message).font(.footnote).foregroundStyle(.secondary).frame(maxWidth: .infinity, alignment: .leading)
                }.padding(20)
            }
            .background(Color(red: 0.035, green: 0.05, blue: 0.10).ignoresSafeArea())
            .foregroundStyle(.white)
            .navigationBarTitleDisplayMode(.inline)
            .fileImporter(isPresented: $choosingBIOS, allowedContentTypes: [.data], allowsMultipleSelection: false) { result in
                handle(result, assign: { selectedBIOS = $0 }, label: "BIOS")
            }
            .fileImporter(isPresented: $choosingDisc, allowedContentTypes: [.data], allowsMultipleSelection: false) { result in
                handle(result, assign: { selectedDisc = $0 }, label: "game image")
            }
        }.preferredColorScheme(.dark)
    }

    private var graphicsPreviewCard: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("GRAPHICS PIPELINE TEST").font(.caption.weight(.bold)).tracking(2).foregroundStyle(.secondary)
            Group {
                if let graphicsPreview {
                    Image(uiImage: graphicsPreview).resizable().interpolation(.none).aspectRatio(contentMode: .fit)
                } else {
                    RoundedRectangle(cornerRadius: 10).fill(.black.opacity(0.3))
                        .overlay(Text("No preview rendered yet").foregroundStyle(.secondary))
                }
            }.frame(maxWidth: .infinity).frame(height: 220).clipShape(RoundedRectangle(cornerRadius: 12))
            Button(action: renderGraphicsPreview) {
                Label("Render GIF pipeline test frame", systemImage: "play.rectangle")
                    .frame(maxWidth: .infinity).padding(12)
                    .background(.cyan.opacity(0.16), in: RoundedRectangle(cornerRadius: 12))
            }.buttonStyle(.plain)
            Text("Synthetic triangle test only — not output from a running PS2 game.")
                .font(.footnote).foregroundStyle(.orange)
        }.padding(16).frame(maxWidth: .infinity, alignment: .leading)
         .background(.white.opacity(0.05), in: RoundedRectangle(cornerRadius: 18))
    }

    private func renderGraphicsPreview() {
        let width = 320
        let height = 180
        var bytes = [UInt8](repeating: 0, count: width * height * 4)
        let success = bytes.withUnsafeMutableBufferPointer { buffer in
            rs_render_gif_pipeline_preview(buffer.baseAddress, UInt64(buffer.count), UInt32(width), UInt32(height))
        }
        guard success == 1,
              let provider = CGDataProvider(data: Data(bytes) as CFData),
              let cgImage = CGImage(width: width, height: height, bitsPerComponent: 8,
                                    bitsPerPixel: 32, bytesPerRow: width * 4,
                                    space: CGColorSpaceCreateDeviceRGB(),
                                    bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.last.rawValue),
                                    provider: provider, decode: nil, shouldInterpolate: false,
                                    intent: .defaultIntent) else {
            message = "GIF pipeline preview failed. Check the engine build."
            return
        }
        graphicsPreview = UIImage(cgImage: cgImage)
        message = "GIF test stream rendered through the software rasterizer; not game output."
    }

    private var statusCard: some View {
        VStack(alignment: .leading, spacing: 10) {
            Label("ORIGINAL CORE", systemImage: "cpu").font(.headline).foregroundStyle(.cyan)
            Text("CPU, memory bus, timer, interrupt and DMA prototypes")
            Text("Not yet compatible with commercial PS2 games").font(.footnote).foregroundStyle(.orange)
        }.frame(maxWidth: .infinity, alignment: .leading).padding(16).background(.white.opacity(0.07), in: RoundedRectangle(cornerRadius: 18))
    }

    private func fileButton(title: String, subtitle: String, icon: String, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            HStack(spacing: 14) {
                Image(systemName: icon).font(.title2).foregroundStyle(.cyan).frame(width: 30)
                VStack(alignment: .leading, spacing: 4) {
                    Text(title).font(.headline).foregroundStyle(.white)
                    Text(subtitle).font(.caption).foregroundStyle(.secondary).lineLimit(2)
                }
                Spacer()
                Image(systemName: "folder").foregroundStyle(.cyan)
            }.padding(16).background(.white.opacity(0.06), in: RoundedRectangle(cornerRadius: 16))
        }.buttonStyle(.plain)
    }

    private func control(_ symbol: String) -> some View {
        Text(symbol).font(.title2.weight(.bold)).frame(width: 48, height: 48).background(.white.opacity(0.10), in: Circle()).overlay(Circle().stroke(.white.opacity(0.12)))
    }

    private func handle(_ result: Result<[URL], Error>, assign: (URL) -> Void, label: String) {
        switch result {
        case .success(let urls):
            guard let sourceURL = urls.first else { return }
            let hasScopedAccess = sourceURL.startAccessingSecurityScopedResource()
            defer {
                if hasScopedAccess { sourceURL.stopAccessingSecurityScopedResource() }
            }
            do {
                let supportURL = try FileManager.default.url(
                    for: .applicationSupportDirectory,
                    in: .userDomainMask,
                    appropriateFor: nil,
                    create: true
                )
                let importDirectory = supportURL.appendingPathComponent("ImportedFiles", isDirectory: true)
                try FileManager.default.createDirectory(at: importDirectory, withIntermediateDirectories: true)
                let destinationURL = importDirectory.appendingPathComponent("\(UUID().uuidString)-\(sourceURL.lastPathComponent)")
                try FileManager.default.copyItem(at: sourceURL, to: destinationURL)
                assign(destinationURL)
                if label == "game image" {
                    let inspection = destinationURL.path.withCString { path in
                        rs_inspect_ps2_disc(path)
                    }
                    if let inspection {
                        message = String(cString: inspection)
                    } else {
                        message = "The engine returned no disc inspection result."
                    }
                } else {
                    let result = destinationURL.path.withCString { path in
                        rs_load_ps2_bios(path)
                    }
                    message = result.map { String(cString: $0) } ?? "The engine returned no BIOS load result."
                }
            } catch {
                message = "Could not import \(label): \(error.localizedDescription)"
            }
        case .failure(let error):
            message = "Could not select \(label): \(error.localizedDescription)"
        }
    }
}
