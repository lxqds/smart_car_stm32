#include "vehicle_ports.h"
#include "vehicle_app.h"
#include "tim.h"
#include "main.h"
#include "motor_pwm.h"

static uint8_t configured;
static int8_t direction;
/* No blocking delay. At least one full 10ms cycle with PWM=0 on reversal. */
void VehiclePorts_Disable(void)
{
    if(configured) {
        __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,0);
        htim2.Instance->EGR=TIM_EGR_UG;
    }
    direction=0;
}
static int configure_motor(uint32_t timer_hz,uint32_t motor_hz)
{
    if(configured) {
        VehiclePorts_Disable();HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_2);
    }
    configured=0;
    if(timer_hz<1000000U || timer_hz>480000000U ||
       motor_hz<1000U || motor_hz>20000U ||
       timer_hz%motor_hz)return 0;
    MX_TIM2_Init();
    /* TIM2 PA1 PWM + PA0 direction; TIM5/PA2 are not initialized or written. */
    __HAL_TIM_SET_PRESCALER(&htim2,0U);
    __HAL_TIM_SET_AUTORELOAD(&htim2,timer_hz/motor_hz-1U);
    __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,0);
    htim2.Instance->EGR=TIM_EGR_UG;
    configured=1;VehiclePorts_Disable();
    if(HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2)!=HAL_OK) {
        VehiclePorts_Disable();HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_2);
        configured=0;return 0;
    }
    return 1;
}
static void apply_motor(int32_t duty)
{
    if(!configured)return;
    if(duty>(int32_t)MOTOR_PWM_FULL_SCALE)duty=MOTOR_PWM_FULL_SCALE;
    if(duty< -(int32_t)MOTOR_PWM_FULL_SCALE)duty=-(int32_t)MOTOR_PWM_FULL_SCALE;
    int8_t requested=duty>0?1:(duty<0?-1:0);
    if(requested!=direction) {
        __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,0);
        /* Force zero preload into active CCR before changing GPIO direction. */
        htim2.Instance->EGR=TIM_EGR_UG;
        if(direction) {direction=0;return;}
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,requested>0?GPIO_PIN_SET:GPIO_PIN_RESET);
        direction=requested;return;
    }
    uint32_t magnitude=(uint32_t)(duty<0?-duty:duty);
    uint32_t period=__HAL_TIM_GET_AUTORELOAD(&htim2)+1U;
    __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,(uint32_t)(((uint64_t)period*magnitude)/MOTOR_PWM_FULL_SCALE));
}

int VehiclePorts_Configure(uint32_t timer_hz,uint32_t motor_hz,uint32_t servo_hz)
{
    (void)servo_hz;
    if(!VEHICLE_OUTPUTS_ALLOWED)return 0;
    return configure_motor(timer_hz,motor_hz);
}
void VehiclePorts_Apply(int16_t duty,uint16_t pulse)
{
    (void)pulse;
    /* Closed-loop ABI stays permille; convert once at this boundary. */
    if(VEHICLE_OUTPUTS_ALLOWED) {
        int32_t bounded=duty>1000?1000:(duty< -1000?-1000:duty);
        apply_motor(bounded*(int32_t)(MOTOR_PWM_FULL_SCALE/1000U));
    }
}
int MotorPort_Configure(uint32_t timer_hz,uint32_t motor_hz)
{
    if(!MOTOR_PWM_COMMISSIONING)return 0;
    return configure_motor(timer_hz,motor_hz);
}
void MotorPort_Apply(int16_t duty)
{
    if(!MOTOR_PWM_COMMISSIONING)return;
    /* Signed drive; control core requires neutral + stationary before reversal. */
    if(duty< -MOTOR_PWM_LIMIT)duty=-MOTOR_PWM_LIMIT;
    if(duty>MOTOR_PWM_LIMIT)duty=MOTOR_PWM_LIMIT;
    apply_motor(duty);
}

uint32_t MotorPort_Compare(void) { return configured?__HAL_TIM_GET_COMPARE(&htim2,TIM_CHANNEL_2):0; }
uint32_t MotorPort_Period(void) { return configured?__HAL_TIM_GET_AUTORELOAD(&htim2):0; }
