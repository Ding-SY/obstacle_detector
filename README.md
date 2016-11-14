# The obstacle_detector package 

The `obstacle_detector` package provides utilities to detect and track obstacles from data provided by 2D laser scans. Detected obstacles come in a form of segments or circles. Detection of obstacles is based solely on the geometric information of the points from current scan. The tracking process solves the correspondence problem between two consecutive obstacle sets and uses Kalman filter to supersample and refine obstacles parameters. The package was designed for a robot equipped with two laser scanners therefore it contains several additional utilities.

The package requires [Armadillo C++](http://arma.sourceforge.net) library for compilation and runtime.

-----------------------
<p align="center">
  <img src="https://cloud.githubusercontent.com/assets/1482514/15776148/2fc8f610-2986-11e6-88ed-6c6142e87465.png" alt="Visual example of obstacle detector output."/>
  <br/>
  Fig. 1. Visual example of obstacle detector output.
</p>
-----------------------

1. The nodes 
  1.1 The scans_merger node 
  1.2 The obstacle_detector node 
  1.3 The obstacle_tracker node 
  1.4 The obstacle_visualizer node 
  1.5 The obstacle_recorder node 
  1.6 The obstacle_publisher node 
  1.7. The static_scan_publisher node 
2. The messages
4. Launch files


## 1. The nodes

The package provides separate nodes to perform separate tasks. In general solution the data is processed in a following manner:

`two laser scans` -> `scans merger` -> `merged scan or pcl` -> `obstacle detector` -> `obstacles` -> `obstacle tracker` -> `refined obstacles`

For some scenarios the pure obstacle detection (without tracking) might be sufficient. The nodes are configurable with the use of ROS parameter server. Any numerical parameters required by the nodes must be provided in SI units.

All of the nodes provide a local `params` service, which allows the process to get the latest parameters from the parameter server. 

All of the nodes can be either in active or in sleep mode triggered by setting appropriate variable in the parameter server and calling `params` service. In the sleep mode, any subscribers or publishers are shut down and the node does nothing until activated again.

### 1.1. The scans_merger node

This node converts two messages of type `sensor_msgs/LaserScan` from topics `front_scan` and `rear_scan` into a single laser scan of the same type, published under topic `scan` and/or a point cloud of type `sensor_msgs/PointCloud`, published under topic `pcl`. The difference between both is that the resulting laser scan divides the area into finite number of circular sectors and put one point (or actually one range value) in each section occupied by some measured points, whereas the resulting point cloud simply copies all of the points obtained from sensors.

The resulting messages contain geometric data described with respect to a specific coordinate frame (e.g. `scanners_base`). Assuming that the coordinate frames attached to two laser scanners are called `front_scanner` and `rear_scanner`, both transformation from `scanners_base` frame to `front_scanner` frame and from `scanners_base` frame to `rear_scanner` frame must be provided. The node allows to artificially restrict measured points to some rectangular region around the `scanners_base` frame as well as to limit the resulting laser scan range. The points falling behind this region or ranges excluded from the limit will be discarded.

Even if only one laser scanner is used, the node can be useful for simple data pre-processing, e.g. range restriction or recalculation of points to a different coordinate frame. The node uses the following set of local parameters:

* `~active` (bool, default: true) - active/sleep mode,
* `~publish_scan` (bool, default: true) - publish the merged laser scan message,
* `~publish_pcl` (bool, default: true) - publish the merged point cloud message,
* `~ranges_num` (int, default: 1000) - number of ranges (circular sectors) contained in the 360 deg laser scan message,
* `~min_scanner_range` (double, default: 0.05) - minimal allowable range value for produced laser scan message,
* `~max_scanner_range` (double, default: 10.0) - maximal allowable range value for produced laser scan message,
* `~max_x_range` (double, default: 10.0) - limitation for points coordinates (points with coordinates behind these limitations will be discarded),
* `~min_x_range` (double, default: -10.0) - as above,
* `~max_y_range` (double, default: 10.0) - as above,
* `~min_y_range` (double, default: -10.0) - as above,
* `~frame_id` (string, default: scanners_base) - name of the coordinate frame used as the origin for the produced laser scan or point cloud.

-----------------------
<p align="center">
  <img src="https://cloud.githubusercontent.com/assets/1482514/16087445/4af50edc-3323-11e6-88c7-c7ee12b6d63b.gif" alt="Visual example of obstacle detector output"/>
  <br/>
  Fig. 2. Visual example of scans merging with coordinates restrictions.
</p>
-----------------------

### 1.2. The obstacle_detector node 

This node converts messages of type `sensor_msgs/LaserScan` from topic `scan` or messages of type `sensor_msgs/PointCloud` from topic `pcl` into obstacles which are published as messages of custom type `obstacles_detector/Obstacles` under topic `raw_obstacles`. The point cloud message must be ordered in the angular fashion, because the algorithm exploits the polar nature of laser scanners. 

The input points are firstly grouped into subsets. The algorithm extracts segments from each points subset. Next, the segments are checked for possible merging between each other. The circular obstacles are extracted from segments and also checked for possible merging. Resulting set of obstacles can be described with respect to coordinate frame provided by the input messages or to other, custom coordinate frame.

The node is configurable with the following set of local parameters:

* `~active` (bool, default: true) - active/sleep mode,
* `~use_scan` (bool, default: true) - use laser scan messages,
* `~use_pcl` (bool, default: false) - use point cloud messages (if both scan and pcl are chosen, scans will have priority),
* `~discard_converted_segments` (bool, default: true) - do not publish segments, from which the circles were spawned,
* `~transform_coordinates` (bool, default: true) - transform the coordinates of obstacles to a frame described with `frame_id` parameter,
* `~frame_id` (string, default: world) - name of coordinate frame used as origin for produced obstacles (used only if `transform_coordinates` flag is set to true).

The following set of local parameters is dedicated to the algorithm itself:

* `~use_split_and_merge` (bool, default: false) - choose wether to use Iterative End Point Fit (false) or Split And Merge (true) algorithm to detect segments,
* `~min_group_points` (int, default: 5) - minimum number of points comprising a group to be further processed,
* `~max_group_distance` (double, default: 0.100) - if the distance between two points is greater than this value, start a new group,
* `~distance_proportion` (double, default: 0.006136) - enlarge the allowable distance between points proportionally to the range of point (use scan angle increment in radians),
* `~max_split_distance` (double, default: 0.100) - if a point in group lays further from a leading line than this value, split the group, 
* `~max_merge_separation` (double, default: 0.100) - if distance between obstacles is smaller than this value, consider merging them,
* `~max_merge_spread` (double, default: 0.070) - merge two segments if all of their extreme points lay closer to the leading line than this value,
* `~max_circle_radius` (double, default: 0.200) - if a circle would have greater radius than this value, skip it, 
* `~radius_enlargement` (double, default: 0.020) - artificially enlarge the circles radius by this value.

-----------------------
<p align="center">
  <img src="https://cloud.githubusercontent.com/assets/1482514/16087483/63733baa-3323-11e6-8a72-f9e17b6691d5.gif" alt="Visual example of obstacle detector output"/>
  <br/>
  Fig. 3. Visual example of obstacles detection.
</p>
-----------------------

### 1.3. The obstacle_tracker node

This node tracks and filters the circular obstacles with the use of Kalman filter. It subscribes to messages of custom type `obstacle_detector/Obstacles` from topic `raw_obstacles` and publishes messages of the same type under topic `tracked_obstacles`. The tracking algorithm is applied only to the circular obstacles, hence the segments list in the published message is simply a copy of the original segments. In fact, published messages contain both tracked and untracked circular obstacles. One can distinguish them by checking the `bool tracked` field of the `obstacle_detector/CircleObstacle` message type. 

The node works in a synchronous manner with the default rate of 100 Hz. If detected obstacles are published less often, the tracker will supersample them and smoothen their position and radius. The following set of local parameters can be used to tune the node:

* `~active` (bool, default: true) - active/sleep mode,
* `~copy_segments` (bool, default: true) - copy detected segments into tracked obstacles message,
* `~loop_rate` (double, default: 100.0) - the main loop rate in Hz,
* `~tracking_duration` (double, default: 2.0) - the duration of obstacle tracking in the case of lack of incomming data (after this time from the last corresponding measurement the tracked obstacle will be removed from the list),
* `~min_correspondence_cost` (double, default 0.3) - a threshold for correspondence test,
* `~std_correspondence_dev` (double, default 0.15) - (experimental) standard deviation of the position ellipse in the correspondence test,
* `~process_variance` (double, default 0.01) - variance of obstacles position and radius (parameter of Kalman Filter),
* `~process_rate_variance` (double, default 0.1) - variance of rate of change of obstacles values (parameter of Kalman Filter),
* `~measurement_variance` (double, default 1.0) - variance of measured obstacles values (parameter of Kalman Filter).

-----------------------
<p align="center">
  <img src="https://cloud.githubusercontent.com/assets/1482514/16087421/32d1f52c-3323-11e6-86bb-c1ac851d1b77.gif" alt="Visual example of obstacle detector output"/>
  <br/>
  Fig. 4. Visual example of obstacle tracking.
</p>
-----------------------

### 1.4. The obstacle_visualizer node

The auxiliary node which converts messages of type `obstacles_detector/Obstacles` from topic `obstacles` into Rviz markers of type `visualization_msgs/MarkerArray`, published under topic `obstacles_markers`. The node uses few local parameters to customize the markers:

* `~active` (bool, default: true) - active/sleep mode,
* `~show_labels` (bool, default: false) - (experimental) publish the markers with labels provided by obstacle tracker node,
* `~tracked_circles_color` (int, default: 1) - a color code for tracked circular obstacles (0: black, 1: white, 2: red, 3: green, 4: blue, 5: yellow, 6: magenta, 7: cyan),
* `~untracked_circles_color` (int, default: 1) - as above but for untracked circular obstacles, 
* `~segments_color` (int, default: 1) - as above but for segment obstacles,
* `~text_color` (int, default: 1) - as above but for names of tracked obstacles,
* `~alpha` (double, default: 1.0) - alpha (opacity) value,
* `~z_layer` (double, default: 0.0) - position of obstacles in Z axis.

### 1.5. The obstacle_recorder node

The auxiliary node which enables obstacles recording by saving the data into text file. To start recording the data one must set the appropriate parameter and call the `params` service of the node. The node is configurable with the following parameters:

* `~active` (bool, default: true) - active/sleep mode,
* `~recording` (bool, default: false) - trigger for data recording,

* `~min_x_limit` (double, default: -10.0) - restriction on obstacles coordinates (in their frame of reference); obstacles with center points falling behind the restriction will be discarded,
* `~max_x_limit` (double, default: 10.0) - as above,
* `~min_y_limit` (double, default: -10.0) - as above,
* `~max_y_limit` (double, default: 10.0) - as above,

* `~filename_prefix` (string, default: "tracked_") - prefix for text files produced by the recorder.

Although the parameter is dedicated to text files, it also changes the service name (e.g. `/raw_recording_trigger`).

### 1.6. The obstacle_publisher node

The auxiliary node which allows to publishes a set of virtual, circular obstacles in the form of message of type `obstacles_detector/Obstacles` under topic `obstacles`. The node is mostly used for off-line tests. The following parameters are used to configure the node:

* `~active` (bool, default: true) - active/sleep mode,
* `~reset` (bool, default: false) - reset time for obstacles motion calculation,
* `~loop_rate` (double, default: 10.0) - the main loop rate in Hz.

The following parameters are used to provide the node with a set of obstacles:

* `~x_vector` (std::vector<double>, default: []) - the array of x-coordinates of obstacles center points,
* `~y_vector` (std::vector<double>, default: []) - the array of y-coordinates of obstacles center points,
* `~r_vector` (std::vector<double>, default: []) - the array of obstacles radii,
* `~x_vector` (std::vector<double>, default: []) - the array of velocities of obstacles center points in x direction,
* `~y_vector` (std::vector<double>, default: []) - the array of velocities of obstacles center points in y direction.

### 1.7. The static_scan_publisher node

The auxiliary node which imitates a laser scanner and publishes a static, 360 deg laser scan of type `sensor_msgs/LaserScan` under topic `scan`. The node is mostly used for off-line tests.

## 2. The messages

The package provides three custom message types. All of their numerical values are provided in SI units.

* `CircleObstacle`
    - `geometry_msgs/Point center` - center of circular obstacle,
    - `geometry_msgs/Vector3 velocity` - linear velocity of circular obstacle,
    - `float64 radius` - radius of circular obstacle,
    - `string obstacle_id` - id of circular obstacle (only tracked),
    - `bool tracked` - flag representing whether the circular obstacle is tracked.
* `SegmentObstacle`
    - `geometry_msgs/Point first_point` - first point of the segment (in counter-clockwise direction),
    - `geometry_msgs/Point last_point` - end point of the segment.
* `Obstacles`
    - `Header header`
    - `obstacle_detector/SegmentObstacle[] segments`
    - `obstacle_detector/CircleObstacle[] circles`

## 3. The launch files

Provided launch files are good examples of how to use obstacle_detector package. They give a full list of parameters used by each of provided nodes.

* `single_scanner.launch` - Starts a single `hokuyo_node` to obtain laser scans from Hokuyo device, a `laser_scan_matcher_node` or `static_transform_publisher` to provide appropriate transformation from global to local coordinate frame, `obstacle_detector`, `obstacle_tracker` and `obstacle_visualizator` nodes, as well as `rviz` with provided configuration file.
* `two_scanners.launch` - Starts two `hokuyo_node`s, assuming that the udev configuration provides links to both devices (if not, familiarize with the description in the `resources/` folder or change the devices names to `/dev/ttyACM0` and `/dev/ttyACM1` appropriately), provides appropriate transformations with `laser_scan_matcher_node` or `static_transform_publisher`s, uses `scans_merger` to convert both scans into pcl, and runs `obstacle_detector`, `obstacle_tracker` and `obstacle_visualizator` nodes as well as `rviz`.
* `virtual_scanner` - Used for debug and tests purposes. Starts a `static_scan_publisher`, provides global to local transformation and runs `obstacle_detector`, `obstacle_tracker`, `obstacle_visualizator` nodes and `rviz`.
* `virtual_obstacles` - Used for debug and tests purposes. Starts a `virtual_obstacles_publisher`, provides global to local transformation and runs `obstacle_tracker`, `obstacle_visualizator` nodes and `rviz`.

Author:
_Mateusz Przybyla_

