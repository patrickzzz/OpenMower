/*
 * Copyright 2024 Jörg Ebeling (Apehaenger)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.* Testing/examples for reading/writing flash on the RP2040.
 *
 * Heavily inspired, copied from, as well as must-read:
 * https://github.com/MakerMatrix/RP2040_flash_programming
 */

#include <Arduino.h>

// #define NV_CONFIG_MAX_SAVE_INTERVAL 60000UL // Don't save more often than once every 1 minute(s)
#define NV_CONFIG_MAX_SAVE_INTERVAL 2000UL // DBG: Don't save more often than once every 1 second(s)

// config_bitmask
#define LL_CONFIG_BIT_HL_CONFIG_RECEIVED 0x1 // ROS config packet received
#define LL_CONFIG_BIT_DFPIS5V 0x2            // DFP is set to 5V

#define NV_RECORD_ID 0x4F4D4331                  // Record struct identifier "OMC1" for future flexible length Record.config
#define NV_RECORD_ALIGNMENT (_Alignof(uint32_t)) // Ptr alignment of Record.id for quick in memory access

namespace nv_config
{
#pragma pack(push, 1)
    // Config struct example with reasonable defaults.
    // This is where our application values should go.
    // It's possible to extended it, but any extension should add an extension related CRC so that an old/last stored Record.config isn't lost.
    //    (a new extension-crc isn't valid with a old Record.config. Thus the new extension values may get set i.e. with default values)
    struct Config
    {
        // Config bitmask:
        // Bit 0: ROS config packet received. See LL_CONFIG_BIT_HL_CONFIG_RECEIVED
        // Bit 1: DFP is 5V (enable full sound). See LL_CONFIG_BIT_DFPIS5V
        uint8_t config_bitmask = 0;
        uint8_t language = 0;          // Sound language index 0 = Folder "01" = English(US), 1 = Folder "49" = German
        uint8_t volume = 100;          // Sound loudness from 0 to 100 %
        uint32_t rain_threshold = 700; // If (stock CoverUI) rain value < rain_threshold then it rains. Expected to differ between C500, SA and SC types

        /* Possible future config settings
        uint16_t free;                 // Future config setting
        uint16_t free_n;               // Future config setting
        uint16_t crc_n;                // Future config CRC16 (for the new member) for detection if loaded (possibly old) config already has the new member */
    } __attribute__((packed));

    // Record(s) get placed sequentially into a flash page.
    // The Record structure shouldn't get changed, because it would make old flash Record's unusable. Instead of, change Record.config
    struct Record
    {
        const uint32_t id = NV_RECORD_ID; // Fixed record identifier, used to identify a possible Record within a flash page. If width get changed, change also NV_RECORD_ALIGNMENT
        uint32_t num_sector_erase = 0;
        uint32_t num_page_write = 0;
        uint16_t crc; // Required to ensure that a found NV_RECORD_ID is really a Record
        Config config;
    } __attribute__((packed));
#pragma pack(pop)

    Config *get();             // Returned pointer hold the last saved Record.config, or the default one. Config member are writable, see delayedSaveChanges()
    void delayedSaveChanges(); // Handle a possible changed nv_config::config member and save it to flash, but only within NV_CONFIG_MAX_SAVE_INTERVAL timeout for wear level protection
}