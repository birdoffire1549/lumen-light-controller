#ifndef HtmlContent_h
    #define HtmlContent_h

    #include <WString.h>
    #include <pgmspace.h>

    const char PROGMEM MAIN_PAGE[] = {
        "<!DOCTYPE HTML> "
        "<html lang=\"en\"> "
            "<head> "
                "<title>${title}</title> "
                "<style> "
                    "body { background-color: #FFFFFF; color: #000000; } "
                    "h1 { text-align: center; background-color: #5878B0; color: #FFFFFF; border: 3px; border-radius: 15px; } "
                    "h2 { text-align: center; background-color: #58ADB0; color: #FFFFFF; border: 3px; } "
                    "#successful { text-align: center; color: #02CF39; } "
                    "#failed { text-align: center; color: #CF0202; } "
                    "#wrapper { background-color: #E6EFFF; padding: 20px; margin-left: auto; margin-right: auto; max-width: 700px; box-shadow: 3px 3px 3px #333; } "
                    "#info { font-size: 25px; font-weight: bold; line-height: 150%; } "
                    "button { background-color: #5878B0; color: white; font-size: 16px; padding: 10px 24px; border-radius: 12px; border: 2px solid black; transition-duration: 0.4s; } "
                    "button:hover { background-color: white; color: black; } "
                "</style> "
            "</head> "
            ""
            "<div id=\"wrapper\"> "
                "<h1>${heading}</h1> "
                "Firmware Version: ${version}"
                "<div id=\"info\">"
                    "<p>"
                        "${status_message}"
                        "Light Status: ${on_off_status}"
                        "<br><br>"
                        "<form action=\"/\" method=\"post\">"
                            "<h3>Timer Control</h3>"
                            "Status: ${timer_on_off} <button type=\"submit\" name=\"do\" value=\"toggle_timer_state\">Toggle</button>"
                            "<br /><br />"
                            "<div ${schedule_hide}>"
                                "<h4>Schedule</h4>"
                                "On at: <input type=\"time\" id=\"onat\" name=\"onat\" value=\"${on_at}\" required />"
                                "Off at: <input type=\"time\" id=\"offat\" name=\"offat\" value=\"${off_at}\" required />"
                                "<br />"
                                "<button type=\"submit\" name=\"do\" value=\"btn_update\">Update</button>"
                                "<br />"
                            "</div>"
                            "<table>"
                                "<tr><td>Manual Controls/Override</td></tr>"
                                "<tr><td><button type=\"submit\" name=\"do\" value=\"btn_on\">On</button></td><td><button type=\"submit\" name=\"do\" value=\"btn_off\">Off</button></td></tr>"
                            "</table>"
                            "<br />"
                            "<button type=\"submit\" name=\"do\" value=\"goto_admin\">Settings</button>"
                        "</form>"
                    "</p>"
                "</div> "
            "</div> "
        "</html>"
    };

    const char PROGMEM SENSOR_OPTION[] = {
        "<option value=\"${id}\" ${selection_flag}>${description}</option>"
    };

    const char PROGMEM STATUS_MESSAGE[] = {
        "<script>alert(\"${message}\");</script>"
    };

    /**
     * This is the HTML content of the Admin/Settings Page.
     * This HTML has replaceable place-holders for dynamic informaton to be
     * added just prior to sending to client.
    */
    const char PROGMEM SETTINGS_PAGE[] = {
        "<!DOCTYPE HTML> "
        "<html lang=\"en\"> "
            "<head> "
                "<title>${title}</title> "
                "<style> "
                    "body { background-color: #FFFFFF; color: #000000; } "
                    "h1 { text-align: center; background-color: #5878B0; color: #FFFFFF; border: 3px; border-radius: 15px; } "
                    "h2 { text-align: center; background-color: #58ADB0; color: #FFFFFF; border: 3px; } "
                    "#successful { text-align: center; color: #02CF39; } "
                    "#failed { text-align: center; color: #CF0202; } "
                    "#wrapper { background-color: #E6EFFF; padding: 20px; margin-left: auto; margin-right: auto; max-width: 700px; box-shadow: 3px 3px 3px #333; } "
                    "#info { font-size: 25px; font-weight: bold; line-height: 150%; } "
                    "button { background-color: #5878B0; color: white; font-size: 16px; padding: 10px 24px; border-radius: 12px; border: 2px solid black; transition-duration: 0.4s; } "
                    "button:hover { background-color: white; color: black; } "
                "</style> "
            "</head> "
            ""
            "<div id=\"wrapper\"> "
                "<h1>${heading}</h1> "
                "Firmware Version: ${version}"
                "<div id=\"info\">"
                    "<form method=\"post\" action=\"/\"> "
                        "<h2>Application</h2> "
                        "<table>"
                            "<tr><td>Title:</td><td><input maxlength=\"50\" type=\"text\" value=\"${title}\" name=\"title\" id=\"title\"></td></tr> "
                            "<tr><td>Heading:</td><td><input maxlength=\"50\" type=\"text\" value=\"${heading}\" name=\"heading\" id=\"heading\"></td></tr> "
                        "</table>"
                        "<h2>WiFi</h2> "
                        "<div>Note: Leave these settings at 'SET_ME' to keep device in AP Mode.</div>"
                        "<table>"
                            "<tr><td>SSID:</td><td><input maxlength=\"32\" type=\"text\" value=\"${ssid}\" name=\"ssid\" id=\"ssid\"></td></tr> "
                            "<tr><td>Password:</td><td><input maxlength=\"63\" type=\"text\" value=\"${pwd}\" name=\"pwd\" id=\"pwd\"></td></tr> "
                        "</table>"
                        "<h2>Admin</h2> "
                        "<table>"
                            "<tr><td>Admin User:</td><td><input maxlength=\"12\" type=\"text\" value=\"${adminuser}\" name=\"adminuser\" id=\"adminuser\"></td></tr> "
                            "<tr><td>Admin Password:</td><td><input maxlength=\"12\" type=\"text\" value=\"${adminpwd}\" name=\"adminpwd\" id=\"adminpwd\"></td></tr> "
                        "</table>"
                        "<br /> "
                        "<button type=\"submit\" name=\"do\" value=\"admin_save\">Save</button><button type=\"submit\" name=\"do\" value=\"admin_exit\">Exit</button>"
                    "</form>"
                "</div> "
            "</div> "
        "</html>"
    };
#endif