#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/optional.hpp>

#include <redis/reply.hpp>
#include <utils/clang_format_workarounds.hpp>

#include <storages/redis/reply_types.hpp>
#include <storages/redis/request_data_base.hpp>

namespace storages {
namespace redis {

template <typename Result, typename ReplyType = Result>
class USERVER_NODISCARD Request final {
 public:
  using Reply = ReplyType;

  explicit Request(std::unique_ptr<RequestDataBase<Result, ReplyType>>&& impl)
      : impl_(std::move(impl)) {}

  void Wait() { impl_->Wait(); }

  void IgnoreResult() const {}

  ReplyType Get(const std::string& request_description = {}) {
    return impl_->Get(request_description);
  }

  ::redis::ReplyPtr GetRaw() { return impl_->GetRaw(); }

 private:
  std::unique_ptr<RequestDataBase<Result, ReplyType>> impl_;
};

using RequestAppend = Request<size_t>;
using RequestDbsize = Request<size_t>;
using RequestDel = Request<size_t>;
using RequestEval = Request<::redis::ReplyData>;
using RequestExists = Request<size_t>;
using RequestExpire = Request<ExpireReply>;
using RequestGet = Request<boost::optional<std::string>>;
using RequestHdel = Request<size_t>;
using RequestHexists = Request<size_t>;
using RequestHget = Request<boost::optional<std::string>>;
using RequestHgetall = Request<std::unordered_map<std::string, std::string>>;
using RequestHincrby = Request<int64_t>;
using RequestHincrbyfloat = Request<double>;
using RequestHkeys = Request<std::vector<std::string>>;
using RequestHlen = Request<size_t>;
using RequestHmget = Request<std::vector<boost::optional<std::string>>>;
using RequestHmset = Request<StatusOk, void>;
using RequestHset = Request<HsetReply>;
using RequestHsetnx = Request<size_t, bool>;
using RequestHvals = Request<std::vector<std::string>>;
using RequestIncr = Request<int64_t>;
using RequestKeys = Request<std::vector<std::string>>;
using RequestLindex = Request<boost::optional<std::string>>;
using RequestLlen = Request<size_t>;
using RequestLpop = Request<boost::optional<std::string>>;
using RequestLpush = Request<size_t>;
using RequestLrange = Request<std::vector<std::string>>;
using RequestLtrim = Request<StatusOk, void>;
using RequestMget = Request<std::vector<boost::optional<std::string>>>;
using RequestPersist = Request<PersistReply>;
using RequestPexpire = Request<ExpireReply>;
using RequestPing = Request<StatusPong, void>;
using RequestPingMessage = Request<std::string>;
using RequestPublish = Request<size_t>;
using RequestRename = Request<StatusOk, void>;
using RequestRpop = Request<boost::optional<std::string>>;
using RequestRpush = Request<size_t>;
using RequestSadd = Request<size_t>;
using RequestScard = Request<size_t>;
using RequestSet = Request<StatusOk, void>;
using RequestSetIfExist = Request<boost::optional<StatusOk>, bool>;
using RequestSetIfNotExist = Request<boost::optional<StatusOk>, bool>;
using RequestSetOptions = Request<SetReply>;
using RequestSetex = Request<StatusOk, void>;
using RequestSismember = Request<size_t>;
using RequestSmembers = Request<std::unordered_set<std::string>>;
using RequestSrandmember = Request<boost::optional<std::string>>;
using RequestSrandmembers = Request<std::vector<std::string>>;
using RequestSrem = Request<size_t>;
using RequestStrlen = Request<size_t>;
using RequestTtl = Request<TtlReply>;
using RequestType = Request<KeyType>;
using RequestZadd = Request<size_t>;
using RequestZaddIncr = Request<double>;
using RequestZaddIncrExisting = Request<boost::optional<double>>;
using RequestZcard = Request<size_t>;
using RequestZrangebyscore = Request<std::vector<std::string>>;
using RequestZrangebyscoreWithScores = Request<std::vector<MemberScore>>;
using RequestZrem = Request<size_t>;
using RequestZscore = Request<boost::optional<double>>;

}  // namespace redis
}  // namespace storages