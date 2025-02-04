#pragma once

#include <memory>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

struct EmplaceEnabler final {
  // Disable {}-initialization in Queue's constructor
  explicit EmplaceEnabler() = default;
};

struct NoToken final {
  template <typename LockFreeQueue>
  explicit NoToken(LockFreeQueue& /*unused*/) {}
};

/// @warning A single Producer must not be used from multiple threads
/// concurrently
template <typename QueueType, typename ProducerToken>
class Producer final {
  using ValueType = typename QueueType::ValueType;

 public:
  Producer(const Producer&) = delete;
  Producer(Producer&&) noexcept = default;
  Producer& operator=(const Producer&) = delete;
  Producer& operator=(Producer&&) noexcept = default;

  ~Producer() {
    if (queue_) queue_->MarkProducerIsDead();
  }

  /// Push element into queue. May wait asynchronously if the queue is full.
  /// @returns whether push succeeded before the deadline.
  bool Push(ValueType&& value, engine::Deadline deadline = {}) const {
    UASSERT(queue_);
    return queue_->Push(token_, std::move(value), deadline);
  }

  /// Try to push element into queue without blocking.
  /// @returns whether push succeeded.
  bool PushNoblock(ValueType&& value) const {
    UASSERT(queue_);
    return queue_->PushNoblock(token_, std::move(value));
  }

  void Release() && {
    if (queue_) queue_->MarkProducerIsDead();
    queue_.reset();
  }

  /// Const access to source queue.
  std::shared_ptr<const QueueType> Queue() const { return {queue_}; }

  /// @cond
  // For internal use only
  Producer(std::shared_ptr<QueueType> queue, EmplaceEnabler /*unused*/)
      : queue_(std::move(queue)), token_(queue_->queue_) {}
  /// @endcond

 private:
  std::shared_ptr<QueueType> queue_;
  mutable ProducerToken token_;
};

/// @warning A single Consumer must not be used from multiple threads
/// concurrently
template <typename QueueType>
class Consumer final {
  using ValueType = typename QueueType::ValueType;
  using ConsumerToken = typename QueueType::ConsumerToken;

 public:
  Consumer(const Consumer&) = delete;
  Consumer(Consumer&&) noexcept = default;
  Consumer& operator=(const Consumer&) = delete;
  Consumer& operator=(Consumer&&) noexcept = default;

  ~Consumer() {
    if (queue_) queue_->MarkConsumerIsDead();
  }

  /// Pop element from queue. May wait asynchronously if the queue is empty,
  /// but the producer is alive.
  /// @returns whether something was popped before the deadline.
  /// @note `false` can be returned before the deadline
  /// when the producer is no longer alive.
  bool Pop(ValueType& value, engine::Deadline deadline = {}) const {
    return queue_->Pop(token_, value, deadline);
  }

  /// Try to pop element from queue without blocking.
  /// @return whether something was popped.
  bool PopNoblock(ValueType& value) const {
    return queue_->PopNoblock(token_, value);
  }

  /// Const access to source queue.
  std::shared_ptr<const QueueType> Queue() const { return {queue_}; }

  /// @cond
  // For internal use only
  Consumer(std::shared_ptr<QueueType> queue, EmplaceEnabler /*unused*/)
      : queue_(std::move(queue)), token_(queue_->queue_) {}
  /// @endcond

 private:
  std::shared_ptr<QueueType> queue_{};
  mutable ConsumerToken token_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
