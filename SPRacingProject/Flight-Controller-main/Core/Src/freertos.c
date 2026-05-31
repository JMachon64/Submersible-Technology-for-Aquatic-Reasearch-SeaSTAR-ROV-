/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <pwm.h>
#include <stdint.h>
#include <stdio.h>
#include <usart.h>
#include <i2c.h>
#include "MPU6050.h"
#include "Uart.h"
#include "UartProtocol.h"
#include "TelemetryStream.h"
#include "FSM.h"
#include "ControlLoop.h"
#include "TSYS01TemperatureSensor.h"
#include "MS5837PressureSensor.h"
#include "LeakDetection.h"
#include "tim.h"
#include "TelemetryStream.h"
#include "MPU6050.h"
#include "HMC5883L.h"
#include "Fusion.h"
#include "FusionMath.h"
#include "acs37800.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

TSY01_TemperatureSensor_t temp_sensor;
MS5837_PressureSensor_t pressure_sensor;
extern volatile control_command_t thruster_command;
extern environmental_packet_t environmetal_telemetry_packet;
uint8_t uart2_rx_byte;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MAX_ROLL 30.0f
#define MAX_PITCH 30.0f
#define THRUSTER_SAMPLE_RATE 50


void I2C_Scan(void);

void I2C_Scan(void)
{
    printf("Scanning I2C bus...\r\n");

    for (uint8_t addr = 1; addr < 128; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 2, 10) == HAL_OK)
        {
            printf("Found device at 0x%02X\r\n", addr);
        }
    }

    printf("Scan complete.\r\n");
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

