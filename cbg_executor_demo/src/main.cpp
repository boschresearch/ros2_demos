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

#include <ctime>
#include <cstdlib>

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/executor.hpp>
#include <rclcpp/rclcpp.hpp>

#include "cbg_executor_demo/ping_node.hpp"
#include "cbg_executor_demo/pong_node.hpp"
#include "cbg_executor_demo/utilities.hpp"

using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using namespace std::chrono_literals;

using cbg_executor_demo::PingNode;
using cbg_executor_demo::PongNode;
using cbg_executor_demo::configure_thread;
using cbg_executor_demo::get_thread_time;
using cbg_executor_demo::ThreadPriority;

/// The main function composes the Ping and Pong node (depending on the arguments)
/// and runs the experiment. See README.md for a simple architecture diagram.
/// Here: rt = real-time = high scheduler priority and be = best-effort = low scheduler priority.
int main(int argc, char * argv[])
{
  const std::chrono::seconds EXPERIMENT_DURATION = 10s;

  rclcpp::init(argc, argv);

  // Create two executors within this process.
  rclcpp::executors::SingleThreadedExecutor high_prio_executor;
  rclcpp::executors::SingleThreadedExecutor low_prio_executor;

#ifdef ADD_PING_NODE
  auto ping_node = std::make_shared<PingNode>();
  high_prio_executor.add_node(ping_node);
  rclcpp::Logger logger = ping_node->get_logger();
#endif

#ifdef ADD_PONG_NODE
  auto pong_node = std::make_shared<PongNode>();
  high_prio_executor.add_callback_group(
    pong_node->get_high_prio_callback_group(), pong_node->get_node_base_interface());
  low_prio_executor.add_callback_group(
    pong_node->get_low_prio_callback_group(), pong_node->get_node_base_interface());
#ifndef ADD_PING_NODE
  rclcpp::Logger logger = pong_node->get_logger();
#endif
#endif

  std::thread high_prio_thread([&]() {
      high_prio_executor.spin();
    });
  bool areThreadPriosSet = configure_thread(high_prio_thread, ThreadPriority::HIGH, 0);

  std::thread low_prio_thread([&]() {
      low_prio_executor.spin();
    });
  areThreadPriosSet &= configure_thread(low_prio_thread, ThreadPriority::LOW, 0);

  // Creating the threads immediately started them. Therefore, get start CPU time of each
  // thread now.
  nanoseconds high_prio_thread_begin = get_thread_time(high_prio_thread);
  nanoseconds low_prio_thread_begin = get_thread_time(low_prio_thread);

  if (!areThreadPriosSet) {
    RCLCPP_WARN(logger, "Thread priorities are not configured correctly!");
    RCLCPP_WARN(logger, "Are you root (sudo)? Experiment is performed anyway.");
  }

  RCLCPP_INFO(
    logger, "Running experiment from now on for %ld s ...", EXPERIMENT_DURATION.count());

  std::this_thread::sleep_for(EXPERIMENT_DURATION);

  // Get end CPU time of each thread ...
  nanoseconds high_prio_thread_end = get_thread_time(high_prio_thread);
  nanoseconds low_prio_thread_end = get_thread_time(low_prio_thread);

  // ... and stop the experiment.
  rclcpp::shutdown();
  high_prio_thread.join();
  low_prio_thread.join();

#ifdef ADD_PING_NODE
  ping_node->print_statistics();
#endif

  // Print CPU times.
  int64_t high_prio_thread_duration_ms = std::chrono::duration_cast<milliseconds>(
    high_prio_thread_end - high_prio_thread_begin).count();
  int64_t low_prio_thread_duration_ms = std::chrono::duration_cast<milliseconds>(
    low_prio_thread_end - low_prio_thread_begin).count();
  RCLCPP_INFO(
    logger, "High priority executor thread ran for %ld ms.", high_prio_thread_duration_ms);
  RCLCPP_INFO(
    logger, "Low priority executor thread ran for %ld ms.", low_prio_thread_duration_ms);
  if (!areThreadPriosSet) {
    RCLCPP_WARN(logger, "Again, thread priorities were not configured correctly!");
  }

  return 0;
}
