#ifndef Utils_h
  #define Utils_h

  #include <WString.h>
  #include <Arduino.h>

  /**
   * UTILITY CLASS
   * 
   * This is a Utility function class intented to contain useful utility functions for use in 
   * the containing application.
   * 
   * @author Scott Griffis
   * @date 12-01-2024
   */
  class Utils {
    private:
      Utils();

      static bool displayDigit(int ledPin, int digit);
      static void displayDone(int ledPin);
      static void displayNextDigitIndicator(int ledPin);
      static void displayNextOctetIndicator(int ledPin);
      static void displayOctet(int ledPin, int octet);

    public:
      static String genDeviceIdFromMacAddr(String macAddress);
      static String hashString(String str);
      static void signalIpAddress(int ledPin, String ipAddress, bool quick);
      static float convertCelciusToFahrenheit(float celcius);
      static String intTimeToStringTime(int time24);
      static String intTimeToString12Time(int time24);
      static bool flipSafeHasTimeExpired(unsigned long startMillis, unsigned long expireInMillis);
      static int stringTimeToIntTime(String time24);
      static int adjustIntTimeForTimezone(int time24, int timezone, bool isDst);
  };

#endif