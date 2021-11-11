#pragma once

#include <grpcpp/completion_queue.h>

#include <userver/utils/fast_pimpl.hpp>

/// @file userver/ugrpc/client/queue_holder.hpp
/// @copybrief ugrpc::client::QueueHolder

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Manages a gRPC completion queue, usable only in clients
class QueueHolder final {
 public:
  QueueHolder();

  QueueHolder(QueueHolder&&) = delete;
  QueueHolder& operator=(QueueHolder&&) = delete;
  ~QueueHolder();

  ::grpc::CompletionQueue& GetQueue();

 private:
  struct Impl;
  utils::FastPimpl<Impl, 136, 8> impl_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END