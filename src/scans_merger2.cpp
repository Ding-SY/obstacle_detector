/*
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2015, Poznan University of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Author: Mateusz Przybyla
 */

#include "../include/scans_merger2.h"

using namespace obstacle_detector;

ScansMerger::ScansMerger() : nh_(""), nh_local_("~") {
  nh_local_.param<std::string>("base_frame", p_frame_id_, "scanner_base");

  nh_local_.param<bool>("omit_overlapping_scans", p_omit_overlapping_scans_, true);

  nh_local_.param<double>("max_scanner_range", p_max_scanner_range_, 10.0);
  nh_local_.param<double>("max_x_range", p_max_x_range_,  10.0);
  nh_local_.param<double>("min_x_range", p_min_x_range_, -10.0);
  nh_local_.param<double>("max_y_range", p_max_y_range_,  10.0);
  nh_local_.param<double>("min_y_range", p_min_y_range_, -10.0);

  front_scan_sub_ = nh_.subscribe("front_scan", 10, &ScansMerger::frontScanCallback, this);
  rear_scan_sub_ = nh_.subscribe("rear_scan", 10, &ScansMerger::rearScanCallback, this);
  pcl_pub_ = nh_.advertise<sensor_msgs::PointCloud>("pcl", 10);

  front_scan_received_ = false;
  rear_scan_received_ = false;

  unreceived_front_scans_ = 0;
  unreceived_rear_scans_ = 0;

  ROS_INFO("Scans Merger [OK]");
  ros::spin();
}

void ScansMerger::frontScanCallback(const sensor_msgs::LaserScan::ConstPtr& front_scan) {
  try {
    geometry_msgs::Point32 local_point, base_point;
    tf::StampedTransform transform;
    front_tf_.lookupTransform(p_frame_id_, front_scan->header.frame_id, ros::Time(0), transform);

    float phi = front_scan->angle_min;
    for (const float r : front_scan->ranges) {
      if (r > front_scan->range_min && r < front_scan->range_max && r <= p_max_scanner_range_) {
        local_point.x = r * cos(phi);
        local_point.y = r * sin(phi);

        base_point = transformPoint(local_point, transform);

        if (!(p_omit_overlapping_scans_ && base_point.x < 0.0) && checkPointInLimits(base_point, p_min_x_range_, p_max_x_range_, p_min_y_range_, p_max_y_range_))
          pcl_msg_.points.push_back(base_point);
      }
      phi += front_scan->angle_increment;
    }

    front_scan_received_ = true;

    if (rear_scan_received_ || unreceived_rear_scans_ > 0) {
      publishAll();

      unreceived_front_scans_ = 0;
    }
    else unreceived_rear_scans_++;
  }
  catch (tf::TransformException ex) {
    ROS_ERROR("%s",ex.what());
  }
}

void ScansMerger::rearScanCallback(const sensor_msgs::LaserScan::ConstPtr& rear_scan) {
  try {
    geometry_msgs::Point32 local_point, base_point;
    tf::StampedTransform transform;
    rear_tf_.lookupTransform(p_frame_id_, rear_scan->header.frame_id, ros::Time(0), transform);

    float phi = rear_scan->angle_min;
    for (const float r : rear_scan->ranges) {
      if (r > rear_scan->range_min && r < rear_scan->range_max && r <= p_max_scanner_range_) {
        local_point.x = r * cos(phi);
        local_point.y = r * sin(phi);

        base_point = transformPoint(local_point, transform);

        if (!(p_omit_overlapping_scans_ && base_point.x > 0.0) && checkPointInLimits(base_point, p_min_x_range_, p_max_x_range_, p_min_y_range_, p_max_y_range_))
          pcl_msg_.points.push_back(base_point);
      }
      phi += rear_scan->angle_increment;
    }

    rear_scan_received_ = true;

    if (front_scan_received_ || unreceived_front_scans_ > 0) {
      publishAll();

      unreceived_rear_scans_ = 0;
    }
    else unreceived_front_scans_++;
  }
  catch (tf::TransformException ex) {
    ROS_ERROR("%s",ex.what());
  }
}

void ScansMerger::publishAll() {
  pcl_msg_.header.frame_id = p_frame_id_;
  pcl_msg_.header.stamp = ros::Time::now();
  pcl_pub_.publish(pcl_msg_);

  pcl_msg_.points.clear();

  front_scan_received_ = false;
  rear_scan_received_ = false;
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "scans_merger");
  ScansMerger SM;
  return 0;
}
