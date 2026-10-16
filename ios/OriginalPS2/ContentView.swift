import SwiftUI
import UniformTypeIdentifiers

struct ContentView: View {
    @State private var showingBIOSPicker = false
    @State private var showingGamePicker = false
    @State private var biosName: String?
    @State private var gameName: String?
    @State private var biosURL: URL?
    @State private var gameURL: URL?
    @State private var statusMessage = "Checking engine bridge…"

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
                        Text("The Swift wrapper now queries the engine C ABI. The iOS app target must still link the retroshelf_engine library; the engine currently cannot boot games.")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                    .padding(.horizontal, 4)
                }
                .padding()
            }
            .navigationBarTitleDisplayMode(.inline)
            .onAppear {
                statusMessage = EngineBridge.readinessMessage
                restoreImportedFiles()
            }
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
                let fileSize = try importedURL.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0
                guard fileSize > 0 else {
                    try? FileManager.default.removeItem(at: importedURL)
                    statusMessage = "Import failed: the selected file is empty."
                    return
                }
                if isBIOS {
                    // File-level sanity check only: size does not prove that this
                    // is a genuine or compatible PS2 BIOS image.
                    let plausibleBIOSSize = (1 * 1024 * 1024)...(16 * 1024 * 1024)
                    let biosSizeLooksPlausible = plausibleBIOSSize.contains(fileSize)

                    biosURL = importedURL
                    biosName = sourceURL.lastPathComponent
                    UserDefaults.standard.set(importedURL.lastPathComponent, forKey: "hush.importedBIOS")
                    UserDefaults.standard.set(sourceURL.lastPathComponent, forKey: "hush.importedBIOSDisplayName")
                    statusMessage = biosSizeLooksPlausible
                        ? "BIOS file saved and readable — compatibility not verified; boot support is not implemented yet"
                        : "BIOS file saved, but its size looks unusual — check your own BIOS dump; boot support is not implemented yet"
                } else {
                    gameURL = importedURL
                    gameName = sourceURL.lastPathComponent
                    UserDefaults.standard.set(importedURL.lastPathComponent, forKey: "hush.importedGame")
                    UserDefaults.standard.set(sourceURL.lastPathComponent, forKey: "hush.importedGameDisplayName")
                    statusMessage = "Game image saved — boot support is not implemented yet"
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

    private func restoreImportedFiles() {
        guard let documentsURL = FileManager.default.urls(
            for: .documentDirectory,
            in: .userDomainMask
        ).first else { return }
        let importDirectory = documentsURL.appendingPathComponent("ImportedFiles", isDirectory: true)

        if let storedName = UserDefaults.standard.string(forKey: "hush.importedBIOS") {
            let url = importDirectory.appendingPathComponent(storedName)
            if FileManager.default.fileExists(atPath: url.path) {
                biosURL = url
                biosName = UserDefaults.standard.string(forKey: "hush.importedBIOSDisplayName") ?? storedName
            } else {
                UserDefaults.standard.removeObject(forKey: "hush.importedBIOS")
                UserDefaults.standard.removeObject(forKey: "hush.importedBIOSDisplayName")
            }
        }

        if let storedName = UserDefaults.standard.string(forKey: "hush.importedGame") {
            let url = importDirectory.appendingPathComponent(storedName)
            if FileManager.default.fileExists(atPath: url.path) {
                gameURL = url
                gameName = UserDefaults.standard.string(forKey: "hush.importedGameDisplayName") ?? storedName
            } else {
                UserDefaults.standard.removeObject(forKey: "hush.importedGame")
                UserDefaults.standard.removeObject(forKey: "hush.importedGameDisplayName")
            }
        }
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
