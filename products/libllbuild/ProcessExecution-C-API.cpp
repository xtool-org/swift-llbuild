// This source file is part of the Swift.org open source project.
//
// Copyright (c) 2026 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception.
// See https://swift.org/LICENSE.txt for license information.

#include <llbuild/llbuild.h>
#include "ProcessExecution-C-API-Private.h"

#include <vector>

using namespace llbuild;

llvm::Optional<basic::ProcessExecutionResult> llbuild::executeProcessWithDelegate(
    llb_process_execution_hook_t hook, void* context,
    ArrayRef<StringRef> commandLine, const basic::POSIXEnvironment& environment,
    basic::ProcessAttributes attributes) {
  using namespace llbuild::basic;
  if (!hook)
    return llvm::None;
  auto data = [](StringRef value) -> llb_data_t {
    return {value.size(), reinterpret_cast<const uint8_t*>(value.data())};
  };
  std::vector<llb_data_t> arguments;
  for (auto argument : commandLine)
    arguments.push_back(data(argument));
  // getEnvp() freezes its environment. Keep the original mutable so native
  // spawning can still add the control-channel binding if the hook defers.
  auto hookEnvironment = environment;
  llb_process_execution_request_t request{
      arguments.data(), arguments.size(), hookEnvironment.getEnvp(),
      data(attributes.workingDir), attributes.canSafelyInterrupt,
      attributes.connectToConsole, attributes.controlEnabled};
  llb_process_execution_result_t supplied{};
  bool handled = hook(context, &request, &supplied);
  ProcessExecutionResult result;
  if (handled) {
    switch (supplied.result) {
    case llb_buildsystem_command_result_succeeded: result.result.status = ProcessStatus::Succeeded; break;
    case llb_buildsystem_command_result_cancelled: result.result.status = ProcessStatus::Cancelled; break;
    case llb_buildsystem_command_result_skipped: result.result.status = ProcessStatus::Skipped; break;
    default: result.result.status = ProcessStatus::Failed; break;
    }
    result.result.exitCode = supplied.exit_status;
    if (supplied.output.length)
      result.output.assign(reinterpret_cast<const char*>(supplied.output.data), supplied.output.length);
    if (supplied.error.length)
      result.error.assign(reinterpret_cast<const char*>(supplied.error.data), supplied.error.length);
  }
  llb_data_destroy(&supplied.output);
  llb_data_destroy(&supplied.error);
  if (!handled)
    return llvm::None;
  return result;
}
