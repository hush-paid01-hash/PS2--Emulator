import SwiftUI
import UniformTypeIdentifiers

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
                    VStack(spacing: 12) {
                        fileButton(title: "Choose PS2 BIOS", subtitle: selectedBIOS?.lastPathComponent ?? "Select a BIOS file you legally obtained", icon: "cpu") { choosingBIOS = true }
                        fileButton(title: "Choose game image", subtitle: selectedDisc?.lastPathComponent ?? "ISO, BIN, IMG or CHD", icon: "opticaldisc") { choosingDisc = true }
                    }
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
                message = "Copied \(label) into app storage: \(sourceURL.lastPathComponent). File loading and emulation are not implemented yet."
            } catch {
                message = "Could not import \(label): \(error.localizedDescription)"
            }
        case .failure(let error):
            message = "Could not select \(label): \(error.localizedDescription)"
        }
    }
}
