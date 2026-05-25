import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32MultiArray, String, Float32, Bool
import csv
import shutil
import time
from datetime import datetime
#-----------------------------------------------------------------------------------
class TelemetryRecorder(Node):
    def __init__(self):
        super().__init__('telemetry_recorder') #ros node name.
        self.recording = False
        self.file = None
        self.writer = None
        self.filename = None
        self.latest_filename = "/home/starfy/SeaSTAR_ROS/src/MissionTelemetryLogs/telemetry_latest.csv"

        #updated environmental values.
        self.temp = 0.0
        self.depth = 0.0
        self.pressure = 0.0
        self.env_time = 0.0

        #updated positional values.
        self.roll = 0.0
        self.pitch = 0.0
        self.yaw = 0.0
        self.pos_time = 0.0

        #updated power values.
        self.voltage = 0.0
        self.current = 0.0
        self.power = 0.0
        self.power_time = 0.0

        #updated leak value.
        self.leak_detected = False
#-----------------------------------------------------------------------------------
#ROS2 subscriptions 
        self.create_subscription( #subscribe to mission control for the "START_TELEMETRY and STOP_TELEMETRY"
            String,
            '/mission/control',
            self.mission_callback,
            10
        ) 
    
        self.create_subscription( #subscribe to /uart/telemetry_enviornmental.
            Float32MultiArray,
            '/uart/telemetry_environmental', #[temp, depth, pressure, time].
            self.environmental_callback,
            10
        )

        self.create_subscription( #subscribe to /uart/telemetry_positional.
            Float32MultiArray,
            '/uart/telemetry_positional', #[roll, pitch, yaw, time].
            self.positional_callback,
            10
        )

        self.create_subscription( #subscribe to uart/telemetry_power.
            Float32MultiArray,
            '/uart/telemetry_power', #[voltage, current, power, time].
            self.power_callback,
            10
        )

        self.create_subscription( #subscribe to /uart/sensordata with leak info (0 or 1).
            Float32MultiArray,
            '/uart/leak_sensor', #True or False.
            self.leak_callback,
            10
        )
        self.create_subscription(
            Float32MultiArray,
            '/uart/sample_telemetry',
            self.sample_callback,
            10
        )
#-----------------------------------------------------------------------------------
#ROS2 Publishers
        self.depth_pub = self.create_publisher( #publishes depth to the UI.
            Float32, 
            '/sensors/depth', 
            10
        ) 
        self.temp_pub = self.create_publisher( #publishes temperature to the UI.
            Float32, 
            '/sensors/temp', 
            10
        ) 
        self.pressure_pub = self.create_publisher( #publishes pressure to the UI.
            Float32, 
            '/sensors/pressure', 
            10
        ) 
        self.voltage_pub = self.create_publisher( #publishes voltage to the UI.
            Float32, 
            '/power/voltage', 
            10
        ) 
        self.current_pub = self.create_publisher( #publishes current to the UI.
            Float32, 
            '/power/current', 
            10
        ) 
        self.power_pub = self.create_publisher( #publishes power to the UI.
            Float32, 
            '/power/power', 
            10
        ) 
        self.roll_pub = self.create_publisher( #publishes roll to the UI.
            Float32, 
            '/position/roll', 
            10
        ) 
        self.pitch_pub = self.create_publisher( #publishes pitch to the UI.
            Float32, 
            '/position/pitch', 
            10
        ) 
        self.yaw_pub = self.create_publisher( #publishes yaw to the UI.
            Float32, 
            '/position/yaw', 
            10
        ) 
        self.leak_pub = self.create_publisher(
            Bool, 
            '/sensors/leak', 
            10
        )
        self.get_logger().info('Telemetry Recorder Ready.') #once it subscribed to these it is ready to go hehe.
#-----------------------------------------------------------------------------------
    def mission_callback(self, msg):
        #this function listens to /mission/control and when it hears start_telemetry it 
        #opens a csv file and starts recording data. When it hears stop_telemetry it stops
        #writing data and saves the file.
        if msg.data == "START_TELEMETRY":
            self.start_recording()
        elif msg.data == "STOP_TELEMETRY":
            self.stop_recording()
#-----------------------------------------------------------------------------------
    def start_recording(self): #start csv.
        if self.recording: #if already recording then it will say it's already recording and return that.
            #It will not start a second csv file.
            self.get_logger().info("Already recording.")
            return

        filename = datetime.now().strftime("Mission_Telemetry_DATE:%Y-%m-%d_TIME:%H-%M-%S.csv") #it should save the file with the current time. But this might be a problem since we are IN THE MIDDLE OF THE OCEAN!!!
        self.filename = f"/home/starfy/SeaSTAR_ROS/src/MissionTelemetryLogs/{filename}" #file path of where the csv file will be saved.

        self.file = open(self.filename, mode='w', newline='',) #opens csv file in write mode.
        self.writer = csv.writer(self.file) #rows for csv file.

        # self.writer.writerow([ #rows.
        #     "system_time",
        #     "env_time",
        #     "depth",
        #     "temp",
        #     "pressure",
        #     "pos_time",
        #     "roll",
        #     "pitch",
        #     "yaw",
        #     "power_time",
        #     "voltage",
        #     "current",
        #     "power",
        #     "leak"
        # ])

        self.writer.writerow([ #rows.
            "unix_time",
            "system_time",
            "sampling_time",
            "depth",
            "temp",
            "pressure",

        ])

        self.recording = True #we change this to true because it is now recording.
        self.get_logger().info(f"Recording started: {self.filename}") #prints to terminal.
