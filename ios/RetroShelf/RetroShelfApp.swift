import SwiftUI
import UniformTypeIdentifiers
import GameController

/// Native front-end scaffold. The emulator core must be linked before Start can boot a game.
@main
struct RetroShelfApp: App {
    var body: some Scene {
        WindowGroup { EmulatorHomeView() }
    }
}

struct EmulatorHomeView: View {
    @State private var selectedGame: URL?
    @State private var selectedBIOS: URL?
    @State private var showingGamePicker = false
    @State private var showingBIOSPicker = false
    @State private var message = "Import a game image and your legally obtained PS2 BIOS."
    @State private var controllerName = GCController.controllers().first?.vendorName ?? "No controller detected"

    private let gameTypes: [UTType] = [
        UTType(filenameExtension: "iso") ?? .data,
        UTType(filenameExtension: "bin") ?? .data,
        UTType(filenameExtension: "img") ?? .data,
        UTType(filenameExtension: "chd") ?? .data
    ]

    var body: some View {
        VStack(spacing: 18) {
            Image(systemName: "gamecontroller.fill")
                .font(.system(size: 44))
                .foregroundStyle(.tint)
            Text("RetroShelf").font(.largeTitle.bold())
            Text("PlayStation 2 • iPhone").font(.headline).foregroundStyle(.secondary)

            GroupBox("Game image") {
                VStack(alignment: .leading, spacing: 10) {
                    Text(selectedGame?.lastPathComponent ?? "No game selected")
                        .lineLimit(2)
                    Button("Choose ISO / BIN / IMG / CHD") { showingGamePicker = true }
                        .buttonStyle(.borderedProminent)
                }.frame(maxWidth: .infinity, alignment: .leading)
            }

            GroupBox("BIOS") {
                VStack(alignment: .leading, spacing: 10) {
                    Text(selectedBIOS?.lastPathComponent ?? "No BIOS selected")
                        .lineLimit(2)
                    Button("Choose BIOS in Files") { showingBIOSPicker = true }
                        .buttonStyle(.bordered)
                }.frame(maxWidth: .infinity, alignment: .leading)
            }

            GroupBox("Controller") {
                HStack {
                    Image(systemName: "gamecontroller")
                    Text(controllerName)
                    Spacer()
                    Button("Refresh") { refreshController() }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }

            // These controls prove the native touch layout; they do not yet send input to an EE core.
            HStack(spacing: 22) {
                touchButton("◀", label: "Left")
                VStack(spacing: 12) {
                    touchButton("▲", label: "Up")
                    HStack(spacing: 12) {
                        touchButton("▼", label: "Down")
                        touchButton("▶", label: "Right")
                    }
                }
                Spacer()
                VStack(spacing: 12) {
                    touchButton("△", label: "Triangle")
                    HStack(spacing: 12) {
                        touchButton("□", label: "Square")
                        touchButton("○", label: "Circle")
                        touchButton("×", label: "Cross")
                    }
                }
            }
            .padding(.vertical, 8)

            Button("Start emulation") {
                message = "The iOS shell is not connected to a complete PS2 core yet. No game has been booted."
            }
            .buttonStyle(.borderedProminent)
            .disabled(selectedGame == nil || selectedBIOS == nil)

            Text(message)
                .font(.footnote)
                .foregroundStyle(.secondary)
                .frame(maxWidth: .infinity, alignment: .leading)
            Spacer(minLength: 0)
        }
        .padding()
        .fileImporter(isPresented: $showingGamePicker, allowedContentTypes: gameTypes, allowsMultipleSelection: false) { result in
            handleImport(result, kind: .game)
        }
        .fileImporter(isPresented: $showingBIOSPicker, allowedContentTypes: [.data], allowsMultipleSelection: false) { result in
            handleImport(result, kind: .bios)
        }
        .onAppear { refreshController() }
    }

    private enum ImportKind { case game, bios }

    private func handleImport(_ result: Result<[URL], Error>, kind: ImportKind) {
        switch result {
        case .success(let urls):
            guard let url = urls.first else { return }
            let hasAccess = url.startAccessingSecurityScopedResource()
            defer { if hasAccess { url.stopAccessingSecurityScopedResource() } }

            do {
                // Copy into app-owned storage so the imported file remains available after picker access ends.
                let folder = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
                    .appendingPathComponent("Imported", isDirectory: true)
                try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
                let destination = folder.appendingPathComponent(url.lastPathComponent)
                if FileManager.default.fileExists(atPath: destination.path) {
                    try FileManager.default.removeItem(at: destination)
                }
                try FileManager.default.copyItem(at: url, to: destination)
                if kind == .game {
                    selectedGame = destination
                    message = "Game image imported. Booting is unavailable until the PS2 core and disc reader are integrated."
                } else {
                    selectedBIOS = destination
                    message = "BIOS imported. BIOS execution and hardware initialization are not implemented yet."
                }
            } catch {
                message = "Import failed: \(error.localizedDescription)"
            }
        case .failure(let error):
            message = "File selection failed: \(error.localizedDescription)"
        }
    }

    private func refreshController() {
        GCController.startWirelessControllerDiscovery {}
        controllerName = GCController.controllers().first?.vendorName ?? "No controller detected"
    }

    private func touchButton(_ symbol: String, label: String) -> some View {
        Button {} label: {
            Text(symbol).font(.title2.bold()).frame(width: 44, height: 44)
        }
        .buttonStyle(.bordered)
        .accessibilityLabel(label)
        .disabled(true) // Intentionally disabled until an emulator input bridge exists.
    }
}
