/*
  This firmware code is intended to be used for a lighting controller I refer
  to as the Lumen Lighting Controller. The original intent of this controller is
  to simply control some under cabnit white LED lights, for use at my mom's house.
  
  This controller remembers the last setting of on/off for the lights. This means if
  you turn the lights on, then un-plug the controller. The next time you plug it in
  the lights will default to on. This is how my mom originally wanted to control the 
  on/off status of the lights. I decided to add an external button that could be used
  to turn on/off the lights so she wouldn't be unplugging it all the time. However since
  the device was an ESP 8266 I decided not to stop there, so I made a web interface that
  can control the lights, and from that interface one can also setup a simple daily 
  timer that automatically turns the lights on/off, if the device is connected to a network
  with Internet access.

  The device remains in AP mode even when connected to a network so that there is an easy to
  find way of connecting to the device to change it's settings. Once connected to the AP which
  will be an SSID starting with 'Lumen_'. One can open the internally hosted web page by typing
  in any HTTP address in a browser. 
  
  For example: http://lumen.local

  From there all functionality of the device can be leveraged.

  Aside from the web interface for controlling the firmware is designed so the device can have an external
  on/off button for toggling the lights on/off. Also, a button can be included to act as a factory reset 
  button. That button if held down when the device powers up will reset the device instantly. To reset the device 
  once it is powered up the button must be held down for 10 seconds or more.

  Hardware: ...... ESP8266
  Written by: .... Scott Griffis
  Date: .......... 12/01/2024
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Settings.h>
#include <IpUtils.h>
#include <Utils.h>
#include <HtmlContent.h>

// =================================
// Define Statements
// =================================
#define FIRMWARE_VERSION "1.1.2"

#define LIGHT_PIN 5 // <----- D1
#define ON_OFF_PIN 14 // <--- D5 
#define RESTORE_PIN 13 // <-- D7 

// =================================
// Function Prototypes
// =================================
void initWiFiAPMode(void);
void initWiFiSTAMode(void);
void doCheckForFactoryReset(bool isPowerOn);
void doDeviceTasks(void);
void doTimerFunctions(void);
void webHandleMainPage(void);
void doHandleMainPage(String popupMessage);
void webHandleSettingsPage(void);
void doHandleIncomingArgs(bool enabled);
bool inOnZone(int time24);

// =================================
// Setup of Services
// =================================
Settings settings;
ESP8266WebServer web(80);
DNSServer dns;
WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp, "pool.ntp.org"); 

// =================================
// Worker Vars
// =================================
String deviceId = "";
bool isSTAConnected = false;

/**
 * =================================
 * SETUP FUNCTION
 * =================================
 * 
 * This is the initial entry point for the firmware's 
 * runtime. It handles all the onetime setup tasks needed
 * to initialize the device and prepare it for its regular
 * runtime.
 * 
 */
void setup() {
  // Initialize Pins
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(RESTORE_PIN, INPUT);
  pinMode(ON_OFF_PIN, INPUT);

  // Initialize Serial
  Serial.begin(74880);
  yield();

  // Reset and/or load settings
  doCheckForFactoryReset(true);
  settings.loadSettings();

  // Initialize Lights on/off status
  if (settings.isLightsOn()) {
    digitalWrite(LIGHT_PIN, HIGH);
  } else {
    digitalWrite(LIGHT_PIN, LOW);
  }

  // Determine Device ID
  deviceId = Utils::genDeviceIdFromMacAddr(WiFi.macAddress()).c_str();
  
  // Initialize Networking
  WiFi.setOutputPower(20.5F);
  WiFi.setHostname("lumen");
  WiFi.mode(WiFiMode::WIFI_AP_STA);

  // Start AP and connect to WiFi if available
  initWiFiAPMode();
  initWiFiSTAMode();

  // Set page handlers for Web Server
  web.on(F("/"), webHandleMainPage);
  web.on(F("/admin"), webHandleSettingsPage);
  web.onNotFound(webHandleMainPage);

  web.begin();
}

/**
 * =================================
 * LOOP FUNCTION
 * =================================
 * 
 * This is the looping portion of the runtime, as such this
 * funtion drives all the repeatitive tasks the device will 
 * perform during its normal operation.
 */
void loop() {
  web.handleClient();
  dns.processNextRequest();
  doDeviceTasks();

  yield();
}

// ===============================================================
// INIT FUNCTION BELOW
// ===============================================================

/**
 * INIT FUNCTION 
 * Initializes the WiFi for AP Mode.
 * 
 */
