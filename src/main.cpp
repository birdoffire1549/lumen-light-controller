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
#define FIRMWARE_VERSION "1.0.0"
#define LIGHT_PIN 4
#define ON_OFF_PIN 5
#define RESTORE_PIN 14

// =================================
// Function Prototypes
// =================================
bool initWiFiAPMode(void);
bool initWiFiSTAMode(void);
void doCheckForFactoryReset(bool isPowerOn);
void doDeviceTasks(void);
void doTimerFunctions(void);
void webHandleMainPage(void);
void webHandleSettingsPage(void);
void webHandleIncomingArgs(void);
bool inOnZone(int time24);
String intTimeToStringTime(int time24);

// =================================
// Setup of Services
// =================================
Settings settings = Settings();
ESP8266WebServer web(80);
DNSServer dns;
WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp, "pool.ntp.org"); 

// =================================
// Worker Vars
// =================================
String deviceId = "";
bool isSTAMode = false;

/**
 * =================================
 * SETUP FUNCTION
 * =================================
 */
void setup() {
  // Initialize Pins
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(RESTORE_PIN, INPUT);

  // Initialize Serial
  Serial.begin(115200);
  yield();

  // Reset and/or load settings
  doCheckForFactoryReset(true);
  settings.loadSettings();

  // Initialize Lights on/off status
  if (settings.isLightsOn()) {
    digitalWrite(LIGHT_PIN, HIGH);
  }

  // Determine Device ID
  deviceId = Utils::genDeviceIdFromMacAddr(WiFi.macAddress());
  
  // Initialize Networking
  WiFi.setOutputPower(20.5F);
  WiFi.setHostname(settings.getHostname(deviceId).c_str());

  if (initWiFiSTAMode()) {
    // STA init successful
    isSTAMode = true;
    ntpClient.begin();
  } else {
    // STA init failed
    Serial.println("Could not connect to a WiFi network; Falling back to AP Mode...");
    if (initWiFiAPMode()) {
      // AP init successful; Starting captive portal
      dns.start(53u, "*", IpUtils::stringIPv4ToIPAddress(settings.getApNetIp()));
    } else {
      // AP init failed
      Serial.println("Something went wrong; Unable to initialize AP!");
      Serial.println("Rebooting in 15 Seconds...");
      delay(15000ul);

      ESP.restart();
    }
  }

  // Set page handlers for Web Server
  web.on("/", webHandleMainPage);
  web.on("/admin", webHandleSettingsPage);
  web.onNotFound(webHandleMainPage);
}

/**
 * =================================
 * LOOP FUNCTION
 * =================================
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
 * INIT FUNCTION: 
 * Initializes the WiFi into AP Mode.
 * 
 * @return Returns a bool true if device was able to enter AP mode
 * successfully otherwise a false is returned.
 * 
 */
bool initWiFiAPMode() {
  WiFi.mode(WiFiMode::WIFI_AP);
  WiFi.softAPConfig(
    IpUtils::stringIPv4ToIPAddress(settings.getApNetIp()), 
    IpUtils::stringIPv4ToIPAddress(settings.getApGateway()), 
    IpUtils::stringIPv4ToIPAddress(settings.getApSubnet())
  );
  
  return WiFi.softAP(settings.getApSsid(deviceId), settings.getApPwd());
}

/**
 * INIT FUNCTION: 
 * Initialize the WiFi into STA Mode.
 * 
 * @return Returns a bool true if the device was able to connect
 * to a WiFi network otherwise a false is returned.
 * 
 */
bool initWiFiSTAMode() {
  if (
    !settings.getSsid().equals(settings.getDefaultSsid()) 
    && !settings.getPwd().equals(settings.getDefaultPwd())
  ) {
    WiFi.mode(WiFiMode::WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(settings.getSsid(), settings.getPwd());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
      yield();
    }

    if (WiFi.status() == WL_CONNECTED) {
      // Connected
      
      return true;
    }
  }

  return false;
}

// ===============================================================
// ACTION FUNCTIONS BELOW
// ===============================================================

/**
 * ACTION FUNCTION:
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
 * ACTION FUNCTION: 
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
 * ACTION FUNCTION:
 * This action function performs the processes related to 
 * the functionality of the timer function. The timer capabilities
 * of the firmware all reside here.
 * 
 */
void doTimerFunctions() {
  static int timerLastUpdate = -1;

  if (isSTAMode) {
    // On a network so NTP Possible
    ntpClient.update();

    if (ntpClient.isTimeSet() && settings.isTimerOn()) {
      // Timer is turned on and we can know the time
      int time24 = (ntpClient.getHours() * 100) + ntpClient.getMinutes();
    
      // Determine what on/off zone we are in
      bool curInOnZone = inOnZone(time24);
    
      // Perform on/off change if applicable
      if (timerLastUpdate == -1 || inOnZone(timerLastUpdate) != curInOnZone) {
        // Perform an update
        if (curInOnZone) {
          settings.setLightsOn(true);
        } else {
          settings.setLightsOn(false);
        }
        timerLastUpdate = time24;
      }
    }
  }
}

// ===============================================================
// WEB FUNCTIONS BELOW
// ===============================================================

void webHandleMainPage() {
  webHandleIncomingArgs();
  
  // Generate Main Page
  String content = MAIN_PAGE;
  content.replace("${title}", settings.getTitle());
  content.replace("${heading}", settings.getHeading());
  content.replace("${version}", FIRMWARE_VERSION);
  // FIXME: Need to figure out ${status_message}
  content.replace("${on_off_status}", settings.isLightsOn() ? "On" : "Off");
  content.replace("${timer_on_off}", settings.isTimerOn() ? "Enabled" : "Disabled");
  content.replace("${schedule_hide}", settings.isTimerOn() ? "" : "hidden"); 
  content.replace("${on_at}", intTimeToStringTime(settings.getOnTime()));
  content.replace("${off_at}", intTimeToStringTime(settings.getOffTime()));
  
  // Send Main Page
  web.send(200, "text/html", content);
  yield();
}

void webHandleSettingsPage() {
  // Generate Settings Page
  // Send Settings Page
}

void webHandleIncomingArgs() {

}

// ===============================================================
// UTILITY FUNCTIONS BELOW
// ===============================================================

/**
 * UTILITY FUNCTION:
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
        time24 > settings.getOnTime() 
        || time24 <= settings.getOffTime()
      )
    )
  );
}

String intTimeToStringTime(int time24) {
  if (time24 < 10) {
    String temp = "00:0";
    temp.concat(String(time24));

    return temp;
  } else if (time24 < 100) {
    String temp = "00:";
    temp.concat(String(time24));

    return temp;
  } else if (time24 < 1000) {
    String time = String(time24);
    String temp = time.substring(0,1);
    temp.concat(":");
    temp.concat(time.substring(1, 3));

    return temp;
  } else {
    String time = String(time24);
    String temp = time.substring(0, 2);
    temp.concat(":");
    temp.concat(time.substring(2, 4));

    return temp;
  }
}