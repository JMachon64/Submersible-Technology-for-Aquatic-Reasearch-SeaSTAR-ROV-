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
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <pwm.h>
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
#include "Mux.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
TSY01_TemperatureSensor_t temp_sensor;
MS5837_PressureSensor_t pressure_sensor;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


uint8_t uart2_rx_byte;

osThreadId_t UartStreamTaskHandle;
const osThreadAttr_t UartStreamTask_attributes = {
  .name = "UartStreamTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t TelemetryStreamTaskHandle;
const osThreadAttr_t TelemetryStreamTask_attributes = {
  .name = "TelemetryStreamTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t StateMachineTaskHandle;
const osThreadAttr_t StateMachineTask_attributes = {
  .name = "StateMachineTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh2,
};
osThreadId_t SystemHealthMonitorTask;
const osThreadAttr_t SystemHealthMonitorTask_attributes = {
  .name = "SystemHealthMonitorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh3,
};

osThreadId_t PropulsionTaskHandle;
const osThreadAttr_t PropulsionTask_attributes = {
  .name = "PropulsionTask",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
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
void StartUartTask(void *argument);
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
  // defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
   uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);
  // UartStreamTaskHandle = osThreadNew(StartWiredUARTStream, NULL, &UartStreamTask_attributes);
  // StateMachineTaskHandle = osThreadNew(StartStateMachineTask, NULL, &StateMachineTask_attributes);
  // SystemHealthMonitorTask = osThreadNew(StartSystemHealthMonitorTask, NULL, &SystemHealthMonitorTask_attributes);
  TelemetryStreamTaskHandle = osThreadNew(StartTelemetryStreamTask, NULL, &TelemetryStreamTask_attributes);
  // PropulsionTaskHandle = osThreadNew(Propulsion_Task, NULL, &PropulsionTask_attributes);
 
  if (UartStreamTaskHandle == NULL) {
      printf("Failed to create UART task\r\n");
  }
  if (StateMachineTaskHandle == NULL) {
      printf("Failed to create state machine task\r\n");
  }
  if (TelemetryStreamTaskHandle == NULL) {
      printf("Failed to create telemetry task\r\n");
  }

  if (PropulsionTaskHandle == NULL) {
      printf("Failed to create propulsion task\r\n");
  }
  /* USER CODE END RTOS_THREADS */

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
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  PWM_Init();


  printf("Started default task...\n");

  uint8_t test = MPU6050_Init(&hi2c1);

    osDelay(100);

  MPU6050_t data;


  while(1){

    MPU6050_Read_All(&data);
    //printf("%f, %f, %f\n", data.Ax, data.Ay, data.Az);

    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
// 	HAL_UART_Receive_IT(&huart1, buffer, 1);
// 	HAL_UART_Transmit(&huart1, buffer, 1, 0xFFFF);
// }

void StartUartTask(void *argument){


  printf("\r\n");
  printf("|Sampling Task has been initialized...|\r\n");
  printf("\r\n");
  for(;;)
  {

    if (sample_flag)
    {
        sample_flag = 0;

        if (baseline_set)
        {
        pressure_sensor.Depth_m = MS5837_GetDepth(&pressure_sensor, 1000.0f);

        environmetal_telemetry_packet.pressure_Pa =
            (int32_t)MS5837_CorrectPressurePa(pressure_sensor.Pressure_Pa);
        TelemetryStream_SendEnvironmental(&environmetal_telemetry_packet);
        }
    }
    osDelay(1);
  }
}

void StartWiredUARTStream(void *argument)
{
    UART3_INIT_RxDMA();
    printf("\r\n");
    printf("|Uart Task has been initialized...|\r\n");
    printf("\r\n");
    Packet_Sent_t rxPacket;

    for (;;)
    {
        Protocol_ProcessTxQueue();
        UART3_Traverse_RxDMA();
        if (BuildRxPacket(&rxPacket, 0))
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

    osDelay(1);
  }
  /* USER CODE END 5 */
}

void StartSystemHealthMonitorTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  printf("\r\n");
  printf("|Initializing system health monitor task...|\r\n");
  printf("\r\n");
  /* Infinite loop */
  for(;;)
  {
    LeakSensor_Read();
    osDelay(200);
  }
  /* USER CODE END 5 */
}
void Propulsion_Task(void *argument)
{

    PWM_Init();
    // PWM_SampleOpen();
    printf("\r\n");
    printf("|Thruster Stream Task has been initialized...|\r\n");
    printf("\r\n");
    while (1)
    {
        osDelay(1); // small consistent loop rate
    }
}

void StartTelemetryStreamTask(void *argument)
{
    printf("\r\n");
    printf("|Telemetry Stream Task has been initialized...|\r\n");
    printf("\r\n");
    PWM_Init();


    printf("Started default task...\n");

    uint8_t test = MPU6050_Init(&hi2c1);

    printf("MUX OFF SCAN:\r\n");
    I2C_Scan();


    MPU6050_t data;

    //Init_TSYS01(&temp_sensor, &hi2c1);
    uint8_t ok = Init_MS5837(&pressure_sensor, &hi2c1);
    if (!ok)
    {
        printf("MS5837 INIT FAILED\r\n");
    }
    else
    {
        surface_pressure_pa =
            Callibrate_MS5837(&pressure_sensor, SURFACE_PRESSURE_AVERAGE);
    }
    //surface_pressure_pa = Callibrate_MS5837(&pressure_sensor, SURFACE_PRESSURE_AVERAGE);

  for (;;)
  {
      if (sensor_flag)
      {
          sensor_flag = 0;

          //Read_TSYS01(&temp_sensor);
          Read_MS5837(&pressure_sensor);
      }
      osDelay(1);
  }
}




/* USER CODE END Application */