#-----------------------------------------------------------------------------------
    def stop_recording(self): #stop csv
        if not self.recording: #if self recording is False, then return not recording.
            self.get_logger().info("Not recording.")
            return

        self.recording = False #if not come to this line, this line changes the state of recording from true to false.

        if self.file: #checks if the CSV file is currently open,
            self.file.close() #if so close. 

            #copies the timestamped csv file into telemetry_latest.csv so the UI download button always knows what file to download.
            shutil.copyfile(self.filename, self.latest_filename)

            self.file = None #clears the file variable.
            self.writer = None #clears writer variable 

        self.get_logger().info("Recording stopped.") #prints that the recording has stopped in the terminal.
        self.get_logger().info(f"Latest telemetry file updated: {self.latest_filename}") #prints where the file was saved.
#-----------------------------------------------------------------------------------
    def environmental_callback(self, msg):
        if len(msg.data) < 4:
            self.get_logger().warn("Not enough environmental values.") # expected: [temp, depth, pressure, time]
            return
        #if it does have all expected values then temp will be stored in index 0 and so on.
        self.temp = float(msg.data[0])
        self.depth = float(msg.data[1])
        self.pressure = float(msg.data[2])
        self.env_time = float(msg.data[3])

        self.depth_pub.publish(Float32(data=self.depth)) #publish depth even if csv recording is off.
        self.temp_pub.publish(Float32(data=self.temp)) #publish temp even if csv recording is off.
        self.pressure_pub.publish(Float32(data=self.pressure)) #publish pressure even if csv recording is off.

        self.write_csv_row() #if recording is true it will also write to the CSV file, if not then nothing happens.
#-----------------------------------------------------------------------------------
    def positional_callback(self, msg):
        if len(msg.data) < 4:
            self.get_logger().warn("Not enough positional values.") # expected: [roll, pitch, yaw, time]
            return

        self.roll = float(msg.data[0])
        self.pitch = float(msg.data[1])
        self.yaw = float(msg.data[2])
        self.pos_time = float(msg.data[3])

        self.roll_pub.publish(Float32(data=self.roll)) #publish roll even if csv recording is off.
        self.pitch_pub.publish(Float32(data=self.pitch)) #publish pitch even if csv recording is off.
        self.yaw_pub.publish(Float32(data=self.yaw)) #publish yaw even if csv recording is off.

        # self.write_csv_row()
#-----------------------------------------------------------------------------------
    def power_callback(self, msg):
        if len(msg.data) < 4:
            self.get_logger().warn("Not enough power values.") # expected: [voltage, current, power, time]
            return

        self.voltage = float(msg.data[0])
        self.current = float(msg.data[1])
        self.power = float(msg.data[2])
        self.power_time = float(msg.data[3])

        self.voltage_pub.publish(Float32(data=self.voltage)) #publish voltage even if csv recording is off.
        self.current_pub.publish(Float32(data=self.current)) #publish current even if csv recording is off.
        self.power_pub.publish(Float32(data=self.power)) #publish power even if csv recording is off.

        # self.write_csv_row()
#-----------------------------------------------------------------------------------
    def leak_callback(self, msg):
        if len(msg.data) < 1:
            self.get_logger().warn("Not enough leak values.") # expected: [leak]
            return
        #my toughts will echo your name, until i see you again
        #these r the words i held back as I was leaving too soon
        #all i can say is it was enchanting to meet you.

        #the lingering question kept me up, 2am WHO DO YOU LOVE?
        leak_raw = msg.data[0] #whatever.
        self.leak_detected = bool(leak_raw)

        self.leak_pub.publish(Bool(data=self.leak_detected)) #publish leak even if csv recording is off.

        # self.write_csv_row() #write if the state is true, if not, nothing happens.
#-----------------------------------------------------------------------------------
    def write_csv_row(self): #write rows.
        if not self.recording: #if recording is off do not write anything to the CSV.
            return
        
        if not self.writer: #do not write.
            return
        
        # timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S") #gets the date and time of the rasp pi.
        self.unix_time = time.time()
        self.readable_time = datetime.now().isoformat(timespec="milliseconds")

        # self.writer.writerow([
        #     unix_time,
        #     readable_time,
        #     self.env_time,
        #     self.depth,
        #     self.temp,
        #     self.pressure,
        #     self.pos_time,
        #     self.roll,
        #     self.pitch,
        #     self.yaw,
        #     self.power_time,
        #     self.voltage,
        #     self.current,
        #     self.power,
        #     self.leak_detected
        # ])

        self.writer.writerow([
            self.unix_time,
            self.readable_time,
            self.env_time,
            self.depth,
            self.temp,
            self.pressure,

        ])
        self.file.flush() #saves file immediately.
#-----------------------------------------------------------------------------------
def main(args=None):
    rclpy.init(args=args) #initialize ROS.
    node = TelemetryRecorder()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass

    if node.file:
        node.file.close()

    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
#-----------------------------------------------------------------------------------