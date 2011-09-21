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
 *    * Neither the name of the Weather Channel nor the names of the 
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
#include "xoapweather.h"

/**
 * Static function declarations
 * Needed for internal use only
 */
static int  xoap_weather_error_check(xmlNode *root);
static int  xoap_weather_parse_forecast(const struct xoap_weather_memory *xml, xoap_weather_forecast *forecast);
static void xoap_weather_parse_head(xmlNode *cur_node, xoap_weather_forecast *forecast);
static void xoap_weather_parse_lnks(xmlNode *cur_node, xoap_weather_forecast *forecast);
static void xoap_weather_parse_loc(xmlNode *cur_node, xoap_weather_forecast *forecast);
static void xoap_weather_parse_swa(xmlNode *cur_node, xoap_weather_forecast *forecast);
static void xoap_weather_parse_cc(xmlNode *cur_node, xoap_weather_forecast *forecast);
static void xoap_weather_parse_dayf(xmlNode *cur_node, xoap_weather_forecast *forecast);
static void xoap_weather_parse_wind(xmlNode *cur_node, xoap_weather_wind *wind);
static void xoap_weather_parse_day(xmlNode *cur_node, xoap_weather_day *day);
static void xoap_weather_parse_part(xmlNode *cur_node, xoap_weather_weather *weather);
static void xoap_weather_free_weather(xoap_weather_weather *w);
static size_t xoap_weather_memory_callback(void *ptr, size_t size, size_t nmemb, void *data);

/** variables */
char *partnerID;
char *licenseKey;
CURL *curlHandle;


xoap_weather_handle *
xoap_weather_init(const char *ptID, const char *lkey) {
	if ( ptID == NULL || lkey == NULL ) {
	fprintf(stderr, "ERROR: partner ID and/or License key passed to xoap_weather_init() is NULL\n");
		return NULL;
	}

	partnerID = strdup(ptID);
	licenseKey = strdup(lkey);
	/* initialize a curl easy handle */
	curlHandle = curl_easy_init();

	xoap_weather_handle *h = malloc(sizeof(xoap_weather_handle));
	h->locid = NULL;
	h->unit = 's'; // Make STANDARD the default unit
	return h;
}

int
xoap_weather_setopt(xoap_weather_handle *hand, xoap_weather_opt opt, ...) {
	if (hand == NULL) {
		fprintf(stderr, "ERROR: handle passed to xoap_weather_setopt() is NULL\n");
		return 1;
	}
	
	va_list ap;
	va_start(ap, opt);
	switch (opt) {
		case XOAP_WEATHER_OPT_ID:
			hand->locid = strdup(va_arg(ap, char*));
			break;
		case XOAP_WEATHER_OPT_UNIT:
			hand->unit = (va_arg(ap, xoap_weather_unit) == XOAP_WEATHER_UNIT_METRIC) ? 'm': 's';
			break;
		case XOAP_WEATHER_OPT_FETCH:
			hand->fetch = va_arg(ap, xoap_weather_fetch_type);
			break;
		default:
			//ERROR
			va_end(ap);
			fprintf(stderr, "ERROR: invalid option passed to xoap_weather_setopt()\n");
			return 1;
	}
	va_end(ap);
	return 0;
}

int
xoap_weather_error_check(xmlNode *root) {
	if (strncmp((char *)root->name, "error", 5) == 0) {
		xmlNode *node;
		for (node = root->children; (node != NULL); node = node->next) {
			if (node->type == XML_ELEMENT_NODE && strncmp((char *)node->name, "err", 3) == 0) {
				xmlChar *prop = xmlGetProp(node, (const xmlChar *)"type");
				switch ((int)atoi((char *)prop)) {
					case XOAP_WEATHER_INVALID_LOCATION:
						fprintf(stderr, "ERROR: Invalid location set in handle\n");
						xmlFree(prop);
						return 1;
					case XOAP_WEATHER_UNKNOWN_ERROR:
						fprintf(stderr, "ERROR: UNKNOWN ERROR\n");
						xmlFree(prop);
						return 1;
					case XOAP_WEATHER_NO_LOCATION:
						fprintf(stderr, "ERROR: No location set in handle\n");
						xmlFree(prop);
						return 1;
					case XOAP_WEATHER_INVALID_PID:
						fprintf(stderr, "ERROR: Invalid partner id\n");
						xmlFree(prop);
						return 1;
					case XOAP_WEATHER_INVALID_PCODE:
						fprintf(stderr, "ERROR: Invalid product code\n");
						xmlFree(prop);
						return 1;
					case XOAP_WEATHER_INVALID_LKEY:
						fprintf(stderr, "ERROR: Invalid license key\n");
						xmlFree(prop);
						return 1;
				}
				xmlFree(prop);
			}
		}
	}
	return 0;
}

