/* 
 * Copyright (c) 2009-2010, Michael John
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    * Neither the name of the Weather Channel, nor the names of the
 *       contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MICHAEL JOHN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Use of this software also means you agree to the conditions stated in the
 * TWCi XML Data Feed License Agreement.
 *
 */

#ifndef XOAP_WEATHER_H_
#define XOAP_WEATHER_H_

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#ifdef macos
#include <libxml2/libxml/xmlmemory.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#endif
#ifdef linux
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif

#define XOAP_WEATHER_VERSION "1.0.0"
#define XOAP_WEATHER_MAJOR_VERSION (1)
#define XOAP_WEATHER_MINOR_VERSION (0)
#define XOAP_WEATHER_MIRCO_VERSION (0)

#define XOAP_WEATHER_LIBNAME "libxoapweather"

/** URL to use to fetch  weather forecast from */
/** The Weather Channel Interactive (TWCi) */
#define TWCi_SEARCH_URL "http://xoap.weather.com/search/search"
#define TWCi_WEATHER_URL "http://xoap.weather.com/weather/local/"
#define WU_SEARCH_URL "http://api.wunderground.com/auto/wui/geo/GeoLookupXML/index.xml?query="
#define WU_AIRPORT_URL "http://api.wunderground.com/auto/wui/geo/WXCurrentObXML/index.xml?query="
#define WU_WEATHER_STATION_URL "http://api.wunderground.com/weatherstation/WXCurrentObXML.asp?ID="

/** Version this all supports */
#define TWCi_VERSION 2.0

/** Descriptive text for Required Links limit */
#define TWCi_RL_LIMITS 36

/** Numbers of Required Links */
#define TWCi_RL_COUNT 4

/** Macro: for storing the content from an xml element */
#define XOAPW_SE(n, s) \
	if (!xmlStrcmp((n)->name, (const xmlChar *)(s))) { \
		xmlChar *con = xmlNodeGetContent((n)); \
		if (con) {

/** Macro: ending for storing the content */
#define XOAPW_SE_END \
		} \
		xmlFree(con); \
	}

/** Macro: Storing element (STRING) */
#define XOAPW_SES(n, s, v) \
	XOAPW_SE((n), (s)) \
	(v) = strdup((char *)con); \
	XOAPW_SE_END

/** Macro: Storing element (SHORT) */
#define XOAPW_SEI(n, s, v) \
	XOAPW_SE((n), (s)) \
	(v) = ((short)atoi((char *)con)); \
	XOAPW_SE_END

/** Macro: Storing element (FLOAT) */
#define XOAPW_SEF(n, s, v) \
	XOAPW_SE((n), (s)) \
	(v) = ((float)atof((char *)con)); \
	XOAPW_SE_END

/** Macro: test before attempting to free */
#define XOAPW_FREE(o) \
	if((o)) \
		(free((o)))

/** Error types */
typedef enum {
	/** === TWCi ERROR CODES === */

	/** An unknown error has occurred */
	XOAP_WEATHER_UNKNOWN_ERROR = 0,
	/** No loaction provided */
	XOAP_WEATHER_NO_LOCATION = 1,
	/** Invalid location */
	XOAP_WEATHER_INVALID_LOCATION = 2,
	/** Invalid partner Id */
	XOAP_WEATHER_INVALID_PID = 100,
	/** Invalid product code */
	XOAP_WEATHER_INVALID_PCODE = 101,
	/** Invalid license key */
	XOAP_WEATHER_INVALID_LKEY = 102,
} xoap_weather_error;

/** Options for units */
typedef enum {
	XOAP_WEATHER_UNIT_STANDARD,
	XOAP_WEATHER_UNIT_METRIC
} xoap_weather_unit;

/** Options for fetching local weather */
typedef enum {
	/** Data returned: head, loc */
	XOAP_WEATHER_FETCH_LOC,
	/** Data returned: head, loc, cc */
	XOAP_WEATHER_FETCH_CC,
	/** Data returned: head, loc, cc, forecast for 1 day */
	XOAP_WEATHER_FETCH_DAYF1,
	/** Data returned: head, loc, cc, forecast for 2 days */
	XOAP_WEATHER_FETCH_DAYF2,
	/** Data returned: head, loc, cc, forecast for 3 days */
	XOAP_WEATHER_FETCH_DAYF3,
	/** Data returned: head, loc, cc, forecast for 4 days */
	XOAP_WEATHER_FETCH_DAYF4,
	/** Data returned: head, loc, cc, forecast for 5 days */
	XOAP_WEATHER_FETCH_DAYF5,
	/** Data returned: loc,cc */
	XOAP_WEATHER_FETCH_RAIN
} xoap_weather_fetch_type;

typedef enum {
	XOAP_WEATHER_OPT_ID,
	XOAP_WEATHER_OPT_UNIT,
	XOAP_WEATHER_OPT_FETCH
} xoap_weather_opt;

typedef struct {
	char *link;		// The URL
	char *description;	// (Should never be longer than TWCi_RL_LIMITS)
} xoap_weather_links;

