// wxdata.h
// Contains the classes and structures for storing weather data
#ifndef __WXDATA_H__
#define __WXDATA_H__

//=============================================================================
// CloudLayer class
//=============================================================================
class CloudLayer {
public:
	// Public enum for layer type
	enum Type { UNKN, FEW, SCT, BRK, OVC };
	
	// private members
private:
	Type _coverage;	// The type of cloud layer, in terms of sky coverage
	int _base_agl;	// The height of the layer base, in feet AGL
	
public:
	CloudLayer() { }	// Default 'do nothing' constructor
	CloudLayer(Type layerType, int baseHeight);	// Initialize values
	// Can I create a constructor that takes only the portion of the xml data relevent to the layer?
	
	// Data access
	int getLayerBaseHeight() { return _base_agl; }
	string getString();
	
	// Operators
	bool operator <(const CloudLayer &ref_layer) { return this->_coverage < ref_layer._coverage; }
	bool operator <=(const CloudLayer &ref_layer) { return this->_coverage <= ref_layer._coverage; }
	bool operator >(const CloudLayer &ref_layer) { return this->_coverage > ref_layer._coverage; }
	bool operator >=(const CloudLayer &ref_layer) { return this->_coverage >= ref_layer._coverage; }
	bool operator ==(const CloudLayer &ref_layer) { return this->-coverage == ref_layer._coverage; }
};

//=============================================================================
// WXObservation class
//=============================================================================

//=============================================================================
// WXWrapper abstract class
//=============================================================================
class WXWrapper {
protected:
	// Protected members
	string _data;		// The raw text for the observation
	string _station;	// The station identifier
	string _timestamp;	// The date/time the data was issued or observed
	
public:
	WXData() { };			// Do nothing constructor
	WXData(string xmlString);	// Constructor to set the text and extract the information
	
	virtual void readXML(string xmlData) = 0;
};

//=============================================================================
// Metar class
//=============================================================================
class Metar : public WXWrapper {
    double temp;
    double wind_dir;
    double wind_vel;
    double pressure;
    string clouds;
};

//=============================================================================
// Taf class
//=============================================================================
class Taf : public WXWrapper {
};
#endif	// __WXDATA_H__