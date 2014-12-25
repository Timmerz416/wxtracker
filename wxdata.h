// wxdata.h
// Contains the classes and structures for storing weather data
#ifndef __WXDATA_H__
#define __WXDATA_H__

// wxdata abstract class
class wxdata {
protected:
	// Protected members
	string _data;		// The raw text for the observation
	string _station;	// The station identifier
	string _timestamp;	// The date/time the data was issued or observed
	
public:
	wxdata() { };			// Do nothing constructor
	wxdata(string xmlString);	// Constructor to set the text and extract the information
	
	virtual void readXML(string xmlData) = 0;
};

// Metar structure
struct Metar {
    enum CloudType { UNKN, FEW, SCT, BKN, OVC };

	string station;
    string time;
    double temp;
    double wind_dir;
    double wind_vel;
    double pressure;
//    double cloud_alt;
//    CloudType cloud_type;
    string clouds;

    void Clear() {
		station = "";
        time = "";
        temp = wind_dir = wind_vel = pressure = 0.0;
//        cloud_alt = -1.0;
//        cloud_type = Metar::UNKN;
        clouds = "";
    }
};

#endif	// __WXDATA_H__