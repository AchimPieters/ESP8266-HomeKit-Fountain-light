/*
   Copyright 2019 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Build upon: Life-Cycle-Manager (LCM) - Apache-2.0 License - Copyright (c) 2021 HomeAccessoryKid
 */

#ifndef __HOMEKIT_CUSTOM_CHARACTERISTICS__
#define __HOMEKIT_CUSTOM_CHARACTERISTICS__

#define HOMEKIT_CUSTOM_UUID(value) (value "-0e36-4a42-ad11-745a73b84f2b")

#define HOMEKIT_SERVICE_CUSTOM_SETUP HOMEKIT_CUSTOM_UUID("000000FF")

#define HOMEKIT_CHARACTERISTIC_CUSTOM_OTA_TRIGGER HOMEKIT_CUSTOM_UUID("F0000001")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_OTA_TRIGGER(_value, ...) \
        .type = HOMEKIT_CHARACTERISTIC_CUSTOM_OTA_TRIGGER, \
        .description = "Firmware Update", \
        .format = homekit_format_bool, \
        .permissions = homekit_permissions_paired_read \
                       | homekit_permissions_paired_write \
                       | homekit_permissions_notify, \
        .value = HOMEKIT_BOOL_(_value), \
        ## __VA_ARGS__

unsigned int  ota_read_sysparam(char **revision);

void ota_update(void *arg);

void ota_set(homekit_value_t value);

#define API_OTA_TRIGGER HOMEKIT_CHARACTERISTIC_(CUSTOM_OTA_TRIGGER, false, .setter=ota_set)

#endif
