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
  deviceId = Utils::genDeviceIdFromMacAddr(WiFi.macAddress()).c_str();
  
  // Initialize Networking
  WiFi.setOutputPower(20.5F);
  WiFi.setHostname("lumen");
  WiFi.mode(WiFiMode::WIFI_AP_STA);

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
 * Initializes the WiFi into AP Mode.
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
 * Initialize the WiFi into STA Mode.
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
      time24 = Utils::adjustIntTimeForTimezone(time24, -6);

      // Determine what on/off zone we are in
      bool curInOnZone = inOnZone(time24);
    
      // Perform on/off change if applicable
      static int timerLastUpdate = -1;
      if (timerLastUpdate == -1 || inOnZone(timerLastUpdate) != curInOnZone) {
        // Perform an update
        if (curInOnZone) {
          if (!settings.isLightsOn()) { 
            settings.setLightsOn(true);
            settings.saveSettings();
          }
        } else {
          if (settings.isLightsOn()) {
            settings.setLightsOn(false);
            settings.saveSettings();
          }
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
  doHandleMainPage("");
}

void doHandleMainPage(String popupMessage) {
  doHandleIncomingArgs(popupMessage.isEmpty());
  
  // Generate Main Page
  String content = MAIN_PAGE;
  content.replace(F("${version}"), FIRMWARE_VERSION);
  content.replace(F("${wifi_addr}"), !WiFi.isConnected() ? F("N/A") : WiFi.localIP().toString());
  content.replace(F("${ssid}"), !WiFi.isConnected() ? F("Not Connected") : WiFi.SSID());
  content.replace(F("${status_message}"), popupMessage);
  content.replace(F("${toggle_hidden}"), ntpClient.isTimeSet() ? F("") : F("hidden"));
  content.replace(F("${on_off_status}"), settings.isLightsOn() ? F("On") : F("Off"));
  if (ntpClient.isTimeSet()) {
    String sTime12 = Utils::intTimeToString12Time(Utils::adjustIntTimeForTimezone(((ntpClient.getHours() * 100) + ntpClient.getMinutes()), -6));
    content.replace(F("${cur_time}"), sTime12);
  } else {
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

void webHandleSettingsPage() {
  /* Ensure user authenticated */
  if (!web.authenticate(settings.getAdminUser().c_str(), settings.getAdminPwd().c_str())) {
    // User not yet authenticated

    return web.requestAuthentication(DIGEST_AUTH, "AdminRealm", "Authentication failed!");
  }

  String content = SETTINGS_PAGE;

  content.replace(F("${version}"), FIRMWARE_VERSION);
  content.replace(F("${ap_pwd}"), settings.getApPwd());
  content.replace(F("${ssid}"), settings.getSsid());
  content.replace(F("${pwd}"), settings.getPwd());
  content.replace(F("${adminuser}"), settings.getAdminUser());
  content.replace(F("${adminpwd}"), settings.getAdminPwd());
  
  web.send(200, F("text/html"), content);
  yield();
}

void doHandleIncomingArgs(bool enabled) {
  if (enabled && web.method() == HTTP_POST) {
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
        settings.setOnTime(Utils::stringTimeToIntTime(on));
        settings.setOffTime(Utils::stringTimeToIntTime(off));
        settings.saveSettings();
      }
    } else if (doAction.equals(F("goto_admin"))) {
      webHandleSettingsPage();

      return;
    } else if (doAction.equals(F("admin_save"))) {
      // Save admin settings
      String appwd = web.arg(F("appwd"));
      String ssid = web.arg(F("ssid"));
      String pwd = web.arg(F("pwd"));
      String adminUser = web.arg(F("adminuser"));
      String adminPwd = web.arg(F("adminpwd"));

      if (
        !ssid.isEmpty()
        && !appwd.isEmpty()
        && !pwd.isEmpty()
        && !adminUser.isEmpty()
        && !adminPwd.isEmpty()
      ) {
        bool needReboot = !settings.getSsid().equals(ssid) || !settings.getPwd().equals(pwd) || !settings.getApPwd().equals(appwd);

        settings.setApPwd(appwd.c_str());
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
        time24 >= settings.getOnTime() 
        || time24 < settings.getOffTime()
      )
    )
  );
}