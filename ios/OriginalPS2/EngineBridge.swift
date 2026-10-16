import Foundation

// Temporary direct symbol declarations for the C ABI.
// The eventual Xcode target must link the retroshelf_engine static library.
// Keep these signatures in sync with engine/include/retroshelf/engine_c_api.h.
@_silgen_name("retroshelf_engine_abi_version")
private func retroshelfEngineABIVersion() -> UInt32

@_silgen_name("retroshelf_engine_status_message")
private func retroshelfEngineStatusMessage() -> UnsafePointer<CChar>?

@_silgen_name("retroshelf_engine_can_boot_games")
private func retroshelfEngineCanBootGames() -> Int32

@_silgen_name("retroshelf_engine_inspect_bios_image")
private func retroshelfEngineInspectBiosImage(_ path: UnsafePointer<CChar>?) -> Int32

@_silgen_name("retroshelf_engine_validate_disc_boot_image")
private func retroshelfEngineValidateDiscBootImage(_ path: UnsafePointer<CChar>?) -> Int32

enum EngineBridge {
    static var abiVersion: UInt32 { retroshelfEngineABIVersion() }

    static var statusMessage: String {
        guard let message = retroshelfEngineStatusMessage() else {
            return "Engine returned no status message"
        }
        return String(cString: message)
    }

    static var canBootGames: Bool { retroshelfEngineCanBootGames() != 0 }

    static func inspectBIOSImage(at url: URL) -> Int32 {
        url.path.withCString { path in
            retroshelfEngineInspectBiosImage(path)
        }
    }

    static func validateDiscBootImage(at url: URL) -> Bool {
        url.path.withCString { path in
            retroshelfEngineValidateDiscBootImage(path) != 0
        }
    }

    static var readinessMessage: String {
        canBootGames
            ? "Engine reports game boot support"
            : "\(statusMessage) (ABI v\(abiVersion))"
    }
}
