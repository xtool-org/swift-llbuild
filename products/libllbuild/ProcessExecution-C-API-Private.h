// This source file is part of the Swift.org open source project.
// Licensed under Apache License v2.0 with Runtime Library Exception.
// See https://swift.org/LICENSE.txt for license information.

#ifndef LLBUILD_PROCESS_EXECUTION_C_API_PRIVATE_H
#define LLBUILD_PROCESS_EXECUTION_C_API_PRIVATE_H

#include <llbuild/llbuild.h>
#include "llbuild/Basic/ProcessExecutionHook.h"

namespace llbuild {
llvm::Optional<basic::ProcessExecutionResult> executeProcessWithDelegate(
    llb_process_execution_hook_t hook, void* context,
    ArrayRef<StringRef> commandLine, const basic::POSIXEnvironment& environment,
    basic::ProcessAttributes attributes);
}

#endif
