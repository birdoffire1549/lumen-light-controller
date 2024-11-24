/*
  Utils - This is a Utility function class intented to contain useful utility functions for use in 
  the containing application.

  Written by: Scott Griffis
  Date: 11-22-2024
*/

#include "Utils.h"

/**
 * Function used to perform a MD5 Hash on a given string
 * the result is the MD5 Hash.
 * 
 * @param string The string to hash as String.
 * 
 * @return Returns the generated MD5 Hash as String.
*/
String Utils::hashString(String str) {
    MD5Builder builder = MD5Builder();
    builder.begin();
    builder.add(str);
    builder.calculate();
    
    return builder.toString();
}

/**
 * Generates a six character Device ID based on the
 * given macAddress.
 * 
 * @param macAddress The device's MAC Address as String.
 * 
 * @return Returns a six digit Device ID as String.
*/
String Utils::genDeviceIdFromMacAddr(String macAddress) {
    String result = hashString(macAddress);
    int len = result.length();
    if (len > 6) {
        result = result.substring((len - 6), len);
    }
    result.toUpperCase();

    return result;
}

/*
  Function handles flashing of the LED for signaling the given IP Address
  entirely or simply its last octet as determined by the passed boolean 
  refered to as quick. If quick is TRUE then last Octet is signaled, if 
  FALSE then entire IP is signaled.

  @param ledPin - The pin number the LED is attached to as int.
  @param ipAddress - The IP Address in dot notation as String.
  @param quick - Indicates if signaling is for last octet or whole IP as bool.
*/
void Utils::signalIpAddress(int ledPin, String ipAddress, bool quick) {
  if (!quick) { // Whole IP Requested...
    int octet[3];
    
    // Parse 3 decimal values from each octet...
    int index = ipAddress.indexOf('.');
    int index2 = ipAddress.indexOf('.', index + 1);
    int index3 = ipAddress.lastIndexOf('.');
    octet[0] = ipAddress.substring(0, index).toInt();
    octet[1] = ipAddress.substring(index + 1, index2).toInt();
    octet[2] = ipAddress.substring(index2 + 1, index3).toInt();

    for (int i = 0; i < 3; i++) { // Iterate first 3 octets and signal...
      displayOctet(ledPin, octet[i]);
      displayNextOctetIndicator(ledPin);
    }
  }

  // Signals 4th octet regardless of quick or not
  int fourth = ipAddress.substring(ipAddress.lastIndexOf('.') + 1).toInt();
  displayOctet(ledPin, fourth);
  displayDone(ledPin); // Fast blink...
} 

int Utils::stringTimeToIntTime(String time24) {
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
String Utils::intTimeToStringTime(int time24) {
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

int Utils::adjustIntTimeForTimezone(int time24, int timezone) {
  int mins = (time24 % 100);
  int hours = (time24 / 100) + timezone;
  if (hours < 0) {
    hours += 24;
  }

  return ((hours * 100) + mins);
}

String Utils::intTimeToString12Time(int time24) {
  int h = time24 / 100;
  int m = time24 % 100;
  
  String ap = "AM";
  if (h > 12) {
    h -= 12;
    ap = "PM";
  }

  String time = String(h);
  time.concat(m < 10 ? ":0" : ":");
  time.concat(String(m));
  time.concat(" ");
  time.concat(ap);

  return time;
}

bool Utils::flipSafeHasTimeExpired(unsigned long startMillis, unsigned long expireInMillis) {
  unsigned long now = millis();
  if (now >= startMillis) {

    return now - startMillis >= expireInMillis;
  }
  
  return (((__LONG_MAX__ * 2UL + 1UL) - startMillis + now) >= expireInMillis);
}




/*
=============================================================================
PRIVATE FUNCTIONS BELOW
=============================================================================
*/




/*
  PRIVATE: This function is in charge of displaying or signaling a single
  octet of an IP address.

  @param ledPin - The pin number the LED is attached to as int.
  @param octet - The value of the octet to signal as int. 
*/
void Utils::displayOctet(int ledPin, int octet) {
  if (displayDigit(ledPin, octet / 100)) { // A non-zero value was in the 100's place...
    displayNextDigitIndicator(ledPin);
    octet = octet % 100;
  }
  if (displayDigit(ledPin, octet / 10)) { // A non-zero value was in the 10's palce...
    displayNextDigitIndicator(ledPin);
  }
  displayDigit(ledPin, octet % 10);
}




/*
  PRIVATE: This function's job is to generate flashes for 
  a single digit.

  @param ledPin - The pin number of the LED as int.
  @param digit - The digit being a 0 to 9 value as int.
  @return Returns true if digit was a non-zero value otherwise false as bool.
*/
bool Utils::displayDigit(int ledPin, int digit) {
  digitalWrite(ledPin, LOW);
  bool result = false; // Indicates a non-zero value if true.
  for (int i = 0; i < digit; i++) { // Once per value of the digit...
    result = true;
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }

  return result;
}




/*
  PRIVATE: Displays or signals the separator between octets which
  is simply 2 Next Digit Indicators.

  @param ledPin - The pin number of the LED as int.
*/
void Utils::displayNextOctetIndicator(int ledPin) {
  displayNextDigitIndicator(ledPin);
  displayNextDigitIndicator(ledPin);
}




/*
  PRIVATE: This displays the Next Digit Indicator which is simply a way to visually
  break up digit flashes.

  @param ledPin - The pin number of the LED as int.
*/
void Utils::displayNextDigitIndicator(int ledPin) {
  digitalWrite(ledPin, LOW);
  delay(700);
  for (int i = 0; i < 3; i++) { // Flash 3 times...
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
  delay(900);
}




/*
  PRIVATE: This displays or signals the Display Done flashes.
  This is a visual way for the Device to say it is done 
  signaling the requested IP Address or last Octet.

  @param ledPin - The pin number of the LED as int.
*/
void Utils::displayDone(int ledPin) {
  digitalWrite(ledPin, LOW);
  delay(1000);
  for (int i = 0; i < 20; i++) { // Do 20 flashes...
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

float Utils::convertCelciusToFahrenheit(float celcius) {
  return ((celcius * 9/5) + 32);
}

