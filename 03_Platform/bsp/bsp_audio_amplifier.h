/**
 ******************************************************************************
 *@file               :   bsp_audio_amplifier.h
 *@brief              :   Provide the fixed board audio amplifier capability.
 *@version            :   V1.0
 *@note               :   1 tab == 4 spaces!  2026
 ******************************************************************************
 */
#ifndef BSP_AUDIO_AMPLIFIER_H
#define BSP_AUDIO_AMPLIFIER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes -----------------------------------------------------------------*/
#include <stdint.h>
#include "plat_error.h"



#define UNMUTES_AMP         (0U)
#define ISMUTES_AMP         (1U)
/* function  ----------------------------------------------------------------*/
/**
  * @brief            : Initialize the board amplifier in muted shutdown mode.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or PLATFORM_ERR_HW.
  */
platform_err_t bsp_audio_amplifier_init(void);

/**
  * @brief            : Enable the amplifier while keeping it muted.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_HW.
  * @note             : Call bsp_audio_amplifier_mute_set(0U) only after the
  *                     upstream audio signal is stable.
  */
platform_err_t bsp_audio_amplifier_enable(void);

/**
  * @brief            : Mute and shut down the amplifier.
  * @retval           : PLATFORM_ERR_OK or PLATFORM_ERR_HW.
  */
platform_err_t bsp_audio_amplifier_shutdown(void);

/**
  * @brief            : Set the amplifier mute state.
  * @param[in]        : is_muted 0 unmutes; 1 mutes the amplifier.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or PLATFORM_ERR_HW.
  */
platform_err_t bsp_audio_amplifier_mute_set(uint8_t is_muted);

/**
  * @brief            : Read the amplifier fault status.
  * @param[out]       : p_is_fault Set to 1 on fault, otherwise 0.
  * @retval           : PLATFORM_ERR_OK, PLATFORM_ERR_PARAM or PLATFORM_ERR_HW.
  */
platform_err_t bsp_audio_amplifier_fault_get(uint8_t *p_is_fault);

#ifdef __cplusplus
}
#endif

/* end of file --------------------------------------------------------------*/
#endif /* BSP_AUDIO_AMPLIFIER_H */
