// ----------------------------------------------------------------------------
// Copyright 2016-2017 ARM Ltd.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "mbed.h"

#include "update-client-paal/arm_uc_paal_update.h"
#include "update-client-common/arm_uc_types.h"

#include "mbed_bootloader_info.h"
#include "bootloader_platform.h"
#include "active_application.h"
#include "bootloader_common.h"
#include "mbed_application.h"
#include "upgrade.h"

#if defined(BOOTLOADER_POWER_CUT_TEST) && (BOOTLOADER_POWER_CUT_TEST == 1)
#include "bootloader_power_cut_test.h"
#include "firmware_update_test.h"
#endif

#if defined(FIRMWARE_UPDATE_TEST) && (FIRMWARE_UPDATE_TEST == 1)
#include "firmware_update_test.h"
#endif

const arm_uc_installer_details_t bootloader = {
    .arm_hash = BOOTLOADER_ARM_SOURCE_HASH,
    .oem_hash = BOOTLOADER_OEM_SOURCE_HASH,
    .layout   = BOOTLOADER_STORAGE_LAYOUT
};

/* use a cut down version of ARM_UCP_FLASHIAP_BLOCKDEVICE to reduce
   binary size if ARM_UC_USE_PAL_BLOCKDEVICE is set and not running tests */
#if defined(ARM_UC_USE_PAL_BLOCKDEVICE) && (ARM_UC_USE_PAL_BLOCKDEVICE==1) && \
    (!defined(BOOTLOADER_POWER_CUT_TEST) || (BOOTLOADER_POWER_CUT_TEST != 1))
#undef  MBED_CLOUD_CLIENT_UPDATE_STORAGE
#define MBED_CLOUD_CLIENT_UPDATE_STORAGE ARM_UCP_FLASHIAP_BLOCKDEVICE_READ_ONLY
#endif

#ifdef MBED_CLOUD_CLIENT_UPDATE_STORAGE
extern ARM_UC_PAAL_UPDATE MBED_CLOUD_CLIENT_UPDATE_STORAGE;
#else
#error Update client storage must be defined in user configuration file
#endif

#ifndef MBED_CONF_MBED_BOOTLOADER_APPLICATION_START_ADDRESS
#error Application start address must be defined
#endif

/* If jump address is not set then default to start address. */
#ifndef MBED_CONF_APP_APPLICATION_JUMP_ADDRESS
#define MBED_CONF_APP_APPLICATION_JUMP_ADDRESS MBED_CONF_MBED_BOOTLOADER_APPLICATION_START_ADDRESS
#endif

int main(void)
{
    /* Use malloc to allocate uint64_t version number on the heap */
    heapVersion = (uint64_t *) malloc(sizeof(uint64_t));
    bootCounter = (uint8_t *) malloc(1);

    /* Set PAAL Update implementation before initializing Firmware Manager */
    ARM_UCP_SetPAALUpdate(&MBED_CLOUD_CLIENT_UPDATE_STORAGE);

    /* Initialize PAL */
    arm_uc_error_t ucp_result = ARM_UCP_Initialize(arm_ucp_event_handler);

#if defined(BOOTLOADER_POWER_CUT_TEST) && (BOOTLOADER_POWER_CUT_TEST == 1)
    power_cut_test_setup();
    const uint32_t firmware_size = 1024;
    copyAppToSDCard(firmware_size);
    power_cut_test_assert_state(POWER_CUT_TEST_STATE_START);
#elif defined(FIRMWARE_UPDATE_TEST) && (FIRMWARE_UPDATE_TEST == 1)
    const uint32_t firmware_size = 1024 * 16;
    copyAppToSDCard(firmware_size);
    firmware_update_test_setup();
#endif

    /*************************************************************************/
    /* Print bootloader information                                          */
    /*************************************************************************/

    boot_debug("[DBG ] Mbed Bootloader\r\n");

    /*************************************************************************/
    /* Update                                                                */
    /*************************************************************************/

    /* Default to booting application */
    bool canForward = true;

    /* check UCP initialization result */
    if (ucp_result.error == ERR_NONE) {
        /* Initialize internal flash */
        bool storageResult = activeStorageInit();

        if (storageResult) {
            /* Try to update firmware from journal */
            canForward = upgradeApplicationFromStorage();

            /* deinit storage driver */
            activeStorageDeinit();
        }
    }

    /* forward control to ACTIVE application if it is deemed sane */
    if (canForward) {
#if defined(BOOTLOADER_POWER_CUT_TEST) && (BOOTLOADER_POWER_CUT_TEST == 1)
        power_cut_test_assert_state(POWER_CUT_TEST_STATE_END);
        wait(5);
        power_cut_test_end(); // Notify test end, host test will determine actual result
#elif defined(FIRMWARE_UPDATE_TEST) && (FIRMWARE_UPDATE_TEST == 1)
        firmware_update_test_end();
#endif

        mbed_start_application(MBED_CONF_APP_APPLICATION_JUMP_ADDRESS);
    }

    /* Reset bootCounter; this allows a user to reapply a new bootloader
       without having to power cycle the device.
    */
    if (bootCounter) {
        *bootCounter = 0;
    }

    MBED_BOOTLOADER_ASSERT(false, "Failed to jump to application!");

    /* coverity[no_escape] */
    for (;;) {
        __WFI();
    }
}
