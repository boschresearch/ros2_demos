// Copyright (c) 2020 Robert Bosch GmbH
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

#include "cbg_executor_demo/pong_node.hpp"

#include <cassert>

#include <chrono>
#include <memory>

#include "cbg_executor_demo/utilities.hpp"

namespace cbg_executor_demo
{

PongNode::PongNode()
: rclcpp::Node("pong_node")
{
  using std::placeholders::_1;
  using std_msgs::msg::Int32;

  declare_parameter<double>("high_busyloop", 0.01);
  high_pong_publisher_ = this->create_publisher<Int32>("high_pong", rclcpp::SensorDataQoS());
  high_ping_subscription_ = this->create_subscription<Int32>(
    "high_ping", rclcpp::SensorDataQoS(),
    std::bind(&PongNode::high_ping_received, this, _1));

  auto second_cb_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
  assert(second_cb_group == get_low_prio_callback_group());

  declare_parameter<double>("low_busyloop", 0.01);
  low_pong_publisher_ = this->create_publisher<Int32>("low_pong", rclcpp::SensorDataQoS());
  rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> options;
  options.callback_group = second_cb_group;
  low_ping_subscription_ = this->create_subscription<Int32>(
    "low_ping", rclcpp::SensorDataQoS(),
    std::bind(&PongNode::low_ping_received, this, _1), options);
}

rclcpp::CallbackGroup::SharedPtr PongNode::get_high_prio_callback_group()
{
  return get_callback_groups()[0].lock();  // ... the default callback group.
}

rclcpp::CallbackGroup::SharedPtr PongNode::get_low_prio_callback_group()
{
  return get_callback_groups()[1].lock();  // ... the second callback group created in the ctor.
}

void PongNode::high_ping_received(const std_msgs::msg::Int32::SharedPtr msg)
{
  std::chrono::nanoseconds busyloop = get_nanos_from_secs_parameter(this, "high_busyloop");
  burn_cpu_cycles(busyloop);
  high_pong_publisher_->publish(*msg);
}

void PongNode::low_ping_received(const std_msgs::msg::Int32::SharedPtr msg)
{
  std::chrono::nanoseconds busyloop = get_nanos_from_secs_parameter(this, "low_busyloop");
  burn_cpu_cycles(busyloop);
  low_pong_publisher_->publish(*msg);
}

void PongNode::burn_cpu_cycles(std::chrono::nanoseconds duration)
{
  if (duration > std::chrono::nanoseconds::zero()) {
    auto endTime = get_current_thread_time() + duration;
    int x = 0;
    bool doAgain = true;
    while (doAgain) {
      while (x != std::rand() && x % 1000 != 0) {
        x++;
      }
      doAgain = (get_current_thread_time() < endTime);
    }
  }
}

}  // namespace cbg_executor_demo
