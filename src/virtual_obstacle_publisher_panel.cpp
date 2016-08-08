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

#include "../include/virtual_obstacle_publisher_panel.h"

namespace obstacle_detector
{

VirtualObstaclePublisherPanel::VirtualObstaclePublisherPanel(QWidget* parent) : rviz::Panel(parent), nh_(""), nh_local_("virtual_obstacle_publisher") {
  params_cli_ = nh_local_.serviceClient<std_srvs::Empty>("params");
  getParams();

  activate_checkbox_ = new QCheckBox("On/Off");
  obstacles_list_    = new QListWidget(this);
  add_button_        = new QPushButton("Add");
  reset_button_      = new QPushButton("Reset");
  x_input_           = new QLineEdit("0.0");
  y_input_           = new QLineEdit("0.0");
  r_input_           = new QLineEdit("0.0");
  vx_input_          = new QLineEdit("0.0");
  vy_input_          = new QLineEdit("0.0");

  obstacles_list_->setSelectionMode(QAbstractItemView::NoSelection);
  new QListWidgetItem("x: ", obstacles_list_);
  new QListWidgetItem("z: ", obstacles_list_);

  QFrame* lines[3];
  for (auto& line : lines) {
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
  }

  QSpacerItem* margin = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);

  QHBoxLayout* xyr_layout = new QHBoxLayout;
  xyr_layout->addItem(margin);
  xyr_layout->addWidget(new QLabel("x:"));
  xyr_layout->addWidget(x_input_);
  xyr_layout->addWidget(new QLabel("m, "));
  xyr_layout->addWidget(new QLabel("y:"));
  xyr_layout->addWidget(y_input_);
  xyr_layout->addWidget(new QLabel("m, "));
  xyr_layout->addWidget(new QLabel("r:"));
  xyr_layout->addWidget(r_input_);
  xyr_layout->addWidget(new QLabel("m"));
  xyr_layout->addItem(margin);

  QHBoxLayout* vxvy_layout = new QHBoxLayout;
  vxvy_layout->addItem(margin);
  vxvy_layout->addWidget(new QLabel("v<sub>x</sub>:"));
  vxvy_layout->addWidget(vx_input_);
  vxvy_layout->addWidget(new QLabel("m/s, "));
  vxvy_layout->addWidget(new QLabel("v<sub>y</sub>:"));
  vxvy_layout->addWidget(vy_input_);
  vxvy_layout->addWidget(new QLabel("m/s"));
  vxvy_layout->addItem(margin);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(activate_checkbox_);
  layout->addWidget(lines[0]);
  layout->addLayout(xyr_layout);
  layout->addLayout(vxvy_layout);
  layout->addWidget(add_button_, Qt::AlignCenter);
  layout->addWidget(lines[1]);
  layout->addWidget(obstacles_list_);
  layout->addWidget(lines[2]);
  layout->addWidget(reset_button_, Qt::AlignCenter);
  layout->setAlignment(layout, Qt::AlignCenter);
  setLayout(layout);

  connect(activate_checkbox_, SIGNAL(clicked()), this, SLOT(processInputs()));
  connect(add_button_, SIGNAL(clicked()), this, SLOT(addObstacle()));
  connect(reset_button_, SIGNAL(clicked()), this, SLOT(processInputs()));

  evaluateParams();
}

void VirtualObstaclePublisherPanel::processInputs() {
  verifyInputs();
  setParams();
  evaluateParams();
  notifyParamsUpdate();
}

void VirtualObstaclePublisherPanel::addObstacle() {
  verifyInputs();

  p_x_vector_.push_back(x_);
  p_y_vector_.push_back(y_);
  p_r_vector_.push_back(r_);

  p_vx_vector_.push_back(vx_);
  p_vy_vector_.push_back(vy_);

  setParams();
  evaluateParams();
  notifyParamsUpdate();
}

