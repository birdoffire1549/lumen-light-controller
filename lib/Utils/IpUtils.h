#ifndef IpUtils_h
    #define IpUtils_h

    #include <WString.h>
    #include <IPAddress.h>

    /**
     * UTILITY CLASS
     * 
     * This is a Utility function class intented to contain useful utility functions for use in 
     * the containing application for working with IP Addresses in various ways.
     * 
     * @author Scott Griffis
     * @date 04-10-2024
     */
    class IpUtils {
        private:

        public:
            static IPAddress stringIPv4ToIPAddress(String ip);
    };

#endif