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

#define LIGHT_PIN 5 // <----- D1
#define ON_OFF_PIN 14 // <--- D5 
#define RESTORE_PIN 13 // <-- D7 

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
int stringTimeToIntTime(String time24);

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
  }

  // Determine Device ID
  deviceId = Utils::genDeviceIdFromMacAddr(WiFi.macAddress());
  
  // Initialize Networking
  WiFi.setOutputPower(20.5F);
  WiFi.setHostname("lumen");

  if (initWiFiSTAMode()) {
    // STA init successful
    isSTAMode = true;
    ntpClient.begin();
  } else {
    // STA init failed
    Serial.println(F("Could not connect to a WiFi network; Falling back to AP Mode..."));
    if (initWiFiAPMode()) {
      Serial.println(F("WiFi AP Mode setup."));
      // AP init successful; Starting captive portal
      dns.start(53u, "*", IpUtils::stringIPv4ToIPAddress(settings.getApNetIp()));
    } else {
      // AP init failed
      Serial.println(F("Something went wrong; Unable to initialize AP!"));
      Serial.println(F("Rebooting in 15 Seconds..."));
      delay(15000ul);

      ESP.restart();
    }
  }

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
  Serial.printf("AP IP: %s\nGateway: %s\nSubnet: %s\n", settings.getApNetIp().c_str(), settings.getApGateway().c_str(), settings.getApSubnet().c_str());
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
    Serial.println(F("Attempting to connect to WiFi..."));
    WiFi.mode(WiFiMode::WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(settings.getSsid(), settings.getPwd());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
      yield();
    }

    Serial.printf("WiFi connection was %s!\n", WiFi.status() == WL_CONNECTED ? "successful" : "failure");

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
 * ACTION FUNCTION
 * This action function performs the processes related to 
 * the functionality of the timer function. The timer capabilities
 * of the firmware all reside here.
 * 
 */
void doTimerFunctions() {
  static int timerLastUpdate = -1;

  if (isSTAMode && settings.isTimerOn()) {
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
  content.replace(F("${title}"), settings.getTitle());
  content.replace(F("${heading}"), settings.getHeading());
  content.replace(F("${version}"), FIRMWARE_VERSION);
  // FIXME: Need to figure out ${status_message}
  content.replace(F("${toggle_hidden}"), (WiFi.getMode() == WiFiMode::WIFI_STA ? F("") : F("hidden")));
  content.replace(F("${on_off_status}"), settings.isLightsOn() ? F("On") : F("Off"));
  content.replace(F("${cur_time}"), (ntpClient.isTimeSet() ? ntpClient.getFormattedTime() : F("Unknown")));
  content.replace(F("${timer_on_off}"), settings.isTimerOn() ? F("Enabled") : F("Disabled"));
  content.replace(F("${schedule_hide}"), settings.isTimerOn() ? F("") : F("hidden")); 
  content.replace(F("${on_at}"), intTimeToStringTime(settings.getOnTime()));
  content.replace(F("${off_at}"), intTimeToStringTime(settings.getOffTime()));
  
  // Send Main Page
  web.send(200, F("text/html"), content);
  yield();
}

void webHandleSettingsPage() {
  /* Ensure user authenticated */
  if (!web.authenticate(settings.getAdminUser().c_str(), settings.getAdminPwd().c_str())) {
    // User not yet authenticated

    return web.requestAuthentication(DIGEST_AUTH, "AdminRealm", "Authentication failed!");
  }

  String content = SETTINGS_PAGE;

  content.replace(F("${title}"), settings.getTitle());
  content.replace(F("${heading}"), settings.getHeading());
  content.replace(F("${version}"), FIRMWARE_VERSION);
  content.replace(F("${ssid}"), settings.getSsid());
  content.replace(F("${pwd}"), settings.getPwd());
  content.replace(F("${adminuser}"), settings.getAdminUser());
  content.replace(F("${adminpwd}"), settings.getAdminPwd());
  
  web.send(200, F("text/html"), content);
  yield();
}

void webHandleIncomingArgs() {
  if (web.method() == HTTP_POST) {
    String doAction = web.arg(F("do"));
    if (doAction.equals(F("btn_on"))) {
      // Turn on lights if applicable
      if (!settings.isLightsOn()) {
        settings.setLightsOn(true);
        settings.saveSettings();
      }
    } else if (doAction.equals(F("btn_off"))) {
      // Turn off lights if applicable
      if (settings.isLightsOn()) {
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
        settings.setOnTime(stringTimeToIntTime(on));
        settings.setOffTime(stringTimeToIntTime(off));
        settings.saveSettings();
      }
    } else if (doAction.equals(F("goto_admin"))) {
      webHandleSettingsPage();

      return;
    } else if (doAction.equals(F("admin_save"))) {
      // Save admin settings
      String title = web.arg(F("title"));
      String heading = web.arg(F("heading"));
      String ssid = web.arg(F("ssid"));
      String pwd = web.arg(F("pwd"));
      String adminUser = web.arg(F("adminuser"));
      String adminPwd = web.arg(F("adminpwd"));

      if (
        !title.isEmpty()
        && !heading.isEmpty()
        && !ssid.isEmpty()
        && !pwd.isEmpty()
        && !adminUser.isEmpty()
        && !adminPwd.isEmpty()
      ) {
        bool needReboot = !settings.getSsid().equals(ssid) || !settings.getPwd().equals(pwd);

        settings.setTitle(title.c_str());
        settings.setHeading(heading.c_str());
        settings.setSsid(ssid.c_str());
        settings.setPwd(pwd.c_str());
        settings.setAdminUser(adminUser.c_str());
        settings.setAdminPwd(adminPwd.c_str());

        settings.saveSettings();
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
        time24 > settings.getOnTime() 
        || time24 <= settings.getOffTime()
      )
    )
  );
}

int stringTimeToIntTime(String time24) {
  int sepIndex = time24.indexOf(":");
  if (sepIndex != -1) {
    int hours = time24.substring(0, sepIndex).toInt();
    int mins = time24.substring(sepIndex + 1).toInt();

    return ((hours * 100) + mins);
  }

  return 0;
}

/**
 * UTILITY FUNCTION
 * Converts an int which represents a 24 hour time
 * into a String time with a colon between the 
 * hours and minutes.
 */
String intTimeToStringTime(int time24) {
  if (time24 < 10) {
    String temp = F("00:0");
    temp.concat(String(time24));

    return temp;
  } else if (time24 < 100) {
    String temp = F("00:");
    temp.concat(String(time24));

    return temp;
  } else if (time24 < 1000) {
    String t = String(time24);
    String temp = t.substring(0,1);
    temp.concat(F(":"));
    temp.concat(t.substring(1, 3));

    return temp;
  } else {
    String t = String(time24);
    String temp = t.substring(0, 2);
    temp.concat(F(":"));
    temp.concat(t.substring(2, 4));

    return temp;
  }
}