import SwiftUI
import UniformTypeIdentifiers

struct ContentView: View {
    @State private var showingBIOSPicker = false
    @State private var showingGamePicker = false
    @State private var biosName: String?
    @State private var gameName: String?
    @State private var biosURL: URL?
    @State private var gameURL: URL?
    @State private var statusMessage = "Engine not connected"

    private let supportedGameExtensions: Set<String> = ["iso", "bin", "img", "chd", "rom"]

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 20) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Original PS2").font(.largeTitle.bold())
                        Text("A native iPhone front end for our own C++ emulator engine.")
                            .foregroundStyle(.secondary)
                    }

                    HStack(spacing: 14) {
                        Image(systemName: "gamecontroller.fill")
                            .font(.system(size: 30))
                            .foregroundStyle(.cyan)
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Emulation core").font(.headline)
                            Text(statusMessage)
                                .font(.subheadline)
                                .foregroundStyle(.secondary)
                        }
                        Spacer(minLength: 0)
                    }
                    .padding()
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))

                    VStack(alignment: .leading, spacing: 12) {
                        Label("System BIOS", systemImage: "cpu").font(.headline)
                        Text(biosName ?? "No BIOS imported")
                            .foregroundStyle(.secondary)
                            .lineLimit(2)
                        Button {
                            showingBIOSPicker = true
                        } label: {
                            Label("Import BIOS from Files", systemImage: "folder")
                                .frame(maxWidth: .infinity)
                        }
                        .buttonStyle(.bordered)
                    }
                    .padding()
                    .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))

                    VStack(alignment: .leading, spacing: 12) {
                        Label("Game image", systemImage: "opticaldisc").font(.headline)
                        Text(gameName ?? "No game image imported")
                            .foregroundStyle(.secondary)
                            .lineLimit(2)
                        Button {
                            showingGamePicker = true
                        } label: {
                            Label("Import game from Files", systemImage: "folder")
                                .frame(maxWidth: .infinity)
                        }
                        .buttonStyle(.borderedProminent)
                    }
                    .padding()
                    .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))

                    VStack(alignment: .leading, spacing: 8) {
                        Label("Development status", systemImage: "hammer").font(.headline)
                        Text("Files are copied into this app's private storage. Importing does not start a game; the original C++ engine still needs to be connected.")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                    .padding(.horizontal, 4)
                }
                .padding()
            }
            .navigationBarTitleDisplayMode(.inline)
            .fileImporter(
                isPresented: $showingBIOSPicker,
                allowedContentTypes: [.data],
                allowsMultipleSelection: false
            ) { handleImport($0, isBIOS: true) }
            .fileImporter(
                isPresented: $showingGamePicker,
                allowedContentTypes: [.data],
                allowsMultipleSelection: false
            ) { handleImport($0, isBIOS: false) }
        }
        .preferredColorScheme(.dark)
    }

    private func handleImport(_ result: Result<[URL], Error>, isBIOS: Bool) {
        switch result {
        case .success(let urls):
            guard let sourceURL = urls.first else {
                statusMessage = "No file was selected"
                return
            }

            if !isBIOS && !supportedGameExtensions.contains(sourceURL.pathExtension.lowercased()) {
                statusMessage = "Unsupported game file. Choose ISO, BIN, IMG, CHD, or ROM."
                return
            }

            do {
                let importedURL = try copyIntoAppStorage(sourceURL)
                if isBIOS {
                    biosURL = importedURL
                    biosName = importedURL.lastPathComponent
                    statusMessage = "BIOS imported — core integration pending"
                } else {
                    gameURL = importedURL
                    gameName = importedURL.lastPathComponent
                    statusMessage = "Game image imported — boot support pending"
                }
            } catch {
                statusMessage = "Import failed: \(error.localizedDescription)"
            }

        case .failure(let error):
            statusMessage = "File import failed: \(error.localizedDescription)"
        }
    }

    private func copyIntoAppStorage(_ sourceURL: URL) throws -> URL {
        let fileManager = FileManager.default
        guard let documentsURL = fileManager.urls(
            for: .documentDirectory,
            in: .userDomainMask
        ).first else {
            throw ImportError.documentsDirectoryUnavailable
        }

        let importDirectory = documentsURL.appendingPathComponent("ImportedFiles", isDirectory: true)
        try fileManager.createDirectory(
            at: importDirectory,
            withIntermediateDirectories: true
        )

        let hasAccess = sourceURL.startAccessingSecurityScopedResource()
        defer {
            if hasAccess {
                sourceURL.stopAccessingSecurityScopedResource()
            }
        }

        let safeName = sourceURL.lastPathComponent.isEmpty ? "imported-file" : sourceURL.lastPathComponent
        let destinationURL = importDirectory.appendingPathComponent(
            "\(UUID().uuidString)-\(safeName)",
            isDirectory: false
        )
        try fileManager.copyItem(at: sourceURL, to: destinationURL)
        return destinationURL
    }

    private enum ImportError: LocalizedError {
        case documentsDirectoryUnavailable

        var errorDescription: String? {
            switch self {
            case .documentsDirectoryUnavailable:
                return "The app's Documents folder is unavailable."
            }
        }
    }
}
