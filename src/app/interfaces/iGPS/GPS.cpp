/************************************************************/
/*    FILE: GPS.cpp
/*    ORGN: ENSTA Bretagne Robotics - moos-ivp-enstabretagne
/*    AUTH: Thomas Le Mezo
/*    DATE: 2015
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "GPS.h"

using namespace std;

//---------------------------------------------------------
// Constructor

GPS::GPS()
: m_io(), m_serial(m_io) {
    m_uart_port = "/dev/ttyUSB0";

    m_buffer.prepare(2048); // Alocate some space for the buffer (according to NMEA size of trames)

    nmea_zero_INFO(&m_info);
    nmea_parser_init(&m_parser);

    m_speed = 0;
    m_heading = 0;
}

GPS::~GPS() {
    m_serial.close();
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool GPS::OnNewMail(MOOSMSG_LIST &NewMail) {
    AppCastingMOOSApp::OnNewMail(NewMail);

    MOOSMSG_LIST::iterator p;
    for (p = NewMail.begin(); p != NewMail.end(); p++) {
        CMOOSMsg &msg = *p;
        string key = msg.GetKey();

#if 0 // Keep these around just for template
        string comm = msg.GetCommunity();
        double dval = msg.GetDouble();
        string sval = msg.GetString();
        string msrc = msg.GetSource();
        double mtime = msg.GetTime();
        bool mdbl = msg.IsDouble();
        bool mstr = msg.IsString();
#endif

        if (key != "APPCAST_REQ") // handle by AppCastingMOOSApp
            reportRunWarning("Unhandled Mail: " + key);
    }

    return (true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool GPS::OnConnectToServer() {
    registerVariables();
    return (true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool GPS::Iterate() {
    AppCastingMOOSApp::Iterate();
    // Read trame from COM port

    boost::asio::read_until(m_serial, m_buffer, "\r\n", m_error);

    // Debug
    //Notify("GPS_buffer_size", buffer.size()); // Debug
    //Notify("GPS_ERROR", error.message()); // Debug

    if (m_error.value() == 0.0) { // No error from asio read
        // Extract one trame
        std::istream is(&m_buffer);
        string line;
        std::getline(is, line);
        line += '\n'; // to be compatible with nmea_parser (expect \n at the end of line)

        // Parse NMEA
        nmea_parse(&m_parser, line.c_str(), line.length(), &m_info);
        nmea_info2pos(&m_info, &m_dpos); // in rad

        // Notify variable
        m_fix = m_info.fix;
        m_sig = m_info.sig;
        m_speed = m_info.speed;
        m_heading = m_info.track;

        Notify("GPS_RAW_NMEA", line);
        Notify("GPS_SIG", m_sig); //
        Notify("GPS_FIX", m_fix); // 1,2,3 dimension
        Notify("GPS_SPEED", m_speed);
        Notify("GPS_HEADING", m_heading); // En degres

        m_lat = m_dpos.lat * 180.0 / M_PI;
        m_lon = m_dpos.lon * 180.0 / M_PI;

        Notify("GPS_LAT", m_lat);
        Notify("GPS_LONG", m_lon);
    } else if (m_error.value() == 2.0) {
        //Case end of line beacause the trame is not arrived completly yet
    } else {
        reportRunWarning("Error reading GPS Com : " + m_error.message());
    }

    AppCastingMOOSApp::PostReport();
    return (true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool GPS::OnStartUp() {
    AppCastingMOOSApp::OnStartUp();

    STRING_LIST sParams;
    m_MissionReader.EnableVerbatimQuoting(false);
    if (!m_MissionReader.GetConfiguration(GetAppName(), sParams))
        reportConfigWarning("No config block found for " + GetAppName());

    STRING_LIST::iterator p;
    for (p = sParams.begin(); p != sParams.end(); p++) {
        string orig = *p;
        string line = *p;
        string param = toupper(biteStringX(line, '='));
        string value = line;

        bool handled = false;
        if (param == "SERIAL_PORT") {
            m_uart_port = value.c_str();
            handled = true;
        }
        if (param == "UART_BAUD_RATE") {
            m_uart_baud_rate = atoi(value.c_str());
            handled = true;
        }
        if (!handled)
            reportUnhandledConfigWarning(orig);
    }

    registerVariables();
    // Init GPS connection


    m_error = m_serial.open(m_uart_port,m_error);
    if (m_error.value() != 0.0)
    {
        // No error from asio read
        reportConfigWarning("Serial open error");
    }
    else
        m_serial.set_option(boost::asio::serial_port_base::baud_rate(m_uart_baud_rate));
    //serial.set_option(boost::asio::serial_port_base::character_size());
    //serial.set_option(boost::asio::serial_port_base::flow_control());
    //serial.set_option(boost::asio::serial_port_base::parity());
    //serial.set_option(boost::asio::serial_port_base::stop_bits());
    if(!m_serial.is_open()){
        reportConfigWarning("Serial port not open");
    }

    return (true);
}

//---------------------------------------------------------
// Procedure: registerVariables

void GPS::registerVariables() {
    AppCastingMOOSApp::RegisterVariables();
}


//------------------------------------------------------------
// Procedure: buildReport()

bool GPS::buildReport() {
  #if 0 // Keep these around just for template
    m_msgs << "============================================ \n";
    m_msgs << "File:                                        \n";
    m_msgs << "============================================ \n";

    ACTable actab(4);
    actab << "Alpha | Bravo | Charlie | Delta";
    actab.addHeaderLines();
    actab << "one" << "two" << "three" << "four";
    m_msgs << actab.getFormattedString();
  #endif

  ACTable actab(6);
  actab << "Serial port | Baudrate | Fix | Sig | Lat | Lon";
  actab.addHeaderLines();

  actab << m_uart_port << m_uart_baud_rate << m_fix << m_sig << doubleToString(m_lat, GPS_PRECISION) << doubleToString(m_lon, GPS_PRECISION);
  m_msgs << actab.getFormattedString();

  return true;
}

bool GPS::Notify_GNSS(float *lat, float *lon) {
    string msg;
    msg += "LAT=" + doubleToString(*lat, GPS_PRECISION) + ",";
    msg += "LON=" + doubleToString(*lon, GPS_PRECISION);
    Notify("GPS_NAV", msg);

    return true;
}

// reportEvent("Good msg received: " + message);
// reportRunWarning("Bad msg received: " + message);
// reportConfigWarning("Problem configuring FOOBAR. Expected a number but got: " + str);*