xoap_weather_locs *
xoap_weather_search(const char *loc) {
	if ( !curlHandle ) {
		fprintf(stderr, "ERROR: curl handle was NULL. Call xoap_weather_init() first\n");
		return NULL;
	}

	/* build search URL */
	int urlSize = 180;
	char url[urlSize];
	snprintf(url, urlSize, TWCi_SEARCH_URL"?where=%s", loc);

	struct xoap_weather_memory xml;
	xml.memory = NULL; 
	xml.size = 0;    /* no data at this point */

	/* set options for curl handle */
	curl_easy_setopt(curlHandle, CURLOPT_URL, url);
	curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&xml);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, xoap_weather_memory_callback);
	
	/* perform file transfer */
	CURLcode result;
	if ( (result = curl_easy_perform(curlHandle)) != 0 ) {
		fprintf(stderr, "%s\n", curl_easy_strerror(result));
		fprintf(stderr, "ERROR: file transfer failed\n");
		return NULL;
	}

	/* create data structure */
	xoap_weather_locs *wloc = malloc(sizeof(xoap_weather_locs));
	wloc->total_locations = 0;
	wloc->loc = NULL;

	/* Open XML document */
	xmlDocPtr doc;
	doc = xmlReadMemory(xml.memory, xml.size, "xml", "UTF-8", 0);
	if (doc == NULL) {
		fprintf(stderr, "ERROR: failed to parse XML document: xoap_weather_search_location()\n");
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wloc);
		xmlCleanupParser();
		return NULL;
	}

	/*Get the root element node */
	xmlNode *root = NULL;
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		fprintf(stderr, "ERROR: failed to get root element from XML document\n");
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wloc);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return NULL;
	}

	if (xoap_weather_error_check(root) != 0) {
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wloc);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return NULL;
	}

	/* parse xml data */
	xmlNode *cur_node;
	//xmlChar *id, *content;
	for(cur_node = root->children; cur_node != NULL; cur_node = cur_node->next) {
		if ( cur_node->type == XML_ELEMENT_NODE && strcmp((char*)cur_node->name, "loc") == 0 ) {
			wloc->loc = realloc(wloc->loc, (wloc->total_locations+1) * (sizeof(xoap_weather_loc)));
			//id = xmlGetProp(cur_node, (const xmlChar *)"id");
			wloc->loc[wloc->total_locations].id = (char *)xmlGetProp(cur_node, (const xmlChar *)"id");
			//content = xmlNodeGetContent(cur_node);
			wloc->loc[wloc->total_locations].name = (char *)xmlNodeGetContent(cur_node);
			wloc->total_locations++;
		}
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();
	XOAPW_FREE(xml.memory);

	return wloc;
}