void initWiFiAPMode() {
  Serial.printf("AP IP: %s\nGateway: %s\nSubnet: %s\n", settings.getApNetIp().c_str(), settings.getApGateway().c_str(), settings.getApSubnet().c_str());
  WiFi.softAPConfig(
    IpUtils::stringIPv4ToIPAddress(settings.getApNetIp()), 
    IpUtils::stringIPv4ToIPAddress(settings.getApGateway()), 
    IpUtils::stringIPv4ToIPAddress(settings.getApSubnet())
  );
  
  if (WiFi.softAP(settings.getApSsid(deviceId), settings.getApPwd())) {
    Serial.println(F("WiFi AP Mode setup."));
    dns.start(53u, "*", IpUtils::stringIPv4ToIPAddress(settings.getApNetIp()));

    return;
  }

  Serial.println(F("Something went wrong; Unable to initialize AP!"));
  Serial.println(F("Rebooting in 15 Seconds..."));
  delay(15000);

  ESP.restart();
}

/**
 * INIT FUNCTION 
 * Initialize the WiFi for STA Mode so it can connect to
 * a WiFi network if configured to do so.
 * 
 */
void initWiFiSTAMode() {
  if (
    !settings.getSsid().equals(settings.getDefaultSsid()) 
    && !settings.getPwd().equals(settings.getDefaultPwd())
  ) {
    Serial.println(F("Attempting to connect to WiFi..."));
    WiFi.setAutoReconnect(true);
    WiFi.begin(settings.getSsid(), settings.getPwd());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && !Utils::flipSafeHasTimeExpired(start, 15000UL)) {
      yield();
    }

    Serial.printf("WiFi connection was %s!\n", WiFi.status() == WL_CONNECTED ? "successful" : "failure");

    if (WiFi.status() == WL_CONNECTED) {
      // Connected
      isSTAConnected = true;
      ntpClient.begin();
      
      return;
    }
  }
  isSTAConnected = false;
}

// ===============================================================
// ACTION FUNCTIONS BELOW
// ===============================================================

/**
 * ACTION FUNCTION
 * This action function handles performing all the the 
 * device's custom runtime functionality.
 * 
 */
void doDeviceTasks() {
  doCheckForFactoryReset(false);
  doTimerFunctions();
  
  // Toggle light state based on button press
  if (digitalRead(ON_OFF_PIN) == HIGH) {
    settings.setLightsOn(!settings.isLightsOn());
    settings.saveSettings();
  }

  // Set light to appropriate state
  if (settings.isLightsOn() &&  digitalRead(LIGHT_PIN) == LOW) {
    // Is Off but should be On
    digitalWrite(LIGHT_PIN, HIGH);
  } else if (!settings.isLightsOn() && digitalRead(LIGHT_PIN) == HIGH) {
    // Is On but should be Off
    digitalWrite(LIGHT_PIN, LOW);
  }

  // Prevent multi-react to single long press
  while (digitalRead(ON_OFF_PIN) == HIGH) {
    yield();
  }
}

/**
 * ACTION FUNCTION
 * Handles factory resetting instantly on powerup or during 
 * runtime when reset button is held for 10 seconds or more.
 * All factory resetting capabilities are handled by this function.
 * The reset procedure handled by this function is different if 
 * being called from setup vs loop.
 * 
 * @param isPowerOn Indicates that the function was called by the setup
 * function during power up of the device if suppled a true as bool. A
 * false being supplied indicates the call is from the main firmware
 * loop of the application.  
 * 
 */
void doCheckForFactoryReset(bool isPowerOn) {
  if (digitalRead(RESTORE_PIN) == HIGH) {
    // Possible reset condition
    bool doReset = false;
    if (isPowerOn) {
      // Immidiate reset
      doReset = true;
    } else {
      // Long hold reset
      unsigned long start = millis();
      unsigned long elapsed = 0ul;
      while (digitalRead(RESTORE_PIN) == HIGH && (elapsed = millis() - start) < 10000ul) {
        // Button is being held
        yield();
      }
      if (elapsed >= 10000ul) {
        doReset = true;
      }
    }

    /* Handle Factory Reset and Reboot */
    if (doReset) {
      Serial.printf("Factory Reset %s!", (settings.factoryDefault() ? "Successful" : "Failed"));
      if (!isPowerOn) {
        // Reboot is needed
        ESP.restart();
      } 
    }
  }
}

/**
 * ACTION FUNCTION
 * This action function performs the processes related to 
 * the functionality of the timer function. The timer capabilities
 * of the firmware all reside here.
 * 
 */
