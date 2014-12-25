#include <iostream>
#include <cstdlib>
#include <sstream>
#include <ctime>

#include <unistd.h>

#include "httpdownloader.h"

#include "mysql_driver.h"
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>

using namespace std;


// Exception
class ProgramException {
private:
    string _message;
public:
    ProgramException(string Message) {
        _message = Message;
    }

    string ToString() {
        return _message;
    }
};

// Functions - defined after main
bool isMetar(xmlDocPtr Document, xmlNodePtr ParentNode);

// Main line
int main(void) {
    // Variables
    int returnValue = 0;    // The return value for the program - non-zero indicates an error
    xmlDocPtr pDoc = NULL;
    xmlXPathContextPtr pXPathContext = NULL;
    xmlXPathObjectPtr pXPathObj = NULL;
    xmlNodeSetPtr pNodeset = NULL;
    xmlNodePtr pNode = NULL;
    Metar metarData;
    HTTPDownloader netConn;
	bool hasUpdated = false;	// Keeps track if the metar update for the hour has been processed

    // Set the string for retreiving the info
    string cytzUrl = "http://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&stationString=cytz&hoursBeforeNow=2";
    string cyyzUrl = "http://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&stationString=cyyz&hoursBeforeNow=2";
    string cykzUrl = "http://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&stationString=cykz&hoursBeforeNow=2";

	// Start infinite loop
	while(true) {
		// Get the current time
		time_t epoch_time = time(0);
		struct tm* now = localtime(&epoch_time);
		int minutes = now->tm_min;	// Get the number of minutes after the hour
		
		if(minutes >= 30 && !hasUpdated) {	// Update the metar records
			// Iterate through the three stations
			for(register int i = 0; i < 3; i++) {
				string xmlMetar;
				if(i == 0) xmlMetar = netConn.download(cytzUrl);
				else if(i == 1) xmlMetar = netConn.download(cyyzUrl);
				else xmlMetar = netConn.download(cykzUrl);

				try {
					// Create the xml document structure
					pDoc = xmlParseMemory(xmlMetar.c_str(), xmlMetar.length());
					if(pDoc == NULL) throw ProgramException("Error creating the xml data structure for:\n" + xmlMetar); // Signal an error

					// Create the XPath context
					pXPathContext = xmlXPathNewContext(pDoc);
					if(pXPathContext == NULL) throw ProgramException("Failed to create XPath context\n");   // Signal an error

					// Get a listing of all METAR nodes
					pXPathObj = xmlXPathEvalExpression((xmlChar *) "/response/data/METAR", pXPathContext);
					if(pXPathObj == NULL) throw ProgramException("Failed to evaluate XPath\n"); // Signal an error

					// Check for results
					if(!xmlXPathNodeSetIsEmpty(pXPathObj->nodesetval)) {
						// Iterate through each result looking for the first regular METAR
						bool metarAbsent = true;    // Signals that a METAR-type record hasn't been found
						pNodeset = pXPathObj->nodesetval;
						for(register int i = 0; i < pNodeset->nodeNr; i++) {
							pNode = pNodeset->nodeTab[i];
							if((i == 0) || (isMetar(pDoc, pNode) && metarAbsent)) {
								// Initialize some data
								if(isMetar(pDoc, pNode)) metarAbsent = false;
								metarData.Clear();

								// Get the data by iterating through each item
								xmlNodePtr curNode = pNode->xmlChildrenNode;
								while(curNode) {
									// Look for all the recorded data
									xmlChar *value = xmlNodeListGetString(pDoc, curNode->xmlChildrenNode, 1);
									if(!xmlStrcmp(curNode->name, (const xmlChar *) "station_id"))	// Get the station
										metarData.station = (char *) value;
									else if(!xmlStrcmp(curNode->name, (const xmlChar *) "observation_time"))    // Get the metar time
										metarData.time = (char *) value;
									else if(!xmlStrcmp(curNode->name, (const xmlChar *) "temp_c")) // Get the temperature
										metarData.temp = atof((const char *) value);
									else if(!xmlStrcmp(curNode->name, (const xmlChar *) "wind_dir_degrees")) // Get the wind direction
										metarData.wind_dir = atof((const char *) value);
									else if(!xmlStrcmp(curNode->name, (const xmlChar *) "wind_speed_kt"))    // Get the wind speed
										metarData.wind_vel = atof((const char *) value);
									else if(!xmlStrcmp(curNode->name, (const xmlChar *) "sea_level_pressure_mb"))    // Get the pressure
										metarData.pressure = atof((const char *) value);
									else if(!xmlStrcmp(curNode->name, (const xmlChar *) "sky_condition")) {    // Get the cloud layer info
										// Check for the sky_cover attribute
										string cloudType;
										xmlAttrPtr attr_ptr = xmlHasProp(curNode, (const xmlChar *) "sky_cover");
										if(attr_ptr != NULL) {
											xmlChar *cloud_type = xmlGetProp(curNode, (const xmlChar *) "sky_cover");  // Get the cloud type as a string
											cloudType = string((const char *) cloud_type);
											if(cloud_type) xmlFree(cloud_type);

											// Check for the cloud base attribute
											attr_ptr = xmlHasProp(curNode, (const xmlChar *) "cloud_base_ft_agl");
											if(attr_ptr != NULL) {
												double cloud_base = atof((const char *) xmlGetProp(curNode, (const xmlChar *) "cloud_base_ft_agl")); // Get the cloud base

												// Convert the cloud layer to a string
												int base_int = (int)(cloud_base/100.0);
												string base_str = static_cast<ostringstream*>(&(ostringstream() << base_int))->str();
												if(base_int < 10) base_str = "00" + base_str;
												else if(base_int < 100) base_str = "0" + base_str;
												cloudType += base_str;	// Add the height to the cloud type
											}
										}

										// Add the layer to the current cloud information
										if(cloudType.length() > 0) {
											if(metarData.clouds.length() > 0) metarData.clouds += ";";
											metarData.clouds += cloudType;
										}
									}

									// Free the character array and move on to next node
									if(value) xmlFree(value);
									curNode = curNode->next;
								}
							}
						}
					}

					// Define variables for the database update
					sql::mysql::MySQL_Driver *pDriver;
					sql::Connection *pConn;
					sql::Statement *pQuery;
					sql::ResultSet *pResult;
					
					// Create the query string
					ostringstream query_stream;
					ostringstream check_stream;
					query_stream << "INSERT INTO metars VALUES(NULL,'" << metarData.station << "',FROM_UNIXTIME(UNIX_TIMESTAMP(STR_TO_DATE('" << metarData.time << "','%Y-%m-%dT%H:%i:%sZ')))," << metarData.temp << "," << metarData.wind_dir << "," << metarData.wind_vel << "," << metarData.pressure << ",'" << metarData.clouds << "');\n" << flush;
					check_stream << "SELECT * FROM metars WHERE station='" << metarData.station << "' AND zulu_time=STR_TO_DATE('" << metarData.time << "','%Y-%m-%dT%H:%i:%sZ');";
					string query_str = query_stream.str();
					string check_str = check_stream.str();
					//cout << query_str << "\n";
					// cout << "INSERT INTO metars VALUES(NULL,'" << metarData.station << "',STR_TO_DATE('" << metarData.time << "','%Y-%m-%dT%H:%i:%sZ')," << metarData.temp << "," << metarData.wind_dir << "," << metarData.wind_vel << "," << metarData.pressure << ",'" << metarData.clouds << "');\n" << flush;
					
					try {
						// Open the connection
						pDriver = sql::mysql::get_mysql_driver_instance();
						pConn = pDriver->connect("tcp://localhost:3306", "data_logger", "QwTXBQ3pQjdUXrMH");
						pConn->setSchema("home_monitor");	// Connect to the database
						
						// Check if the data exists
						pQuery = pConn->createStatement();
						pResult = pQuery->executeQuery(check_str);
						
						// Insert the new data, if it does not exist
						if(pResult->rowsCount() == 0) pQuery->execute(query_str);
						
						// Terminate the mysql connection
						//pConn->close();
						
						// Kill the connection
						delete pResult;
						delete pQuery;
						delete pConn;
//						delete pDriver;
					} catch(sql::SQLException &mysql_error) {
						cerr << "Error " << mysql_error.getErrorCode() << ": " << mysql_error.what() << endl;
					}
				} catch(ProgramException error) {
					returnValue = -1;
					cerr << error.ToString();
				}
			}
			hasUpdated = true;	// Signal that the update was made
//			usleep(10*60*1000);	// Sleep for about 10 minutes
		} else if(minutes < 30 && hasUpdated) hasUpdated = false;	// Reset the update flag
		
		// Sleep for 10 minutes
		usleep(10*60*1000);
	}
	
    // Free up the libxml objects
    if(pDoc) xmlFreeDoc(pDoc);
    if(pXPathContext) xmlXPathFreeContext(pXPathContext);
    if(pXPathObj) xmlXPathFreeObject(pXPathObj);

    return returnValue;
}

bool isMetar(xmlDocPtr Document, xmlNodePtr ParentNode) {
    // Get the first child node and iterate through series of nodes
    bool is_metar = false;
    xmlNodePtr curNode = ParentNode->xmlChildrenNode;
    while(curNode && !is_metar) {
        // Check that the current node is of metar_type
        if(!xmlStrcmp(curNode->name, (const xmlChar *) "metar_type")) {
            xmlChar *value = xmlNodeListGetString(Document, curNode->xmlChildrenNode, 1);
            if(!xmlStrcmp(value, (const xmlChar *) "METAR")) is_metar = true;
            xmlFree(value);
        }
        curNode = curNode->next;    // Advance through the list
    }
    return is_metar;
}