xoap_weather_rain *
xoap_weather_rain(const xoap_weather_handle *hand) {
	if (hand == NULL || hand->locid == NULL) {
		fprintf(stderr, "ERROR: handle is NULL or handle option XOAP_WEATHER_OPT_ID not set");
		return NULL;
	}

	/* build URL */
	int urlSize = 180
	char url[urlSize];
	snprintf(url, urlSize, WU_SEARCH_URL"%s", loc);

	struct xoap_weather_memory xml;
	xml.memory = NULL;
	xml.size = 0;    /* no data at this point */

	/* set options for curl handle */
	curl_easy_setopt(curlHandle, CURLOPT_URL, url);
	curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&xml);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, xoap_weather_memory_callback);

	/* perform file transfer */
	CURLcode result;
	if ( (result = curl_easy_perform(curlHandle)) != 0 ) {
		fprintf(stderr, "%s\n", curl_easy_strerror(result));
		fprintf(stderr, "ERROR: file transfer failed\n");
		return NULL;
	}

	/* create data structure */
	xoap_weather_rain *wrain = malloc(sizeof(xoap_weather_rain));
	wrain->airport_id = NULL;
	wrain->station_id = NULL;
	wrain->rain_last_day = 0;
	wrain->rain_last_hr = 0;
	wrain->is_raining = 0;

	/* Open XML document */
	xmlDocPtr doc;
	doc = xmlReadMemory(xml.memory, xml.size, "xml", "UTF-8", 0);
	if (doc == NULL) {
		fprintf(stderr, "ERROR: failed to parse XML document: xoap_weather_search_location()\n");
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wrain);
		xmlCleanupParser();
		return NULL;
	}

	/*Get the root element node */
	xmlNode *root = NULL;
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		fprintf(stderr, "ERROR: failed to get root element from XML document\n");
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wrain);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return NULL;
	}

	if (xoap_weather_error_check(root) != 0) {
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wrain);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return NULL;
	}

	/* parse xml data */
	xmlNode *cur_node,c_node,cc_node,cc_node,ccc_node;
	for(cur_node = root->children; cur_node != NULL; cur_node = cur_node->next) {
		if ( cur_node->type == XML_ELEMENT_NODE &&
				strcmp((char*)cur_node->name, "nearby_weather_stations") == 0 ) {
			for (c_node = cur_node->children; c_node != NULL; c_node = c_node->next) {
				/* first let's deal with the airports */
				if ( c_node->type == XML_ELEMENT_NODE && strcmp(c_node->name, "airport") == 0){
					cc_node = c_node->children; //get just the first station
					if (cc_node->type == XML_ELEMENT_NODE && strcmp(cc_node->name, "station") == 0){
						for (ccc_node = cc_node->children; ccc_node != NULL; ccc_node->next) {
							if (ccc_node->type == XML_ELEMENT_NODE && strcmp(ccc_node->name, "icao") == 0){
								wrain->airport_id = (char *)xmlNodeGetContent(ccc_node);
							}
						}
					}
				}
				if (c_node->type == XML_ELEMENT_NODE && strcmp(c_node->name, "pws") == 0) {
					cc_node = c_node->children; //get just the first station
					if (cc_node->type == XML_ELEMENT_NODE && strcmp(cc_node->name, "station") == 0) {
						for (ccc_node = cc_node->children; ccc_node != NULL; ccc_node->next) {
							if (ccc_node->type == XML_ELEMENT_NODE && strcmp(ccc_node->name, "id") == 0){
								wrain->station_id = (char *)xmlNodeGetContent(ccc_node);
							}
						}
					}
				}
			}
		}
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
	XOAPW_FREE(xml.memory);

	/* Now we need to do this again to get the rain data */
	snprintf(url, urlSize, WU_WEATHER_STATION_URL"%s", wrain->station_id);

	xml.memory = NULL;
	xml.size = 0;    /* no data at this point */

	/* set options for curl handle */
	curl_easy_setopt(curlHandle, CURLOPT_URL, url);
	curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&xml);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, xoap_weather_memory_callback);

	/* perform file transfer */
	CURLcode result;
	if ( (result = curl_easy_perform(curlHandle)) != 0 ) {
		fprintf(stderr, "%s\n", curl_easy_strerror(result));
		fprintf(stderr, "ERROR: file transfer failed\n");
		return NULL;
	}

	/* Open XML document */
	doc = xmlReadMemory(xml.memory, xml.size, "xml", "UTF-8", 0);
	if (doc == NULL) {
		fprintf(stderr, "ERROR: failed to parse XML document: xoap_weather_search_location()\n");
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wrain);
		xmlCleanupParser();
		return NULL;
	}

	/*Get the root element node */
	*root = NULL;
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		fprintf(stderr, "ERROR: failed to get root element from XML document\n");
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wrain);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return NULL;
	}

	if (xoap_weather_error_check(root) != 0) {
		XOAPW_FREE(xml.memory);
		XOAPW_FREE(wrain);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return NULL;
	}

	for(cur_node = root->children; cur_node != NULL; cur_node = cur_node->next) {
		if ( cur_node->type == XML_ELEMENT_NODE){
			XOAPW_SEI(cur_node,"precip_1_hr_in",wrain->rain_last_hr);
			XOAPW_SEI(cur_node,"precip_today_in",wrain->rain_last_day);
		}

	xmlFreeDoc(doc);
	xmlCleanupParser();
	XOAPW_FREE(xml.memory);

	return wrain;
}