typedef struct {
	char *speed;		// either an integer or word (ex. calm)
	char *gust;		// either an integer or "N/A"
	short direction_n;	// direction as a number (0-359) (N=0,E=90,S=180,W=270)
	char *direction;	// N, NE, NNE, etc
} xoap_weather_wind;

typedef struct {
	short icon;		// corresponding weather icon (provided by Weather.com(TWCi))
	char *cond;		// cloudy, rain, etc
	char *cond_short; 	// Instead of "Showers Early" => "Shwrs Early"
	short chance_precipitation; // percent chance (value range 0-100)
	short humidity;
	xoap_weather_wind wind;
} xoap_weather_weather;

typedef struct {
	char *week_day;	// week day name
	char *date;	// month and date for the day
	short high;	// High temperature for the day
	short low;	// Low temperature for the day
	char *sun_rise;	// Format (6:30 AM)
	char *sun_set;	// Format (8:50 PM)
	xoap_weather_weather day;	// Weather for 7:00 to 19:00 (7:00am-7:00pm)
	xoap_weather_weather night;	// Weather for 19:00 to 7:00 (7:00pm-7:00am)
} xoap_weather_day;

typedef struct {
	char *lsup;	// Time forecast was last updated
	char *obst;	// Observation station forecast recorded from
	short temp;	// Temperature
	short feels;	// Feels like
	short icon;	// Corresponding icon for weather
	char *cond;	// Condition (cloudy, rain, fair, etc)
	short humidity;
	char *swa;	// Severe Weather Advisory (not always present)
	char *swal;	// Link to the severe weather alert
	float visibility;
	short dewp;	// Dew point
	float barp;	// Barometric Pressure
	char *bart;	// Barometric Trend (ex. falling)
	short moon_icon;// Corresponding icon for moon
	char *moon_name;// Name/Label for icon
	float uvi;	// UV Index
	char *uvid;	// UV Index Description
	xoap_weather_wind wind;
} xoap_weather_cc; // Current Conditions

typedef struct {
	xoap_weather_links links[TWCi_RL_COUNT];
	xoap_weather_cc *cc;
	xoap_weather_day *days;
	short total_days;
	char *id;	// ID for location
	char *name;	// Name for location (City, State/Country)
	float lat;	// Latitude
	float lon;	// Longitude
	char *sun_rise;	// Format (6:30 AM)
	char *sun_set;	// Format (8:50 PM)
	short zone;	// Time zone
	char *time_fetched; // time forecast was fetched
	struct {
		char *locale;
		char *ut; // units for temperature
		char *ud; // units for distance
		char *us; // units for speed
		char *up; // units for precipitation
		char *ur; // units for pressure
	} units;
} xoap_weather_forecast;

/** Forecast Handle */
typedef struct {
	char *locid;
	char  unit;
	xoap_weather_fetch_type fetch;
} xoap_weather_handle;

typedef struct {
	char *id;	// id for location
	char *name;	// name of location
} xoap_weather_loc;

typedef struct {
	short total_locations;
	xoap_weather_loc *loc;
} xoap_weather_locs;

typedef struct {
	float rain_last_hr;
	float rain_last_day;
	int is_raining;
	char *airport_id;
	char *station_id;
} xoap_weather_rain;

/** Used only for when storing the fetched xml document in memory */
struct xoap_weather_memory {
	char *memory;
	size_t size;
};


/** function declarations */

/**
 * Initialize function
 * RETURN: [failed: NULL] [success: a xoap_weather_handle object]
 */
xoap_weather_handle* xoap_weather_init(const char *ptID, const char *lkey);

/**
 * Use to set options for the handle object
 * RETURN: [failed: 1] [success: 0]
 */
int xoap_weather_setopt(xoap_weather_handle *hand, xoap_weather_opt opt, ...);

/**
 * Search for possible locations to fetch a forecast from.
 * Can search using the name of the city or the city, state.
 * NOTE: All 5 digit US Postal Code are already valid LocIDs, so a search is not needed.
 * RETURN: [failed: NULL] [success: a xoap_weather_search object]
 */
xoap_weather_locs* xoap_weather_search(const char *loc);
/**
 * Get the rain conditions from Weather Underground
 * RETURN: [failed: NULL] [success: a xoap_weather_rain object]
 */
xoap_weather_rain* xoap_weather_rain(xoap_weather_handle *hand);
/**
 * Fetch the forecast for a LocID (set in the handle object)
 * RETURN: [failed: NULL] [success: a xoap_weather_forecast object]
 */
xoap_weather_forecast* xoap_weather_fetch(const xoap_weather_handle *hand);

/**
 * Frees any memory that may have been used by globals
 * RETURN: NONE
 */
void xoap_weather_cleanup(xoap_weather_handle *hand);

/**
 * Frees a xoap_weather_search object
 * RETURN: NONE
 */
void xoap_weather_free_locs(xoap_weather_locs *s);

/**
 * Frees a xoap_weather_rain object
 * RETURN: NONE
 */
void xoap_weather_free_rain(xoap_weather_rain *r);

/**
 * Frees a xoap_weather_forecast object
 * RETURN: NONE
 */
void xoap_weather_free_forecast(xoap_weather_forecast *f);

#endif //XOAP_WEATHER_H_
