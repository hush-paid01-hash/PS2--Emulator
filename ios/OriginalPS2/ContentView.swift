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
                        Text(biosName ?? "No BIOS selected").foregroundStyle(.secondary).lineLimit(2)
                        Button { showingBIOSPicker = true } label: {
                            Label("Choose BIOS in Files", systemImage: "folder").frame(maxWidth: .infinity)
                        }.buttonStyle(.bordered)
                    }.padding().background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))
                    VStack(alignment: .leading, spacing: 12) {
                        Label("Game image", systemImage: "opticaldisc").font(.headline)
                        Text(gameName ?? "No game image selected").foregroundStyle(.secondary).lineLimit(2)
                        Button { showingGamePicker = true } label: {
                            Label("Import game from Files", systemImage: "folder").frame(maxWidth: .infinity)
                        }.buttonStyle(.borderedProminent)
                    }.padding().background(.thinMaterial, in: RoundedRectangle(cornerRadius: 16))
                    VStack(alignment: .leading, spacing: 8) {
                        Label("Development status", systemImage: "hammer").font(.headline)
                        Text("This is the native app shell only. Selecting files does not start a game yet; the original C++ engine still needs to be connected to the iOS UI and its hardware implementation completed.")
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
            guard let url = urls.first else { return }
            if isBIOS { biosName = url.lastPathComponent; statusMessage = "BIOS selected — core integration pending" }
            else { gameName = url.lastPathComponent; statusMessage = "Game selected — boot support pending" }
        case .failure(let error):
            statusMessage = "File import failed: \(error.localizedDescription)"
        }
    }
}