void VirtualObstaclePublisherPanel::verifyInputs() {
  p_active_ = activate_checkbox_->isChecked();

  try { x_ = boost::lexical_cast<double>(x_input_->text().toStdString()); }
  catch(boost::bad_lexical_cast &) { x_ = 0.0; x_input_->setText("0.0"); }

  try { y_ = boost::lexical_cast<double>(y_input_->text().toStdString()); }
  catch(boost::bad_lexical_cast &) { y_ = 0.0; y_input_->setText("0.0"); }

  try { r_ = boost::lexical_cast<double>(r_input_->text().toStdString()); }
  catch(boost::bad_lexical_cast &) { r_ = 0.0; r_input_->setText("0.0"); }

  try { vx_ = boost::lexical_cast<double>(vx_input_->text().toStdString()); }
  catch(boost::bad_lexical_cast &) { vx_ = 0.0; vx_input_->setText("0.0"); }

  try { vy_ = boost::lexical_cast<double>(vy_input_->text().toStdString()); }
  catch(boost::bad_lexical_cast &) { vy_ = 0.0; vy_input_->setText("0.0"); }
}

void VirtualObstaclePublisherPanel::setParams() {
  nh_local_.setParam("active", p_active_);
  nh_local_.setParam("reset", p_reset_);

  nh_local_.setParam("x_vector", p_x_vector_);
  nh_local_.setParam("y_vector", p_y_vector_);
  nh_local_.setParam("r_vector", p_r_vector_);

  nh_local_.setParam("vx_vector", p_vx_vector_);
  nh_local_.setParam("vy_vector", p_vy_vector_);
}

void VirtualObstaclePublisherPanel::getParams() {
  nh_local_.param<bool>("active", p_active_, false);
  nh_local_.param<bool>("reset", p_reset_, false);

  nh_local_.getParam("x_vector", p_x_vector_);
  nh_local_.getParam("y_vector", p_y_vector_);
  nh_local_.getParam("r_vector", p_r_vector_);

  nh_local_.getParam("vx_vector", p_vx_vector_);
  nh_local_.getParam("vy_vector", p_vy_vector_);
}

void VirtualObstaclePublisherPanel::evaluateParams() {
  activate_checkbox_->setChecked(p_active_);

  add_button_->setEnabled(p_active_);
  reset_button_->setEnabled(p_active_);

  x_input_->setEnabled(p_active_);
  y_input_->setEnabled(p_active_);
  r_input_->setEnabled(p_active_);

  vx_input_->setEnabled(p_active_);
  vy_input_->setEnabled(p_active_);

  if (p_x_vector_.size() < p_y_vector_.size() || p_x_vector_.size() < p_r_vector_.size() ||
      p_x_vector_.size() < p_vx_vector_.size() || p_x_vector_.size() < p_vy_vector_.size())
    return;

  for (int idx = 0; idx < p_x_vector_.size(); ++idx) {
    new QListWidgetItem("x: " + QString::number(p_x_vector_[idx]) + " m " +
                        "y: " + QString::number(p_y_vector_[idx]) + " m " +
                        "r: " + QString::number(p_r_vector_[idx]) + " m " +
                        "v<sub>x</sub>:" + QString::number(p_vx_vector_[idx]) + " m/s " +
                        "v<sub>y</sub>:" + QString::number(p_vx_vector_[idx]) + " m/s "
                        , obstacles_list_);
  }
}

void VirtualObstaclePublisherPanel::notifyParamsUpdate() {
  std_srvs::Empty empty;
  if (!params_cli_.call(empty)) {
    p_active_ = false;
    setParams();
    notifyParamsUpdate();
  }
}

void VirtualObstaclePublisherPanel::save(rviz::Config config) const {
  rviz::Panel::save(config);
}

void VirtualObstaclePublisherPanel::load(const rviz::Config& config) {
  rviz::Panel::load(config);
}

} // end namespace obstacle_detector

#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(obstacle_detector::VirtualObstaclePublisherPanel, rviz::Panel)
