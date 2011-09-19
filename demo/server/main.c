/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Weather station portion of this code is
* Copyright (C) 2010 Paul Brandt <pbrandt@techenergysolutions.com>
*
* Weather station portion altered is
* Copyright (C) 2011 Blake Howell <beh9540@rit.edu>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "address.h"
#include "bacdef.h"
#include "handlers.h"
#include "client.h"
#include "dlenv.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "tsm.h"
#include "device.h"
#include "bacfile.h"
#include "datalink.h"
#include "dcc.h"
#include "net.h"
#include "txbuf.h"
#include "lc.h"
#include "version.h"
/* include the objects */
#include "device.h"
#include "ai.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"
#include "bacfile.h"
#include "weather.h"


/* buffers used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

static void Init_Object(
    BACNET_OBJECT_TYPE object_type,
    rpm_property_lists_function rpm_list_function,
    read_property_function rp_function,
    object_valid_instance_function object_valid_function,
    write_property_function wp_function,
    object_count_function count_function,
    object_index_to_instance_function index_function,
    object_name_function name_function)
{
    handler_read_property_object_set(object_type, rp_function,
        object_valid_function);
    handler_write_property_object_set(object_type, wp_function);
    handler_read_property_multiple_list_set(object_type, rpm_list_function);
    Device_Object_Function_Set(object_type, count_function, index_function,
        name_function);
}

static void Init_Objects(
    void)
{
    Device_Init();
    Init_Object(OBJECT_DEVICE, Device_Property_Lists,
        Device_Encode_Property_APDU, Device_Valid_Object_Instance_Number,
        Device_Write_Property, NULL, NULL, NULL);

    Analog_Input_Init();
    Init_Object(OBJECT_ANALOG_INPUT, Analog_Input_Property_Lists,
        Analog_Input_Encode_Property_APDU, Analog_Input_Valid_Instance, NULL,
        Analog_Input_Count, Analog_Input_Index_To_Instance, Analog_Input_Name);

#if defined(BACFILE)
    bacfile_init();
    Init_Object(OBJECT_FILE, BACfile_Property_Lists,
        bacfile_encode_property_apdu, bacfile_valid_instance,
        bacfile_write_property, bacfile_count, bacfile_index_to_instance,
        bacfile_name);
#endif
}

static void Init_Service_Handlers(
    void)
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
#if defined(BACFILE)
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE,
        handler_atomic_read_file);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
        handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        handler_timesync_utc);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        handler_timesync);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV,
        handler_cov_subscribe);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_COV_NOTIFICATION,
        handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

static void cleanup(
    void)
{
    datalink_cleanup();
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 1000;    /* milliseconds */
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t last_reset = 0;
    struct tm *last_reset_tm;
    uint32_t time_reset = 0;
    uint32_t weather_time = 3600; //one hour in seconds: get weather update every hour
    uint32_t elapsed_seconds = 0;
    uint32_t elapsed_milliseconds = 0;

    /* allow the device ID to be set */
    if (argc > 1)
        Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
    /*print some information for the user*/
    printf("BACnet Weather Station\n" "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n" "Max APDU: %d\n", BACnet_Version,
        Device_Object_Instance_Number(), MAX_APDU);
    Init_Objects();
    Init_Service_Handlers();
    dlenv_init();
    atexit(cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        current_seconds = time(NULL);

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
            last_seconds = current_seconds;
            dcc_timer_seconds(elapsed_seconds);
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
            Load_Control_State_Machine_Handler();
            elapsed_milliseconds = elapsed_seconds * 1000;
            handler_cov_task(elapsed_seconds);
            tsm_timer_milliseconds(elapsed_milliseconds);
        }
	      time_reset = current_seconds - last_reset;

	      last_reset_tm = localtime(&last_reset);
	      float weather_since_hr = (float) last_reset_tm->tm_hour;
         float weather_since_min = (float) last_reset_tm->tm_min;
         float weather_since_month = (float) last_reset_tm->tm_mon;
         float weather_since_day= (float) last_reset_tm->tm_mday;
         float weather_since_year = (float) last_reset_tm->tm_year;
	      Analog_Input_Present_Value_Set(12,weather_since_hr);
         Analog_Input_Present_Value_Set(13,weather_since_min);
         Analog_Input_Present_Value_Set(14,weather_since_month);
         Analog_Input_Present_Value_Set(15,weather_since_day);
         Analog_Input_Present_Value_Set(16,1900 + weather_since_year);
	      if (time_reset >= weather_time) { 
	          if (updateWeather() ==0)
               last_reset = current_seconds; //reset timer
         }
	 
    }
}
