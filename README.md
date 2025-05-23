# Client Template for ROS2 Jazzy

This template demonstrates how ROS2 packages can be used on the make87 platform.

## Important Note

Migrating ROS2 packages is possible with the make87 platform. However, we recommend to properly migrate
the package logic into a new make87 package to ensure compatibility with the make87 platform. ROS2 applications are
significantly larger and will consume a lot more resources on nodes.

## Required Files

### ros2_run

This is a bash script which is executed when the application starts. Include any necessary commands to run the ROS2
package(s). Do not source the ROS2 workspace in this script, this is handled outside already.
