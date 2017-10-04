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

#include "db.h"
#include "memkv_store.h"

#include <consensus/base/coding.h>
#include <consensus/replicated_log.h>
#include <fmt/format.h>

namespace memkv {

enum OpType {
  kWrite = 1,
  kDelete = 2,
};

// Log format:
//   - WRITE: type path value
//            type = 1 byte
//            path, value = varstring
//   - DELETE: type path
//
static std::string LogEncode(OpType type, const Slice &path, const Slice &value) {
  using consensus::VarintLength;
  using consensus::PutLengthPrefixedSlice;

  std::string result;
  result.push_back(static_cast<unsigned char>(type));

  PutLengthPrefixedSlice(&result, path);
  if (type == kWrite) {
    PutLengthPrefixedSlice(&result, value);
  }
  return result;
}

class DB::Impl {
 public:
  Impl() : kv_(new MemKvStore) {}

  Status Get(const Slice &path, std::string *data) {
    return kv_->Get(path, data);
  }

  Status Delete(const Slice &path) {
    std::string log = LogEncode(OpType::kDelete, path, nullptr);

    consensus::Status s = log_->Write(log);
    if (!s.IsOK()) {
      return Status::Make(Error::ConsensusError, s.ToString());
    }

    RETURN_NOT_OK(kv_->Delete(path));
    return Status::OK();
  }

  Status Write(const Slice &path, const Slice &value) {
    std::string log = LogEncode(OpType::kWrite, path, value);

    consensus::Status s = log_->Write(log);
    if (!s.IsOK()) {
      return Status::Make(Error::ConsensusError, s.ToString());
    }

    RETURN_NOT_OK(kv_->Write(path, value));
    return Status::OK();
  }

 private:
  friend class DB;

  std::unique_ptr<consensus::ReplicatedLog> log_;
  std::unique_ptr<MemKvStore> kv_;
};

StatusWith<DB *> DB::Bootstrap(const DBOptions &options) {
  using consensus::ReplicatedLogOptions;
  using consensus::ReplicatedLog;

  auto db = new DB;

  ReplicatedLogOptions rlogOptions;
  rlogOptions.id = options.member_id;
  rlogOptions.heartbeat_interval = 100;
  rlogOptions.election_timeout = 1000;

  consensus::wal::WriteAheadLogOptions walOptions;
  walOptions.log_dir = options.wal_dir;

  consensus::Status s =
      consensus::wal::WriteAheadLog::Default(walOptions, &rlogOptions.wal, rlogOptions.memstore);
  if (!s.IsOK()) {
    return Status::Make(Error::ConsensusError, s.ToString());
  }

  consensus::StatusWith<ReplicatedLog *> sw = ReplicatedLog::New(rlogOptions);
  if (!sw.IsOK()) {
    return Status::Make(Error::ConsensusError, sw.ToString());
  }
  db->impl_->log_.reset(sw.GetValue());

  return db;
}

Status DB::Get(const Slice &path, std::string *data) {
  return impl_->Get(path, data);
}

Status DB::Delete(const Slice &path) {
  return impl_->Delete(path);
}

Status DB::Write(const Slice &path, const Slice &value) {
  return impl_->Write(path, value);
}

DB::DB() : impl_(new Impl) {}

DB::~DB() = default;

}  // namespace memkv