xoap_weather_forecast *
xoap_weather_fetch(const xoap_weather_handle *hand) {
	if (hand == NULL || hand->locid == NULL) {
		fprintf(stderr, "ERROR: handle is NULL or handle option XOAP_WEATHER_OPT_ID not set\n");
		return NULL;
	}


	/* build URL */
	int urlSize = 180;
	char url[urlSize];
	switch (hand->fetch) {
		case XOAP_WEATHER_FETCH_LOC:
			snprintf(url, urlSize, 
				TWCi_WEATHER_URL"%s?link=xoap&prod=xoap&par=%s&key=%s&unit=%c",
				hand->locid, partnerID, licenseKey, hand->unit
			);
			break;
		case XOAP_WEATHER_FETCH_CC:
			snprintf(url, urlSize, 
				TWCi_WEATHER_URL"%s?cc=*&link=xoap&prod=xoap&par=%s&key=%s&unit=%c",
				hand->locid, partnerID, licenseKey, hand->unit
			);
			break;
		case XOAP_WEATHER_FETCH_DAYF1:
		case XOAP_WEATHER_FETCH_DAYF2:
		case XOAP_WEATHER_FETCH_DAYF3:
		case XOAP_WEATHER_FETCH_DAYF4:
		case XOAP_WEATHER_FETCH_DAYF5:
			snprintf(url, urlSize, 
				TWCi_WEATHER_URL"%s?cc=*&dayf=%u&link=xoap&prod=xoap&par=%s&key=%s&unit=%c",
				hand->locid, hand->fetch - 1, partnerID, licenseKey, hand->unit
				);
			break;
		default:
			/* Invalid XOAP_WEATHER_FETCH type given */
			fprintf(stderr, "ERROR: invalid XOAP_WEATHER_FETCH was set to options handle\n");
			return NULL;
			break;
	}

	struct xoap_weather_memory xml;
	xml.memory=NULL; 
	xml.size = 0;    /* no data at this point */

	/* set options for curl handle */
	curl_easy_setopt(curlHandle, CURLOPT_URL, url);
	curl_easy_setopt(curlHandle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&xml);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, xoap_weather_memory_callback);
	/* perform file transfer */
	CURLcode result;
	if ( (result = curl_easy_perform(curlHandle)) != 0 ) {
		fprintf(stderr, "%s\n", curl_easy_strerror(result));
		return NULL;
	}

	/* create data structure */
	xoap_weather_forecast *fcast = malloc(sizeof(xoap_weather_forecast));
	fcast->cc = NULL;
	fcast->days = NULL;
	fcast->total_days = 0;

	/* allocate space and initialize for cc and days (if needed) */
	if (hand->fetch != XOAP_WEATHER_FETCH_LOC) {
		fcast->cc = malloc(sizeof(xoap_weather_cc));
		fcast->cc->swa = NULL;
		fcast->cc->swal = NULL;
		fcast->cc->lsup = NULL;
		fcast->cc->obst = NULL;
		fcast->cc->cond = NULL;
		fcast->cc->bart = NULL;
		fcast->cc->moon_name = NULL;
		fcast->cc->uvid = NULL;
		fcast->cc->wind.speed = NULL;
		fcast->cc->wind.gust = NULL;
		fcast->cc->wind.direction = NULL;
	}

	if (hand->fetch != XOAP_WEATHER_FETCH_LOC || hand->fetch != XOAP_WEATHER_FETCH_CC) {
		fcast->days = malloc(sizeof(xoap_weather_day) * (hand->fetch - 1));
		fcast->total_days = hand->fetch - 1;
		xoap_weather_day *day;
		int i;
		for (i = 0; (i < fcast->total_days); i++) {
			day = &fcast->days[i];
			day->week_day = NULL;
			day->date = NULL;
			day->sun_rise = NULL;
			day->sun_set = NULL;
			day->day.cond = NULL;
			day->day.cond_short = NULL;
			day->day.wind.speed = NULL;
			day->day.wind.gust = NULL;
			day->day.wind.direction = NULL;
			day->night.cond = NULL;
			day->night.cond_short = NULL;
			day->night.wind.speed = NULL;
			day->night.wind.gust = NULL;
			day->night.wind.direction = NULL;
		}
	}

	/* parse xml data */
	if (xoap_weather_parse_forecast(&xml, fcast) != 0) {
		XOAPW_FREE(fcast->cc);
		XOAPW_FREE(fcast->days);
		XOAPW_FREE(fcast);
		fcast = NULL;
	}

	XOAPW_FREE(xml.memory);
	return fcast;
}

