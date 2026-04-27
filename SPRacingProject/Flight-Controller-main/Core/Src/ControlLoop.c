
#include "ControlLoop.h"
#include "UartProtocol.h"
#include <stdint.h>
#include "pwm.h"


volatile control_command_t thruster_command = {0};

void Control_Update_Command(int16_t x1, int16_t y1, int16_t x2, int16_t y2, 
    int16_t trigger, uint32_t seq)
{

    thruster_command.joystick1x = x1;
    thruster_command.joystick1y = y1;
    thruster_command.joystick2x = x2;
    thruster_command.joystick2y = y2;
    thruster_command.trigger = trigger;
    thruster_command.seq = seq;
    thruster_command.last_update_ms = HAL_GetTick();

   
    uint16_t s1 = (x1/2.0)+1500;
    uint16_t s2 = (y1/2.0)+1500;
    uint16_t s3 = (x2/2.0)+1500;
    uint16_t s4 = (y2/2.0)+1500;
    uint16_t s5 = (trigger/2.0)+1500;

    // printf("%d\n", x1);

    // PWM_SetThrusterPeriods(s1,s2,s3,s4,s5);
    // if (x1>100){
    //     printf("RIGHT\n");
    // }
    // if (x1<-100){
    //     printf("LEFT\n");
    // }

}

void Set_Servo(uint16_t angle){




    
}