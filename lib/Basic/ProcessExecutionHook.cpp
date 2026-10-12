// This source file is part of the Swift.org open source project.
//
// Copyright (c) 2026 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception.
// See https://swift.org/LICENSE.txt for license information.

#include "llbuild/Basic/ProcessExecutionHook.h"
#include "llvm/ADT/Twine.h"

using namespace llbuild;
using namespace llbuild::basic;

void llbuild::basic::spawnProcessWithHook(
    ProcessExecutionDelegate& executionDelegate, ProcessDelegate& delegate,
    ProcessContext* ctx, ProcessGroup& pgrp,
    ProcessHandle handle, ArrayRef<StringRef> commandLine,
    POSIXEnvironment environment, ProcessAttributes attr,
    ProcessReleaseFn&& releaseFn, ProcessCompletionFn&& completionFn) {
  llvm::Optional<ProcessExecutionResult> result;
  {
    std::lock_guard<std::mutex> guard(pgrp.mutex);
    if (pgrp.isClosed())
      result = ProcessExecutionResult{ProcessResult::makeCancelled(), {}, {}};
  }
  if (!result)
    result = executionDelegate.executeProcess(commandLine, environment, attr);
  {
    std::lock_guard<std::mutex> guard(pgrp.mutex);
    // Cancellation wins over both a replacement and a deferred spawn.
    if (pgrp.isClosed())
      result = ProcessExecutionResult{ProcessResult::makeCancelled(), {}, {}};
  }
  if (result) {
    delegate.processStarted(ctx, handle, result->result.pid);
    if (!result->error.empty())
      delegate.processHadError(ctx, handle, result->error);
    if (!result->output.empty())
      delegate.processHadOutput(ctx, handle, result->output);
    delegate.processFinished(ctx, handle, result->result);
    completionFn(result->result);
    return;
  }
  spawnProcess(delegate, ctx, pgrp, handle, commandLine, std::move(environment),
               attr, std::move(releaseFn), std::move(completionFn));
}
