import Foundation

@_silgen_name("retroshelf_engine_abi_version")
private func retroshelfEngineABIVersion() -> UInt32
@_silgen_name("retroshelf_engine_status_message")
private func retroshelfEngineStatusMessage() -> UnsafePointer<CChar>?
@_silgen_name("retroshelf_engine_can_boot_games")
private func retroshelfEngineCanBootGames() -> Int32
@_silgen_name("retroshelf_engine_validate_disc_boot_image")
private func retroshelfEngineValidateDiscBootImage(_ path: UnsafePointer<CChar>?) -> Int32
@_silgen_name("retroshelf_engine_run_disc_subset")
private func retroshelfEngineRunDiscSubset(_ path: UnsafePointer<CChar>?, _ budget: UInt64, _ executed: UnsafeMutablePointer<UInt64>?, _ pc: UnsafeMutablePointer<UInt64>?, _ reason: UnsafeMutablePointer<Int32>?) -> Int32

struct EngineExecutionReport {
    let instructionsExecuted: UInt64
    let finalPC: UInt64
    let stopReason: Int32
    var summary: String {
        let reasonText: String
        switch stopReason {
        case 0: reasonText = "instruction budget reached"
        case 1: reasonText = "unsupported instruction or CPU exception"
        case 2: reasonText = "invalid instruction fetch"
        default: reasonText = "unknown stop reason"
        }
        return "Smoke test only: \(instructionsExecuted) instructions; PC 0x\(String(finalPC, radix: 16)); \(reasonText). Not gameplay."
    }
}

enum EngineBridge {
    static var abiVersion: UInt32 { retroshelfEngineABIVersion() }
    static var statusMessage: String {
        guard let message = retroshelfEngineStatusMessage() else { return "Engine returned no status message" }
        return String(cString: message)
    }
    static var canBootGames: Bool { retroshelfEngineCanBootGames() != 0 }
    static func validateDiscBootImage(at url: URL) -> Bool {
        url.path.withCString { retroshelfEngineValidateDiscBootImage($0) != 0 }
    }
    static func runDiscSmokeTest(at url: URL, instructionBudget: UInt64 = 10_000) -> EngineExecutionReport? {
        var executed: UInt64 = 0
        var pc: UInt64 = 0
        var reason: Int32 = -1
        let success = url.path.withCString {
            retroshelfEngineRunDiscSubset($0, instructionBudget, &executed, &pc, &reason)
        }
        guard success != 0 else { return nil }
        return EngineExecutionReport(instructionsExecuted: executed, finalPC: pc, stopReason: reason)
    }
    static var readinessMessage: String {
        canBootGames ? "Engine reports game boot support" : "\(statusMessage) (ABI v\(abiVersion))"
    }
}
