// This source file is part of the Swift.org open source project.
//
// Copyright (c) 2026 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception.
// See https://swift.org/LICENSE.txt for license information.

#ifndef LLBUILD_BASIC_PROCESSEXECUTIONHOOK_H
#define LLBUILD_BASIC_PROCESSEXECUTIONHOOK_H

#include "llbuild/Basic/Subprocess.h"
#include "llvm/ADT/ArrayRef.h"

#include <string>

namespace llbuild {
  namespace basic {

    /// A client-supplied process result. Output is merged stdout/stderr; error
    /// describes a failure to execute, rather than output from the process.
    struct ProcessExecutionResult {
      ProcessResult result;
      std::string output;
      std::string error;
    };

    /// Per-build interception, invoked concurrently on execution workers.
    /// The delegate must remain alive until all executions have completed.
    class ProcessExecutionDelegate {
    public:
      virtual ~ProcessExecutionDelegate() = default;

      /// Return None to spawn normally. Must eventually return even when the
      /// build is cancelled. Do not recursively execute llbuild processes here.
      virtual llvm::Optional<ProcessExecutionResult> executeProcess(
          ArrayRef<StringRef> commandLine, const POSIXEnvironment& environment,
          ProcessAttributes attributes) {
        return llvm::None;
      }
    };

    /// Ask the build delegate to execute the process, falling back to native
    /// spawning if it defers. Both paths report through the original process
    /// delegate and completion callback.
    void spawnProcessWithHook(ProcessExecutionDelegate& executionDelegate,
                              ProcessDelegate& delegate, ProcessContext* context,
                              ProcessGroup& group, ProcessHandle handle,
                              ArrayRef<StringRef> commandLine,
                              POSIXEnvironment environment,
                              ProcessAttributes attributes,
                              ProcessReleaseFn&& releaseFn,
                              ProcessCompletionFn&& completionFn);

  } // namespace basic
} // namespace llbuild

#endif
