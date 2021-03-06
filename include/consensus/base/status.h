// Copyright 2017 Wu Tao
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "consensus/base/slice.h"

#include <silly/status.h>

namespace consensus {

class Error {
 public:
  enum ErrorCodes {
    OK,

    IOError,
    NotSupported,
    Corruption,
    LogCompacted,
    OutOfBound,
    YARaftError,
    BadConfig,
    RpcError,
    IllegalState,
    RuntimeError,
    InvalidArgument,
    WalWriteToNonLeader,
  };

  static std::string toString(unsigned int errorCode);
};

typedef silly::Status<Error> Status;

template <typename T>
using StatusWith = silly::StatusWith<Status, T>;

#ifdef FMT_Status
#undef FMT_Status
#endif
#define FMT_Status(code, msg, args...) Status::Make(Error::code, fmt::format(msg, ##args))

}  // namespace consensus