int
xoap_weather_parse_forecast(const struct xoap_weather_memory *xml, xoap_weather_forecast *fcast) {
	/* Open XML document */
	xmlDocPtr doc;
	doc = xmlReadMemory(xml->memory, xml->size, "xml", "UTF-8", 0);
	if (doc == NULL) {
		fprintf(stderr, "ERROR: failed to parse XML document\n");
		xmlCleanupParser();
		return 1;
	}
	/* Get the root element node */
	xmlNode *root = NULL;
	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		fprintf(stderr, "ERROR: failed to get root element from XML document\n");
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return 1;
	}

	/* Check if there are any errors */
	if (xoap_weather_error_check(root) != 0) {
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return 1;
	}


	/* See if we support the version of the given data */
	xmlChar *prop = xmlGetProp(root, (const xmlChar *)"ver");
	if ( TWCi_VERSION != (float)atof((char *)prop))
		fprintf(stderr,"WARNING!!! "XOAP_WEATHER_LIBNAME" does not support fetched data. Parsing results UNKNOWN!!!\n");
	xmlFree(prop);


	xmlNode *cur_node;
	for(cur_node = root->children; (cur_node != NULL); cur_node = cur_node->next) {
		if ( cur_node->type == XML_ELEMENT_NODE ) {

			/* head */
			if ( strcmp((char *)cur_node->name, "head") == 0 ) {
				xoap_weather_parse_head(cur_node, fcast);
			}

			/* links */
			if ( strcmp((char *)cur_node->name, "lnks") == 0 ) {
				xoap_weather_parse_lnks(cur_node, fcast);
			}

			/* loc */
			if ( strcmp((char *)cur_node->name, "loc") == 0 ) {
				fcast->id = (char *)xmlGetProp(cur_node, (const xmlChar *)"id");
				xoap_weather_parse_loc(cur_node, fcast);
			}

			/* swa */ 
			if ( strcmp((char *)cur_node->name, "swa") == 0 ) {
				xoap_weather_parse_swa(cur_node, fcast);
			}

			/* cc */
			if ( strcmp((char *)cur_node->name, "cc") == 0 ) {
				xoap_weather_parse_cc(cur_node, fcast);
			}
			
			/* dayf */
			if (  strcmp((char *)cur_node->name, "dayf") == 0 ) {
				xoap_weather_parse_dayf(cur_node, fcast);
			}

		}
	}

	/* free the document */
	xmlFreeDoc(doc);
	/* Free global variables that may have been allocated by the parser */
	xmlCleanupParser();
	return 0;
}


void
xoap_weather_parse_head(xmlNode *cur_node, xoap_weather_forecast *fcast) {
	xmlNode *node; // child node
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			XOAPW_SES(node, "locale", fcast->units.locale)
			XOAPW_SES(node, "ut", fcast->units.ut)
			XOAPW_SES(node, "ud", fcast->units.ud)
			XOAPW_SES(node, "us", fcast->units.us)
			XOAPW_SES(node, "up", fcast->units.up)
			XOAPW_SES(node, "ur", fcast->units.ur)
		}
	}
}

void
xoap_weather_parse_lnks(xmlNode *cur_node, xoap_weather_forecast *fcast) {
	xmlNode *node, *c_node; // child node, child's child(c_node)
	int i = 0;
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			if ( strcmp((char *)node->name, "link") == 0 ) {
				for (c_node = node->children; (c_node != NULL); c_node = c_node->next) {
					if ( c_node->type == XML_ELEMENT_NODE ) {
						XOAPW_SES(c_node, "l", fcast->links[i].link)
						XOAPW_SES(c_node, "t", fcast->links[i].description)
					}
				}
				i++;
			}
		}
	}
}

void
xoap_weather_parse_loc(xmlNode *cur_node, xoap_weather_forecast *fcast) {
	xmlNode *node; // child node
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			XOAPW_SES(node, "tm", fcast->time_fetched)
			XOAPW_SEF(node, "lat", fcast->lat)
			XOAPW_SEF(node, "lon", fcast->lon)
			XOAPW_SES(node, "sunr", fcast->sun_rise)
			XOAPW_SES(node, "suns", fcast->sun_set)
			XOAPW_SEI(node, "zone", fcast->zone)
			XOAPW_SES(node, "dnam", fcast->name)
		}
	}
}

