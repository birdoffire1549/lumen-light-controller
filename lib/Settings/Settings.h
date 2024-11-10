/*
    Settings - A class to contain, maintain, store and retreive settings needed
    by the application. This Class object is intented to be the sole manager of 
    data used throughout the applicaiton. It handles storing both volitile and 
    non-volatile data, where by definition the non-volitile data is persisted
    in flash memory and lives beyond the running life of the software and the 
    volatile data is lost and defaulted each time the software runs.

    Written by: Scott Griffis
*/

#ifndef Settings_h
    #define Settings_h

    #include <string.h> // NEEDED by ESP_EEPROM and MUST appear before WString
    #include <ESP_EEPROM.h>
    #include <WString.h>
    #include <core_esp8266_features.h>
    #include <HardwareSerial.h>
    #include <MD5Builder.h>

    class Settings {
        private:
            // *****************************************************************************
            // Structure used for storing of settings related data and persisted into flash
            // *****************************************************************************
            struct NonVolatileSettings {
                char           ssid             [33]  ; // 32 chars is max size + 1 null
                char           pwd              [64]  ; // 63 chars is max size + 1 null
                char           adminUser        [51]  ;
                char           adminPwd         [51]  ;
                char           apPwd            [51]  ;
                char           title            [51]  ;
                char           heading          [51]  ;
                bool           timerOn                ;
                int            onTime                 ;
                int            offTime                ;
                bool           lightsOn               ;
                char           sentinel         [33]  ; // Holds a 32 MD5 hash + 1
            } nvSettings;

            struct NonVolatileSettings factorySettings = {
                "SET_ME", // <----------------------- ssid
                "SET_ME", // <----------------------- pwd
                "admin", // <------------------------ adminUser
                "admin", // <------------------------ adminPwd
                "P@ssw0rd123", // <------------------ apPwd
                "Lumen Lighting Controller", // <---- title
                "Device Info", // <------------------ heading
                false, // <-------------------------- timerOn
                1700, // <--------------------------- onTime
                2200, // <--------------------------- offTime
                false, // <-------------------------- lightsOn
                "NA" // <---------------------------- sentinel
            };

            // ******************************************************************
            // Structure used for storing of settings related data NOT persisted
            // ******************************************************************
            struct VolatileSettings {
                // Nothing yet
            } vSettings;

            // *****************************************************************************
            // Structure used for storing of settings related data that is set prior to 
            // compile time and constant in nature.
            // *****************************************************************************
            struct ConstSettings {
                String hostname;
                String apSsid;
                String apPwd;
                String apNetIp;
                String apSubnet;
                String apGateway;
            } cSettings = {
                "LumenCtrl", // <---------- hostname (*later ID is added)
                "LumenCtrl_", // <--------- apSsid (*later ID is added)
                "192.168.1.1", // <-------- apNetIp
                "255.255.255.0", // <------ apSubnet
                "192.168.1.1", // <-------- apGateway
            };
            
            void defaultSettings();
            String hashNvSettings(NonVolatileSettings nvSet);


        public:
            Settings();

            bool factoryDefault();
            bool loadSettings();
            bool saveSettings();
            bool isFactoryDefault();
            
            /*
            =========================================================
                                Getters and Setters 
            =========================================================
            */
            // Default getters
            String         getDefaultSsid      ()                       ;
            String         getDefaultPwd       ()                       ;

            // General device settings
            void           setAdminUser        (const char *user)       ;
            String         getAdminUser        ()                       ;
            void           setAdminPwd         (const char *pwd)        ;
            String         getAdminPwd         ()                       ;
            void           setApPwd            (const char *pwd)        ;
            String         getApPwd            ()                       ;
            void           setTitle            (const char *title)      ;
            String         getTitle            ()                       ;
            void           setHeading          (const char *heading)    ;
            String         getHeading          ()                       ;

            // WiFi STA Settings
            void           setSsid             (const char *ssid)       ;
            String         getSsid             ()                       ;
            void           setPwd              (const char *pwd)        ;
            String         getPwd              ()                       ;

            // Used for timer functionality
            void           setTimerOn          (bool on)                ;
            bool           isTimerOn           ()                       ;
            void           setOnTime           (int time24)             ;
            int            getOnTime           ()                       ;
            void           setOffTime          (int time24)             ;
            int            getOffTime          ()                       ;

            // Used for ligthing functionality
            void           setLightsOn         (bool on)                ;
            bool           isLightsOn          ()                       ;
            
            // WiFi AP Settings
            String         getHostname       (String deviceId)    ;
            String         getApSsid         (String deviceId)    ;
            String         getApNetIp        ()                   ;
            String         getApSubnet       ()                   ;    
            String         getApGateway      ()                   ;
    };
#endif