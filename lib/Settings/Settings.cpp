/*
    Settings - A class to contain, maintain, store and retreive settings needed
    by the application. This Class object is intented to be the sole manager of 
    data used throughout the applicaiton. It handles storing both volitile and 
    non-volatile data, where by definition the non-volitile data is persisted
    in flash memory and lives beyond the running life of the software and the 
    volatile data is lost and defaulted each time the software runs.

    Written by: Scott Griffis
*/

#include "Settings.h"

/**
 * #### CLASS CONSTRUCTOR ####
 * Allows for external instantiation of
 * the class into an object.
*/
Settings::Settings() {
    // Initially default the settings...
    defaultSettings();
}

/**
 * Performs a factory default on the information maintained by this class
 * where that the data is first set to its factory default settings then
 * it is persisted to flash.
 * 
 * @return Returns true if successful saving defaulted settings otherwise
 * returns false as bool.
*/
bool Settings::factoryDefault() {
    defaultSettings();
    bool ok = saveSettings();

    return ok;
}

/**
 * Used to load the settings from flash memory.
 * After the settings are loaded from flash memory the sentinel value is 
 * checked to ensure the integrity of the loaded data. If the sentinel 
 * value is wrong then the contents of the memory are deemed invalid and
 * the memory is wiped and then a factory default is instead performed.
 * 
 * @return Returns true if data was loaded from memory and the sentinel 
 * value was valid.
*/
bool Settings::loadSettings() {
    bool ok = false;
    // Setup EEPROM for loading and saving...
    EEPROM.begin(sizeof(NonVolatileSettings));

    // Persist default settings or load settings...
    delay(15);

    /* Load from EEPROM if applicable... */
    if (EEPROM.percentUsed() >= 0) { // Something is stored from prior...
        Serial.println(F("\nLoading settings from EEPROM..."));
        EEPROM.get(0, nvSettings);
        if (strcmp(nvSettings.sentinel, hashNvSettings(nvSettings).c_str()) != 0) { // Memory is corrupt...
            EEPROM.wipe();
            factoryDefault();
            Serial.println("Stored settings footprint invalid, stored settings have been wiped and defaulted!");
        } else { // Memory seems ok...
            Serial.print(F("Percent of ESP Flash currently used is: "));
            Serial.print(EEPROM.percentUsed());
            Serial.println(F("%"));
            ok = true;
        }
    }
    
    EEPROM.end();

    return ok;
}

/**
 * Used to provide a hash of the given NonVolatileSettings.
 * 
 * @param nvSet An instance of NonVolatileSettings to calculate a hash for.
 * 
 * @return Returns the calculated hash value as String.
*/
String Settings::hashNvSettings(NonVolatileSettings nvSet) {
    String content = "";
    content = content + String(nvSet.ssid);
    content = content + String(nvSet.pwd);
    content = content + String(nvSet.adminUser);
    content = content + String(nvSet.adminPwd);
    content = content + String(nvSet.apPwd);
    content = content + String(nvSet.timerOn ? "true" : "false");
    content = content + String(nvSet.onTime);
    content = content + String(nvSet.offTime);
    content = content + (nvSet.lightsOn ? "true" : "false");
    
    MD5Builder builder = MD5Builder();
    builder.begin();
    builder.add(content);
    builder.calculate();

    return builder.toString();
}

/**
 * Used to save or persist the current value of the non-volatile settings
 * into flash memory.
 *
 * @return Returns a true if save was successful otherwise a false as bool.
*/
bool Settings::saveSettings() {
    strcpy(nvSettings.sentinel, hashNvSettings(nvSettings).c_str()); // Ensure accurate Sentinel Value.
    EEPROM.begin(sizeof(NonVolatileSettings));

    EEPROM.wipe(); // usage seemd to grow without this.
    EEPROM.put(0, nvSettings);
    
    bool ok = EEPROM.commit();

    EEPROM.end();
    
    return ok;
}

