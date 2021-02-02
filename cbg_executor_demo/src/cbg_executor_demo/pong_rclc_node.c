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


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>


#define STRING_BUFFER_LEN 100

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc); return 1;}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}
#define RCUNUSED(fn) { rcl_ret_t temp_rc __attribute__((unused)); temp_rc = fn; }


rcl_subscription_t high_ping_subscription_;
std_msgs__msg__Int32 high_ping_msg_;
rcl_publisher_t high_pong_publisher_;
rcl_subscription_t low_ping_subscription_;
std_msgs__msg__Int32 low_ping_msg_;
rcl_publisher_t low_pong_publisher_;


void burn_cpu_cycles(long duration)
{
  if (duration > 0) {
    clockid_t clockId;
    clock_getcpuclockid(pthread_self(), &clockId);
    struct timespec startTimeP;
    clock_gettime(clockId, &startTimeP);
    int x = 0;
    bool doAgain = true;
    while (doAgain) {
      while (x != rand() && x % 10 != 0) {
        x++;
      }
      struct timespec currentTimeP;
      clock_gettime(clockId, &currentTimeP);
      long currentDuration = (currentTimeP.tv_sec - startTimeP.tv_sec) * 1000000000 + (currentTimeP.tv_nsec - startTimeP.tv_nsec);
      doAgain = (currentDuration < duration);
    }
  }
}


void high_ping_received(const void * pong_msg)
{
  burn_cpu_cycles(100000000);  // TODO: Get this value from parameter 'high_busyloop'.
  RCUNUSED(rcl_publish(&high_pong_publisher_, pong_msg, NULL));
}


void low_ping_received(const void * pong_msg)
{
  burn_cpu_cycles(100000000);  // TODO: Get this value from parameter 'low_busyloop'.
  RCUNUSED(rcl_publish(&low_pong_publisher_, pong_msg, NULL));
}


int main()
{
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rclc_support_t support;

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  rcl_node_t node = rcl_get_zero_initialized_node();
  RCCHECK(rclc_node_init_default(&node, "pong_rclc_node", "", &support));

  RCCHECK(rclc_publisher_init_best_effort(&high_pong_publisher_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "high_pong"));
  RCCHECK(rclc_subscription_init_best_effort(&high_ping_subscription_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "high_ping"));

  RCCHECK(rclc_publisher_init_best_effort(&low_pong_publisher_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "low_pong"));
  RCCHECK(rclc_subscription_init_best_effort(&low_ping_subscription_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "low_ping"));

  rclc_executor_t high_executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&high_executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_subscription(&high_executor, &high_ping_subscription_, &high_ping_msg_, &high_ping_received, ON_NEW_DATA));

  rclc_executor_t low_executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&low_executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_subscription(&low_executor, &low_ping_subscription_, &low_ping_msg_, &low_ping_received, ON_NEW_DATA));

  // TODO: Use RBS-enabled rclc Executor here:
  while(true) {
    rclc_executor_spin_some(&high_executor, 1000000);
    rclc_executor_spin_some(&low_executor, 1000000);
  }

  RCCHECK(rcl_subscription_fini(&high_ping_subscription_, &node));
  RCCHECK(rcl_publisher_fini(&high_pong_publisher_, &node));
  RCCHECK(rcl_subscription_fini(&low_ping_subscription_, &node));  
  RCCHECK(rcl_publisher_fini(&low_pong_publisher_, &node));

  RCCHECK(rcl_node_fini(&node));
}
