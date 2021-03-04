/* mbed Microcontroller Library
 * Copyright 2019-2021 Pelion Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef BOOT_NVM_STORAGE
#define BOOT_NVM_STORAGE

#define KVSTORE 1111
#define PDBSTORE 1112

#ifdef __cplusplus
extern "C" {
#endif

int nvm_storage_init(void);

int nvm_storage_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // BOOT_NVM_STORAGE
