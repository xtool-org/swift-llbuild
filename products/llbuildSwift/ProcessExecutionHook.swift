// This source file is part of the Swift.org open source project.
//
// Copyright (c) 2026 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception.
// See https://swift.org/LICENSE.txt for license information.

import Dispatch

#if !LLBUILD_FRAMEWORK
import llbuild
#endif

/// A process offered to the build system delegate. The environment includes
/// inherited bindings and queue variables. No control-channel FD exists yet.
public struct ProcessExecutionRequest: Sendable {
    public var commandLine: [String]
    public var environment: [String: String]
    public var workingDirectory: String
    public var canSafelyInterrupt: Bool
    public var connectToConsole: Bool
    public var controlEnabled: Bool
}

/// A replacement process result. Output contains merged stdout/stderr;
/// error describes an execution failure rather than process output.
public struct ProcessExecutionResult: Sendable {
    public var result: CommandResult
    /// Same platform-specific status representation as CommandExtendedResult.exitStatus.
    public var exitStatus: Int32
    public var output: [UInt8]
    public var error: String?

    public init(result: CommandResult, exitStatus: Int32, output: [UInt8] = [], error: String? = nil) {
        self.result = result
        self.exitStatus = exitStatus
        self.output = output
        self.error = error
    }
}

// The semaphore publishes the single writer's result before the worker reads it.
private final class ProcessExecutionHookCompletion: @unchecked Sendable {
    private let semaphore = DispatchSemaphore(value: 0)
    private var result: ProcessExecutionResult?

    func finish(_ result: ProcessExecutionResult?) {
        self.result = result
        semaphore.signal()
    }

    func wait() -> ProcessExecutionResult? {
        semaphore.wait()
        return result
    }
}

extension BuildSystemDelegate {
    @available(macOS 10.15, iOS 13, tvOS 13, watchOS 6, *)
    public func executeProcess(_ request: ProcessExecutionRequest) async -> ProcessExecutionResult? {
        nil
    }
}

// Build system delegates already receive concurrent native callbacks. Retain
// the delegate while crossing into Swift concurrency and waiting for its result.
private final class ProcessExecutionDelegateBox: @unchecked Sendable {
    let delegate: BuildSystemDelegate

    init(_ delegate: BuildSystemDelegate) {
        self.delegate = delegate
    }

    @available(macOS 10.15, iOS 13, tvOS 13, watchOS 6, *)
    func execute(_ request: ProcessExecutionRequest) -> ProcessExecutionResult? {
        let completion = ProcessExecutionHookCompletion()
        _Concurrency.Task {
            completion.finish(await self.delegate.executeProcess(request))
        }
        // This is an llbuild worker, outside Swift's concurrency pool. The lane
        // stays occupied until the callback completes, including on cancellation.
        return completion.wait()
    }
}

extension BuildSystem {
    @available(macOS 10.15, iOS 13, tvOS 13, watchOS 6, *)
    func executeProcess(
        _ request: UnsafePointer<llb_process_execution_request_t>,
        _ result: UnsafeMutablePointer<llb_process_execution_result_t>
    ) -> Bool {
        let request = request.pointee
        let commandLine = (0..<Int(request.argument_count)).map { stringFromData(request.arguments![$0]) }
        var environment: [String: String] = [:]
        var entry = request.environment
        while let value = entry.pointee {
            let binding = String(cString: value).split(separator: "=", maxSplits: 1, omittingEmptySubsequences: false)
            if binding.count == 2 {
                environment[String(binding[0])] = String(binding[1])
            }
            entry += 1
        }
        let input = ProcessExecutionRequest(
            commandLine: commandLine, environment: environment,
            workingDirectory: stringFromData(request.working_directory),
            canSafelyInterrupt: request.can_safely_interrupt,
            connectToConsole: request.connect_to_console,
            controlEnabled: request.control_enabled)
        guard let supplied = ProcessExecutionDelegateBox(delegate).execute(input) else {
            return false
        }
        result.pointee.result = supplied.result
        result.pointee.exit_status = supplied.exitStatus
        result.pointee.output = copiedDataFromBytes(supplied.output)
        result.pointee.error = copiedDataFromBytes(Array((supplied.error ?? "").utf8))
        return true
    }
}
