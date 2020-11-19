// Copyright 2020 Intelligent Robotics Lab
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

#ifndef MROS2_WRAPPER__MROS2_WRAPPER_HPP_
#define MROS2_WRAPPER__MROS2_WRAPPER_HPP_


#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"

namespace mros2_wrapper
{

using std::placeholders::_1;
using std::placeholders::_2;

template<class Ros2ActionT, class Mros2ActionT>
class Mros2Wrapper : public rclcpp_lifecycle::LifecycleNode
{
public:
  using Ptr = std::shared_ptr<Mros2Wrapper<Ros2ActionT, Mros2ActionT>>;
  using GoalHandleMros2 = rclcpp_action::ServerGoalHandle<Mros2ActionT>;
  typedef std::function<void ()> ExecuteCallback;

  Mros2Wrapper(
    const std::string & node_name,
    const std::string & action_name,
    const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : LifecycleNode(node_name, options),
    action_name_(action_name),
    execute_callback_([]{})
  {
  }

  Mros2Wrapper(
    const std::string & node_name,
    const std::string & action_name,
    ExecuteCallback execute_callback,
    const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : LifecycleNode(node_name, options),
    action_name_(action_name), 
    execute_callback_(execute_callback)
  {
  }

protected:
  std::shared_ptr<rclcpp_action::ServerGoalHandle<Mros2ActionT>> current_handle_;
  std::future<void> execution_future_;
  std::string action_name_;
  ExecuteCallback execute_callback_;

  typename rclcpp_action::Server<Mros2ActionT>::SharedPtr mros2_action_server_;

  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID & /*uuid*/,
    std::shared_ptr<const typename Mros2ActionT::Goal> /*goal*/)
  {
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleMros2>/*goal_handle*/)
  {
    RCLCPP_INFO(this->get_logger(), "Received request to cancel goal");
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandleMros2> goal_handle)
  {
    current_handle_ = goal_handle;
    execution_future_ = std::async(std::launch::async, [this]() {work();});
  }  

  void work()
  {
    auto start = now();
    while (rclcpp::ok() && (now() - start).seconds() < 1 && is_active(current_handle_)) {
      execute_callback_();
    }
    std::cerr << "Finished action" << std::endl;  
  }

  constexpr bool is_active(
    const std::shared_ptr<GoalHandleMros2> handle) const
  {
    return handle != nullptr && handle->is_active();
  }

  using CallbackReturnT =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  CallbackReturnT on_configure(const rclcpp_lifecycle::State &)
  {
    return CallbackReturnT::SUCCESS;
  }

  CallbackReturnT on_activate(const rclcpp_lifecycle::State &)
  {
    mros2_action_server_ = rclcpp_action::create_server<Mros2ActionT>(
      shared_from_this(),
      action_name_ + "_qos",
      std::bind(&Mros2Wrapper::handle_goal, this, _1, _2),
      std::bind(&Mros2Wrapper::handle_cancel, this, _1),
      std::bind(&Mros2Wrapper::handle_accepted, this, _1));

    std::cerr << "Action server created" << std::endl;
    return CallbackReturnT::SUCCESS;
  }

  CallbackReturnT on_deactivate(const rclcpp_lifecycle::State &)
  {
    return CallbackReturnT::SUCCESS;
  }

  CallbackReturnT on_cleanup(const rclcpp_lifecycle::State &)
  {
    return CallbackReturnT::SUCCESS;
  }

  CallbackReturnT on_shutdown(const rclcpp_lifecycle::State &)
  {
    return CallbackReturnT::SUCCESS;
  }

  CallbackReturnT on_error(const rclcpp_lifecycle::State &)
  {
    return CallbackReturnT::SUCCESS;
  }
};

}  // namespace mros2_wrapper

#endif  // MROS2_WRAPPER__MROS2_WRAPPER_HPP_