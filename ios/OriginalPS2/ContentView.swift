import SwiftUI
import UniformTypeIdentifiers

struct ContentView: View {
    @State private var showingBIOSPicker = false
    @State private var showingGamePicker = false
    @State private var biosName: String?
    @State private var gameName: String?
    @State private var statusMessage = "Engine not connected"

    private let gameTypes: [UTType] = ["iso", "bin", "img", "chd", "rom"].compactMap { UTType(filenameExtension: $0) }

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 20) {
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Original PS2").font(.largeTitle.bold())
                        Text("A native iPhone front end for our own C++ emulator engine.").foregroundStyle(.secondary)
                    }
                    HStack(spacing: 14) {
                        Image(systemName: "gamecontroller.fill").font(.system(size: 30)).foregroundStyle(.cyan)
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Emulation core").font(.headline)
                            Text(statusMessage).font(.subheadline).foregroundStyle(.secondary)
                        }
                        Spacer(minLength: 0)
                    }
                    .padding().frame(maxWidth: .infinity, alignment: .leading)
                    .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))
                    VStack(alignment: .leading, spacing: 12) {
                        Label("System BIOS", systemImage: "cpu").font(.headline)
                        Text(biosName ?? "No BIOS imported").foregroundStyle(.secondary).lineLimit(2)
                        Button { showingBIOSPicker = true } label: {
                            Label("Import BIOS from Files", systemImage: "folder").frame(maxWidth: .infinity)
                        }.buttonStyle(.bordered)
                    }.padding().background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))
                    VStack(alignment: .leading, spacing: 12) {
                        Label("Game image", systemImage: "opticaldisc").font(.headline)
                        Text(gameName ?? "No game image imported").foregroundStyle(.secondary).lineLimit(2)
                        Button { showingGamePicker = true } label: {
                            Label("Import ISO / BIN / IMG / CHD / ROM", systemImage: "folder").frame(maxWidth: .infinity)
                        }.buttonStyle(.borderedProminent)
                    }.padding().background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))
                    VStack(alignment: .leading, spacing: 8) {
                        Label("Development status", systemImage: "hammer").font(.headline)
                        Text("Imported files are copied into this app's private Documents folder. The emulator core is not connected yet, so importing files does not start a game.")
                            .font(.footnote).foregroundStyle(.secondary)
                    }.padding(.horizontal, 4)
                }.padding()
            }
            .navigationBarTitleDisplayMode(.inline)
            .fileImporter(isPresented: $showingBIOSPicker, allowedContentTypes: [.data], allowsMultipleSelection: false) { handleImport($0, isBIOS: true) }
            .fileImporter(isPresented: $showingGamePicker, allowedContentTypes: gameTypes.isEmpty ? [.data] : gameTypes, allowsMultipleSelection: false) { handleImport($0, isBIOS: false) }
        }.preferredColorScheme(.dark)
    }

    private func handleImport(_ result: Result<[URL], Error>, isBIOS: Bool) {
        switch result {
        case .success(let urls):
            guard let sourceURL = urls.first else { return }
            let didAccess = sourceURL.startAccessingSecurityScopedResource()
            defer { if didAccess { sourceURL.stopAccessingSecurityScopedResource() } }
            do {
                let destinationURL = try copyIntoAppDocuments(sourceURL)
                if isBIOS {
                    biosName = destinationURL.lastPathComponent
                    statusMessage = "BIOS imported — core integration pending"
                } else {
                    gameName = destinationURL.lastPathComponent
                    statusMessage = "Game image imported — boot support pending"
                }
            } catch {
                statusMessage = "Could not import file: \(error.localizedDescription)"
            }
        case .failure(let error):
            statusMessage = "File import failed: \(error.localizedDescription)"
        }
    }

    private func copyIntoAppDocuments(_ sourceURL: URL) throws -> URL {
        let fileManager = FileManager.default
        let documentsURL = try fileManager.url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: true)
        let importDirectory = documentsURL.appendingPathComponent("ImportedFiles", isDirectory: true)
        try fileManager.createDirectory(at: importDirectory, withIntermediateDirectories: true)
        let uniqueName = UUID().uuidString + "-" + sourceURL.lastPathComponent
        let destinationURL = importDirectory.appendingPathComponent(uniqueName)
        try fileManager.copyItem(at: sourceURL, to: destinationURL)
        return destinationURL
    }
}
