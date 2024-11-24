#ifndef Settings_h
    #define Settings_h

    #include <string.h> // NEEDED by ESP_EEPROM and MUST appear before WString
    #include <ESP_EEPROM.h>
    #include <WString.h>
    #include <core_esp8266_features.h>
    #include <HardwareSerial.h>
    #include <MD5Builder.h>

    /**
     * The Settings class instantiates into an object which is intended to be the gateway
     * thru which the software interacts with all settings, including those persisted to
     * the Flash Memory.
     * 
     * @author Scott Griffis
     * @date 11-23-24 
     */
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
                String  hostname   ;
                String  apSsid     ;
                String  apNetIp    ;
                String  apSubnet   ;
                String  apGateway  ;
            } cSettings = {
                "Lumen", // <-------------- hostname (*later ID is added)
                "Lumen_", // <------------- apSsid (*later ID is added)
                "192.168.1.1", // <-------- apNetIp
                "255.255.255.0", // <------ apSubnet
                "0.0.0.0", // <------------ apGateway
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
            String       getHostname       (String deviceId)    ;
            String       getApSsid         (String deviceId)    ;
            String       getApNetIp        ()                   ;
            String       getApSubnet       ()                   ;    
            String       getApGateway      ()                   ;
    };
#endif