void
xoap_weather_parse_swa(xmlNode *cur_node, xoap_weather_forecast *fcast) {
	xmlNode *node, *c_node; // child node, child's child(c_node)
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE && strcmp((char *)node->name, "a") == 0 ) {
			for (c_node = node->children; (c_node != NULL); c_node = c_node->next) {
				if ( c_node->type == XML_ELEMENT_NODE ) {
					XOAPW_SES(c_node, "t", fcast->cc->swa)
					XOAPW_SES(c_node, "l", fcast->cc->swal)
				}
			}
		}
	}
}

void
xoap_weather_parse_cc(xmlNode *cur_node, xoap_weather_forecast *fcast) {
	xmlNode *node, *c_node; // child node, child's child(c_node)
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			XOAPW_SES(node, "lsup", fcast->cc->lsup)
			XOAPW_SES(node, "obst", fcast->cc->obst)
			XOAPW_SEI(node, "tmp", fcast->cc->temp)
			XOAPW_SEI(node, "flik", fcast->cc->feels)
			XOAPW_SES(node, "t", fcast->cc->cond)
			XOAPW_SEI(node, "icon", fcast->cc->icon)
			XOAPW_SEI(node, "hmid", fcast->cc->humidity)
			XOAPW_SEF(node, "vis", fcast->cc->visibility)
			XOAPW_SEI(node, "dewp", fcast->cc->dewp)
			if ( strcmp((char *)node->name, "bar") == 0 ) {
				for (c_node = node->children; (c_node != NULL); c_node = c_node->next) {
					if ( c_node->type == XML_ELEMENT_NODE ) {
						XOAPW_SEF(c_node, "r", fcast->cc->barp)
						XOAPW_SES(c_node, "d", fcast->cc->bart)
					}
				}
			}
			if ( strcmp((char *)node->name, "uv") == 0 ) {
				for (c_node = node->children; (c_node != NULL); c_node = c_node->next) {
					if ( c_node->type == XML_ELEMENT_NODE ) {
						XOAPW_SEF(c_node, "i", fcast->cc->uvi)
						XOAPW_SES(c_node, "t", fcast->cc->uvid)
					}
				}
			}
			if ( strcmp((char *)node->name, "moon") == 0 ) {
				for (c_node = node->children; (c_node != NULL); c_node = c_node->next) {
					if ( c_node->type == XML_ELEMENT_NODE ) {
						XOAPW_SEI(c_node, "icon", fcast->cc->moon_icon)
						XOAPW_SES(c_node, "t", fcast->cc->moon_name)
					}
				}
			}
			if ( strcmp((char *)node->name, "wind") == 0 ) {
				xoap_weather_parse_wind(node, &fcast->cc->wind);
			}
		}
	}
}

void
xoap_weather_parse_dayf(xmlNode *cur_node, xoap_weather_forecast *forecast) {
	xmlNode *node;
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			if ( strcmp((char *)node->name, "day") == 0 ) {
				/* Get the day's spot in the forcast (0=>today, 1=>tomorrow, etc.) */
				xmlChar *prop = xmlGetProp(node, (const xmlChar *)"d");
				short day_number = (short)atoi((char *)prop);
				xmlFree(prop);
				
				/* Fetch the name for the day of the week */
				forecast->days[day_number].week_day = (char *)xmlGetProp(node, (const xmlChar *)"t");
				/* Fetch the date for the day */
				forecast->days[day_number].date = (char *)xmlGetProp(node, (const xmlChar *)"dt");
				xoap_weather_parse_day(node, &forecast->days[day_number]);
			}
		}
	}
}

void
xoap_weather_parse_wind(xmlNode *cur_node, xoap_weather_wind *wind) {
	xmlNode *node; // child node
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			XOAPW_SES(node, "s", wind->speed)
			XOAPW_SES(node, "gust", wind->gust)
			XOAPW_SEI(node, "d", wind->direction_n)
			XOAPW_SES(node, "t", wind->direction)
		}
	}
}

void
xoap_weather_parse_day(xmlNode *cur_node, xoap_weather_day *day) {
	xmlNode *node; // child node
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			XOAPW_SEI(node, "hi", day->high)
			XOAPW_SEI(node, "low", day->low)
			XOAPW_SES(node, "sunr", day->sun_rise)
			XOAPW_SES(node, "suns", day->sun_set)
			if ( strcmp((char *)node->name, "part") == 0 ) {
				/* see if part is for day or night */
				xmlChar *dorn = xmlGetProp(node, (const xmlChar *)"p");
				if ( strcmp((char *)dorn, "d") )
					xoap_weather_parse_part(node, &day->day);
				else
					xoap_weather_parse_part(node, &day->night);
				xmlFree(dorn);
			}
		}
	}
}

