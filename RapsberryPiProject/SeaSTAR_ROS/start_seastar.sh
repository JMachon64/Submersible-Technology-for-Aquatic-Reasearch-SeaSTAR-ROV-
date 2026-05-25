echo "Starting SeaSTAR software..."

source /opt/ros/jazzy/setup.bash #loads ROS.

cd /home/starfy/SeaSTAR_ROS/src #goes to UI folder.

python3 -m http.server 8000 & #starts UI.

ros2 launch rosbridge_server rosbridge_websocket_launch.xml & #starts rosbridge.
# python3 telemetry_recording.py & #start telemetry node.

python3 telemetry_recording.py & #start telemetry node.

python3 NavigationCamera/NavigationCameraManager_node.py & #start telemetry node.

python3 Uart/UartManager_node.py & #start telemetry node.

python3 ControlLoop_node.py & #start telemetry node.

python3 NodeMonitor_node.py &

python3 FSM_node.py & #start telemetry node.

echo "SeaSTAR is ONLINE!"

wait
