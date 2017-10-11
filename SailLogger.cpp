
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <chrono>
#include <date/date.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include "libgpsmm.h"

extern "C" {
	#include "roboticscape.h"
	#include "curl/curl.h"
}

#define  	foresailIP	("192.168.0.90")
#define 	mizzenIP 	("192.168.0.91")
#define 	logfile		("/tmp/saillog.log")

using namespace std;
using namespace std::chrono;
using namespace date;

// And here there is a global so I can implement the callback
rc_imu_data_t imuData;

// This is the write callback for libcurl
// userp is expected to be of type std::string; if it is not, bad strange things will happen
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	string *target = (string *)userp;
	char *ptr = (char *)contents;

	//target->clear();
	size *= nmemb;
	target->reserve(size);
	for (size_t i = 0; i < size; i++) {
		*target += ptr[i];
	}
	return size;
} 

string fetchWingData(string url) {
	CURLcode ret;
	CURL *hnd;
	string response;
	char errbuf[CURL_ERROR_SIZE];

	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
	curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.38.0");
	curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(hnd, CURLOPT_SSH_KNOWNHOSTS, "/home/debian/.ssh/known_hosts");
	curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(hnd, CURLOPT_FAILONERROR, 1);			// trigger a failure on a 400-series return code.

	// set up data writer callback
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&response);

	// make request
	ret = curl_easy_perform(hnd);

	// clean up request
	curl_easy_cleanup(hnd);
	hnd = NULL;

	return response;
}

void printData () {
	static fstream fs;
	static struct gps_data_t* gpsdata = NULL;
	static gpsmm gps_rec(GPSD_SHARED_MEMORY, DEFAULT_GPSD_PORT);
	double lat = 0;
	double lon = 0;
	double speed = 0; 
	double track = 0;

	if (!fs.is_open()) {
		fs.open(logfile);
	}

	cout << ".";

	string fore = fetchWingData(foresailIP);
	string mizz = fetchWingData(mizzenIP);
	boost::algorithm::trim(fore);
	boost::algorithm::trim(mizz);
	sys_time<milliseconds> tp = floor<milliseconds>(system_clock::now());
	gpsdata = gps_rec.read();
	if (gpsdata != NULL) {					// check if we have data
		if (gpsdata->set & LATLON_SET) {	// check if the latitude/longitude are set
			lat = gpsdata->fix.latitude;
			lon = gpsdata->fix.longitude;
		}
		if (gpsdata->set & SPEED_SET) {		// check if the speed over ground is set
			speed = gpsdata->fix.speed;
		}
		if (gpsdata->set & TRACK_SET) {		// check if the course over ground is set
			track = gpsdata->fix.track;
		}
	}


	fs << tp << ",";
	fs << fore << ",";
	fs << mizz << ",";
	fs << to_string(imuData.compass_heading);
	fs << to_string(lat) << "," << to_string(lon) << "'";
	fs << to_string(speed) << "," << to_string(track) << endl;
	cout << tp << ",";
	cout << fore << ",";
	cout << mizz << ",";
	cout << to_string(imuData.compass_heading);
	cout << to_string(lat) << "," << to_string(lon) << "'";
	cout << to_string(speed) << "," << to_string(track) << endl;
}

int main(){
	// always initialize cape library first
	if(rc_initialize()){
		cerr << "ERROR: failed to initialize rc_initialize(), are you root?" << endl;
		return -1;
	}

	// do your own initialization here
	cout << "\nHello Sailboat!" << endl;

	// configure IMU
	rc_imu_config_t imuConf;
	rc_set_imu_config_to_defaults(&imuConf);
	imuConf.enable_magnetometer = 1;
	imuConf.orientation = ORIENTATION_X_FORWARD;
	imuConf.dmp_sample_rate = 20;
	rc_set_imu_interrupt_func(printData);

	if(rc_initialize_imu_dmp(&imuData, imuConf)) {
		cerr << "ERROR: failed to run rc_initialize_imu_dmp()" << endl;
		return -1;
	}

	// done initializing so set state to RUNNING
	cout << "Starting process running" << endl;
	rc_set_state(RUNNING); 

	// Keep looping until state changes to EXITING
	while(rc_get_state()!=EXITING) {
		usleep(10000);
	}

	// exit cleanly
	rc_cleanup(); 
	return 0;
}