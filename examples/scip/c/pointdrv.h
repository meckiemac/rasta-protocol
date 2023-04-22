#ifndef POINTDRV_HEADER_
#define POINTDRV_HEADER_ 1

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
              // used by C++ source code
#endif

#include <bcm2835.h>

struct point_drv_cfg
{
    RPiGPIOPin drv_pin_1;
    RPiGPIOPin drv_pin_2;
    RPiGPIOPin drv_pin_3;
    RPiGPIOPin drv_pin_4;
    RPiGPIOPin pos_pin_1;
    RPiGPIOPin pos_pin_2;
    int        half_steps;
};

#ifdef __cplusplus
}
#endif

#endif /* POINTDRV_HEADER_ */