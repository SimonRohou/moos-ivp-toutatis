/************************************************************/
/*    FILE: NavScreen.h
/*    ORGN: ENSTA Bretagne Robotics - moos-ivp-enstabretagne
/*    AUTH: Simon Rohou
/*    DATE: 2015
/************************************************************/

#ifndef NavScreen_HEADER
#define NavScreen_HEADER

#include <map>
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class NavScreen : public AppCastingMOOSApp
{
  public:
    NavScreen();
    ~NavScreen() {};

  protected: // Standard MOOSApp functions to overload  
    bool OnNewMail(MOOSMSG_LIST &NewMail);
    bool Iterate();
    bool OnConnectToServer();
    bool OnStartUp();

  protected: // Standard AppCastingMOOSApp functions to overload 
    bool buildReport();
    void registerVariables();

  protected: // NavScreen functions


  private: // Configuration variables


  private: // State variables
    std::map<std::string,double> m_moosvars;
};

#endif 