void
xoap_weather_parse_part(xmlNode *cur_node, xoap_weather_weather *w) {
	xmlNode *node; // child node
	for (node = cur_node->children; (node != NULL); node = node->next) {
		if ( node->type == XML_ELEMENT_NODE ) {
			XOAPW_SEI(node, "icon", w->icon)
			XOAPW_SES(node, "t", w->cond)
			XOAPW_SES(node, "bt", w->cond_short)
			XOAPW_SEI(node, "ppcp", w->chance_precipitation)
			XOAPW_SEI(node, "hmid", w->humidity)
			if ( strcmp((char *)node->name, "wind") == 0 ) {
				xoap_weather_parse_wind(node, &w->wind);
			}
		}
	}
}


void
xoap_weather_cleanup(xoap_weather_handle *h) {
	curl_easy_cleanup(curlHandle);
	curl_global_cleanup();
	XOAPW_FREE(partnerID);
	XOAPW_FREE(licenseKey);
	XOAPW_FREE(h->locid);
	XOAPW_FREE(h);
}

void
xoap_weather_free_locs(xoap_weather_locs *s) {
	if (s == NULL || s->loc == NULL)
		return;

	short i;
	for (i = 0; (i < s->total_locations); i++) {
		XOAPW_FREE(s->loc[i].id);
		XOAPW_FREE(s->loc[i].name);
	}
	free(s);
}

void
xoap_weather_free_rain(xoap_weather_rain *r) {
	XOAPW_FREE(r->rain_last_day);
	XOAPW_FREE(r->rain_last_hr);
	XOAPW_FREE(r->is_raining);
	XOAPW_FREE(r->airport_id);
	XOAPW_FREE(r->station_id);
	free(r);
}

void
xoap_weather_free_forecast(xoap_weather_forecast *f) {
	XOAPW_FREE(f->time_fetched);
	XOAPW_FREE(f->id);
	XOAPW_FREE(f->name);
	XOAPW_FREE(f->sun_rise);
	XOAPW_FREE(f->sun_set);
	
	// Units
	XOAPW_FREE(f->units.locale);
	XOAPW_FREE(f->units.ut);
	XOAPW_FREE(f->units.ud);
	XOAPW_FREE(f->units.us);
	XOAPW_FREE(f->units.up);
	XOAPW_FREE(f->units.ur);

	// Links
	int i;
	for (i = 0; (i < TWCi_RL_COUNT); i++) {
		XOAPW_FREE(f->links[i].link);
		XOAPW_FREE(f->links[i].description);
	}

	// Current conditions
	if (f->cc) {
		XOAPW_FREE(f->cc->lsup);
		XOAPW_FREE(f->cc->obst);
		XOAPW_FREE(f->cc->cond);
		XOAPW_FREE(f->cc->swa);
		XOAPW_FREE(f->cc->swal);
		XOAPW_FREE(f->cc->bart);
		XOAPW_FREE(f->cc->moon_name);
		XOAPW_FREE(f->cc->uvid);
		XOAPW_FREE(f->cc->wind.speed);
		XOAPW_FREE(f->cc->wind.gust);
		XOAPW_FREE(f->cc->wind.direction);
		free(f->cc);
	}

	// Days
	if (f->days) {
		for (i = 0; (i < f->total_days); i++) {
			XOAPW_FREE(f->days[i].week_day);
			XOAPW_FREE(f->days[i].date);
			XOAPW_FREE(f->days[i].sun_rise);
			XOAPW_FREE(f->days[i].sun_set);
			xoap_weather_free_weather(&f->days[i].day);
			xoap_weather_free_weather(&f->days[i].night);
		}
		free(f->days);
	}
	free(f);
}

void
xoap_weather_free_weather(xoap_weather_weather *w) {
	if (w) {
		XOAPW_FREE(w->cond);
		XOAPW_FREE(w->cond_short);
		XOAPW_FREE(w->wind.speed);
		XOAPW_FREE(w->wind.gust);
		XOAPW_FREE(w->wind.direction);
	}
}

size_t
xoap_weather_memory_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct xoap_weather_memory *mem = (struct xoap_weather_memory *)data;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}
