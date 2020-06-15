#pragma once

#include <stdexcept>

#include <engine/task/task_with_result.hpp>
#include <storages/redis/subscription_token.hpp>

#include "subscription_queue.hpp"

namespace storages {
namespace redis {

class SubscriptionTokenImpl : public SubscriptionTokenImplBase {
 public:
  using OnMessageCb = SubscriptionToken::OnMessageCb;

  SubscriptionTokenImpl(::redis::SubscribeSentinel& subscribe_sentinel,
                        std::string channel, OnMessageCb on_message_cb,
                        const ::redis::CommandControl& command_control);

  ~SubscriptionTokenImpl() override;

  void SetMaxQueueLength(size_t length) override;

  void Unsubscribe() override;

 private:
  void ProcessMessages();

  std::string channel_;
  std::unique_ptr<SubscriptionQueue<ChannelSubscriptionQueueItem>> queue_;
  OnMessageCb on_message_cb_;
  engine::TaskWithResult<void> subscriber_task_;
};

class PsubscriptionTokenImpl : public SubscriptionTokenImplBase {
 public:
  using OnPmessageCb = SubscriptionToken::OnPmessageCb;

  PsubscriptionTokenImpl(::redis::SubscribeSentinel& subscribe_sentinel,
                         std::string pattern, OnPmessageCb on_pmessage_cb,
                         const ::redis::CommandControl& command_control);

  ~PsubscriptionTokenImpl() override;

  void SetMaxQueueLength(size_t length) override;

  void Unsubscribe() override;

 private:
  void ProcessMessages();

  std::string pattern_;
  std::unique_ptr<SubscriptionQueue<PatternSubscriptionQueueItem>> queue_;
  OnPmessageCb on_pmessage_cb_;
  engine::TaskWithResult<void> subscriber_task_;
};

}  // namespace redis
}  // namespace storages