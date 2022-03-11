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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <rboot-api.h>
#include <sysparam.h>

// the first function is the ONLY thing needed for a repo to support ota after having started with ota-boot
// in ota-boot the user gets to set the wifi and the repository details and it then installs the ota-main binary

void ota_update(void *arg) {  //arg not used
        rboot_set_temp_rom(1); //select the OTA main routine
        sdk_system_restart(); //#include <rboot-api.h>
        // there is a bug in the esp SDK such that if you do not power cycle the chip after serial flashing, restart is unreliable
}

// this function is optional to couple Homekit parameters to the sysparam variables and github parameters
unsigned int  ota_read_sysparam(char **revision) {
        sysparam_status_t status;
        char *value;

        status = sysparam_get_string("ota_version", &value);
        if (status == SYSPARAM_OK) {
                *revision=value;
        } else *revision="0.0.0";


        unsigned int c_hash=0;
        char version[16];
        char* rev=version;
        char* dot;
        strncpy(rev,*revision,16);
        if ((dot=strchr(rev,'.'))) {dot[0]=0; c_hash=atoi(rev); rev=dot+1;}
        if ((dot=strchr(rev,'.'))) {dot[0]=0; c_hash=c_hash*1000+atoi(rev); rev=dot+1;}
        c_hash=c_hash*1000+atoi(rev);
        //c_hash=c_hash*10  +configuration_variant; //possible future extension
        printf("revision=\'%s\' c#=%d\n",*revision,c_hash);
        return c_hash;
}

#include <homekit/characteristics.h>
#include <esplibs/libmain.h>
#include <etstimer.h>

static ETSTimer update_timer;

void ota_set(homekit_value_t value) {
        if (value.format != homekit_format_bool) {
                printf("Invalid ota-value format: %d\n", value.format);
                return;
        }
        if (value.bool_value) {
        //make a distinct light pattern or other feedback to the user = call identify routine
                sdk_os_timer_setfn(&update_timer, ota_update, NULL);
                sdk_os_timer_arm(&update_timer, 500, 0); //wait 0.5 seconds to trigger the reboot so gui can update and events sent
        }
}
