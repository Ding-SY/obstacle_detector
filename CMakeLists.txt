cmake_minimum_required(VERSION 2.8.3)
project(obstacle_detector)

set(CMAKE_CXX_FLAGS "-std=c++11 -fpermissive ${CMAKE_CXX_FLAGS} -Wfatal-errors\ ")

find_package(catkin REQUIRED COMPONENTS roscpp nodelet rviz std_msgs std_srvs geometry_msgs sensor_msgs tf message_generation)
find_package(Armadillo REQUIRED)
find_package(Boost 1.54.0 REQUIRED system)

add_message_files(FILES CircleObstacle.msg SegmentObstacle.msg Obstacles.msg)
generate_messages(DEPENDENCIES std_msgs geometry_msgs)

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES scans_merger obstacle_extractor obstacle_tracker obstacle_publisher obstacle_recorder
  CATKIN_DEPENDS roscpp nodelet rviz std_msgs std_srvs geometry_msgs sensor_msgs tf message_runtime
)

include_directories(include ${catkin_INCLUDE_DIRS} ${ARMADILLO_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

#
# Build libs
#
add_library(scans_merger
  src/scans_merger.cpp
  include/scans_merger.h
  include/utilities/math_utilities.h)
target_link_libraries(scans_merger ${catkin_LIBRARIES})
add_dependencies(scans_merger ${catkin_EXPORTED_TARGETS})

add_library(obstacle_extractor
  src/obstacle_extractor.cpp
  include/obstacle_extractor.h
  include/utilities/math_utilities.h
  include/utilities/point.h
  include/utilities/segment.h
  include/utilities/circle.h
  include/utilities/figure_fitting.h
  include/utilities/point_set.h)
target_link_libraries(obstacle_extractor ${catkin_LIBRARIES} ${ARMADILLO_LIBRARIES})
add_dependencies(obstacle_extractor ${catkin_EXPORTED_TARGETS} ${${PROJECT_NAME}_EXPORTED_TARGETS})

add_library(obstacle_tracker
  src/obstacle_tracker.cpp
  include/obstacle_tracker.h
  include/utilities/kalman.h
  include/utilities/tracked_obstacle.h)
target_link_libraries(obstacle_tracker ${catkin_LIBRARIES} ${ARMADILLO_LIBRARIES})
add_dependencies(obstacle_tracker ${catkin_EXPORTED_TARGETS} ${${PROJECT_NAME}_EXPORTED_TARGETS})

add_library(obstacle_publisher
    src/obstacle_publisher.cpp
    include/obstacle_publisher.h)
target_link_libraries(obstacle_publisher ${catkin_LIBRARIES})
add_dependencies(obstacle_publisher ${catkin_EXPORTED_TARGETS} ${${PROJECT_NAME}_EXPORTED_TARGETS})

add_library(obstacle_recorder
    src/obstacle_recorder.cpp
    include/obstacle_recorder.h)
target_link_libraries(obstacle_recorder ${catkin_LIBRARIES})
add_dependencies(obstacle_recorder ${catkin_EXPORTED_TARGETS} ${${PROJECT_NAME}_EXPORTED_TARGETS})

#
# Build nodes
#
add_executable(scans_merger_node src/nodes/scans_merger_node.cpp)
target_link_libraries(scans_merger_node scans_merger)

add_executable(obstacle_extractor_node src/nodes/obstacle_extractor_node.cpp)
target_link_libraries(obstacle_extractor_node obstacle_extractor)

add_executable(obstacle_tracker_node src/nodes/obstacle_tracker_node.cpp)
target_link_libraries(obstacle_tracker_node obstacle_tracker)

add_executable(obstacle_publisher_node src/nodes/obstacle_publisher_node.cpp)
target_link_libraries(obstacle_publisher_node obstacle_publisher)

add_executable(obstacle_recorder_node src/nodes/obstacle_recorder_node.cpp)
target_link_libraries(obstacle_recorder_node obstacle_recorder)

#
# Build nodelets
#
add_library(${PROJECT_NAME}_nodelets
  src/nodelets/scans_merger_nodelet.cpp
  src/nodelets/obstacle_extractor_nodelet.cpp
  src/nodelets/obstacle_tracker_nodelet.cpp
  src/nodelets/obstacle_publisher_nodelet.cpp
  src/nodelets/obstacle_recorder_nodelet.cpp)
target_link_libraries(${PROJECT_NAME}_nodelets scans_merger obstacle_extractor obstacle_tracker obstacle_publisher obstacle_recorder)

#
# Build rviz plugins
#
set(CMAKE_AUTOMOC ON)

if(rviz_QT_VERSION VERSION_LESS "5")
  message(STATUS "Using Qt4 based on the rviz_QT_VERSION: ${rviz_QT_VERSION}")
  find_package(Qt4 ${rviz_QT_VERSION} EXACT REQUIRED QtCore QtGui)
  include(${QT_USE_FILE})
else()
  message(STATUS "Using Qt5 based on the rviz_QT_VERSION: ${rviz_QT_VERSION}")
  find_package(Qt5 ${rviz_QT_VERSION} EXACT REQUIRED Core Widgets)
  set(QT_LIBRARIES Qt5::Widgets)
endif()

add_definitions(-DQT_NO_KEYWORDS)

add_library(${PROJECT_NAME}_gui
  src/displays/obstacles_display.cpp include/displays/obstacles_display.h
  src/displays/circle_visual.cpp include/displays/circle_visual.h
  src/displays/segment_visual.cpp include/displays/segment_visual.h
  #
  src/panels/scans_merger_panel.cpp include/panels/scans_merger_panel.h
  src/panels/obstacle_extractor_panel.cpp include/panels/obstacle_extractor_panel.h
  src/panels/obstacle_tracker_panel.cpp include/panels/obstacle_tracker_panel.h
  src/panels/obstacle_recorder_panel.cpp include/panels/obstacle_recorder_panel.h
  src/panels/obstacle_publisher_panel.cpp include/panels/obstacle_publisher_panel.h)
target_link_libraries(${PROJECT_NAME}_gui ${QT_LIBRARIES} ${catkin_LIBRARIES})
add_dependencies(${PROJECT_NAME}_gui ${catkin_EXPORTED_TARGETS})

#
# Create folders and copy resources
#
file(MAKE_DIRECTORY $ENV{HOME}/ObstacleDetector/records)
file(MAKE_DIRECTORY $ENV{HOME}/ObstacleDetector/resources)
file(COPY resources/play.png resources/stop.png DESTINATION $ENV{HOME}/ObstacleDetector/resources)
