import Foundation

@_silgen_name("retroshelf_engine_abi_version")
private func retroshelfEngineABIVersion() -> UInt32
@_silgen_name("retroshelf_engine_status_message")
private func retroshelfEngineStatusMessage() -> UnsafePointer<CChar>?
@_silgen_name("retroshelf_engine_can_boot_games")
private func retroshelfEngineCanBootGames() -> Int32
@_silgen_name("retroshelf_engine_inspect_bios_image")
private func retroshelfEngineInspectBIOSImage(_ path: UnsafePointer<CChar>?, _ sizeBytes: UnsafeMutablePointer<UInt64>?, _ plausibleSize: UnsafeMutablePointer<Int32>?) -> Int32
@_silgen_name("retroshelf_engine_validate_disc_boot_image")
private func retroshelfEngineValidateDiscBootImage(_ path: UnsafePointer<CChar>?) -> Int32
@_silgen_name("retroshelf_engine_trace_count")
private func retroshelfEngineTraceCount() -> UInt64
@_silgen_name("retroshelf_engine_trace_pc")
private func retroshelfEngineTracePC(_ index: UInt64) -> UInt64
@_silgen_name("retroshelf_engine_trace_instruction")
private func retroshelfEngineTraceInstruction(_ index: UInt64) -> UInt32
@_silgen_name("retroshelf_engine_trace_fetched")
private func retroshelfEngineTraceFetched(_ index: UInt64) -> Int32
@_silgen_name("retroshelf_engine_run_disc_subset")
private func retroshelfEngineRunDiscSubset(_ path: UnsafePointer<CChar>?, _ budget: UInt64, _ executed: UnsafeMutablePointer<UInt64>?, _ pc: UnsafeMutablePointer<UInt64>?, _ reason: UnsafeMutablePointer<Int32>?) -> Int32
@_silgen_name("retroshelf_engine_run_bios_reset_vector")
private func retroshelfEngineRunBIOSResetVector(_ path: UnsafePointer<CChar>?, _ budget: UInt64, _ executed: UnsafeMutablePointer<UInt64>?, _ pc: UnsafeMutablePointer<UInt64>?, _ reason: UnsafeMutablePointer<Int32>?) -> Int32


@_silgen_name("retroshelf_engine_render_demo_frame")
private func retroshelfEngineRenderDemoFrame() -> Int32
@_silgen_name("retroshelf_engine_framebuffer_pixels")
private func retroshelfEngineFramebufferPixels() -> UnsafePointer<UInt8>?
@_silgen_name("retroshelf_engine_framebuffer_width")
private func retroshelfEngineFramebufferWidth() -> UInt32
@_silgen_name("retroshelf_engine_framebuffer_height")
private func retroshelfEngineFramebufferHeight() -> UInt32

struct EngineTraceEntry {
    let pc: UInt64
    let instruction: UInt32
    let fetched: Bool
}

struct EngineExecutionReport {
    let instructionsExecuted: UInt64
    let finalPC: UInt64
    let stopReason: Int32
    let trace: [EngineTraceEntry]

    var traceSummary: String {
        guard !trace.isEmpty else { return "No instruction trace was captured." }
        let lines = trace.prefix(12).map { entry -> String in
            let pc = String(entry.pc, radix: 16).uppercased().padding(toLength: 8, withPad: "0", startingAt: 0)
            let word = String(entry.instruction, radix: 16).uppercased().padding(toLength: 8, withPad: "0", startingAt: 0)
            return "PC 0x\(pc)  instruction 0x\(word)\(entry.fetched ? "" : "  [fetch failed]")"
        }
        return "Instruction trace (first \(lines.count) entries):\n" + lines.joined(separator: "\n")
    }
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
    static func renderDemoPreview() -> (width: Int, height: Int, rgba: Data)? {
        guard retroshelfEngineRenderDemoFrame() != 0,
              let pixels = retroshelfEngineFramebufferPixels() else { return nil }
        let width = Int(retroshelfEngineFramebufferWidth())
        let height = Int(retroshelfEngineFramebufferHeight())
        guard width > 0, height > 0, width <= 4096, height <= 4096 else { return nil }
        return (width, height, Data(bytes: pixels, count: width * height * 4))
    }
    static var abiVersion: UInt32 { retroshelfEngineABIVersion() }
    static var statusMessage: String {
        guard let message = retroshelfEngineStatusMessage() else { return "Engine returned no status message" }
        return String(cString: message)
    }
    static var canBootGames: Bool { retroshelfEngineCanBootGames() != 0 }
    static func inspectBIOS(at url: URL) -> (sizeBytes: UInt64, plausibleSize: Bool)? {
        var sizeBytes: UInt64 = 0
        var plausibleSize: Int32 = 0
        let readable = url.path.withCString {
            retroshelfEngineInspectBIOSImage($0, &sizeBytes, &plausibleSize)
        }
        guard readable != 0 else { return nil }
        return (sizeBytes, plausibleSize != 0)
    }
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
        let traceCount = min(Int(retroshelfEngineTraceCount()), 256)
        let trace = (0..<traceCount).map { index in
            let i = UInt64(index)
            return EngineTraceEntry(
                pc: retroshelfEngineTracePC(i),
                instruction: retroshelfEngineTraceInstruction(i),
                fetched: retroshelfEngineTraceFetched(i) == 1
            )
        }
        return EngineExecutionReport(instructionsExecuted: executed, finalPC: pc, stopReason: reason, trace: trace)
    }
    static func runBIOSResetVectorSmokeTest(at url: URL, instructionBudget: UInt64 = 10_000) -> EngineExecutionReport? {
        var executed: UInt64 = 0
        var pc: UInt64 = 0
        var reason: Int32 = -1
        let success = url.path.withCString {
            retroshelfEngineRunBIOSResetVector($0, instructionBudget, &executed, &pc, &reason)
        }
        guard success != 0 else { return nil }
        return EngineExecutionReport(instructionsExecuted: executed, finalPC: pc, stopReason: reason, trace: [])
    }
    static var readinessMessage: String {
        canBootGames ? "Engine reports game boot support" : "\(statusMessage) (ABI v\(abiVersion))"
    }
}