void doTimerFunctions() {
  if (isSTAConnected) {
    // On a network so NTP Possible
    ntpClient.update();

    if (settings.isTimerOn() && ntpClient.isTimeSet()) {
      // Timer is turned on and we can know the time
      int time24 = (ntpClient.getHours() * 100) + ntpClient.getMinutes();
      time24 = Utils::adjustIntTimeForTimezone(time24, settings.getTimeZone(), settings.isDst());

      // Determine what on/off zone we are in
      bool curInOnZone = inOnZone(time24);
    
      // Perform on/off change if applicable
      static int timerLastUpdate = -1;
      if (timerLastUpdate == -1 || inOnZone(timerLastUpdate) != curInOnZone) {
        // Perform an update
        if (curInOnZone) {
          if (!settings.isLightsOn()) { 
            // Light off and needs set to on
            settings.setLightsOn(true);
            settings.saveSettings();
          }
        } else {
          if (settings.isLightsOn()) {
            // Light on and needs set to off
            settings.setLightsOn(false);
            settings.saveSettings();
          }
        }
        timerLastUpdate = time24;
      }
    }
  }
}

/**
 * ACTION FUNCITON
 * This action function is called upon to handle all incoming web
 * requests for the main page as well as any POST requests made by
 * any of the sub pages.
 * 
 * @param popupMessage A popup message to display to the user just before
 * the page finishes loading as String.
 */
void doHandleMainPage(String popupMessage) {
  doHandleIncomingArgs(popupMessage.isEmpty());
  
  // Generate Main Page
  String content = MAIN_PAGE;
  content.replace(F("${version}"), FIRMWARE_VERSION);
  content.replace(F("${wifi_addr}"), !WiFi.isConnected() ? F("N/A") : WiFi.localIP().toString());
  content.replace(F("${ssid}"), !WiFi.isConnected() ? F("Not Connected") : WiFi.SSID());
  if (popupMessage.isEmpty()) {
    // No popup message to send
    content.replace(F("${status_message}"), "");
  } else {
    // Message found so create a popup for it
    String popup = STATUS_MESSAGE;
    popup.replace(F("${message}"), popupMessage);
    content.replace(F("${status_message}"), popup);
  }
  content.replace(F("${toggle_hidden}"), ntpClient.isTimeSet() ? F("") : F("hidden"));
  content.replace(F("${on_off_status}"), settings.isLightsOn() ? F("On") : F("Off"));
  if (ntpClient.isTimeSet()) {
    // Time is set so display it
    String sTime12 = Utils::intTimeToString12Time(
      Utils::adjustIntTimeForTimezone(
        ((ntpClient.getHours() * 100) + ntpClient.getMinutes()), 
        settings.getTimeZone(), 
        settings.isDst()
      )
    );
    content.replace(F("${cur_time}"), sTime12);
  } else {
    // Time is unknown
    content.replace(F("${cur_time}"), F("Unknown"));
  }
  content.replace(F("${timer_on_off}"), settings.isTimerOn() ? F("Enabled") : F("Disabled"));
  content.replace(F("${schedule_hide}"), settings.isTimerOn() && ntpClient.isTimeSet() ? F("") : F("hidden")); 
  content.replace(F("${on_at}"), Utils::intTimeToStringTime(settings.getOnTime()));
  content.replace(F("${off_at}"), Utils::intTimeToStringTime(settings.getOffTime()));
  
  // Send Main Page
  web.send(200, F("text/html"), content);
  yield();
}

/**
 * ACTION FUNCTION
 * This action function if enabled handles all incoming POST requests
 * all requests will be handled without the need for authentication except
 * for requests which modify any of the core settings. For those settings to
 * take place the user must authenticate with the settings page prior to submitting
 * the POST to modify settings.
 */
