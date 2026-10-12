// This source file is part of the Swift.org open source project.
//
// Copyright (c) 2026 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception.
// See https://swift.org/LICENSE.txt for license information.

#ifndef LLBUILD_PUBLIC_PROCESS_EXECUTION_H
#define LLBUILD_PUBLIC_PROCESS_EXECUTION_H

#if !defined(LLBUILD_PUBLIC_LLBUILD_H) && !defined(__clang_tapi__)
#error Clients must include the "llbuild.h" umbrella header.
#endif

#include "buildsystem.h"

LLBUILD_ASSUME_NONNULL_BEGIN

/// Borrowed data, valid only for the duration of a process execution hook.
/// Environment includes inherited bindings and llbuild's queue variables.
/// The control-channel descriptor is created only if execution is deferred.
typedef struct llb_process_execution_request_t_ {
  const llb_data_t * _Nullable arguments;
  size_t argument_count;
  /// NULL-terminated array of NUL-terminated KEY=VALUE strings.
  const char * _Nullable const * _Nonnull environment;
  llb_data_t working_directory;
  bool can_safely_interrupt;
  bool connect_to_console;
  bool control_enabled;
} llb_process_execution_request_t;

/// A handled process result. Output is merged stdout/stderr; error describes
/// a failure to execute. exit_status uses the native wait status representation
/// (as in llb_buildsystem_command_extended_result_t).
typedef struct llb_process_execution_result_t_ {
  llb_buildsystem_command_result_t result;
  int exit_status;
  llb_data_t output;
  llb_data_t error;
} llb_process_execution_result_t;

/// Return false to spawn normally, or true to replace execution with result.
/// result is zero-initialized. Any output/error buffers must be allocated with
/// malloc (or llb_data_copy); llbuild destroys them, including on a false return.
/// Called synchronously and concurrently on execution workers, without a lock.
/// Must eventually return, including when the build is cancelled. Do not allow
/// exceptions to escape or recursively launch another llbuild process here.
typedef bool (*llb_process_execution_hook_t)(
    void * _Nullable context, const llb_process_execution_request_t *request,
    llb_process_execution_result_t *result);

LLBUILD_ASSUME_NONNULL_END

#endif
