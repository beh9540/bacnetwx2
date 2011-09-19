/**************************************************************************
*
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
#include "xoapweather.h"
#include "ai.h"
#include "parser.h"
#include "weather.h"

int updateWeather() {
   parse_handle *handle;
   parse_line *line;
   handle = parseInit(ETC_FILE);
   if ( handle==NULL )
   {
      fprintf(stderr,"Couldn't find the file %s\n",ETC_FILE);
      return -1;
   }   
   line = parseOptions(handle); 
   char *PID=line->PID;
   char *LKEY=line->LKEY;
   char *LOCID=line->LOCID;
   int i = 0; //counter
   xoap_weather_forecast *forecast;
	xoap_weather_handle *h;
	xoap_weather_rain *rain;
	/*First we have to initialize the XOAP Weather*/
	h = xoap_weather_init(PID, LKEY);
	if (h) {
	    	/*Then we set the XOAP Options(5 Days Forecast, our Zip Code,
	    	 * US Standard Units, and that's it*/
         if ( line->FORECASTED_DAYS )
         {
            char *forecastDayString = line->FORECASTED_DAYS;
            int forecast_days = atoi(forecastDayString);
            switch (forecast_days) {
               case 1:
                  xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF1);
                  break;
               case 2:
                  xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF2);
                  break;
               case 3:
                  xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF3);
                  break;
               case 4:
                  xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF4);
                  break;
               case 5:
                  xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF5);
                  break;
               default:
                  xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF1);
                  break;
            }
         }
         else
            xoap_weather_setopt(h,XOAP_WEATHER_OPT_FETCH,XOAP_WEATHER_FETCH_DAYF1);
	    	xoap_weather_setopt(h, XOAP_WEATHER_OPT_ID,LOCID);
	    	xoap_weather_setopt(h, XOAP_WEATHER_OPT_UNIT,XOAP_WEATHER_UNIT_STANDARD);
  	    	/*get the forecasted data*/
	    rain=xoap_weather_rain(h);
		forecast=xoap_weather_fetch(h);
		if (forecast) {
	    		/*current weather*/
		    	Analog_Input_Present_Value_Set(0,forecast->cc->temp);
		    	Analog_Input_Present_Value_Set(1,forecast->cc->feels);
		    	Analog_Input_Present_Value_Set(2,forecast->cc->humidity);
		    	Analog_Input_Present_Value_Set(3,forecast->cc->visibility);
		    	Analog_Input_Present_Value_Set(4,forecast->cc->dewp);
		    	Analog_Input_Present_Value_Set(5,forecast->cc->barp);
		    	Analog_Input_Present_Value_Set(6,forecast->cc->uvi);
		    	Analog_Input_Present_Value_Set(7,atof(forecast->cc->wind.speed) );
		    	Analog_Input_Present_Value_Set(8,atof(forecast->cc->wind.gust) );
		    	Analog_Input_Present_Value_Set(9,forecast->cc->wind.direction_n);
		    	Analog_Input_Present_Value_Set(10,rain->rain_last_day);
		    	Analog_Input_Present_Value_Set(11,rain->rain_last_hr);
		    	/*forecasted weather, five days*/
		    	xoap_weather_day *day;
		    	for (i=0; (i < forecast->total_days); ++i) {
		    	   day = &forecast->days[i];
		    	   Analog_Input_Present_Value_Set(6*i+17,day->high);
		    	   Analog_Input_Present_Value_Set(6*i+18,day->low);
		    	   Analog_Input_Present_Value_Set(6*i+19,day->day.chance_precipitation);
		    	   Analog_Input_Present_Value_Set(6*i+20,day->night.chance_precipitation);
		    	   Analog_Input_Present_Value_Set(6*i+21,atof(day->day.wind.speed) );
		    	   Analog_Input_Present_Value_Set(6*i+22,atof(day->night.wind.speed) );
			   }
		}
	  xoap_weather_free_rain(rain);
      xoap_weather_free_forecast(forecast);	   
   }
   else
   {
      xoap_weather_cleanup(h);
      freeParserHandle(handle);
      freeParseOptions(line);
      fprintf(stderr,"Could Not Update Weather");
      return -1;
   }
   xoap_weather_cleanup(h);
   freeParserHandle(handle);
   freeParseOptions(line);
   return 0;
}
