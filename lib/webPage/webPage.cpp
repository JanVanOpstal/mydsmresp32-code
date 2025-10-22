#ifndef webPagecpp
#define webPagecpp

#include <WiFi.h>
//#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

class WebPage{
    public:
    String title = "";
    String css = "\
            body {\
                background-color: black;\
                color: rgb(240, 240, 240);\
                font-size: 10px;\
                font-family: verdana, sans-serif;\
            }\
            div.center{\
                margin-left: auto;\
                margin-right: auto;\
                width: 100%;\
                max-width: 346px;\
                text-align: center;\
            }\
            div.button{\
                width: 100%;\
                height: 40px;\
                border-radius: 8px;\
                background-color: rgb(255, 166, 0);\
                color: rgb(255, 255, 255);\
                text-align: center;\
                line-height: 40px;\
                font-family: Arial, Helvetica, sans-serif;\
                font-size: 20px;\
            }\
            a{\
                text-decoration: none;\
            }\
            div.button:hover{\
                background-color: rgb(223, 145, 0);\
                cursor: pointer;\
            }\
            div.bottomdiv{\
                margin-top: 16px;\
                width:100%;\
                border-top-width: 1px;\
                border-top-color: white;\
                border-top-style: solid;\
                font-size:10px;\
                text-align: right;\
                padding-top: 2px;\
            }\
            div.textbox,\
            div.list{\
                width:100%;\
                padding-top: 8px;\
                text-align: left;\
                font-size: 16px;\
                font-family: verdana, sans-serif;\
            }\
            div.textbox input{\
                width:98%;\
                line-height: 24px;\
            }\
            div.bigspacer {\
                width:100%;\
                height: 32px;\
            }\
            div.spacer {\
                width:100%;\
                height: 24px;\
            }\
            div.smallspacer {\
                width:100%;\
                height: 8px;\
            }\
            div.button.safe {\
                background-color: green;\
            }\
            div.button.safe:hover {\
                background-color: darkgreen;\
            }\
            div.button.danger {\
                background-color: red;\
            }\
            div.button.danger:hover {\
                background-color: darkred;\
            }\
            span.ssid {\
                cursor: pointer;\
            }";
    int contentCount = 0;
    String content[20];

    String BuildPage(){
        String fullContent = "";
        for (int i=0; i < contentCount; i++){
            fullContent += content[i];
        }
        String html = "\
<!DOCTYPE html>\
<html>\
    <head>\
        <meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">\
        <title>SMPS data</title>\
        <script src=\"https://kit.fontawesome.com/b2ad5bb56c.js\" crossorigin=\"anonymous\"></script>\
        <style>\
            " + css + "\
        </style>\
        <script>\
            function setSSID (newValue){\
                document.getElementById('SSID').value = newValue;\
                document.getElementById('Password').value = '';\
            }\
            function save(){\
                ssid = document.getElementById('SSID').value;\
                passwd = document.getElementById('Password').value;\
                sendWifi(ssid, passwd);\
            }\
            function saveMQTT(){\
                host = document.getElementById('Host').value;\
                port = document.getElementById('Port').value;\
                clientID = document.getElementById('Client').value;\
                topic = document.getElementById('Topic').value;\
                user = document.getElementById('User').value;\
                passwd = document.getElementById('Password').value;\
                sendMQTT(host, port, clientID, topic, user, passwd);\
            }\
            function clearW(){\
                clearWifi();\
            }\
            const sendWifi = async (ssid, passwd) => {\
                senddoc = JSON.stringify({\
                    'SSID':ssid,\
                    'passwd':passwd\
                });\
                const response = await fetch('./savewifi', {\
                    method: 'POST',\
                    body: senddoc, \
                    headers: {\
                        'Content-Type': 'application/json'\
                    }\
                });\
                const myJson = await response.json(); \
                if (myJson.result == 'OK'){\
                    alert('Settings were ok. Device will restart and connect using new settings.');\
                }\
            };\
            const sendMQTT = async (host, port, clientID, topic, user, passwd) => {\
                senddoc = JSON.stringify({\
                    'host':host,\
                    'port':port,\
                    'clientID':clientID,\
                    'topic':topic,\
                    'user':user,\
                    'passwd':passwd\
                });\
                const response = await fetch('./savemqtt', {\
                    method: 'POST',\
                    body: senddoc, \
                    headers: {\
                        'Content-Type': 'application/json'\
                    }\
                });\
                const myJson = await response.json(); \
                if (myJson.result == 'OK'){\
                    alert('Settings were ok. MQTTSettings will be saved and connection will be recreated.');\
                }\
            };\
            const clearWifi = async () => {\
                senddoc = JSON.stringify({});\
                const response = await fetch('./clearwifi', {\
                    method: 'POST',\
                    body: senddoc, \
                    headers: {\
                        'Content-Type': 'application/json'\
                    }\
                });\
                const myJson = await response.json(); \
            };\
        </script>\
    </head>\
    <body>\
        <div class=\"center\">\
            <h1>SMP-Sensor</h1>";
            if (title != ""){
                html += "<h2>" + title + "</h2>";
            }
            html += fullContent + "\
            <div class=\"bottomdiv\">\
                V0.0.4 - By Stan, Tom & Jan Van Opstal\
            </div>\
        </div>\
    </body>\
</html>";
        return html;
    }

    void addButton(String text, String path, String addClass = ""){
        content[contentCount] = "<a href=\"./" + path + "\">\
                <div class=\"button " + addClass + "\">\
                    " + text + "\
                </div>\
            </a>";
        contentCount += 1;
    }

    void addPostButton(String text, String method, String addClass = ""){
        content[contentCount] = "<div class=\"button " + addClass + "\" onclick=\"" + method + "\">\
                " + text + "\
            </div>";;
        contentCount += 1;
    }

    void addTextbox(String bijschrift, String name, String value, String placeholder){
        content[contentCount] = "<div class=\"textbox\">\
                " + bijschrift + ":<br>\
                <input id=\"" + name + "\" type=\"text\" placeholder=\"" + placeholder + "\" value=\"" + value + "\" name=\"" + name + "\"> \
            </div>";
        contentCount += 1;
    }

    void addPassword(String bijschrift, String name, String value, String placeholder){
        content[contentCount] = "<div class=\"textbox\">\
                " + bijschrift + ":<br>\
                <input id=\"" + name + "\" type=\"password\" placeholder=\"" + placeholder + "\" value=\"" + value + "\" name=\"" + name + "\"> \
            </div>";
        contentCount += 1;
    }

    void addSpacer(){
        content[contentCount] = "\
            <div class=\"spacer\"></div>";
        contentCount += 1;
    }

    void addSmallSpacer(){
        content[contentCount] = "\
            <div class=\"smallspacer\"></div>";
        contentCount += 1;
    }

    void addBigSpacer(){
        content[contentCount] = "\
            <div class=\"bigspacer\"></div>";
        contentCount += 1;
    }

    void addListOverview(String items[], int itemCount){
        content[contentCount] = "<div class=\"list ssids\">";
        for (int i = 0; i < itemCount; ++i) {
            content[contentCount] += items[i];
        }
        content[contentCount] += "</div>";
    
        contentCount += 1;
    }
};

#endif