/**
 * Used to determine if the current network settings are in default values.
 * 
 * @return Returns a true if default values otherwise a false as bool. 
*/
bool Settings::isFactoryDefault() {
    
    return (strcmp(hashNvSettings(nvSettings).c_str(), hashNvSettings(factorySettings).c_str()) == 0);
}

/*
=================================================================
Getter and Setter Functions
=================================================================
*/

String Settings::getSsid() {

    return String(nvSettings.ssid);
}

void Settings::setSsid(const char *ssid) {
    if (sizeof(ssid) <= sizeof(nvSettings.ssid)) {
        strcpy(nvSettings.ssid, ssid);
    }
}


String Settings::getPwd() {

    return String(nvSettings.pwd);
}

void Settings::setPwd(const char *pwd) {
    if (sizeof(pwd) <= sizeof(nvSettings.pwd)) {
        strcpy(nvSettings.pwd, pwd);
    }
}


bool Settings::isTimerOn() {
    
    return nvSettings.timerOn;
}

void Settings::setTimerOn(bool on) {
    nvSettings.timerOn = on;
}


int Settings::getOnTime() {

    return nvSettings.onTime;
}

void Settings::setOnTime(int time24) {
    nvSettings.onTime = time24;
}


int Settings::getOffTime() {

    return nvSettings.offTime;
}

void Settings::setOffTime(int time24) {
    nvSettings.offTime = time24;
}


String Settings::getHostname(String deviceId) {
    String result = cSettings.hostname;
    result.concat(deviceId); 
    
    return result;
}


String Settings::getAdminUser() {

    return String(nvSettings.adminUser);
}

void Settings::setAdminUser(const char *user) {

    if (sizeof(user) <= sizeof(nvSettings.adminUser)) {
        strcpy(nvSettings.adminUser, user);
    }
}


String Settings::getAdminPwd() {

    return String(nvSettings.adminPwd);
}

void Settings::setAdminPwd(const char *pwd) {
    if (sizeof(pwd) <= sizeof(nvSettings.adminPwd)) {
        strcpy(nvSettings.adminPwd, pwd);
    }
}


void Settings::setApPwd(const char *pwd) {
    if (sizeof(pwd) <= sizeof(nvSettings.apPwd)) {
        strcpy(nvSettings.apPwd, pwd);
    }
}

String Settings::getApPwd() {

    return String(nvSettings.apPwd);
}


String Settings::getApSsid(String deviceId) {
    String result = cSettings.apSsid;
    result.concat(deviceId);

    return result;
}


String Settings::getApNetIp() {

    return cSettings.apNetIp;
}


String Settings::getApSubnet() {

    return cSettings.apSubnet;
}


String Settings::getApGateway() {

    return cSettings.apGateway;
}


bool Settings::isLightsOn() {
    
    return nvSettings.lightsOn;
}

void Settings::setLightsOn(bool lightsOn) {
    nvSettings.lightsOn = lightsOn;
}


String Settings::getDefaultSsid() {

    return String(factorySettings.ssid);
}

String Settings::getDefaultPwd() {

    return String(factorySettings.pwd);
}


/*
=================================================================
Private Functions
=================================================================
*/

/**
 * #### PRIVATE ####
 * This function is used to set or reset all settings to 
 * factory default values but does not persist the value 
 * changes to flash.
*/
void Settings::defaultSettings() {
    // Default the settings..
    strcpy(nvSettings.ssid, factorySettings.ssid);
    strcpy(nvSettings.pwd, factorySettings.pwd);
    strcpy(nvSettings.adminUser, factorySettings.adminUser);
    strcpy(nvSettings.adminPwd, factorySettings.adminPwd);
    strcpy(nvSettings.apPwd, factorySettings.apPwd);
    nvSettings.timerOn = factorySettings.timerOn;
    nvSettings.onTime = factorySettings.onTime;
    nvSettings.offTime = factorySettings.offTime;
    nvSettings.lightsOn = factorySettings.lightsOn;
    strcpy(nvSettings.sentinel, hashNvSettings(factorySettings).c_str());
}