void doHandleIncomingArgs(bool enabled) {
  if (enabled && web.method() == HTTP_POST) {
    String doAction = web.arg(F("do"));
    if (doAction.equals(F("btn_on"))) {
      // Turn on lights if applicable
      if (!settings.isLightsOn()) {
        // Light was off and needs to be on
        settings.setLightsOn(true);
        settings.saveSettings();
      }
    } else if (doAction.equals(F("btn_off"))) {
      // Turn off lights if applicable
      if (settings.isLightsOn()) {
        // Light was on and needs to be off
        settings.setLightsOn(false);
        settings.saveSettings();
      }
    } else if (doAction.equals(F("toggle_timer_state"))) {
      // Hide or show timer controls/Enable or disable timer
      settings.setTimerOn(!settings.isTimerOn());
      settings.saveSettings();
    } else if (doAction.equals(F("btn_update"))) {
      // Save timer settings
      String on = web.arg(F("onat"));
      String off = web.arg(F("offat"));
      if (!on.isEmpty() && !off.isEmpty()) {
        // convert and store updated times
        settings.setOnTime(Utils::stringTimeToIntTime(on));
        settings.setOffTime(Utils::stringTimeToIntTime(off));
        settings.saveSettings();
      }
    } else if (doAction.equals(F("goto_admin"))) {
      // Settings button clicked so show settings page
      webHandleSettingsPage();

      return;
    } else if (
      doAction.equals(F("admin_save"))
      && web.authenticate(settings.getAdminUser().c_str(), settings.getAdminPwd().c_str())
    ) {
      // Save admin settings user is authenticated
      String appwd = web.arg(F("appwd"));
      String ssid = web.arg(F("ssid"));
      String pwd = web.arg(F("pwd"));
      String adminUser = web.arg(F("adminuser"));
      String adminPwd = web.arg(F("adminpwd"));
      String timeZone = web.arg(F("timezone"));
      String dst = web.arg(F("dst"));

      if (
        !ssid.isEmpty()
        && !appwd.isEmpty()
        && !pwd.isEmpty()
        && !adminUser.isEmpty()
        && !adminPwd.isEmpty()
        && !timeZone.isEmpty()
      ) {
        /* Determine If A Reboot Will Be Needed To Apply Settings Changes */
        bool needReboot = !settings.getSsid().equals(ssid) || !settings.getPwd().equals(pwd) || !settings.getApPwd().equals(appwd);

        /* Apply The Settings Changes */
        settings.setApPwd(appwd.c_str());
        settings.setSsid(ssid.c_str());
        settings.setPwd(pwd.c_str());
        settings.setAdminUser(adminUser.c_str());
        settings.setAdminPwd(adminPwd.c_str());
        settings.setTimeZone(timeZone.toInt());
        settings.setDst(dst.equalsIgnoreCase("DST") ? true : false);

        /* Save Changes */
        settings.saveSettings();

        /* Reboot If Needed */
        if (needReboot) {
          String message = F("<!DOCTYPE HTML><html lang=\"en\"><head></head><body><script>alert(\"Rebooting to apply settings!\");</script></body></html>");
          web.send(200, F("text/html"), message.c_str());
          yield();
          delay(2000);
          ESP.restart();
        }
      }

      yield();
    }
  }
}

// ===============================================================
// WEB FUNCTIONS BELOW
// ===============================================================

/**
 * WEB HANDLER
 * This function is called directly by the web server to show the main
 * page. To do this it deligates to another function.
 * 
 */
void webHandleMainPage() {
  doHandleMainPage("");
}

/**
 * WEB HANDLER
 * This function is called directly by the web server to show the settings page
 * and is also called by the main page when someone clicks the setting button.
 * This handler requires the user to authenticate initially but then basically 
 * just generates and sends the settings page to the user. Settings updates are 
 * actually sent to the main page handler for processing.
 */
void webHandleSettingsPage() {
  /* Ensure user authenticated */
  if (!web.authenticate(settings.getAdminUser().c_str(), settings.getAdminPwd().c_str())) {
    // User not yet authenticated

    return web.requestAuthentication(DIGEST_AUTH, "AdminRealm", "Authentication failed!");
  }

  /* Build Page Content */
  String content = SETTINGS_PAGE;
  content.replace(F("${version}"), FIRMWARE_VERSION);
  content.replace(F("${ap_pwd}"), settings.getApPwd());
  content.replace(F("${ssid}"), settings.getSsid());
  content.replace(F("${pwd}"), settings.getPwd());
  content.replace(F("${adminuser}"), settings.getAdminUser());
  content.replace(F("${adminpwd}"), settings.getAdminPwd());
  content.replace(F("${time_zone}"), String(settings.getTimeZone()));
  content.replace(F("${checked_status}"), (settings.isDst() ? F("checked") : F("")));
  
  /* Send Page Content */
  web.send(200, F("text/html"), content);
  yield();
}

// ===============================================================
// UTILITY FUNCTIONS BELOW
// ===============================================================

/**
 * UTILITY FUNCTION
 * This function id used to determine if the given time is
 * located within the On Zone as defined by the on and off 
 * time settings of the timer.
 * 
 * @param time24 The given time in 24 hour format as an int.
 * 
 * @return Returns a bool true if given time falls in the On Zone
 * otherwise a false is returned.
 */
bool inOnZone(int time24) {
  return (
    (
      settings.getOnTime() < settings.getOffTime() 
      && time24 >= settings.getOnTime() 
      && time24 < settings.getOffTime()
    )
    || (
      settings.getOnTime() > settings.getOffTime()
      && (
        time24 >= settings.getOnTime() 
        || time24 < settings.getOffTime()
      )
    )
  );
}