osThreadId_t SamplingTaskHandle;
const osThreadAttr_t SamplingTask_attributes = {
  .name = "SamplingTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t UartStreamTaskHandle;
const osThreadAttr_t UartStreamTask_attributes = {
  .name = "UartStreamTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};

osThreadId_t TelemetryStreamTaskHandle;
const osThreadAttr_t TelemetryStreamTask_attributes = {
  .name = "TelemetryStreamTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityHigh2,
};

osThreadId_t StateMachineTaskHandle;
const osThreadAttr_t StateMachineTask_attributes = {
  .name = "StateMachineTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};

osThreadId_t SystemHealthMonitorTask;
const osThreadAttr_t SystemHealthMonitorTask_attributes = {
  .name = "SystemHealthMonitorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh4,
};

osThreadId_t PropulsionTaskHandle;
const osThreadAttr_t PropulsionTask_attributes = {
  .name = "PropulsionTask",
  .stack_size = 4094  * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */

osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartSamplingTask(void *argument);
void StartWiredUARTStream(void *argument);
void StartStateMachineTask(void *argument);
void StartTelemetryStreamTask(void *argument);
void StartSystemHealthMonitorTask(void *argument);
void Propulsion_Task(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */

void MX_FREERTOS_Init(void) {

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
   //defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

   //SamplingTaskHandle = osThreadNew(StartSamplingTask, NULL, &SamplingTask_attributes);
   UartStreamTaskHandle      = osThreadNew(StartWiredUARTStream, NULL, &UartStreamTask_attributes);
   StateMachineTaskHandle    = osThreadNew(StartStateMachineTask, NULL, &StateMachineTask_attributes);
   SystemHealthMonitorTask   = osThreadNew(StartSystemHealthMonitorTask, NULL, &SystemHealthMonitorTask_attributes);
   //TelemetryStreamTaskHandle = osThreadNew(StartTelemetryStreamTask, NULL, &TelemetryStreamTask_attributes);
   PropulsionTaskHandle      = osThreadNew(Propulsion_Task, NULL, &PropulsionTask_attributes);

  /* USER CODE END RTOS_THREADS */
  if(StateMachineTaskHandle ==  NULL){
    printf("STATE MACHINE FAILED \n");
  }
  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */


void StartWiredUARTStream(void *argument)
{
    UART3_INIT_RxDMA();
    printf("\r\n");
    printf("|Uart Task has been initialized...|\r\n");
    printf("\r\n");

    Packet_Sent_t rxPacket;

    for (;;)
    {

        UART3_Traverse_RxDMA();

 
        if (environmentalsampleflag && baseline_set)   //Baseline set means the pressure sensor has been callibrated around the initial atmospheric pressure readings 
        {

            environmentalsampleflag = 0;

            environmetal_telemetry_packet.depth       = (int32_t)(pressure_sensor.Depth_m * 1000.0f);
            environmetal_telemetry_packet.pressure_Pa = (int32_t)(pressure_sensor.Pressure_Pa) + pressure_offset;
            environmetal_telemetry_packet.temp        = (int32_t)(temp_sensor.Temperature_C * 1000.00f);

            TelemetryStream_SendEnvironmental(&environmetal_telemetry_packet);

        }
        else if (orientationsampleflag && baseline_set)
        {
            orientationsampleflag = 0;
            TelemetryStream_SendOrientation(&positional_telemetry_packet);
        }
        else if (powersampleflag && baseline_set)
        {
            powersampleflag = 0;
            TelemetryStream_SendPowerStatus(&power_telemetry_packet);
        }
        else if (BuildRxPacket(&rxPacket, 0))
        {
            Protocol_ParsePacket(&rxPacket);
        }
        
        Protocol_UpdateThroughput(); //count the throughput 

        osDelay(1);
    }
  /* USER CODE END 5 */
}

void StartStateMachineTask(void *argument)
{
  /* USER CODE BEGIN 5 */

  printf("\r\n");
  printf("|State Machine Task has been initialized...|\r\n");
  printf("\r\n");

  /* Infinite loop */
  for(;;)
  {
    SeaSTAR_FSM();
    osDelay(500);
  }
  /* USER CODE END 5 */

}

void StartSystemHealthMonitorTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  printf("\r\n");
  printf("|Initializing system health monitor task...|\r\n");
  printf("\r\n");
    PWM_Init();
  /* Infinite loop */
  for(;;)
  {
    //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_4);  //TEST THE NAV LIGHTS BY TOGGLING
   
    //MONITOR LEAKS 
    if(!leakLatched){ LeakSensor_Read();}


    //MONITOR COMMUNICATION STATUS 
    if(heartbeat_enabled ==  1){
          communication_status();   
    }

    // //MONITOR THRUSTER CURRENT


    osDelay(1000);
  }

  /* USER CODE END 5 */

}


void Propulsion_Task(void *argument){
  PWM_Init();

  PWM_SampleClosedPosition();
  Control_Update_Command(0,0,0,0,0);
  Init_TSYS01(&temp_sensor, &hi2c1);
  Init_MS5837(&pressure_sensor, &hi2c1);

  printf("\r\n");
  printf("|Sensor and Propulsion Task has been initialized...|\r\n");
  printf("\r\n");

  acs37800_t currentSensor = {
    .divRes = 2000000,
    .i2c_address = 0x60,
    .i2c_device = &hi2c1,
    .senseRes = 8200,
    .maxCurrent = 30,
    .maxVolt = 160
  };
  acs_setBybassNenable(&currentSensor, true, true);
  acs_setNumberOfSamples(&currentSensor, 1023, true);
    
  MPU6050_Init(&hi2c1);
  HMC5883L_Init(&hi2c1);

  float current = 0;
  float voltage = 0;

  HMC5883L_Configure();



  // Take the average of the initial atmospheric pressure reading
  surface_pressure_pa = Callibrate_MS5837(&pressure_sensor, SURFACE_PRESSURE_AVERAGE);

  MPU6050_t mpu_data;
  int16_t mag_data[3];
  float lastDepth = 0;
  


  FusionMatrix gyroscopeMisalignment = {{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
  FusionVector gyroscopeSensitivity = {{1.0f, 1.0f, 1.0f}};
  FusionVector gyroscopeOffset = {{0}};

  FusionMatrix accelerometerMisalignment = {{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
  FusionVector accelerometerSensitivity = {{1.0f, 1.0f, 1.0f}};
  FusionVector accelerometerOffset = {{0.0f, 0.0f, 0.0f}};

  FusionMatrix softIronMatrix = {{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
  FusionVector hardIronOffset = {{0.0f, 0.0f, 0.0f}};

  for(int i = 0; i < 10; i++){
    MPU6050_Read_All(&mpu_data);
    gyroscopeOffset.axis.x += mpu_data.Gx;
    gyroscopeOffset.axis.y += mpu_data.Gy;
    gyroscopeOffset.axis.z += mpu_data.Gz;
    osDelay(100);
  }
  gyroscopeOffset.axis.x /= 10.0;
  gyroscopeOffset.axis.y /= 10.0;
  gyroscopeOffset.axis.z /= 10.0;

    // Instantiate AHRS algorithm
  FusionAhrs ahrs;
  FusionAhrsInitialise(&ahrs);

  const FusionAhrsSettings settings = {
      .convention = FusionConventionNwu,
      .gain = 0.1f,
      .gyroscopeRange = 50.0f, /* replace with actual gyroscope range */
      .accelerationRejection = 5.0f,
      .magneticRejection = 5.0f,
      .recoveryTriggerPeriod = 1*THRUSTER_SAMPLE_RATE, /* 1 seconds */
  };

  FusionAhrsSetSettings(&ahrs, &settings);

  // Instantiate bias algorithm
  FusionBias bias;
  FusionBiasInitialise(&bias);

  FusionBiasSettings biasSettings = fusionBiasDefaultSettings;
  biasSettings.sampleRate = THRUSTER_SAMPLE_RATE;

  FusionBiasSetSettings(&bias, &biasSettings);

  //HMC5883L_HardIronCalibrate(10000);

  uint16_t t1, t2, t3, t4, t5;

  propulsion_initialized = 1;

  while(1){

      if (sensorsampleflag)
      {
          sensorsampleflag = 0;

          Read_TSYS01(&temp_sensor);
          Read_MS5837(&pressure_sensor);
      }


   MPU6050_Read_All(&mpu_data);
  if (hi2c1.ErrorCode != HAL_I2C_ERROR_NONE)
  {

      HAL_I2C_DeInit(&hi2c1);
      osDelay(5);
      HAL_I2C_Init(&hi2c1);
      osDelay(5);

      MPU6050_Init(&hi2c1);
  }
    HMC5883L_GetCalibratedXData(&mag_data[0]);
    HMC5883L_GetCalibratedYData(&mag_data[1]);
    HMC5883L_GetCalibratedZData(&mag_data[2]);

    uint32_t timestamp = xTaskGetTickCount();
    FusionVector gyroscope = {{mpu_data.Gx, mpu_data.Gy, mpu_data.Gz}};
    FusionVector accelerometer = {{mpu_data.Ax, mpu_data.Ay, mpu_data.Az}};
    FusionVector magnetometer = {{(float)(mag_data[0]), (float)(mag_data[1]), (float)(mag_data[2])}};

    //printf("%f, %f, %f, %f, %f, %f\n", mpu_data.Ax, mpu_data.Ay, mpu_data.Az, mpu_data.Gx, mpu_data.Gy, mpu_data.Gz);

    // Apply calibration
    gyroscope = FusionModelInertial(gyroscope, gyroscopeMisalignment, gyroscopeSensitivity, gyroscopeOffset);
    accelerometer = FusionModelInertial(accelerometer, accelerometerMisalignment, accelerometerSensitivity, accelerometerOffset);
    magnetometer = FusionModelMagnetic(magnetometer, softIronMatrix, hardIronOffset);

    // Update bias algorithm
    gyroscope = FusionBiasUpdate(&bias, gyroscope);

    // Calculate delta time to compensate for gyroscope sample clock errors
    static uint32_t previousTimestamp;
    const float deltaTime = (float) (timestamp - previousTimestamp)/1000.0;
    previousTimestamp = timestamp;

    // Update AHRS algorithm
    FusionAhrsUpdate(&ahrs, gyroscope, accelerometer, magnetometer, deltaTime);

    // Print AHRS outputs
    const FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
    const FusionVector earth = FusionAhrsGetEarthAcceleration(&ahrs);
    // printf("yaw %0.1f, pitch %0.1f, roll %0.1f, X %0.1f, Y %0.1f, Z %0.1f\n",
    //         euler.angle.yaw, euler.angle.pitch, euler.angle.roll,
    //         earth.axis.x, earth.axis.y, earth.axis.z);
    
    // SEND THE EULER ANGLES AS A PACKET //
    
    positional_telemetry_packet.yaw = (int16_t)(euler.angle.yaw * 100.0f);
    positional_telemetry_packet.pitch = (int16_t)(euler.angle.pitch * 100.0f);
    positional_telemetry_packet.roll = (int16_t)(euler.angle.roll * 100.0f);


    float commandedRoll = thruster_command.joystick2x*MAX_ROLL/1000.0;
    float commandedPitch = thruster_command.joystick2y*MAX_PITCH/1000.0;

    // float pitchError = commandedPitch - euler.angle.roll;
    // float rollError = commandedRoll - euler.angle.pitch;
    // float depthError = 0;

    float roll = atan(mpu_data.Ax)*(180.0/M_PI);
    float pitch =  atan(mpu_data.Ay)*(180.0/M_PI);

  //printf("%f, %f\n",roll,pitch);

     float pitchError = commandedPitch - pitch;
    float rollError = commandedRoll - roll;
    float depthError = 0;

   


    if(thruster_command.trigger == 0){
      depthError = lastDepth - (float)environmetal_telemetry_packet.depth;
    }else{
      depthError = (float)(thruster_command.trigger);
      lastDepth = (float)environmetal_telemetry_packet.depth;
    }

     //printf("%f\n", depthError);



    float pitchCoeff = 10.0;
    float rollCoeff = 10.0;
    float depthCoeff = 0.50;



    float pitchCorrection = pitchCoeff*pitchError;
    float rollCorrection = rollCoeff*rollError;
    float depthCorrection = depthCoeff*depthError;

    if(rollCorrection>200)rollCorrection = 200;
    if(rollCorrection<-200)rollCorrection = -200;

    if(pitchCorrection>200)pitchCorrection = 200;
    if(pitchCorrection<-200)pitchCorrection = -200;


   //printf("%f, %f, %f\n", commandedPitch, commandedRoll, lastDepth);


    acs_getInstCurrVolt(&currentSensor, &current, &voltage);
    current = fabs(current);
    voltage = fabs(voltage);
    
    power_telemetry_packet.voltage_mv = (int16_t)(voltage* 1000.0f);
    power_telemetry_packet.current_ma = (int16_t)(current * 1000.0f);
    power_telemetry_packet.power_mw = (int16_t)((voltage * current) * 1000.0f);

    // if(current > 25.0){
    //   float currentScalar = -0.2*(current-30.0);
    //   if(currentScalar < 0) currentScalar = 0;
      
    //   t1 = 1500 + currentScalar*((thruster_command.joystick1y/2.0) - (thruster_command.joystick1x/2.0));
    //   t2 = 1500 + currentScalar*((thruster_command.joystick1y/2.0) - (thruster_command.joystick1x/2.0));
    //   t3 = 1500 + currentScalar*(PWM_GetPeriod(PWM_3) - 1500 + pitchCorrection - rollCorrection + depthCorrection);
    //   t4 = 1500 + currentScalar*(PWM_GetPeriod(PWM_4) - 1500  + pitchCorrection + rollCorrection + depthCorrection);
    //   t5 = 1500 + currentScalar*(PWM_GetPeriod(PWM_5) - 1500 - pitchCorrection + depthCorrection);
    // }else {
      t1 = ((-1*(float)thruster_command.joystick1y)/2.0) + (((float)thruster_command.joystick1x)/2.0) +1500;
      t2 = ((-1*(float)thruster_command.joystick1y)/2.0) - (((float)thruster_command.joystick1x)/2.0) +1500;
      t3 = 1500.0 + (float)(-pitchCorrection - rollCorrection + depthCorrection);
      t4 = 1500.0 + (float)(-pitchCorrection + rollCorrection + depthCorrection);
      t5 = 1500.0 + (float)(2.0*pitchCorrection + depthCorrection);
    //}
    //printf("%d, %d\n", t1,t2);
    if(t1>2000)t1=2000;
    if(t2>2000)t2=2000;
    if(t3>2000)t3=2000;
    if(t4>2000)t4=2000;
    if(t5>2000)t5=2000;

    if(t1<1000)t1=1000;
    if(t2<1000)t2=1000;
    if(t3<1000)t3=1000;
    if(t4<1000)t4=1000;
    if(t5<1000)t5=1000;

   // printf("%d, %d, %d, %d, %d\n", t1,t2,t3,t4,t5);
    PWM_SetThrusterPeriods(t1,t2,t3,t4,t5);

    osDelay(50); // small consistent loop rate

    }
}

void StartTelemetryStreamTask(void *argument)
{
    printf("\r\n");
    printf("|Telemetry Stream Task has been initialized...|\r\n");
    printf("\r\n");
    PWM_Init();


    printf("Started Telemetry Stream Task...\n");

    printf("MUX OFF SCAN:\r\n");
    I2C_Scan();

    Init_TSYS01(&temp_sensor, &hi2c1);
    Init_MS5837(&pressure_sensor, &hi2c1);

    // Take the average of the initial atmospheric pressure reading
    surface_pressure_pa = Callibrate_MS5837(&pressure_sensor, SURFACE_PRESSURE_AVERAGE);
    
  for (;;)
  {

      Read_TSYS01(&temp_sensor);
      Read_MS5837(&pressure_sensor);

      osDelay(200);
  }
}

/* USER CODE END Application */
