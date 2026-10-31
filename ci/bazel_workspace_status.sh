#!/bin/sh
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

# Source identity is collected before build actions, just as CMake configures
# it before compilation. Git is not invoked by a sandboxed compilation action.
# Match src/core/CMakeLists.txt, including its source-archive fallback.
naja_revision=$(git log -1 --pretty=format:%h 2>/dev/null) || naja_revision=${NAJA_GIT_HASH:-unknown}
if [ -z "$naja_revision" ]; then naja_revision=unknown; fi
printf 'STABLE_NAJA_GIT_HASH %s\n' "$naja_revision"
