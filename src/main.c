#include "stm32f1xx_hal.h"
#include "bsp/bsp.h"
#include "app/app.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    App_Init();

    while (1) {
        App_Run();
    }
}
