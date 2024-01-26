/*
 Name:		ESP32IGate
 Created:	13-10-2023 14:27:23
 Author:	HS5TQA/Atten
 Github:	https://github.com/nakhonthai
 Facebook:	https://www.facebook.com/atten
 Support IS: host:aprs.dprns.com port:14580 or aprs.hs5tqa.ampr.org:14580
 Support IS monitor: http://aprs.dprns.com:14501 or http://aprs.hs5tqa.ampr.org:14501
*/
#include "AFSK.h"
#include "webservice.h"
#include "base64.hpp"
#include "wireguard_vpn.h"
#include <LibAPRSesp.h>
#include <parse_aprs.h>
#include "jquery_min_js.h"

// Web Server;
WebServer server(80);
AsyncWebServer async_server(81);
AsyncWebSocket ws("/ws");
AsyncWebSocket ws_gnss("/ws_gnss");

String webString;

bool defaultSetting = false;

void serviceHandle()
{
	server.handleClient();
}

void handle_logout()
{
	webString = "Log out";
	server.send(401, "text/html", webString);
}

void setMainPage()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	webString = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
	webString += "<meta name=\"robots\" content=\"index\" />\n";
	webString += "<meta name=\"robots\" content=\"follow\" />\n";
	webString += "<meta name=\"language\" content=\"English\" />\n";
	webString += "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
	webString += "<meta name=\"GENERATOR\" content=\"configure 20230924\" />\n";
	webString += "<meta name=\"Author\" content=\"Mr.Somkiat Nakhonthai (HS5TQA)\" />\n";
	webString += "<meta name=\"Description\" content=\"Web Embedded Configuration\" />\n";
	webString += "<meta name=\"KeyWords\" content=\"ESP32IGATE,APRS\" />\n";
	webString += "<meta http-equiv=\"Cache-Control\" content=\"no-cache, no-store, must-revalidate\" />\n";
	webString += "<meta http-equiv=\"pragma\" content=\"no-cache\" />\n";
	webString += "<link rel=\"shortcut icon\" href=\"http://aprs.dprns.com/images/favicon.ico\" type=\"image/x-icon\" />\n";
	webString += "<meta http-equiv=\"Expires\" content=\"0\" />\n";
	webString += "<title>ESP32IGATE</title>\n";
	webString += "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\" />\n";
	webString += "<script src=\"jquery-3.7.1.js\"></script>\n";
	webString += "<script type=\"text/javascript\">\n";
	webString += "function selectTab(evt, tabName) {\n";
	webString += "var i, tabcontent, tablinks;\n";
	webString += "tablinks = document.getElementsByClassName(\"nav-tabs\");\n";
	webString += "for (i = 0; i < tablinks.length; i++) {\n";
	webString += "tablinks[i].className = tablinks[i].className.replace(\" active\", \"\");\n";
	webString += "}\n";
	webString += "\n";
	webString += "//document.getElementById(tabName).style.display = \"block\";\n";
	webString += "if (tabName == 'DashBoard') {\n";
	webString += "$(\"#contentmain\").load(\"/dashboard\");\n";
	webString += "} else if (tabName == 'Radio') {\n";
	webString += "$(\"#contentmain\").load(\"/radio\");\n";
	webString += "} else if (tabName == 'IGATE') {\n";
	webString += "$(\"#contentmain\").load(\"/igate\");\n";
	webString += "} else if (tabName == 'DIGI') {\n";
	webString += "$(\"#contentmain\").load(\"/digi\");\n";
	webString += "} else if (tabName == 'TRACKER') {\n";
	webString += "$(\"#contentmain\").load(\"/tracker\");\n";
	webString += "} else if (tabName == 'WX') {\n";
	webString += "$(\"#contentmain\").load(\"/wx\");\n";
	webString += "} else if (tabName == 'TLM') {\n";
	webString += "$(\"#contentmain\").load(\"/tlm\");\n";
	webString += "} else if (tabName == 'VPN') {\n";
	webString += "$(\"#contentmain\").load(\"/vpn\");\n";
	webString += "} else if (tabName == 'Wireless') {\n";
	webString += "$(\"#contentmain\").load(\"/wireless\");\n";
	webString += "} else if (tabName == 'MOD') {\n";
	webString += "$(\"#contentmain\").load(\"/mod\");\n";
	webString += "} else if (tabName == 'System') {\n";
	webString += "$(\"#contentmain\").load(\"/system\");\n";
	webString += "} else if (tabName == 'About') {\n";
	webString += "$(\"#contentmain\").load(\"/about\");\n";
	webString += "}\n";
	webString += "\n";
	webString += "if (evt != null) evt.currentTarget.className += \" active\";\n";
	webString += "}\n";
	webString += "</script>\n";
	webString += "</head>\n";
	webString += "\n";
	webString += "<body onload=\"selectTab(event, 'DashBoard')\">\n";
	webString += "\n";
	webString += "<div class=\"container\">\n";
	webString += "<div class=\"header\">\n";
	// webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\">ESP32IGate Firmware V" + String(VERSION) + "</div>\n";
	// webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\"><a href=\"/logout\">[LOG OUT]</a></div>\n";
	webString += "<h1>ESP32IGate Project [APRS ALL IN ONE]</h1>\n";
	webString += "<div style=\"font-size: 8px; text-align: right; padding-right: 8px;\"><a href=\"/logout\">[LOG OUT]</a></div>\n";
	webString += "<div class=\"row\">\n";
	webString += "<ul class=\"nav nav-tabs\" style=\"margin: 5px;\">\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'DashBoard')\">DashBoard</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'Radio')\" id=\"btnRadio\">Radio</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'IGATE')\">IGATE</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'DIGI')\">DIGI</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'TRACKER')\">TRACKER</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'WX')\">WX</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'TLM')\">TLM</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'VPN')\">VPN</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'Wireless')\">Wireless</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'MOD')\">MOD</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'System')\">System</button>\n";
	webString += "<button class=\"nav-tabs\" onclick=\"selectTab(event, 'About')\">About</button>\n";
	webString += "</ul>\n";
	webString += "</div>\n";
	webString += "</div>\n";
	webString += "\n";

	webString += "<div class=\"contentwide\" id=\"contentmain\"  style=\"font-size: 2pt;\">\n";
	webString += "\n";
	webString += "</div>\n";
	webString += "<br />\n";
	webString += "<div class=\"footer\">\n";
	webString += "ESP32IGate Web Configuration<br />Copy right ©2023.\n";
	webString += "<br />\n";
	webString += "</div>\n";
	webString += "</div>\n";
	webString += "<!-- <script type=\"text/javascript\" src=\"/nice-select.min.js\"></script> -->\n";
	webString += "<script type=\"text/javascript\">\n";
	webString += "var selectize = document.querySelectorAll('select')\n";
	webString += "var options = { searchable: true };\n";
	webString += "selectize.forEach(function (select) {\n";
	webString += "if (select.length > 30 && null === select.onchange && !select.name.includes(\"ExtendedId\")) {\n";
	webString += "select.classList.add(\"small\", \"selectize\");\n";
	webString += "tabletd = select.closest('td');\n";
	webString += "tabletd.style.cssText = 'overflow-x:unset';\n";
	webString += "NiceSelect.bind(select, options);\n";
	webString += "}\n";
	webString += "});\n";
	webString += "</script>\n";
	webString += "</body>\n";
	webString += "</html>";
	server.send(200, "text/html", webString); // send to someones browser when asked
}

////////////////////////////////////////////////////////////
// handler for web server request: http://IpAddress/      //
////////////////////////////////////////////////////////////

void handle_css()
{
	const char *css = ".container{width:800px;text-align:left;margin:auto;border-radius:10px 10px 10px 10px;-moz-border-radius:10px 10px 10px 10px;-webkit-border-radius:10px 10px 10px 10px;-khtml-border-radius:10px 10px 10px 10px;-ms-border-radius:10px 10px 10px 10px;box-shadow:3px 3px 3px #707070;background:#fff;border-color: #2194ec;padding: 0px;border-width: 5px;border-style:solid;}body,font{font:12px verdana,arial,sans-serif;color:#fff}.header{background:#2194ec;text-decoration:none;color:#fff;font-family:verdana,arial,sans-serif;text-align:left;padding:5px 0;border-radius:10px 10px 0 0;-moz-border-radius:10px 10px 0 0;-webkit-border-radius:10px 10px 0 0;-khtml-border-radius:10px 10px 0 0;-ms-border-radius:10px 10px 0 0}.content{margin:0 0 0 166px;padding:1px 5px 5px;color:#000;background:#fff;text-align:center;font-size: 8pt;}.contentwide{padding:50px 5px 5px;color:#000;background:#fff;text-align:center}.contentwide h2{color:#000;font:1em verdana,arial,sans-serif;text-align:center;font-weight:700;padding:0;margin:0;font-size: 12pt;}.footer{background:#2194ec;text-decoration:none;color:#fff;font-family:verdana,arial,sans-serif;font-size:9px;text-align:center;padding:10px 0;border-radius:0 0 10px 10px;-moz-border-radius:0 0 10px 10px;-webkit-border-radius:0 0 10px 10px;-khtml-border-radius:0 0 10px 10px;-ms-border-radius:0 0 10px 10px;clear:both}#tail{height:450px;width:805px;overflow-y:scroll;overflow-x:scroll;color:#0f0;background:#000}table{vertical-align:middle;text-align:center;empty-cells:show;padding-left:3;padding-right:3;padding-top:3;padding-bottom:3;border-collapse:collapse;border-color:#0f07f2;border-style:solid;border-spacing:0px;border-width:3px;text-decoration:none;color:#fff;background:#000;font-family:verdana,arial,sans-serif;font-size : 12px;width:100%;white-space:nowrap}table th{font-size: 10pt;font-family:lucidia console,Monaco,monospace;text-shadow:1px 1px #0e038c;text-decoration:none;background:#0525f7;border:1px solid silver}table tr:nth-child(even){background:#f7f7f7}table tr:nth-child(odd){background:#eeeeee}table td{color:#000;font-family:lucidia console,Monaco,monospace;text-decoration:none;border:1px solid #010369}body{background:#edf0f5;color:#000}a{text-decoration:none}a:link,a:visited{text-decoration:none;color:#0000e0;font-weight:400}th:last-child a.tooltip:hover span{left:auto;right:0}ul{padding:5px;margin:10px 0;list-style:none;float:left}ul li{float:left;display:inline;margin:0 10px}ul li a{text-decoration:none;float:left;color:#999;cursor:pointer;font:900 14px/22px arial,Helvetica,sans-serif}ul li a span{margin:0 10px 0 -10px;padding:1px 8px 5px 18px;position:relative;float:left}h1{text-shadow:2px 2px #303030;text-align:center}.toggle{position:absolute;margin-left:-9999px;visibility:hidden}.toggle+label{display:block;position:relative;cursor:pointer;outline:none}input.toggle-round-flat+label{padding:1px;width:33px;height:18px;background-color:#ddd;border-radius:10px;transition:background .4s}input.toggle-round-flat+label:before,input.toggle-round-flat+label:after{display:block;position:absolute;}input.toggle-round-flat+label:before{top:1px;left:1px;bottom:1px;right:1px;background-color:#fff;border-radius:10px;transition:background .4s}input.toggle-round-flat+label:after{top:2px;left:2px;bottom:2px;width:16px;background-color:#ddd;border-radius:12px;transition:margin .4s,background .4s}input.toggle-round-flat:checked+label{background-color:#dd4b39}input.toggle-round-flat:checked+label:after{margin-left:14px;background-color:#dd4b39}@-moz-document url-prefix(){select,input{margin:0;padding:0;border-width:1px;font:12px verdana,arial,sans-serif}input[type=button],button,input[type=submit]{padding:0 3px;border-radius:3px 3px 3px 3px;-moz-border-radius:3px 3px 3px 3px}}.nice-select.small,.nice-select-dropdown li.option{height:24px!important;min-height:24px!important;line-height:24px!important}.nice-select.small ul li:nth-of-type(2){clear:both}.nav{margin-bottom:0;padding-left:10;list-style:none}.nav>li{position:relative;display:block}.nav>li>a{position:relative;display:block;padding:5px 10px}.nav>li>a:hover,.nav>li>a:focus{text-decoration:none;background-color:#eee}.nav>li.disabled>a{color:#999}.nav>li.disabled>a:hover,.nav>li.disabled>a:focus{color:#999;text-decoration:none;background-color:initial;cursor:not-allowed}.nav .open>a,.nav .open>a:hover,.nav .open>a:focus{background-color:#eee;border-color:#428bca}.nav .nav-divider{height:1px;margin:9px 0;overflow:hidden;background-color:#e5e5e5}.nav>li>a>img{max-width:none}.nav-tabs{border-bottom:1px solid #ddd}.nav-tabs>li{float:left;margin-bottom:-1px}.nav-tabs>li>a{margin-right:0;line-height:1.42857143;border:1px solid #ddd;border-radius:10px 10px 0 0}.nav-tabs>li>a:hover{border-color:#eee #eee #ddd}.nav-tabs>button{margin-right:0;line-height:1.42857143;border:2px solid #ddd;border-radius:10px 10px 0 0}.nav-tabs>button:hover{background-color:#25bbfc;border-color:#428bca;color:#eaf2f9;border-bottom-color:transparent;}.nav-tabs>button.active,.nav-tabs>button.active:hover,.nav-tabs>button.active:focus{color:#f7fdfd;background-color:#1aae0d;border:1px solid #ddd;border-bottom-color:transparent;cursor:default}.nav-tabs>li.active>a,.nav-tabs>li.active>a:hover,.nav-tabs>li.active>a:focus{color:#428bca;background-color:#e5e5e5;border:1px solid #ddd;border-bottom-color:transparent;cursor:default}.nav-tabs.nav-justified{width:100%;border-bottom:0}.nav-tabs.nav-justified>li{float:none}.nav-tabs.nav-justified>li>a{text-align:center;margin-bottom:5px}.nav-tabs.nav-justified>.dropdown .dropdown-menu{top:auto;left:auto}.nav-status{float:left;margin:0;padding:3px;width:160px;font-weight:400;min-height:600}#bar,#prgbar {background-color: #f1f1f1;border-radius: 14px}#bar {background-color: #3498db;width: 0%;height: 14px}.switch{position:relative;display:inline-block;width:34px;height:16px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#f55959;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:\"\";height:12px;width:12px;left:2px;bottom:2px;background-color:#fff;-webkit-transition:.4s;transition:.4s}input:checked+.slider{background-color:#5ca30a}input:focus+.slider{box-shadow:0 0 1px #5ca30a}input:checked+.slider:before{-webkit-transform:translateX(16px);-ms-transform:translateX(16px);transform:translateX(16px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}\n";
	server.send_P(200, "text/css", css);
}

void handle_jquery()
{
	server.setContentLength(jquery_3_7_1_min_js_gz_len);
	server.sendHeader(String(F("Content-Encoding")), String(F("gzip")));
	server.send(200, String(F("application/javascript")), "");
	server.sendContent_P(jquery_3_7_1_min_js_gz, jquery_3_7_1_min_js_gz_len);
}

void handle_dashboard()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	webString = "<script type=\"text/javascript\">\n";
	webString += "function reloadSysInfo() {\n";
	webString += "$(\"#sysInfo\").load(\"/sysinfo\", function () { setTimeout(reloadSysInfo, 60000) });\n";
	webString += "}\n";
	webString += "setTimeout(reloadSysInfo(), 100);\n";
	webString += "function reloadSidebarInfo() {\n";
	webString += "$(\"#sidebarInfo\").load(\"/sidebarInfo\", function () { setTimeout(reloadSidebarInfo, 10000) });\n";
	webString += "}\n";
	webString += "setTimeout(reloadSidebarInfo, 200);\n";
	webString += "function reloadlastHeard() {\n";
	webString += "$(\"#lastHeard\").load(\"/lastHeard\", function () { setTimeout(reloadlastHeard, 10000) });\n";
	webString += "}\n";
	webString += "setTimeout(reloadlastHeard, 300);\n";
	webString += "$(window).trigger('resize');\n";
	webString += "</script>\n";

	webString += "<div id=\"sysInfo\">\n";
	webString += "</div>\n";

	webString += "<br />\n";
	webString += "<div class=\"nav-status\">\n";
	webString += "<div id=\"sidebarInfo\">\n";
	webString += "</div>\n";
	webString += "<br />\n";

	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">Radio Info</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>Freq TX</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.freq_tx, 4) + " MHz</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>Freq RX</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.freq_rx, 4) + " MHz</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>H/L</td>\n";
	if (config.rf_power)
		webString += "<td>HIGH</td>\n";
	else
		webString += "<td>LOW</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "\n";
	webString += "<br />\n";
	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">APRS SERVER</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>HOST</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.aprs_host) + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>PORT</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.aprs_port) + "</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "<br />\n";
	webString += "<table>\n";
	webString += "<tr>\n";
	webString += "<th colspan=\"2\">WiFi</th>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>MODE</td>\n";
	String strWiFiMode = "OFF";
	if (config.wifi_mode == WIFI_STA_FIX)
	{
		strWiFiMode = "STA";
	}
	else if (config.wifi_mode == WIFI_AP_FIX)
	{
		strWiFiMode = "AP";
	}
	else if (config.wifi_mode == WIFI_AP_STA_FIX)
	{
		strWiFiMode = "AP+STA";
	}
	webString += "<td style=\"background: #ffffff;\">" + strWiFiMode + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>SSID</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(WiFi.SSID()) + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<td>RSSI</td>\n";
	if (WiFi.isConnected())
		webString += "<td style=\"background: #ffffff;\">" + String(WiFi.RSSI()) + " dBm</td>\n";
	else
		webString += "<td style=\"background:#606060; color:#b0b0b0;\" aria-disabled=\"true\">Disconnect</td>\n";
	webString += "</tr>\n";
	webString += "</table>\n";
	webString += "<br />\n";
#ifdef BLUETOOTH
	webString += "<table>\n";
	webString += "<tr>\n";

	webString += "<th colspan=\"2\">Bluetooth</th>\n";
	webString += "</tr>\n";
	webString += "<td>Master</td>\n";
	if (config.bt_master)
		webString += "<td style=\"background:#0b0; color:#030; width:50%;\">Enabled</td>\n";
	else
		webString += "<td style=\"background:#606060; color:#b0b0b0;\" aria-disabled=\"true\">Disabled</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<tr>\n";
	webString += "<td>NAME</td>\n";
	webString += "<td style=\"background: #ffffff;\">" + String(config.bt_name) + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "<tr>\n";
	webString += "<td>MODE</td>\n";
	String btMode = "";
	if (config.bt_mode == 1)
	{
		btMode = "TNC2";
	}
	else if (config.bt_mode == 2)
	{
		btMode = "KISS";
	}
	else
	{
		btMode = "NONE";
	}
	webString += "<td style=\"background: #ffffff;\">" + btMode + "</td>\n";
	webString += "</tr>\n";
	webString += "<tr>\n";
	webString += "</table>\n";
#endif
	webString += "</div>\n";

	webString += "</div>\n";
	webString += "\n";
	webString += "<div class=\"content\">\n";
	webString += "<div id=\"lastHeard\">\n";
	webString += "</div>\n";

	server.send(200, "text/html", webString); // send to someones browser when asked
	delay(100);
	webString.clear();
}

void handle_sidebar()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	String html = "<table style=\"background:white;border-collapse: unset;\">\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">Modes Enabled</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	if (config.igate_en)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">IGATE</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">IGATE</th>\n";

	if (config.digi_en)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">DIGI</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">DIGI</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	if (config.wx_en)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">WX</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">WX</th>\n";
	html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\">SAT</th>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table style=\"background:white;border-collapse: unset;\">\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">Network Status</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	if (aprsClient.connected() == true)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">APRS-IS</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">APRS-IS</th>\n";
	if (wireguard_active() == true)
		html += "<th style=\"background:#0b0; color:#030; width:50%;border-radius: 10px;border: 2px solid white;\">VPN</th>\n";
	else
		html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">VPN</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">4G LTE</th>\n";
	html += "<th style=\"background:#606060; color:#b0b0b0;border-radius: 10px;border: 2px solid white;\" aria-disabled=\"true\">MQTT</th>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table>\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">STATISTICS</th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">PACKET RX:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.rxCount) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">PACKET TX:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.txCount) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">RF2INET:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.rf2inet) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">INET2RF:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.inet2rf) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">DIGI:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.digiCount) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td style=\"width: 60px;text-align: right;\">DROP/ERR:</td>\n";
	html += "<td style=\"background: #ffffff;\">" + String(status.dropCount) + "/" + String(status.errorCount) + "</td>\n";
	html += "</tr>\n";
	html += "</table>\n";
	html += "<br />\n";
	html += "<table>\n";
	html += "<tr>\n";
	html += "<th colspan=\"2\">GPS Info <a href=\"/gnss\" target=\"_gnss\" style=\"color: yellow;font-size:8pt\">[View]</a></th>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>LAT:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.location.lat(), 5) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>LON:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.location.lng(), 5) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>ALT:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.altitude.meters(), 1) + "</td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td>SAT:</td>\n";
	html += "<td style=\"background: #ffffff;text-align: left;\">" + String(gps.satellites.value()) + "</td>\n";
	html += "</tr>\n";
	html += "</table>\n";

	html += "<script>\n";
	html += "$(window).trigger('resize');\n";
	html += "</script>\n";
	server.send(200, "text/html", html); // send to someones browser when asked
	delay(100);
	html.clear();
}

void handle_symbol()
{
	int i;
	String html = "<table border=\"1\" align=\"center\">\n";
	html += "<tr><th colspan=\"16\">Table '/'</th></tr>\n";
	html += "<tr>\n";
	for (i = 33; i < 129; i++)
	{
		html += "<td><img src=\"http://aprs.dprns.com/symbols/icons/" + String(i) + "-1.png\"></td>\n";
		if (((i % 16) == 0) && (i < 126))
			html += "</tr>\n<tr>\n";
	}
	html += "</tr>";
	html += "</table>\n<br />";
	html += "<table border=\"1\" align=\"center\">\n";
	html += "<tr><th colspan=\"16\">Table '\\'</th></tr>\n";
	html += "<tr>\n";
	for (i = 33; i < 129; i++)
	{
		html += "<td><img src=\"http://aprs.dprns.com/symbols/icons/" + String(i) + "-2.png\"></td>\n";
		if (((i % 16) == 0) && (i < 126))
			html += "</tr>\n<tr>\n";
	}
	html += "</tr>";
	html += "</table>\n";
	server.send(200, "text/html", html); // send to someones browser when asked
}

void handle_sysinfo()
{
	String html = "<table style=\"table-layout: fixed;border-collapse: unset;border-radius: 10px;border-color: #ee800a;border-style: ridge;border-spacing: 1px;border-width: 4px;background: #ee800a;\">\n";
	html += "<tr>\n";
	html += "<th><span><b>Up Time</b></span></th>\n";
	html += "<th><span>Free RAM(KByte)</span></th>\n";
	html += "<th><span>Free PSRAM(KByte)</span></th>\n";
	html += "<th><span>SD CARD(MByte)</span></th>\n";
	html += "<th><span>CPU Temp(C)</span></th>\n";

	html += "</tr>\n";
	html += "<tr>\n";
	time_t tn = now() - systemUptime;
	// String uptime = String(day(tn) - 1, DEC) + "D " + String(hour(tn), DEC) + ":" + String(minute(tn), DEC) + ":" + String(second(tn), DEC);
	String uptime = String(day(tn) - 1, DEC) + "D " + String(hour(tn), DEC) + ":" + String(minute(tn), DEC);
	html += "<td><b>" + uptime + "</b></td>\n";
	html += "<td><b>" + String((float)ESP.getFreeHeap() / 1000, 1) + "/" + String((float)ESP.getHeapSize() / 1000, 1) + "</b></td>\n";
	html += "<td><b>" + String((float)ESP.getFreePsram() / 1000, 1) + "/" + String((float)ESP.getPsramSize() / 1000, 1) + "</b></td>\n";
	uint32_t cardTotal = SD.totalBytes() / (1024 * 1024);
	uint32_t cardUsed = SD.usedBytes() / (1024 * 1024);
	html += "<td><b>" + String(cardUsed) + "/" + String(cardTotal) + "</b></td>\n";
	html += "<td><b>" + String((float)(temprature_sens_read() - 32) / 1.8F, 1) + "</b></td>\n";
	// html += "<td style=\"background: #f00\"><b>" + String(ESP.getCycleCount()) + "</b></td>\n";
	html += "</tr>\n";
	html += "</table>\n";
	server.send(200, "text/html", html); // send to someones browser when asked
	html.clear();
}

void handle_lastHeard()
{
	struct pbuf_t aprs;
	ParseAPRS aprsParse;
	struct tm tmstruct;
	String html = "";
	sort(pkgList, PKGLISTSIZE);

	html = "<table>\n";
	html += "<th colspan=\"7\" style=\"background-color: #070ac2;\">LAST HEARD <a href=\"/tnc2\" target=\"_tnc2\" style=\"color: yellow;font-size:8pt\">[RAW]</a></th>\n";
	html += "<tr>\n";
	html += "<th style=\"min-width:10ch\"><span><b>Time (";
	if (config.timeZone >= 0)
		html += "+";
	// else
	//	html += "-";

	if (config.timeZone == (int)config.timeZone)
		html += String((int)config.timeZone) + ")</b></span></th>\n";
	else
		html += String(config.timeZone, 1) + ")</b></span></th>\n";
	html += "<th style=\"min-width:16px\">ICON</th>\n";
	html += "<th style=\"min-width:10ch\">Callsign</th>\n";
	html += "<th>VIA LAST PATH</th>\n";
	html += "<th style=\"min-width:5ch\">DX</th>\n";
	html += "<th style=\"min-width:5ch\">PACKET</th>\n";
	html += "<th style=\"min-width:5ch\">AUDIO</th>\n";
	html += "</tr>\n";

	for (int i = 0; i < 30; i++)
	{
		if (i >= PKGLISTSIZE)
			break;
		pkgListType pkg = getPkgList(i);
		if (pkg.time > 0)
		{
			String line = String(pkg.raw);
			int packet = pkg.pkg;
			int start_val = line.indexOf(">", 0); // หาตำแหน่งแรกของ >
			if (start_val > 3)
			{
				String src_call = line.substring(0, start_val);
				memset(&aprs, 0, sizeof(pbuf_t));
				aprs.buf_len = 300;
				aprs.packet_len = line.length();
				line.toCharArray(&aprs.data[0], aprs.packet_len);
				int start_info = line.indexOf(":", 0);
				int end_ssid = line.indexOf(",", 0);
				int start_dst = line.indexOf(">", 2);
				int start_dstssid = line.indexOf("-", start_dst);
				String path = "";

				if ((end_ssid > start_dst) && (end_ssid < start_info))
				{
					path = line.substring(end_ssid + 1, start_info);
				}
				if (end_ssid < 5)
					end_ssid = start_info;
				if ((start_dstssid > start_dst) && (start_dstssid < start_dst + 10))
				{
					aprs.dstcall_end_or_ssid = &aprs.data[start_dstssid];
				}
				else
				{
					aprs.dstcall_end_or_ssid = &aprs.data[end_ssid];
				}
				aprs.info_start = &aprs.data[start_info + 1];
				aprs.dstname = &aprs.data[start_dst + 1];
				aprs.dstname_len = end_ssid - start_dst;
				aprs.dstcall_end = &aprs.data[end_ssid];
				aprs.srccall_end = &aprs.data[start_dst];

				// Serial.println(aprs.info_start);
				// aprsParse.parse_aprs(&aprs);
				if (aprsParse.parse_aprs(&aprs))
				{
					pkg.calsign[10] = 0;
					time_t tm = pkg.time;
					localtime_r(&pkg.time, &tmstruct);
					char strTime[10];
					sprintf(strTime, "%02d:%02d:%02d", tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
					// String str = String(tmstruct.tm_hour, DEC) + ":" + String(tmstruct.tm_min, DEC) + ":" + String(tmstruct.tm_sec, DEC);

					html += "<tr><td>" + String(strTime) + "</td>";
					String fileImg = "";
					uint8_t sym = (uint8_t)aprs.symbol[1];
					if (sym > 31 && sym < 127)
					{
						if (aprs.symbol[0] > 64 && aprs.symbol[0] < 91) // table A-Z
						{
							html += "<td><b>" + String(aprs.symbol[0]) + "</b></td>";
						}
						else
						{
							fileImg = String(sym, DEC);
							if (aprs.symbol[0] == 92)
							{
								fileImg += "-2.png";
							}
							else if (aprs.symbol[0] == 47)
							{
								fileImg += "-1.png";
							}
							else
							{
								fileImg = "dot.png";
							}
							html += "<td><img src=\"http://aprs.dprns.com/symbols/icons/" + fileImg + "\"></td>";
						}
					}
					else
					{
						html += "<td><img src=\"http://aprs.dprns.com/symbols/icons/dot.png\"></td>";
					}
					html += "<td>" + src_call;
					if (aprs.srcname_len > 0 && aprs.srcname_len < 10) // Get Item/Object
					{
						char itemname[10];
						memset(&itemname, 0, sizeof(itemname));
						memcpy(&itemname, aprs.srcname, aprs.srcname_len);
						html += "(" + String(itemname) + ")";
					}
					html += +"</td>";
					if (path == "")
					{
						html += "<td style=\"text-align: left;\">RF: DIRECT</td>";
					}
					else
					{
						String LPath = path.substring(path.lastIndexOf(',') + 1);
						// if(path.indexOf("qAR")>=0 || path.indexOf("qAS")>=0 || path.indexOf("qAC")>=0){ //Via from Internet Server
						if (path.indexOf("qA") >= 0 || path.indexOf("TCPIP") >= 0)
						{
							html += "<td style=\"text-align: left;\">INET: " + LPath + "</td>";
						}
						else
						{
							if (LPath.indexOf("*") > 0)
								html += "<td style=\"text-align: left;\">DIGI: " + path + "</td>";
							else
								html += "<td style=\"text-align: left;\">RF: " + path + "</td>";
						}
					}
					// html += "<td>" + path + "</td>";
					if (aprs.flags & F_HASPOS)
					{
						double lat, lon;
						if (gps.location.isValid())
						{
							lat = gps.location.lat();
							lon = gps.location.lng();
						}
						else
						{
							lat = config.igate_lat;
							lon = config.igate_lon;
						}
						double dtmp = aprsParse.direction(lon, lat, aprs.lng, aprs.lat);
						double dist = aprsParse.distance(lon, lat, aprs.lng, aprs.lat);
						html += "<td>" + String(dist, 1) + "km/" + String(dtmp, 0) + "°</td>";
					}
					else
					{
						html += "<td>-</td>\n";
					}
					html += "<td>" + String(packet) + "</td>\n";
					if (pkg.audio_level == 0)
					{
						html += "<td>-</td></tr>\n";
					}
					else
					{
						double Vrms = (double)pkg.audio_level / 1000;
						double audBV = 20.0F * log10(Vrms);
						if (audBV < -20.0F)
						{
							html += "<td style=\"color: #0000f0;\">";
						}
						else if (audBV > -5.0F)
						{
							html += "<td style=\"color: #f00000;\">";
						}
						else
						{
							html += "<td style=\"color: #008000;\">";
						}
						html += String(audBV, 1) + "dBV</td></tr>\n";
					}
				}
			}
		}
	}
	html += "</table>\n";
	server.send(200, "text/html", html); // send to someones browser when asked
	delay(100);
	html.clear();
}

void handle_radio()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	// bool noiseEn=false;
	bool radioEnable = false;
	if (server.hasArg("commitRadio"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "radioEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						radioEnable = true;
					}
				}
			}

			if (server.argName(i) == "nw_band")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.band = server.arg(i).toInt();
						// if (server.arg(i).toInt())
						// 	config.band = 1;
						// else
						// 	config.band = 0;
					}
				}
			}

			if (server.argName(i) == "volume")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.volume = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rf_power")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						if (server.arg(i).toInt())
							config.rf_power = true;
						else
							config.rf_power = false;
					}
				}
			}

			if (server.argName(i) == "sql_level")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.sql_level = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx_freq")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.freq_tx = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "rx_freq")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.freq_rx = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "tx_offset")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.offset_tx = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "rx_offset")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.offset_rx = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx_ctcss")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tone_tx = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "rx_ctcss")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tone_rx = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "rf_type")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.rf_type = server.arg(i).toInt();
				}
			}
		}
		// config.noise=noiseEn;
		// config.agc=agcEn;
		config.rf_en = radioEnable;
		String html = "OK";
		server.send(200, "text/html", html); // send to someones browser when asked
		saveEEPROM();
		delay(500);
		RF_MODULE(false);
	}
	else if (server.hasArg("commitTNC"))
	{
		bool hpf = 0;
		bool bpf = 0;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "HPF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						hpf = true;
					}
				}
			}
			if (server.argName(i) == "BPF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						bpf = true;
					}
				}
			}
			if (server.argName(i) == "timeSlot")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.tx_timeslot = server.arg(i).toInt();
					}
				}
			}
			if (server.argName(i) == "preamble")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.preamble = server.arg(i).toInt();
					}
				}
			}
			if (server.argName(i) == "modem_type")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.modem_type = server.arg(i).toInt();
				}
			}
		}
		config.audio_hpf = hpf;
		config.audio_bpf = bpf;
		afskSetHPF(config.audio_hpf);
		afskSetBPF(config.audio_bpf);
		String html = "OK";
		server.send(200, "text/html", html); // send to someones browser when asked
		saveEEPROM();
	}
	else
	{
		String html = "<script type=\"text/javascript\">\n";
		html += "var sliderVol = document.getElementById(\"sliderVolume\");\n";
		html += "var outputVol = document.getElementById(\"volShow\");\n";
		html += "var sliderSql = document.getElementById(\"sliderSql\");\n";
		html += "var outputSql = document.getElementById(\"sqlShow\");\n";
		html += "outputVol.innerHTML = sliderVol.value;\n";
		html += "outputSql.innerHTML = sliderSql.value;\n";
		html += "\n";
		html += "sliderVol.oninput = function () {\n";
		html += "outputVol.innerHTML = this.value;\n";
		html += "}\n";
		html += "sliderSql.oninput = function () {\n";
		html += "outputSql.innerHTML = this.value;\n";
		html += "}\n";
		html += "\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formRadio\") document.getElementById(\"submitRadio\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formTNC\") document.getElementById(\"submitTNC\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/radio',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "function rfType(){\n";
		html += "var type = document.getElementById(\"rf_type\").value;\n";
		html += "if(type==1||type==4||type==7){document.getElementById(\"tx_freq\").setAttribute(\"max\",174);document.getElementById(\"rx_freq\").setAttribute(\"max\",174);};\n";
		html += "if(type==1){document.getElementById(\"tx_freq\").setAttribute(\"min\",134);document.getElementById(\"rx_freq\").setAttribute(\"min\",134);};\n";
		html += "if(type==4||type==7){document.getElementById(\"tx_freq\").setAttribute(\"min\",136);document.getElementById(\"rx_freq\").setAttribute(\"min\",136);};\n";
		html += "if(type==2||type==5||type==8){document.getElementById(\"tx_freq\").setAttribute(\"max\",470);document.getElementById(\"rx_freq\").setAttribute(\"max\",470);};\n";
		html += "if(type==2||type==5||type==8){document.getElementById(\"tx_freq\").setAttribute(\"min\",400);document.getElementById(\"rx_freq\").setAttribute(\"min\",400);};\n";
		html += "if(type==3){document.getElementById(\"tx_freq\").setAttribute(\"min\",320);document.getElementById(\"rx_freq\").setAttribute(\"min\",320);};\n";
		html += "if(type==3){document.getElementById(\"tx_freq\").setAttribute(\"max\",400);document.getElementById(\"rx_freq\").setAttribute(\"max\",400);};\n";
		html += "if(type==6){document.getElementById(\"tx_freq\").setAttribute(\"min\",350);document.getElementById(\"rx_freq\").setAttribute(\"min\",350);};\n";
		html += "if(type==6){document.getElementById(\"tx_freq\").setAttribute(\"max\",390);document.getElementById(\"rx_freq\").setAttribute(\"max\",390);};\n";
		html += "if(type==1||type==4||type==7){document.getElementById(\"tx_freq\").setAttribute(\"value\",144.390);document.getElementById(\"rx_freq\").setAttribute(\"value\",144.390);};\n";
		html += "if(type==2||type==5||type==8){document.getElementById(\"tx_freq\").setAttribute(\"value\",432.5);document.getElementById(\"rx_freq\").setAttribute(\"value\",432.5);};\n";
		html += "\n";
		html += "}\n";
		html += "</script>\n";
		html += "<form id='formRadio' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>RF Analog Module</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String radioEnFlag = "";
		if (config.rf_en)
			radioEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"radioEnable\" value=\"OK\" " + radioEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Module Type:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"rf_type\" id=\"rf_type\" onchange=\"rfType()\">\n";
		for (int i = 0; i < 9; i++)
		{
			if (config.rf_type == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(RF_TYPE[i]) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(RF_TYPE[i]) + "</option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		float freqMin = 0;
		float freqMax = 0;
		switch (config.rf_type)
		{
		case RF_SA868_VHF:
			freqMin = 134.0F;
			freqMax = 174.0F;
			break;
		case RF_SR_1WV:
		case RF_SR_2WVS:
			freqMin = 136.0F;
			freqMax = 174.0F;
			break;
		case RF_SA868_350:
			freqMin = 320.0F;
			freqMax = 400.0F;
			break;
		case RF_SR_1W350:
			freqMin = 350.0F;
			freqMax = 390.0F;
			break;
		case RF_SA868_UHF:
		case RF_SR_1WU:
		case RF_SR_2WUS:
			freqMin = 400.0F;
			freqMax = 470.0F;
			break;
		default:
			freqMin = 134.0F;
			freqMax = 500.0F;
			break;
		}
		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX Frequency:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"number\" id=\"tx_freq\" name=\"tx_freq\" min=\"" + String(freqMin, 4) + "\" max=\"" + String(freqMax, 4) + "\"\n";
		html += "step=\"0.0001\" value=\"" + String(config.freq_tx, 4) + "\" /> MHz</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>RX Frequency:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"number\" id=\"rx_freq\" name=\"rx_freq\" min=\"" + String(freqMin, 4) + "\" max=\"" + String(freqMax, 4) + "\"\n";
		html += "step=\"0.0001\" value=\"" + String(config.freq_rx, 4) + "\" /> Mhz</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX CTCSS:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"tx_ctcss\" id=\"tx_ctcss\">\n";
		for (int i = 0; i < 39; i++)
		{
			if (config.tone_tx == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(ctcss[i], 1) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(ctcss[i], 1) + "</option>\n";
		}
		html += "</select> Hz\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>RX CTCSS:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"rx_ctcss\" id=\"rx_ctcss\">\n";
		html += "<option value=\"0\" selected>0.0</option>\n";
		for (int i = 0; i < 39; i++)
		{
			if (config.tone_rx == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(ctcss[i], 1) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(ctcss[i], 1) + "</option>\n";
		}
		html += "</select> Hz\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Narrow/Wide:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"nw_band\" id=\"nw_band\">\n";
		String cmSelNWT = "";
		String cmSelNWF = "";
		if (config.band)
		{
			cmSelNWT = "selected";
		}
		else
		{
			cmSelNWF = "selected";
		}
		html += "<option value=\"1\" " + cmSelNWT + ">25.0KHz</option>\n";
		html += "<option value=\"0\" " + cmSelNWF + ">12.5KHz</option>\n";
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX Power:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"rf_power\" id=\"rf_power\">\n";
		String cmRfPwrF = "";
		String cmRfPwrT = "";
		if (config.rf_power)
		{
			cmRfPwrT = "selected";
		}
		else
		{
			cmRfPwrF = "selected";
		}
		html += "<option value=\"1\" " + cmRfPwrT + ">HIGH</option>\n";
		html += "<option value=\"0\" " + cmRfPwrF + ">LOW</option>\n";
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>VOLUME:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input id=\"sliderVolume\" name=\"volume\" type=\"range\"\n";
		html += "min=\"1\" max=\"8\" value=\"" + String(config.volume) + "\" /><b><span style=\"font-size: 14pt;\" id=\"volShow\">" + String(config.volume) + "</span></b></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>SQL Level:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input id=\"sliderSql\" name=\"sql_level\" type=\"range\"\n";
		html += "min=\"0\" max=\"8\" value=\"" + String(config.sql_level) + "\" /><b><span style=\"font-size: 14pt;\" id=\"sqlShow\">" + String(config.sql_level) + "</span></b></td>\n";
		html += "</tr>\n";
		html += "</table>\n";
		html += "<div class=\"form-group\">\n";
		html += "<label class=\"col-sm-4 col-xs-12 control-label\"></label>\n";
		html += "<div class=\"col-sm-2 col-xs-4\"><button type='submit' id='submitRadio' name=\"commitRadio\"> Apply Change </button></div>\n";
		html += "</div><br />\n";
		html += "<input type=\"hidden\" name=\"commitRadio\"/>\n";
		html += "</form>";

		// AFSK,TNC Configuration
		html += "<form id='formTNC' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>AFSK/TNC Configuration</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Modem Type:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"modem_type\" id=\"modem_type\" \">\n";
		for (int i = 0; i < 2; i++)
		{
			if (config.modem_type == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(MODEM_TYPE[i]) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(MODEM_TYPE[i]) + "</option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Audio HPF:</b></td>\n";
		String strFlag = "";
		if (config.audio_hpf)
			strFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"HPF\" value=\"OK\" " + strFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Audio high pass filter >1KHz cutoff 10Khz</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Audio BPF:</b></td>\n";
		strFlag = "";
		if (config.audio_bpf)
			strFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"BPF\" value=\"OK\" " + strFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Audio band pass filter 1Khz-2.5KHz</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX Time Slot:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"number\" name=\"timeSlot\" min=\"2000\" max=\"99999\"\n";
		html += "step=\"1000\" value=\"" + String(config.tx_timeslot) + "\" /> mSec.</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Preamble:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"preamble\">\n";
		for (int i = 1; i < 10; i++)
		{
			if (config.preamble == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(i * 100) + "</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(i * 100) + "</option>\n";
		}
		html += "</select> mSec.\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "</table>\n";
		html += "<div class=\"col-sm-2 col-xs-4\"><button type='submit' id='submitTNC'  name=\"commitTNC\"> Apply Change </button></div>\n";
		html += "<br />\n";
		html += "<input type=\"hidden\" name=\"commitTNC\"/>\n";
		html += "</form>";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_vpn()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("commitVPN"))
	{
		bool vpnEn = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "vpnEnable")
			{
				if (server.arg(i) != "")
				{
					// if (isValidNumber(server.arg(i)))
					if (String(server.arg(i)) == "OK")
						vpnEn = true;
				}
			}

			// if (server.argName(i) == "taretime") {
			//	if (server.arg(i) != "")
			//	{
			//		//if (isValidNumber(server.arg(i)))
			//		if (String(server.arg(i)) == "OK")
			//			taretime = true;
			//	}
			// }
			if (server.argName(i) == "wg_port")
			{
				if (server.arg(i) != "")
				{
					config.wg_port = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "wg_public_key")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_public_key, server.arg(i).c_str());
					config.wg_public_key[44] = 0;
				}
			}

			if (server.argName(i) == "wg_private_key")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_private_key, server.arg(i).c_str());
					config.wg_private_key[44] = 0;
				}
			}

			if (server.argName(i) == "wg_peer_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_peer_address, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "wg_local_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_local_address, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "wg_netmask_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_netmask_address, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "wg_gw_address")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wg_gw_address, server.arg(i).c_str());
				}
			}
		}

		config.vpn = vpnEn;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formVPN\") document.getElementById(\"submitVPN\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/vpn',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		// html += "<h2>System Setting</h2>\n";
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromVPN\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Wireguard Configuration</b></span></th>\n";
		html += "<tr>";

		String syncFlage = "";
		if (config.vpn)
			syncFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"vpnEnable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Address</b></td>\n";
		html += "<td style=\"text-align: left;\"><input  size=\"20\" id=\"wg_peer_address\" name=\"wg_peer_address\" type=\"text\" value=\"" + String(config.wg_peer_address) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Port</b></td>\n";
		html += "<td style=\"text-align: left;\"><input id=\"wg_port\" size=\"5\" name=\"wg_port\" type=\"number\" value=\"" + String(config.wg_port) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Local Address</b></td>\n";
		html += "<td style=\"text-align: left;\"><input id=\"wg_local_address\" name=\"wg_local_address\" type=\"text\" value=\"" + String(config.wg_local_address) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Netmask</b></td>\n";
		html += "<td style=\"text-align: left;\"><input id=\"wg_netmask_address\" name=\"wg_netmask_address\" type=\"text\" value=\"" + String(config.wg_netmask_address) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Gateway</b></td>\n";
		html += "<td style=\"text-align: left;\"><input id=\"wg_gw_address\" name=\"wg_gw_address\" type=\"text\" value=\"" + String(config.wg_gw_address) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Public Key</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"50\" maxlength=\"44\" id=\"wg_public_key\" name=\"wg_public_key\" type=\"text\" value=\"" + String(config.wg_public_key) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Private Key</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"50\" maxlength=\"44\" id=\"wg_private_key\" name=\"wg_private_key\" type=\"text\" value=\"" + String(config.wg_private_key) + "\" /></td>\n";
		html += "</tr>\n";

		html += "</table><br />\n";
		html += "<td><input class=\"btn btn-primary\" id=\"submitVPN\" name=\"commitVPN\" type=\"submit\" value=\"Save Config\" maxlength=\"80\"/></td>\n";
		html += "<input type=\"hidden\" name=\"commitVPN\"/>\n";
		html += "</form>\n";

		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_mod()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("commitGNSS"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					// if (isValidNumber(server.arg(i)))
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "atc")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.gnss_at_command,server.arg(i).c_str());
				}else{
					memset(config.gnss_at_command,0,sizeof(config.gnss_at_command));
				}
			}

			if (server.argName(i) == "Host")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.gnss_tcp_host,server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "Port")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.gnss_tcp_port = server.arg(i).toInt();
				}
			}
			
			if (server.argName(i) == "channel")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.gnss_channel = server.arg(i).toInt();
				}
			}
		}

		config.gnss_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitUART0"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "baudrate")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart0_baudrate = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rx")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart0_rx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart0_tx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rts")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart0_rts_gpio = server.arg(i).toInt();
				}
			}
		}

		config.uart0_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitUART1"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "baudrate")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart1_baudrate = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rx")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart1_rx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart1_tx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rts")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart1_rts_gpio = server.arg(i).toInt();
				}
			}
		}

		config.uart1_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitUART2"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "baudrate")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart2_baudrate = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rx")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart2_rx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart2_tx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rts")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.uart2_rts_gpio = server.arg(i).toInt();
				}
			}
		}

		config.uart2_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitMODBUS"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					// if (isValidNumber(server.arg(i)))
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "channel")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.modbus_channel = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "address")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.modbus_address = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "de")
			{
				if (server.arg(i) != "")
				{
					config.modbus_de_gpio = server.arg(i).toInt();
				}
			}
		}

		config.modbus_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitTNC"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					// if (isValidNumber(server.arg(i)))
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "channel")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.ext_tnc_channel = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "mode")
			{
				if (isValidNumber(server.arg(i)))
				{
					config.ext_tnc_mode = server.arg(i).toInt();
				}
			}
		}

		config.ext_tnc_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitONEWIRE"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					// if (isValidNumber(server.arg(i)))
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "data")
			{
				if (server.arg(i) != "")
				{
					config.onewire_gpio = server.arg(i).toInt();
				}
			}
		}

		config.onewire_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitRF"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "sql_active")
			{
				if (server.arg(i) != "")
				{
					config.rf_sql_active = (bool)server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "pd_active")
			{
				if (server.arg(i) != "")
				{
					config.rf_pd_active = (bool)server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "pwr_active")
			{
				if (server.arg(i) != "")
				{
					config.rf_pwr_active = (bool)server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "ptt_active")
			{
				if (server.arg(i) != "")
				{
					config.rf_ptt_active = (bool)server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "baudrate")
			{
				if (server.arg(i) != "")
				{
					config.rf_baudrate = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "rx")
			{
				if (server.arg(i) != "")
				{
					config.rf_rx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "tx")
			{
				if (server.arg(i) != "")
				{
					config.rf_tx_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "pd")
			{
				if (server.arg(i) != "")
				{
					config.rf_pd_gpio = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "pwr")
			{
				if (server.arg(i) != "")
				{
					config.rf_pwr_gpio = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "ptt")
			{
				if (server.arg(i) != "")
				{
					config.rf_ptt_gpio = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "sql")
			{
				if (server.arg(i) != "")
				{
					config.rf_sql_gpio = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "atten")
			{
				if (server.arg(i) != "")
				{
					config.adc_atten = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "offset")
			{
				if (server.arg(i) != "")
				{
					config.adc_dc_offset = server.arg(i).toInt();
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitI2C0"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "sda")
			{
				if (server.arg(i) != "")
				{
					config.i2c_sda_pin = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "sck")
			{
				if (server.arg(i) != "")
				{
					config.i2c_sck_pin = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "freq")
			{
				if (server.arg(i) != "")
				{
					config.i2c_freq = server.arg(i).toInt();
				}
			}
		}

		config.i2c_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitI2C1"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "sda")
			{
				if (server.arg(i) != "")
				{
					config.i2c1_sda_pin = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "sck")
			{
				if (server.arg(i) != "")
				{
					config.i2c1_sck_pin = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "freq")
			{
				if (server.arg(i) != "")
				{
					config.i2c1_freq = server.arg(i).toInt();
				}
			}
		}

		config.i2c1_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitCOUNTER0"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "gpio")
			{
				if (server.arg(i) != "")
				{
					config.counter0_gpio = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "active")
			{
				if (server.arg(i) != "")
				{
					config.counter0_active = (bool)server.arg(i).toInt();
				}
			}
		}

		config.counter0_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitCOUNTER1"))
	{
		bool En = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));

			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}

			if (server.argName(i) == "gpio")
			{
				if (server.arg(i) != "")
				{
					config.counter1_gpio = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "active")
			{
				if (server.arg(i) != "")
				{
					config.counter1_active = (bool)server.arg(i).toInt();
				}
			}
		}

		config.counter0_enable = En;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formUART0\") document.getElementById(\"submitURAT0\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formUART1\") document.getElementById(\"submitURAT1\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formUART1\") document.getElementById(\"submitURAT1\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formGNSS\") document.getElementById(\"submitGNSS\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formMODBUS\") document.getElementById(\"submitMODBUS\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formTNC\") document.getElementById(\"submitTNC\").disabled=true;\n";
		//html += "if(e.currentTarget.id===\"formONEWIRE\") document.getElementById(\"submitONEWIRE\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formRF\") document.getElementById(\"submitRF\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formI2C0\") document.getElementById(\"submitI2C0\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formI2C1\") document.getElementById(\"submitI2C1\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formCOUNT0\") document.getElementById(\"submitCOUNT0\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formCOUNT1\") document.getElementById(\"submitCOUNT1\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/mod',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\\nRequire hardware RESET!\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		html += "<table style=\"text-align:unset;border-width:0px;background:unset\"><tr style=\"background:unset;vertical-align:top\"><td width=\"32%\" style=\"border:unset;\">";
		// html += "<h2>System Setting</h2>\n";
		/**************UART0(USB) Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromUART0\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>UART0(USB) Modify</b></span></th>\n";
		html += "<tr>";

		String enFlage = "";
		if (config.uart0_enable)
			enFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + enFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"rx\" type=\"number\" value=\"" + String(config.uart0_rx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"tx\" type=\"number\" value=\"" + String(config.uart0_tx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RTS/DE GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"rts\" type=\"number\" value=\"" + String(config.uart0_rts_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Baudrate:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"baudrate\" id=\"baudrate\">\n";
		for (int i = 0; i < 13; i++)
		{
			if (config.uart0_baudrate == baudrate[i])
				html += "<option value=\"" + String(baudrate[i]) + "\" selected>" + String(baudrate[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(baudrate[i]) + "\" >" + String(baudrate[i]) + " </option>\n";
		}
		html += "</select> bps\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitUART0\" name=\"commitUART0\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitUART0\"/>\n";
		html += "</td></tr></table>\n";

		html += "</form><br />\n";
		html += "</td><td width=\"32%\" style=\"border:unset;\">";

		/**************UART1 Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromUART1\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>UART1 Modify</b></span></th>\n";
		html += "<tr>";

		enFlage = "";
		if (config.uart1_enable)
			enFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + enFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"rx\" type=\"number\" value=\"" + String(config.uart1_rx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"tx\" type=\"number\" value=\"" + String(config.uart1_tx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RTS/DE GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"rts\" type=\"number\" value=\"" + String(config.uart1_rts_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Baudrate:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"baudrate\" id=\"baudrate\">\n";
		for (int i = 0; i < 13; i++)
		{
			if (config.uart1_baudrate == baudrate[i])
				html += "<option value=\"" + String(baudrate[i]) + "\" selected>" + String(baudrate[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(baudrate[i]) + "\" >" + String(baudrate[i]) + " </option>\n";
		}
		html += "</select> bps\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitUART1\" name=\"commitUART1\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitUART1\"/>\n";
		html += "</td></tr></table>\n";

		html += "</form><br />\n";
		html += "</td><td width=\"32%\" style=\"border:unset;\">";

		/**************UART2 Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromUART2\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>UART2 Modify</b></span></th>\n";
		html += "<tr>";

		enFlage = "";
		if (config.uart2_enable)
			enFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + enFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"rx\" type=\"number\" value=\"" + String(config.uart2_rx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"tx\" type=\"number\" value=\"" + String(config.uart2_tx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RTS/DE GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"rts\" type=\"number\" value=\"" + String(config.uart2_rts_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Baudrate:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"baudrate\" id=\"baudrate\">\n";
		for (int i = 0; i < 13; i++)
		{
			if (config.uart2_baudrate == baudrate[i])
				html += "<option value=\"" + String(baudrate[i]) + "\" selected>" + String(baudrate[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(baudrate[i]) + "\" >" + String(baudrate[i]) + " </option>\n";
		}
		html += "</select> bps\n";
		html += "</td>\n";
		html += "</tr>\n";
		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitUART2\" name=\"commitUART2\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitUART2\"/>\n";
		html += "</td></tr></table>\n";

		html += "</form><br />\n";
		html += "</td></tr></table>\n";		

		// html += "</td><td width=\"32%\" style=\"border:unset;\">";

		/**************1-Wire Modify******************/
		// html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromONEWIRE\" method=\"post\">\n";
		// html += "<table>\n";
		// html += "<th colspan=\"2\"><span><b>1-Wire Bus Modify</b></span></th>\n";
		// html += "<tr>";

		// syncFlage = "";
		// if (config.onewire_enable)
		// 	syncFlage = "checked";
		// html += "<td align=\"right\"><b>Enable</b></td>\n";
		// html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></td>\n";
		// html += "</tr>\n";

		// html += "<tr>\n";
		// html += "<td align=\"right\"><b>GPIO:</b></td>\n";
		// html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"data\" type=\"number\" value=\"" + String(config.onewire_gpio) + "\" /></td>\n";
		// html += "</tr>\n";

		// html += "<tr><td colspan=\"2\" align=\"right\">\n";
		// html += "<input class=\"btn btn-primary\" id=\"submitONEWIRE\" name=\"commitONEWIRE\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		// html += "<input type=\"hidden\" name=\"commitONEWIRE\"/>\n";
		// html += "</td></tr></table>\n";
		// html += "</form><br />\n";

		// html += "</td></tr></table>\n";

		html += "<table style=\"text-align:unset;border-width:0px;background:unset\"><tr style=\"background:unset;vertical-align:top\"><td width=\"50%\" style=\"border:unset;vertical-align:top\">";
		/**************RF GPIO******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromRF\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>RF GPIO Modify</b></span></th>\n";
		html += "<tr>";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Baudrate:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"baudrate\" id=\"baudrate\">\n";
		for (int i = 0; i < 13; i++)
		{
			if (config.rf_baudrate == baudrate[i])
				html += "<option value=\"" + String(baudrate[i]) + "\" selected>" + String(baudrate[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(baudrate[i]) + "\" >" + String(baudrate[i]) + " </option>\n";
		}
		html += "</select> bps\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>ADC Attenuation:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"atten\" id=\"atten\">\n";
		for (int i = 0; i < 4; i++)
		{
			if (config.adc_atten == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(ADC_ATTEN[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(ADC_ATTEN[i]) + " </option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>ADC DC OFFSET:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"100\" max=\"2500\" name=\"offset\" type=\"number\" value=\"" + String(config.adc_dc_offset) + "\" /> mV     (Current: "+String(offset)+" mV)</td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>RX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"rx\" type=\"number\" value=\"" + String(config.rf_rx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>TX GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"tx\" type=\"number\" value=\"" + String(config.rf_tx_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		String LowFlag = "", HighFlag = "";
		LowFlag = "";
		HighFlag = "";
		if (config.rf_pd_active)
			HighFlag = "checked=\"checked\"";
		else
			LowFlag = "checked=\"checked\"";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PD GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"pd\" type=\"number\" value=\"" + String(config.rf_pd_gpio) + "\" /> Active:<input type=\"radio\" name=\"pd_active\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"pd_active\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
		html += "</tr>\n";

		LowFlag = "";
		HighFlag = "";
		if (config.rf_pwr_active)
			HighFlag = "checked=\"checked\"";
		else
			LowFlag = "checked=\"checked\"";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>H/L GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"pwr\" type=\"number\" value=\"" + String(config.rf_pwr_gpio) + "\" /> Active:<input type=\"radio\" name=\"pwr_active\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"pwr_active\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
		html += "</tr>\n";

		LowFlag = "";
		HighFlag = "";
		if (config.rf_sql_active)
			HighFlag = "checked=\"checked\"";
		else
			LowFlag = "checked=\"checked\"";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>SQL GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"sql\" type=\"number\" value=\"" + String(config.rf_sql_gpio) + "\" /> Active:<input type=\"radio\" name=\"sql_active\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"sql_active\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
		html += "</tr>\n";

		LowFlag = "";
		HighFlag = "";
		if (config.rf_ptt_active)
			HighFlag = "checked=\"checked\"";
		else
			LowFlag = "checked=\"checked\"";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PTT GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\"  name=\"ptt\" type=\"number\" value=\"" + String(config.rf_ptt_gpio) + "\" /> Active:<input type=\"radio\" name=\"ptt_active\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"ptt_active\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitRF\" name=\"commitRF\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitRF\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		html += "</td><td width=\"23%\" style=\"border:unset;\">";

		/**************I2C_0 Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromI2C0\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>I2C_0(OLED) Modify</b></span></th>\n";
		html += "<tr>";

		String syncFlage = "";
		if (config.i2c_enable)
			syncFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>SDA GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"sda\" type=\"number\" value=\"" + String(config.i2c_sda_pin) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>SCK GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"sck\" type=\"number\" value=\"" + String(config.i2c_sck_pin) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Frequency:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"1000\" max=\"800000\" name=\"freq\" type=\"number\" value=\"" + String(config.i2c_freq) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitI2C0\" name=\"commitI2C0\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitI2C0\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		/**************Counter_0 Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromCOUNTER0\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Counter_0 Modify</b></span></th>\n";
		html += "<tr>";

		syncFlage = "";
		if (config.counter0_enable)
			syncFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>INPUT GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"gpio\" type=\"number\" value=\"" + String(config.counter0_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		LowFlag = "";
		HighFlag = "";
		if (config.counter0_active)
			HighFlag = "checked=\"checked\"";
		else
			LowFlag = "checked=\"checked\"";
		html += "<tr>\n";
		html += "<td align=\"right\">Active</td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"radio\" name=\"active\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"active\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitCOUNTER0\" name=\"commitCOUNTER0\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitCOUNTER0\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		html += "</td><td width=\"23%\" style=\"border:unset;\">";
		/**************I2C_1 Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromI2C1\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>I2C_1 Modify</b></span></th>\n";
		html += "<tr>";

		syncFlage = "";
		if (config.i2c1_enable)
			syncFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>SDA GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"sda\" type=\"number\" value=\"" + String(config.i2c1_sda_pin) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>SCK GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"sck\" type=\"number\" value=\"" + String(config.i2c1_sck_pin) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Frequency:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"1000\" max=\"800000\" name=\"freq\" type=\"number\" value=\"" + String(config.i2c1_freq) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitI2C1\" name=\"commitI2C1\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitI2C1\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		/**************Counter_1 Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromCOUNTER1\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Counter_1 Modify</b></span></th>\n";
		html += "<tr>";

		syncFlage = "";
		if (config.counter1_enable)
			syncFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + syncFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>INPUT GPIO:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"gpio\" type=\"number\" value=\"" + String(config.counter1_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		LowFlag = "";
		HighFlag = "";
		if (config.counter1_active)
			HighFlag = "checked=\"checked\"";
		else
			LowFlag = "checked=\"checked\"";
		html += "<tr>\n";
		html += "<td align=\"right\">Active</td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"radio\" name=\"active\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"active\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitCOUNTER1\" name=\"commitCOUNTER1\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitCOUNTER1\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		html += "</td></tr></table>\n";

		//******************
		html += "<table style=\"text-align:unset;border-width:0px;background:unset\"><tr style=\"background:unset;vertical-align:top\"><td width=\"50%\" style=\"border:unset;vertical-align:top\">";
		/**************GNSS Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromGNSS\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>GNSS Modify</b></span></th>\n";
		html += "<tr>";

		enFlage = "";
		if (config.gnss_enable)
			enFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + enFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PORT:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"channel\" id=\"channel\">\n";
		for (int i = 0; i < 5; i++)
		{
			if (config.gnss_channel == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(GNSS_PORT[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(GNSS_PORT[i]) + " </option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "<td align=\"right\"><b>AT Command:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"30\" size=\"20\" id=\"atc\" name=\"atc\" type=\"text\" value=\"" + String(config.gnss_at_command) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<td align=\"right\"><b>TCP Host:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"20\" size=\"15\" id=\"Host\" name=\"Host\" type=\"text\" value=\"" + String(config.gnss_tcp_host) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>TCP Port:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"1024\" max=\"65535\"  id=\"Port\" name=\"Port\" type=\"number\" value=\"" + String(config.gnss_tcp_port) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitGNSS\" name=\"commitGNSS\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitGNSS\"/>\n";
		html += "</td></tr></table>\n";

		html += "</form><br />\n";

		html += "</td><td width=\"23%\" style=\"border:unset;\">";

		/**************MODBUS Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromMODBUS\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>MODBUS Modify</b></span></th>\n";
		html += "<tr>";

		enFlage = "";
		if (config.modbus_enable)
			enFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + enFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PORT:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"channel\" id=\"channel\">\n";
		for (int i = 0; i < 5; i++)
		{
			if (config.modbus_channel == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(GNSS_PORT[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(GNSS_PORT[i]) + " </option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Address:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"address\" type=\"number\" value=\"" + String(config.modbus_address) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>DE:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"-1\" max=\"39\" name=\"de\" type=\"number\" value=\"" + String(config.modbus_de_gpio) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitMODBUS\" name=\"commitMODBUS\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitMODBUS\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		html += "</td><td width=\"23%\" style=\"border:unset;\">";

		/**************External TNC Modify******************/
		html += "<form accept-charset=\"UTF-8\" action=\"#\" class=\"form-horizontal\" id=\"fromTNC\" method=\"post\">\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>External TNC Modify</b></span></th>\n";
		html += "<tr>";

		enFlage = "";
		if (config.ext_tnc_enable)
			enFlage = "checked";
		html += "<td align=\"right\"><b>Enable</b></td>\n";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + enFlage + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PORT:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"channel\" id=\"channel\">\n";
		for (int i = 0; i < 4; i++)
		{
			if (config.ext_tnc_channel == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(TNC_PORT[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(TNC_PORT[i]) + " </option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>MODE:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mode\" id=\"mode\">\n";
		for (int i = 0; i < 4; i++)
		{
			if (config.ext_tnc_mode == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(TNC_MODE[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(TNC_MODE[i]) + " </option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";


		html += "<tr><td colspan=\"2\" align=\"right\">\n";
		html += "<input class=\"btn btn-primary\" id=\"submitTNC\" name=\"commitTNC\" type=\"submit\" value=\"Apply\" maxlength=\"80\"/>\n";
		html += "<input type=\"hidden\" name=\"commitTNC\"/>\n";
		html += "</td></tr></table>\n";
		html += "</form>\n";

		html += "</td></tr></table>\n";

		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_system()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("updateTimeZone"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "SetTimeZone")
			{
				if (server.arg(i) != "")
				{
					config.timeZone = server.arg(i).toFloat();
					// Serial.println("WEB Config Time Zone);
					configTime(3600 * config.timeZone, 0, config.ntp_host);
				}
				break;
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("updateTimeNtp"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "SetTimeNtp")
			{
				if (server.arg(i) != "")
				{
					// Serial.println("WEB Config NTP");
					strcpy(config.ntp_host, server.arg(i).c_str());
					configTime(3600 * config.timeZone, 0, config.ntp_host);
				}
				break;
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("updateTime"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "SetTime")
			{
				if (server.arg(i) != "")
				{
					// struct tm tmn;
					String date = getValue(server.arg(i), ' ', 0);
					String time = getValue(server.arg(i), ' ', 1);
					int yyyy = getValue(date, '-', 0).toInt();
					int mm = getValue(date, '-', 1).toInt();
					int dd = getValue(date, '-', 2).toInt();
					int hh = getValue(time, ':', 0).toInt();
					int ii = getValue(time, ':', 1).toInt();
					int ss = getValue(time, ':', 2).toInt();
					// int ss = 0;

					tmElements_t timeinfo;
					timeinfo.Year = yyyy - 1970;
					timeinfo.Month = mm;
					timeinfo.Day = dd;
					timeinfo.Hour = hh;
					timeinfo.Minute = ii;
					timeinfo.Second = ss;
					time_t timeStamp = makeTime(timeinfo);

					// tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec

					time_t rtc = timeStamp - (config.timeZone * 3600);
					timeval tv = {rtc, 0};
					timezone tz = {(0) + DST_MN, 0};
					settimeofday(&tv, &tz);

					// Serial.println("Update TIME " + server.arg(i));
					Serial.print("Set New Time at ");
					Serial.print(dd);
					Serial.print("/");
					Serial.print(mm);
					Serial.print("/");
					Serial.print(yyyy);
					Serial.print(" ");
					Serial.print(hh);
					Serial.print(":");
					Serial.print(ii);
					Serial.print(":");
					Serial.print(ss);
					Serial.print(" ");
					Serial.println(timeStamp);
				}
				break;
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("REBOOT"))
	{
		esp_restart();
	}
	else if (server.hasArg("commitWebAuth"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "webauth_user")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.http_username, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "webauth_pass")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.http_password, server.arg(i).c_str());
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitPath"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "path1")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[0], server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "path2")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[1], server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "path3")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[1], server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "path4")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.path[3], server.arg(i).c_str());
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitDISP"))
	{
		bool dispRX = false;
		bool dispTX = false;
		bool dispRF = false;
		bool dispINET = false;
		bool oledEN = false;

		config.dispFilter = 0;

		for (uint8_t i = 0; i < server.args(); i++)
		{
			// Serial.print("SERVER ARGS ");
			// Serial.print(server.argName(i));
			// Serial.print("=");
			// Serial.println(server.arg(i));
			if (server.argName(i) == "oledEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						oledEN = true;
					}
				}
			}
			if (server.argName(i) == "filterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "filterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "filterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "filterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "filterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "filterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "filterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "filterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "filterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.dispFilter |= FILTER_POSITION;
				}
			}

			if (server.argName(i) == "dispRF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispRF = true;
				}
			}

			if (server.argName(i) == "dispINET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispINET = true;
				}
			}
			if (server.argName(i) == "txdispEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispTX = true;
				}
			}
			if (server.argName(i) == "rxdispEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						dispRX = true;
				}
			}

			if (server.argName(i) == "dispDelay")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.dispDelay = server.arg(i).toInt();
						if (config.dispDelay < 0)
							config.dispDelay = 0;
					}
				}
			}

			if (server.argName(i) == "oled_timeout")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.oled_timeout = server.arg(i).toInt();
						if (config.oled_timeout < 0)
							config.oled_timeout = 0;
					}
				}
			}
			if (server.argName(i) == "filterDX")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
					{
						config.filterDistant = server.arg(i).toInt();
					}
				}
			}
		}

		config.oled_enable = oledEN;
		config.dispINET = dispINET;
		config.dispRF = dispRF;
		config.rx_display = dispRX;
		config.tx_display = dispTX;
		// config.filterMessage = filterMessage;
		// config.filterStatus = filterStatus;
		// config.filterTelemetry = filterTelemetry;
		// config.filterWeather = filterWeather;
		// config.filterTracker = filterTracker;
		// config.filterMove = filterMove;
		// config.filterPosition = filterPosition;
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		struct tm tmstruct;
		char strTime[20];
		tmstruct.tm_year = 0;
		getLocalTime(&tmstruct, 5000);
		sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formTime\") document.getElementById(\"updateTime\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formNTP\") document.getElementById(\"updateTimeNtp\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formTimeZone\") document.getElementById(\"updateTimeZone\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formReboot\") document.getElementById(\"REBOOT\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formDisp\") document.getElementById(\"submitDISP\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formWebAuth\") document.getElementById(\"submitWebAuth\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formPath\") document.getElementById(\"submitPath\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/system',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		// html += "<h2>System Setting</h2>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>System Setting</b></span></th>\n";
		html += "<tr>";
		// html += "<form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTime\" method=\"post\">\n";
		html += "<td style=\"text-align: right;\">LOCAL<br/>DATE/TIME </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTime\" method=\"post\">\n<input name=\"SetTime\" type=\"text\" value=\"" + String(strTime) + "\" />\n";
		html += "<span class=\"input-group-addon\">\n<span class=\"glyphicon glyphicon-calendar\">\n</span></span>\n";
		// html += "<div class=\"col-sm-3 col-xs-6\"><button class=\"btn btn-primary\" data-args=\"[true]\" data-method=\"getDate\" type=\"button\" data-related-target=\"#SetTime\" />Get Date</button></div>\n";
		html += "<button type='submit' id='updateTime'  name=\"commit\"> Time Update </button>\n";
		html += "<input type=\"hidden\" name=\"updateTime\"/></form>\n</td>\n";
		// html += "<input class=\"btn btn-primary\" id=\"updateTime\" name=\"updateTime\" type=\"submit\" value=\"Time Update\" maxlength=\"80\"/></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">NTP Host </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formNTP\" method=\"post\"><input name=\"SetTimeNtp\" type=\"text\" value=\"" + String(config.ntp_host) + "\" />\n";
		html += "<button type='submit' id='updateTimeNtp'  name=\"commit\"> NTP Update </button>\n";
		html += "<input type=\"hidden\" name=\"updateTimeNtp\"/></form>\n</td>\n";
		// html += "<input class=\"btn btn-primary\" id=\"updateTimeNtp\" name=\"updateTimeNtp\" type=\"submit\" value=\"NTP Update\" maxlength=\"80\"/></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">Time Zone </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formTimeZone\" method=\"post\">\n";
		html += "<select name=\"SetTimeZone\" id=\"SetTimeZone\">\n";
		for (int i = 0; i < 40; i++)
		{
			if (config.timeZone == tzList[i].tz)
				html += "<option value=\"" + String(tzList[i].tz, 1) + "\" selected>" + String(tzList[i].name) + " Sec</option>\n";
			else
				html += "<option value=\"" + String(tzList[i].tz, 1) + "\" >" + String(tzList[i].name) + " Sec</option>\n";
		}
		html += "</select>";
		html += "<button type='submit' id='updateTimeZone'  name=\"commit\"> TZ Update </button>\n";
		html += "<input type=\"hidden\" name=\"updateTimeZone\"/></form>\n</td>\n";
		// html += "<input class=\"btn btn-primary\" id=\"updateTimeZone\" name=\"updateTimeZone\" type=\"submit\" value=\"TZ Update\" maxlength=\"80\"/></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\">SYSTEM REBOOT </td>\n";
		html += "<td style=\"text-align: left;\"><br /><form accept-charset=\"UTF-8\" action=\"#\" enctype='multipart/form-data' id=\"formReboot\" method=\"post\"> <button type='submit' id='REBOOT'  name=\"commit\" style=\"background-color:red;color:white\"> REBOOT </button>\n";
		html += " <input type=\"hidden\" name=\"REBOOT\"/></form>\n</td>\n";
		// html += "<td style=\"text-align: left;\"><input type='submit' class=\"btn btn-danger\" id=\"REBOOT\" name=\"REBOOT\" value='REBOOT'></td>\n";
		html += "</tr></table><br /><br />\n";

		/************************ WEB AUTH **************************/
		html += "<form id='formWebAuth' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Web Authentication</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Web USER:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"32\" maxlength=\"32\" class=\"form-control\" name=\"webauth_user\" type=\"text\" value=\"" + String(config.http_username) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Web PASSWORD:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"63\" maxlength=\"63\" class=\"form-control\" name=\"webauth_pass\" type=\"password\" value=\"" + String(config.http_password) + "\" /></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitWebAuth'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWebAuth\"/>\n";
		html += "</form><br /><br />";

		/************************ PATH USER define **************************/
		html += "<form id='formPath' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>PATH USER Define</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_1:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path1\" type=\"text\" value=\"" + String(config.path[0]) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_2:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path2\" type=\"text\" value=\"" + String(config.path[1]) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_3:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path3\" type=\"text\" value=\"" + String(config.path[2]) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH_4:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"72\" maxlength=\"72\" class=\"form-control\" name=\"path4\" type=\"text\" value=\"" + String(config.path[3]) + "\" /></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitPath'  name=\"commitPath\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitPath\"/>\n";
		html += "</form><br /><br />";
#ifdef OLED
		html += "<form id='formDisp' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>Display Setting</h2>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>Display Setting</b></span></th>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>OLED Enable</b></td>\n";
		String oledFlageEn = "";
		if (config.oled_enable == true)
			oledFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"oledEnable\" value=\"OK\" " + oledFlageEn + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>TX Display</b></td>\n";
		String txdispFlageEn = "";
		if (config.tx_display == true)
			txdispFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"txdispEnable\" value=\"OK\" " + txdispFlageEn + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*All TX Packet for display affter filter.</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>RX Display</b></td>\n";
		String rxdispFlageEn = "";
		if (config.rx_display == true)
			rxdispFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"rxdispEnable\" value=\"OK\" " + rxdispFlageEn + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*All RX Packet for display affter filter.</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>Head Up</b></td>\n";
		String hupFlageEn = "";
		if (config.h_up == true)
			hupFlageEn = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"hupEnable\" value=\"OK\" " + hupFlageEn + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*The compass will rotate in the direction of movement.</i></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>Popup Delay</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"dispDelay\" id=\"dispDelay\">\n";
		for (int i = 0; i < 16; i += 1)
		{
			if (config.dispDelay == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(i) + " Sec</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(i) + " Sec</option>\n";
		}
		html += "</select>\n";
		html += "</td></tr>\n";
		html += "<tr>\n";
		html += "<td style=\"text-align: right;\"><b>OLED Sleep</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"oled_timeout\" id=\"oled_timeout\">\n";
		for (int i = 0; i <= 600; i += 30)
		{
			if (config.oled_timeout == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(i) + " Sec</option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(i) + " Sec</option>\n";
		}
		html += "</select>\n";
		html += "</td></tr>\n";
		String rfFlageEn = "";
		if (config.dispRF == true)
			rfFlageEn = "checked";
		String inetFlageEn = "";
		if (config.dispINET == true)
			inetFlageEn = "checked";
		html += "<tr><td style=\"text-align: right;\"><b>RX Channel</b></td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"dispRF\" value=\"OK\" " + rfFlageEn + "/>RF <input type=\"checkbox\" name=\"dispINET\" value=\"OK\" " + inetFlageEn + "/>Internet </td></tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Filter DX:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input type=\"number\" name=\"filterDX\" min=\"0\" max=\"9999\"\n";
		html += "step=\"1\" value=\"" + String(config.filterDistant) + "\" /> Km.  <label style=\"vertical-align: bottom;font-size: 8pt;\"> <i>*Value 0 is all distant allow.</i></label></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		html += "<fieldset id=\"filterDispGrp\">\n";
		html += "<legend>Filter popup display</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		// html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"dispTNC\" name=\"dispTNC\" type=\"checkbox\" value=\"OK\" " + rfFlageEn + "/>From RF</td>\n";

		// html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"dispINET\" name=\"dispINET\" type=\"checkbox\" value=\"OK\" " + inetFlageEn + "/>From INET</td>\n";

		String filterMessageFlageEn = "";
		if (config.dispFilter & FILTER_MESSAGE)
			filterMessageFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterMessage\" name=\"filterMessage\" type=\"checkbox\" value=\"OK\" " + filterMessageFlageEn + "/>Message</td>\n";

		String filterStatusFlageEn = "";
		if (config.dispFilter & FILTER_STATUS)
			filterStatusFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterStatus\" name=\"filterStatus\" type=\"checkbox\" value=\"OK\" " + filterStatusFlageEn + "/>Status</td>\n";

		String filterTelemetryFlageEn = "";
		if (config.dispFilter & FILTER_TELEMETRY)
			filterTelemetryFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterTelemetry\" name=\"filterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterTelemetryFlageEn + "/>Telemetry</td>\n";

		String filterWeatherFlageEn = "";
		if (config.dispFilter & FILTER_WX)
			filterWeatherFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterWeather\" name=\"filterWeather\" type=\"checkbox\" value=\"OK\" " + filterWeatherFlageEn + "/>Weather</td>\n";

		String filterObjectFlageEn = "";
		if (config.dispFilter & FILTER_OBJECT)
			filterObjectFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterObject\" name=\"filterObject\" type=\"checkbox\" value=\"OK\" " + filterObjectFlageEn + "/>Object</td>\n";

		String filterItemFlageEn = "";
		if (config.dispFilter & FILTER_ITEM)
			filterItemFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterItem\" name=\"filterItem\" type=\"checkbox\" value=\"OK\" " + filterItemFlageEn + "/>Item</td>\n";

		String filterQueryFlageEn = "";
		if (config.dispFilter & FILTER_QUERY)
			filterQueryFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterQuery\" name=\"filterQuery\" type=\"checkbox\" value=\"OK\" " + filterQueryFlageEn + "/>Query</td>\n";

		String filterBuoyFlageEn = "";
		if (config.dispFilter & FILTER_BUOY)
			filterBuoyFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterBuoy\" name=\"filterBuoy\" type=\"checkbox\" value=\"OK\" " + filterBuoyFlageEn + "/>Buoy</td>\n";

		String filterPositionFlageEn = "";
		if (config.dispFilter & FILTER_POSITION)
			filterPositionFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" id=\"filterPosition\" name=\"filterPosition\" type=\"checkbox\" value=\"OK\" " + filterPositionFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";

		html += "</td></tr></table><br />\n";
		html += "<div><button type='submit' id='submitDISP'  name=\"commitDISP\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDISP\"/>\n";
		html += "</form><br />";
#endif
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_igate()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool aprsEn = false;
	bool rf2inetEn = false;
	bool inet2rfEn = false;
	bool posGPS = false;
	bool bcnEN = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool timeStamp = false;

	if (server.hasArg("commitIGATE"))
	{

		for (int i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "igateEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						aprsEn = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name = server.arg(i);
					name.trim();
					name.toUpperCase();
					strcpy(config.aprs_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "igateObject")
			{
				if (server.arg(i) != "")
				{
					String name = server.arg(i);
					name.trim();
					strcpy(config.igate_object, name.c_str());
				}
				else
				{
					memset(config.igate_object,0,sizeof(config.igate_object));
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.aprs_ssid = server.arg(i).toInt();
					if (config.aprs_ssid > 15)
						config.aprs_ssid = 13;
				}
			}
			if (server.argName(i) == "igatePosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "igatePosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "igatePosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "igatePosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "igatePosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}

			if (server.argName(i) == "igateTable")
			{
				if (server.arg(i) != "")
				{
					config.igate_symbol[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "igateSymbol")
			{
				if (server.arg(i) != "")
				{
					config.igate_symbol[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "aprsHost")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.aprs_host, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "aprsPort")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.aprs_port = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "aprsFilter")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.aprs_filter, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "igatePath")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.igate_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "igateComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.igate_comment, server.arg(i).c_str());
				}else{
					memset(config.igate_comment,0,sizeof(config.igate_comment));
				}
			}
			if (server.argName(i) == "texttouse")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.igate_phg, server.arg(i).c_str());
				}
			}

			if (server.argName(i) == "rf2inetEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						rf2inetEn = true;
				}
			}
			if (server.argName(i) == "inet2rfEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						inet2rfEn = true;
				}
			}
			if (server.argName(i) == "igatePos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "igatePos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "igateBcnEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						bcnEN = true;
				}
			}
			if (server.argName(i) == "igateTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}

		config.igate_en = aprsEn;
		config.rf2inet = rf2inetEn;
		config.inet2rf = inet2rfEn;
		config.igate_gps = posGPS;
		config.igate_bcn = bcnEN;
		config.igate_loc2rf = pos2RF;
		config.igate_loc2inet = pos2INET;
		config.igate_timestamp = timeStamp;

		saveEEPROM();
		initInterval = true;
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitIGATEfilter"))
	{
		config.rf2inetFilter = 0;
		config.inet2rfFilter = 0;
		for (int i = 0; i < server.args(); i++)
		{
			// config rf2inet filter
			if (server.argName(i) == "rf2inetFilterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "rf2inetFilterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "rf2inetFilterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "rf2inetFilterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "rf2inetFilterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "rf2inetFilterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "rf2inetFilterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "rf2inetFilterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "rf2inetFilterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.rf2inetFilter |= FILTER_POSITION;
				}
			}
			// config inet2rf filter

			if (server.argName(i) == "inet2rfFilterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "inet2rfFilterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "inet2rfFilterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "inet2rfFilterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "inet2rfFilterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "inet2rfFilterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "inet2rfFilterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "inet2rfFilterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "inet2rfFilterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.inet2rfFilter |= FILTER_POSITION;
				}
			}
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		// html += "document.getElementById(\"submitIGATE\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formIgate\") document.getElementById(\"submitIGATE\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formIgateFilter\") document.getElementById(\"submitIGATEfilter\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/igate',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n<script type=\"text/javascript\">\n";
		html += "function openWindowSymbol() {\n";
		html += "var i, l, options = [{\n";
		html += "value: 'first',\n";
		html += "text: 'First'\n";
		html += "}, {\n";
		html += "value: 'second',\n";
		html += "text: 'Second'\n";
		html += "}],\n";
		html += "newWindow = window.open(\"\", null, \"height=400,width=400,status=no,toolbar=no,menubar=no,location=no\");\n";

		int i;

		html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
		html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '/'</th></tr><tr>\");\n";
		for (i = 33; i < 129; i++)
		{
			html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(" + String(i) + ",1);\\\" src=\\\"http://aprs.dprns.com/symbols/icons/" + String(i) + "-1.png\\\"></td>\");\n";
			if (((i % 16) == 0) && (i < 126))
				html += "newWindow.document.write(\"</tr><tr>\");\n";
		}
		html += "newWindow.document.write(\"</tr></table><br />\");\n";
		html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
		html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '\\\'</th></tr><tr>\");\n";
		for (i = 33; i < 129; i++)
		{
			html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(" + String(i) + ",2);\\\" src=\\\"http://aprs.dprns.com/symbols/icons/" + String(i, DEC) + "-2.png\\\"></td>\");\n";
			if (((i % 16) == 0) && (i < 126))
				html += "newWindow.document.write(\"</tr><tr>\");\n";
		}
		html += "newWindow.document.write(\"</tr></table>\");\n";

		// html += "newWindow.document.write(\"</select>\");\");\n";
		html += "}\n";

		html += "function setValue(symbol,table) {\n";
		html += "document.getElementById('igateSymbol').value = String.fromCharCode(symbol);\n";
		html += "if(table==1){\n document.getElementById('igateTable').value='/';\n";
		html += "}else if(table==2){\n document.getElementById('igateTable').value='\\\\';\n}\n";
		html += "document.getElementById('igateImgSymbol').src = \"http://aprs.dprns.com/symbols/icons/\"+symbol.toString()+'-'+table.toString()+'.png';\n";
		html += "\n}\n";
		html += "function calculatePHGR(){document.forms.formIgate.texttouse.value=\"PHG\"+calcPower(document.forms.formIgate.power.value)+calcHeight(document.forms.formIgate.haat.value)+calcGain(document.forms.formIgate.gain.value)+calcDirection(document.forms.formIgate.direction.selectedIndex)}function Log2(e){return Math.log(e)/Math.log(2)}function calcPerHour(e){return e<10?e:String.fromCharCode(65+(e-10))}function calcHeight(e){return String.fromCharCode(48+Math.round(Log2(e/10),0))}function calcPower(e){if(e<1)return 0;if(e>=1&&e<4)return 1;if(e>=4&&e<9)return 2;if(e>=9&&e<16)return 3;if(e>=16&&e<25)return 4;if(e>=25&&e<36)return 5;if(e>=36&&e<49)return 6;if(e>=49&&e<64)return 7;if(e>=64&&e<81)return 8;if(e>=81)return 9}function calcDirection(e){if(e==\"0\")return\"0\";if(e==\"1\")return\"1\";if(e==\"2\")return\"2\";if(e==\"3\")return\"3\";if(e==\"4\")return\"4\";if(e==\"5\")return\"5\";if(e==\"6\")return\"6\";if(e==\"7\")return\"7\";if(e==\"8\")return\"8\"}function calcGain(e){return e>9?\"9\":e<0?\"0\":Math.round(e,0)}\n";
		html += "function onRF2INETCheck() {\n";
		html += "if (document.querySelector('#rf2inetEnable').checked) {\n";
		// Checkbox has been checked
		html += "document.getElementById(\"rf2inetFilterGrp\").disabled=false;\n";
		html += "} else {\n";
		// Checkbox has been unchecked
		html += "document.getElementById(\"rf2inetFilterGrp\").disabled=true;\n";
		html += "}\n}\n";
		html += "function onINET2RFCheck() {\n";
		html += "if (document.querySelector('#inet2rfEnable').checked) {\n";
		// Checkbox has been checked
		html += "document.getElementById(\"inet2rfFilterGrp\").disabled=false;\n";
		html += "} else {\n";
		// Checkbox has been unchecked
		html += "document.getElementById(\"inet2rfFilterGrp\").disabled=true;\n";
		html += "}\n}\n";
		html += "</script>\n";

		/************************ IGATE Mode **************************/
		html += "<form id='formIgate' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>[IGATE] Internet Gateway Mode</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>[IGATE] Internet Gateway Mode</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String igateEnFlag = "";
		if (config.igate_en)
			igateEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateEnable\" value=\"OK\" " + igateEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.aprs_mycall) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mySSID\" id=\"mySSID\">\n";
		for (uint8_t ssid = 0; ssid <= 15; ssid++)
		{
			if (config.aprs_ssid == ssid)
			{
				html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Symbol:</b></td>\n";
		String table = "1";
		if (config.igate_symbol[0] == 47)
			table = "1";
		if (config.igate_symbol[0] == 92)
			table = "2";
		html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"igateTable\" name=\"igateTable\" type=\"text\" value=\"" + String(config.igate_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"igateSymbol\" name=\"igateSymbol\" type=\"text\" value=\"" + String(config.igate_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"igateImgSymbol\" onclick=\"openWindowSymbol();\" src=\"http://aprs.dprns.com/symbols/icons/" + String((int)config.igate_symbol[1]) + "-" + table + ".png\"> <i>*Click icon for select symbol</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Item/Obj Name:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"9\" size=\"9\" id=\"igateObject\" name=\"igateObject\" type=\"text\" value=\"" + String(config.igate_object) + "\" /><i> *If not used, leave it blank.In use 3-9 charactor</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"igatePath\" id=\"igatePath\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			if (config.igate_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		// html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" id=\"igatePath\" name=\"igatePath\" type=\"text\" value=\"" + String(config.igate_path) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Host:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"20\" size=\"20\" id=\"aprsHost\" name=\"aprsHost\" type=\"text\" value=\"" + String(config.aprs_host) + "\" /> *Support APRS-IS of T2THAI at <a href=\"http://aprs.dprns.com:14501\" target=\"_t2thai\">aprs.dprns.com:14580</a></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Port:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input min=\"1\" max=\"65535\" step=\"1\" id=\"aprsPort\" name=\"aprsPort\" type=\"number\" value=\"" + String(config.aprs_port) + "\" /> *Support AMPR Host at <a href=\"http://aprs.hs5tqa.ampr.org:14501\" target=\"_t2thai\">aprs.hs5tqa.ampr.org:14580</a></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Server Filter:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"30\" size=\"30\" id=\"aprsFilter\" name=\"aprsFilter\" type=\"text\" value=\"" + String(config.aprs_filter) + "\" /> *Filter: <a target=\"_blank\" href=\"http://www.aprs-is.net/javAPRSFilter.aspx\">http://www.aprs-is.net/javAPRSFilter.aspx</a></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" id=\"igateComment\" name=\"igateComment\" type=\"text\" value=\"" + String(config.igate_comment) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>RF2INET:</b></td>\n";
		String rf2inetEnFlag = "";
		if (config.rf2inet)
			rf2inetEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"rf2inetEnable\" name=\"rf2inetEnable\" onclick=\"onRF2INETCheck()\" value=\"OK\" " + rf2inetEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch RF to Internet gateway</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>INET2RF:</b></td>\n";
		String inet2rfEnFlag = "";
		if (config.inet2rf)
			inet2rfEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"inet2rfEnable\" name=\"inet2rfEnable\" onclick=\"onINET2RFCheck()\" value=\"OK\" " + inet2rfEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch Internet to RF gateway</i></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
		String timeStampFlag = "";
		if (config.igate_timestamp)
			timeStampFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n<tr>";

		html += "<td align=\"right\"><b>POSITION:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		String igateBcnEnFlag = "";
		if (config.igate_bcn)
			igateBcnEnFlag = "checked";

		html += "<tr><td style=\"text-align: right;\">Beacon:</td><td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"igateBcnEnable\" value=\"OK\" " + igateBcnEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\">  Interval:<input min=\"0\" max=\"3600\" step=\"1\" id=\"igatePosInv\" name=\"igatePosInv\" type=\"number\" value=\"" + String(config.igate_interval) + "\" />Sec.</label></td></tr>";
		String igatePosFixFlag = "";
		String igatePosGPSFlag = "";
		String igatePos2RFFlag = "";
		String igatePos2INETFlag = "";
		if (config.igate_gps)
			igatePosGPSFlag = "checked=\"checked\"";
		else
			igatePosFixFlag = "checked=\"checked\"";

		if (config.igate_loc2rf)
			igatePos2RFFlag = "checked";
		if (config.igate_loc2inet)
			igatePos2INETFlag = "checked";
		html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"igatePosSel\" value=\"0\" " + igatePosFixFlag + "/>Fix <input type=\"radio\" name=\"igatePosSel\" value=\"1\" " + igatePosGPSFlag + "/>GPS </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">TX Channel:</td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"igatePos2RF\" value=\"OK\" " + igatePos2RFFlag + "/>RF <input type=\"checkbox\" name=\"igatePos2INET\" value=\"OK\" " + igatePos2INETFlag + "/>Internet </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"igatePosLat\" name=\"igatePosLat\" type=\"number\" value=\"" + String(config.igate_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"igatePosLon\" name=\"igatePosLon\" type=\"number\" value=\"" + String(config.igate_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"igatePosAlt\" name=\"igatePosAlt\" type=\"number\" value=\"" + String(config.igate_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
		html += "</table></td>";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PHG:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		html += "<tr>\n";
		html += "<td align=\"right\">Radio TX Power</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"power\" id=\"power\">\n";
		html += "<option value=\"1\" selected>1</option>\n";
		html += "<option value=\"5\">5</option>\n";
		html += "<option value=\"10\">10</option>\n";
		html += "<option value=\"15\">15</option>\n";
		html += "<option value=\"25\">25</option>\n";
		html += "<option value=\"35\">35</option>\n";
		html += "<option value=\"50\">50</option>\n";
		html += "<option value=\"65\">65</option>\n";
		html += "<option value=\"80\">80</option>\n";
		html += "</select> Watts</td>\n";
		html += "</tr>\n";
		html += "<tr><td style=\"text-align: right;\">Antenna Gain</td><td style=\"text-align: left;\"><input size=\"3\" min=\"0\" max=\"100\" step=\"0.1\" id=\"gain\" name=\"gain\" type=\"number\" value=\"6\" /> dBi</td></tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\">Height</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"haat\" id=\"haat\">\n";
		int k = 10;
		for (uint8_t w = 0; w < 10; w++)
		{
			if (w == 0)
			{
				html += "<option value=\"" + String(k) + "\" selected>" + String(k) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(k) + "\">" + String(k) + "</option>\n";
			}
			k += k;
		}
		html += "</select> Feet</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\">Antenna/Direction</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"direction\" id=\"direction\">\n";
		html += "<option>Omni</option><option>NE</option><option>E</option><option>SE</option><option>S</option><option>SW</option><option>W</option><option>NW</option><option>N</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";

		html += "<tr><td align=\"right\"><b>PHG Text</b></td><td align=\"left\"><input name=\"texttouse\" type=\"text\" size=\"6\" style=\"background-color: rgb(97, 239, 170);\" value=\"" + String(config.igate_phg) + "\"/> <input type=\"button\" value=\"Calculate PHG\" onclick=\"javascript:calculatePHGR()\" /></td></tr>\n";
		html += "</table></td>";
		html += "</tr>\n";

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitIGATE'  name=\"commitIGATE\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitIGATE\"/>\n";
		html += "</form><br /><br />";

		html += "<form id='formIgateFilter' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>[IGATE] Filter</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>RF2INET Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		if (config.rf2inet)
			html += "<fieldset id=\"rf2inetFilterGrp\">\n";
		else
			html += "<fieldset id=\"rf2inetFilterGrp\" disabled>\n";
		html += "<legend>Filter RF to Internet</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		String filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Message</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Status</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Telemetry</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Weather</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Object</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Item</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Query</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Buoy</td>\n";

		filterFlageEn = "";
		if (config.rf2inetFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"rf2inetFilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";
		html += "</td></tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>INET2RF Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		if (config.inet2rf)
			html += "<fieldset id=\"inet2rfFilterGrp\">\n";
		else
			html += "<fieldset id=\"inet2rfFilterGrp\" disabled>\n";
		html += "<legend>Filter Internet to RF</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Message</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Status</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Telemetry</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Weather</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Object</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Item</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Query</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Buoy</td>\n";

		filterFlageEn = "";
		if (config.inet2rfFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"inet2rfFilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";
		html += "</td></tr>\n";

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitIGATEfilter'  name=\"commitIGATEfilter\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitIGATEfilter\"/>\n";
		html += "</form><br />";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_digi()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool digiEn = false;
	bool posGPS = false;
	bool bcnEN = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool timeStamp = false;

	if (server.hasArg("commitDIGI"))
	{
		config.digiFilter = 0;
		for (int i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "digiEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						digiEn = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name = server.arg(i);
					name.trim();
					name.toUpperCase();
					strcpy(config.digi_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_ssid = server.arg(i).toInt();
					if (config.digi_ssid > 15)
						config.digi_ssid = 3;
				}
			}
			if (server.argName(i) == "digiDelay")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_delay = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "digiPosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "digiPosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "digiPosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "digiPosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "digiPosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}

			if (server.argName(i) == "digiTable")
			{
				if (server.arg(i) != "")
				{
					config.digi_symbol[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "digiSymbol")
			{
				if (server.arg(i) != "")
				{
					config.digi_symbol[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "digiPath")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.digi_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "digiComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.digi_comment, server.arg(i).c_str());
				}else{
					memset(config.digi_comment,0,sizeof(config.digi_comment));
				}
			}
			if (server.argName(i) == "texttouse")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.digi_phg, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "digiComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.digi_comment, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "digiPos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "digiPos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "digiBcnEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						bcnEN = true;
				}
			}
			// Filter
			if (server.argName(i) == "FilterMessage")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_MESSAGE;
				}
			}

			if (server.argName(i) == "FilterTelemetry")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_TELEMETRY;
				}
			}

			if (server.argName(i) == "FilterStatus")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_STATUS;
				}
			}

			if (server.argName(i) == "FilterWeather")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_WX;
				}
			}

			if (server.argName(i) == "FilterObject")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_OBJECT;
				}
			}

			if (server.argName(i) == "FilterItem")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_ITEM;
				}
			}

			if (server.argName(i) == "FilterQuery")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_QUERY;
				}
			}
			if (server.argName(i) == "FilterBuoy")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_BUOY;
				}
			}
			if (server.argName(i) == "FilterPosition")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						config.digiFilter |= FILTER_POSITION;
				}
			}
			if (server.argName(i) == "digiTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}
		config.digi_en = digiEn;
		config.digi_gps = posGPS;
		config.digi_bcn = bcnEN;
		config.digi_loc2rf = pos2RF;
		config.digi_loc2inet = pos2INET;
		config.digi_timestamp = timeStamp;

		saveEEPROM();
		initInterval = true;
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "document.getElementById(\"submitDIGI\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/digi',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n<script type=\"text/javascript\">\n";
		html += "function openWindowSymbol() {\n";
		html += "var i, l, options = [{\n";
		html += "value: 'first',\n";
		html += "text: 'First'\n";
		html += "}, {\n";
		html += "value: 'second',\n";
		html += "text: 'Second'\n";
		html += "}],\n";
		html += "newWindow = window.open(\"\", null, \"height=400,width=400,status=no,toolbar=no,menubar=no,titlebar=no,location=no\");\n";

		int i;

		html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
		html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '/'</th></tr><tr>\");\n";
		for (i = 33; i < 129; i++)
		{
			html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(" + String(i) + ",1);\\\" src=\\\"http://aprs.dprns.com/symbols/icons/" + String(i) + "-1.png\\\"></td>\");\n";
			if (((i % 16) == 0) && (i < 126))
				html += "newWindow.document.write(\"</tr><tr>\");\n";
		}
		html += "newWindow.document.write(\"</tr></table><br />\");\n";
		html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
		html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '\\\'</th></tr><tr>\");\n";
		for (i = 33; i < 129; i++)
		{
			html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(" + String(i) + ",2);\\\" src=\\\"http://aprs.dprns.com/symbols/icons/" + String(i, DEC) + "-2.png\\\"></td>\");\n";
			if (((i % 16) == 0) && (i < 126))
				html += "newWindow.document.write(\"</tr><tr>\");\n";
		}
		html += "newWindow.document.write(\"</tr></table>\");\n";
		html += "}\n";

		html += "function setValue(symbol,table) {\n";
		html += "document.getElementById('digiSymbol').value = String.fromCharCode(symbol);\n";
		html += "if(table==1){\n document.getElementById('digiTable').value='/';\n";
		html += "}else if(table==2){\n document.getElementById('digiTable').value='\\\\';\n}\n";
		html += "document.getElementById('digiImgSymbol').src = \"http://aprs.dprns.com/symbols/icons/\"+symbol.toString()+'-'+table.toString()+'.png';\n";
		html += "\n}\n";
		html += "function calculatePHGR(){document.forms.formDIGI.texttouse.value=\"PHG\"+calcPower(document.forms.formDIGI.power.value)+calcHeight(document.forms.formDIGI.haat.value)+calcGain(document.forms.formDIGI.gain.value)+calcDirection(document.forms.formDIGI.direction.selectedIndex)}function Log2(e){return Math.log(e)/Math.log(2)}function calcPerHour(e){return e<10?e:String.fromCharCode(65+(e-10))}function calcHeight(e){return String.fromCharCode(48+Math.round(Log2(e/10),0))}function calcPower(e){if(e<1)return 0;if(e>=1&&e<4)return 1;if(e>=4&&e<9)return 2;if(e>=9&&e<16)return 3;if(e>=16&&e<25)return 4;if(e>=25&&e<36)return 5;if(e>=36&&e<49)return 6;if(e>=49&&e<64)return 7;if(e>=64&&e<81)return 8;if(e>=81)return 9}function calcDirection(e){if(e==\"0\")return\"0\";if(e==\"1\")return\"1\";if(e==\"2\")return\"2\";if(e==\"3\")return\"3\";if(e==\"4\")return\"4\";if(e==\"5\")return\"5\";if(e==\"6\")return\"6\";if(e==\"7\")return\"7\";if(e==\"8\")return\"8\"}function calcGain(e){return e>9?\"9\":e<0?\"0\":Math.round(e,0)}\n";
		html += "</script>\n";

		/************************ DIGI Mode **************************/
		html += "<form id='formDIGI' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>[DIGI] Digital Repeater Mode</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>[DIGI] Dital Repeater Mode</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String digiEnFlag = "";
		if (config.digi_en)
			digiEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiEnable\" value=\"OK\" " + digiEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.digi_mycall) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mySSID\" id=\"mySSID\">\n";
		for (uint8_t ssid = 0; ssid <= 15; ssid++)
		{
			if (config.digi_ssid == ssid)
			{
				html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Symbol:</b></td>\n";
		String table = "1";
		if (config.digi_symbol[0] == 47)
			table = "1";
		if (config.digi_symbol[0] == 92)
			table = "2";
		html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"digiTable\" name=\"digiTable\" type=\"text\" value=\"" + String(config.digi_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"digiSymbol\" name=\"digiSymbol\" type=\"text\" value=\"" + String(config.digi_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"digiImgSymbol\" onclick=\"openWindowSymbol();\" src=\"http://aprs.dprns.com/symbols/icons/" + String((int)config.digi_symbol[1]) + "-" + table + ".png\"> <i>*Click icon for select symbol</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"digiPath\" id=\"digiPath\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			if (config.digi_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		// html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" id=\"digiPath\" name=\"digiPath\" type=\"text\" value=\"" + String(config.digi_path) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" id=\"digiComment\" name=\"digiComment\" type=\"text\" value=\"" + String(config.digi_comment) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td style=\"text-align: right;\"><b>Repeat Delay:</b></td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"100\" id=\"digiDelay\" name=\"digiDelay\" type=\"number\" value=\"" + String(config.digi_delay) + "\" /> mSec. <i>*0 is auto,Other random of delay time</i></td></tr>";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
		String timeStampFlag = "";
		if (config.digi_timestamp)
			timeStampFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr><td align=\"right\"><b>POSITION:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		String digiBcnEnFlag = "";
		if (config.digi_bcn)
			digiBcnEnFlag = "checked";

		html += "<tr><td style=\"text-align: right;\">Beacon:</td><td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"digiBcnEnable\" value=\"OK\" " + digiBcnEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\">  Interval:<input min=\"0\" max=\"3600\" step=\"1\" id=\"digiPosInv\" name=\"digiPosInv\" type=\"number\" value=\"" + String(config.digi_interval) + "\" />Sec.</label></td></tr>";
		String digiPosFixFlag = "";
		String digiPosGPSFlag = "";
		String digiPos2RFFlag = "";
		String digiPos2INETFlag = "";
		if (config.digi_gps)
			digiPosGPSFlag = "checked=\"checked\"";
		else
			digiPosFixFlag = "checked=\"checked\"";

		if (config.digi_loc2rf)
			digiPos2RFFlag = "checked";
		if (config.digi_loc2inet)
			digiPos2INETFlag = "checked";
		html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"digiPosSel\" value=\"0\" " + digiPosFixFlag + "/>Fix <input type=\"radio\" name=\"digiPosSel\" value=\"1\" " + digiPosGPSFlag + "/>GPS </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">TX Channel:</td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"digiPos2RF\" value=\"OK\" " + digiPos2RFFlag + "/>RF <input type=\"checkbox\" name=\"digiPos2INET\" value=\"OK\" " + digiPos2INETFlag + "/>Internet </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"digiPosLat\" name=\"digiPosLat\" type=\"number\" value=\"" + String(config.digi_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"digiPosLon\" name=\"digiPosLon\" type=\"number\" value=\"" + String(config.digi_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"digiPosAlt\" name=\"digiPosAlt\" type=\"number\" value=\"" + String(config.digi_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
		html += "</table></td>";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PHG:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		html += "<tr>\n";
		html += "<td align=\"right\">Radio TX Power</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"power\" id=\"power\">\n";
		html += "<option value=\"1\" selected>1</option>\n";
		html += "<option value=\"5\">5</option>\n";
		html += "<option value=\"10\">10</option>\n";
		html += "<option value=\"15\">15</option>\n";
		html += "<option value=\"25\">25</option>\n";
		html += "<option value=\"35\">35</option>\n";
		html += "<option value=\"50\">50</option>\n";
		html += "<option value=\"65\">65</option>\n";
		html += "<option value=\"80\">80</option>\n";
		html += "</select> Watts</td>\n";
		html += "</tr>\n";
		html += "<tr><td style=\"text-align: right;\">Antenna Gain</td><td style=\"text-align: left;\"><input size=\"3\" min=\"0\" max=\"100\" step=\"0.1\" id=\"gain\" name=\"gain\" type=\"number\" value=\"6\" /> dBi</td></tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\">Height</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"haat\" id=\"haat\">\n";
		int k = 10;
		for (uint8_t w = 0; w < 10; w++)
		{
			if (w == 0)
			{
				html += "<option value=\"" + String(k) + "\" selected>" + String(k) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(k) + "\">" + String(k) + "</option>\n";
			}
			k += k;
		}
		html += "</select> Feet</td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\">Antenna/Direction</td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"direction\" id=\"direction\">\n";
		html += "<option>Omni</option><option>NE</option><option>E</option><option>SE</option><option>S</option><option>SW</option><option>W</option><option>NW</option><option>N</option>\n";
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr><td align=\"right\"><b>PHG Text</b></td><td align=\"left\"><input name=\"texttouse\" type=\"text\" size=\"6\" style=\"background-color: rgb(97, 239, 170);\" value=\"" + String(config.digi_phg) + "\"/> <input type=\"button\" value=\"Calculate PHG\" onclick=\"javascript:calculatePHGR()\" /></td></tr>\n";

		html += "</table></tr>";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Filter:</b></td>\n";

		html += "<td align=\"center\">\n";
		html += "<fieldset id=\"FilterGrp\">\n";
		html += "<legend>Filter repeater</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
		html += "<tr style=\"background:unset;\">";

		String filterFlageEn = "";
		if (config.digiFilter & FILTER_MESSAGE)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterMessage\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Message</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_STATUS)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterStatus\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Status</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_TELEMETRY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterTelemetry\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Telemetry</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_WX)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterWeather\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Weather</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_OBJECT)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterObject\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Object</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_ITEM)
			filterFlageEn = "checked";
		html += "</tr><tr style=\"background:unset;\"><td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterItem\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Item</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_QUERY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterQuery\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Query</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_BUOY)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterBuoy\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Buoy</td>\n";

		filterFlageEn = "";
		if (config.digiFilter & FILTER_POSITION)
			filterFlageEn = "checked";
		html += "<td style=\"border:unset;\"><input class=\"field_checkbox\" name=\"FilterPosition\" type=\"checkbox\" value=\"OK\" " + filterFlageEn + "/>Position</td>\n";

		html += "<td style=\"border:unset;\"></td>";
		html += "</tr></table></fieldset>\n";
		html += "</td></tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitDIGI'  name=\"commitDIGI\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitDIGI\"/>\n";
		html += "</form><br />";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_wx()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool En = false;
	bool posGPS = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool timeStamp = false;

	if (server.hasArg("commitWX"))
	{
		for (int i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}
			if (server.argName(i) == "Object")
			{
				if (server.arg(i) != "")
				{
					String name = server.arg(i);
					name.trim();
					strcpy(config.wx_object, name.c_str());
				}
				else
				{
					config.wx_object[0] = 0;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name = server.arg(i);
					name.trim();
					name.toUpperCase();
					strcpy(config.wx_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wx_ssid = server.arg(i).toInt();
					if (config.wx_ssid > 15)
						config.wx_ssid = 3;
				}
			}
			if (server.argName(i) == "PosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wx_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "PosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wx_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "PosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wx_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "PosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wx_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "PosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}

			if (server.argName(i) == "Path")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wx_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "Comment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wx_comment, server.arg(i).c_str());
				}else{
					memset(config.wx_comment,0,sizeof(config.wx_comment));
				}
			}
			if (server.argName(i) == "Pos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "Pos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "wxTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}
		config.wx_en = En;
		config.wx_gps = posGPS;
		config.wx_2rf = pos2RF;
		config.wx_2inet = pos2INET;
		config.wx_timestamp = timeStamp;

		saveEEPROM();
		initInterval = true;
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "document.getElementById(\"submitWX\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/wx',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		/************************ WX Mode **************************/
		html += "<form id='formWX' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>[WX] Weather Station</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String EnFlag = "";
		if (config.wx_en)
			EnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + EnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.wx_mycall) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mySSID\" id=\"mySSID\">\n";
		for (uint8_t ssid = 0; ssid <= 15; ssid++)
		{
			if (config.wx_ssid == ssid)
			{
				html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Object Name:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"9\" size=\"9\" name=\"Object\" type=\"text\" value=\"" + String(config.wx_object) + "\" /><i> *If not used, leave it blank.In use 3-9 charactor</i></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"Path\" id=\"Path\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			if (config.wx_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		// html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" name=\"Path\" type=\"text\" value=\"" + String(config.wx_path) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" name=\"Comment\" type=\"text\" value=\"" + String(config.wx_comment) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
		String timeStampFlag = "";
		if (config.wx_timestamp)
			timeStampFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wxTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";

		html += "<tr><td align=\"right\"><b>POSITION:</b></td>\n";
		html += "<td align=\"center\">\n";
		html += "<table>";
		html += "<tr><td style=\"text-align: right;\">Interval:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"3600\" step=\"1\" id=\"PosInv\" name=\"PosInv\" type=\"number\" value=\"" + String(config.wx_interval) + "\" />Sec.</td></tr>";
		String PosFixFlag = "";
		String PosGPSFlag = "";
		String Pos2RFFlag = "";
		String Pos2INETFlag = "";
		if (config.wx_gps)
			PosGPSFlag = "checked=\"checked\"";
		else
			PosFixFlag = "checked=\"checked\"";

		if (config.wx_2rf)
			Pos2RFFlag = "checked";
		if (config.wx_2inet)
			Pos2INETFlag = "checked";
		html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"PosSel\" value=\"0\" " + PosFixFlag + "/>Fix <input type=\"radio\" name=\"PosSel\" value=\"1\" " + PosGPSFlag + "/>GPS </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">TX Channel:</td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"Pos2RF\" value=\"OK\" " + Pos2RFFlag + "/>RF <input type=\"checkbox\" name=\"Pos2INET\" value=\"OK\" " + Pos2INETFlag + "/>Internet </td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" name=\"PosLat\" type=\"number\" value=\"" + String(config.wx_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" name=\"PosLon\" type=\"number\" value=\"" + String(config.wx_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
		html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" name=\"PosAlt\" type=\"number\" value=\"" + String(config.wx_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
		html += "</table></td>";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PORT:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"channel\" id=\"channel\">\n";
		for (int i = 0; i < 4; i++)
		{
			if (config.wx_channel == i)
				html += "<option value=\"" + String(i) + "\" selected>" + String(WX_PORT[i]) + " </option>\n";
			else
				html += "<option value=\"" + String(i) + "\" >" + String(WX_PORT[i]) + " </option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitWX'  name=\"commitWX\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWX\"/>\n";
		html += "</form><br />";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_tlm()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool En = false;
	bool pos2RF = false;
	bool pos2INET = false;
	String arg = "";

	if (server.hasArg("commitTLM"))
	{
		for (int i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "Enable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						En = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name = server.arg(i);
					name.trim();
					name.toUpperCase();
					strcpy(config.tlm0_mycall, name.c_str());
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tlm0_ssid = server.arg(i).toInt();
					if (config.tlm0_ssid > 15)
						config.tlm0_ssid = 3;
				}
			}
			if (server.argName(i) == "infoInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tlm0_info_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "dataInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tlm0_data_interval = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "Path")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.tlm0_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "Comment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.tlm0_comment, server.arg(i).c_str());
				}else{
					memset(config.tlm0_comment,0,sizeof(config.tlm0_comment));
				}
			}
			if (server.argName(i) == "Pos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "Pos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			for (int x = 0; x < 13; x++)
			{
				arg = "sensorCH" + String(x);
				if (server.argName(i) == arg)
				{
					if (isValidNumber(server.arg(i)))
						config.tml0_data_channel[x] = server.arg(i).toInt();
				}
				arg = "param" + String(x);
				if (server.argName(i) == arg)
				{
					if (server.arg(i) != "")
					{
						strcpy(config.tlm0_PARM[x], server.arg(i).c_str());
					}
				}
				arg = "unit" + String(x);
				if (server.argName(i) == arg)
				{
					if (server.arg(i) != "")
					{
						strcpy(config.tlm0_UNIT[x], server.arg(i).c_str());
					}
				}
				if (x < 5)
				{
					for (int y = 0; y < 3; y++)
					{
						arg = "eqns" + String(x) + String((char)(y + 'a'));
						if (server.argName(i) == arg)
						{
							if (isValidNumber(server.arg(i)))
								config.tlm0_EQNS[x][y] = server.arg(i).toFloat();
						}
					}
				}
			}
			uint8_t b = 1;
			for (int x = 0; x < 8; x++)
			{
				arg = "bitact" + String(x);
				if (server.argName(i) == arg)
				{
					if (isValidNumber(server.arg(i)))
					{
						if (server.arg(i).toInt() == 1)
						{
							config.tlm0_BITS_Active |= b;
						}
						else
						{
							config.tlm0_BITS_Active &= ~b;
						}
					}
				}
				b <<= 1;
			}
		}
		config.tlm0_en = En;
		config.tlm0_2rf = pos2RF;
		config.tlm0_2inet = pos2INET;

		saveEEPROM();
		initInterval = true;
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else
	{

		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "document.getElementById(\"submitTLM\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/tlm',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";

		/************************ TLM Mode **************************/
		html += "<form id='formTLM' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>System Telemetry</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String EnFlag = "";
		if (config.tlm0_en)
			EnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"Enable\" value=\"OK\" " + EnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.tlm0_mycall) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"mySSID\" id=\"mySSID\">\n";
		for (uint8_t ssid = 0; ssid <= 15; ssid++)
		{
			if (config.tlm0_ssid == ssid)
			{
				html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		html += "</tr>\n";

		html += "<tr>\n";
		html += "<td align=\"right\"><b>PATH:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"Path\" id=\"Path\">\n";
		for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
		{
			if (config.tlm0_path == pthIdx)
			{
				html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
			else
			{
				html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
			}
		}
		html += "</select></td>\n";
		// html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" name=\"Path\" type=\"text\" value=\"" + String(config.tlm0_path) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" name=\"Comment\" type=\"text\" value=\"" + String(config.tlm0_comment) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<tr><td style=\"text-align: right;\">Info Interval:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"3600\" step=\"1\" name=\"infoInv\" type=\"number\" value=\"" + String(config.tlm0_info_interval) + "\" />Sec.</td></tr>";
		html += "<tr><td style=\"text-align: right;\">Data Interval:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"3600\" step=\"1\" name=\"dataInv\" type=\"number\" value=\"" + String(config.tlm0_data_interval) + "\" />Sec.</td></tr>";

		String Pos2RFFlag = "";
		String Pos2INETFlag = "";
		if (config.tlm0_2rf)
			Pos2RFFlag = "checked";
		if (config.tlm0_2inet)
			Pos2INETFlag = "checked";
		html += "<tr><td style=\"text-align: right;\">TX Channel:</td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"Pos2RF\" value=\"OK\" " + Pos2RFFlag + "/>RF <input type=\"checkbox\" name=\"Pos2INET\" value=\"OK\" " + Pos2INETFlag + "/>Internet </td></tr>\n";

		// html += "<tr>\n";
		// html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
		// String timeStampFlag = "";
		// if (config.wx_timestamp)
		// 	timeStampFlag = "checked";
		// html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wxTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
		// html += "</tr>\n";
		for (int ax = 0; ax < 5; ax++)
		{
			html += "<tr><td align=\"right\"><b>Channel A" + String(ax + 1) + ":</b></td>\n";
			html += "<td align=\"center\">\n";
			html += "<table>";

			// html += "<tr><td style=\"text-align: right;\">Name:</td><td style=\"text-align: center;\"><i>Sensor Type</i></td><td style=\"text-align: center;\"><i>Parameter</i></td><td style=\"text-align: center;\"><i>Unit</i></td></tr>\n";

			html += "<tr><td style=\"text-align: right;\">Type/Name:</td>\n";
			html += "<td style=\"text-align: left;\">Sensor Type: ";
			html += "<select name=\"sensorCH" + String(ax) + "\" id=\"sensorCH" + String(ax) + "\">\n";
			for (uint8_t idx = 0; idx < SYSTEM_LEN; idx++)
			{
				if (config.tml0_data_channel[ax] == idx)
				{
					html += "<option value=\"" + String(idx) + "\" selected>" + String(SYSTEM_NAME[idx]) + "</option>\n";
				}
				else
				{
					html += "<option value=\"" + String(idx) + "\">" + String(SYSTEM_NAME[idx]) + "</option>\n";
				}
			}
			html += "</select></td>\n";

			html += "<td style=\"text-align: left;\">Parameter: <input maxlength=\"10\" size=\"8\" name=\"param" + String(ax) + "\" type=\"text\" value=\"" + String(config.tlm0_PARM[ax]) + "\" /></td>\n";
			html += "<td style=\"text-align: left;\">Unit: <input maxlength=\"8\" size=\"5\" name=\"unit" + String(ax) + "\" type=\"text\" value=\"" + String(config.tlm0_UNIT[ax]) + "\" /></td></tr>\n";
			html += "<tr><td style=\"text-align: right;\">EQNS:</td><td colspan=\"3\" style=\"text-align: left;\">a:<input min=\"-999\" max=\"999\" step=\"0.1\" name=\"eqns" + String(ax) + "a\" type=\"number\" value=\"" + String(config.tlm0_EQNS[ax][0], 3) + "\" />  b:<input min=\"-999\" max=\"999\" step=\"0.1\" name=\"eqns" + String(ax) + "b\" type=\"number\" value=\"" + String(config.tlm0_EQNS[ax][1], 3) + "\" /> c:<input min=\"-999\" max=\"999\" step=\"0.1\" name=\"eqns" + String(ax) + "c\" type=\"number\" value=\"" + String(config.tlm0_EQNS[ax][2], 3) + "\" /> (av^2+bv+c)</td></tr>\n";
			html += "</table></td>";
			html += "</tr>\n";
		}

		uint8_t b = 1;
		for (int ax = 0; ax < 8; ax++)
		{
			html += "<tr><td align=\"right\"><b>Channel B" + String(ax + 1) + ":</b></td>\n";
			html += "<td align=\"center\">\n";
			html += "<table>";

			// html += "<tr><td style=\"text-align: right;\">Type/Name:</td>\n";
			html += "<td style=\"text-align: left;\">Type: ";
			html += "<select name=\"sensorCH" + String(ax + 5) + "\" id=\"sensorCH" + String(ax) + "\">\n";
			for (uint8_t idx = 0; idx < SYSTEM_BIT_LEN; idx++)
			{
				if (config.tml0_data_channel[ax + 5] == idx)
				{
					html += "<option value=\"" + String(idx) + "\" selected>" + String(SYSTEM_BITS_NAME[idx]) + "</option>\n";
				}
				else
				{
					html += "<option value=\"" + String(idx) + "\">" + String(SYSTEM_BITS_NAME[idx]) + "</option>\n";
				}
			}
			html += "</select></td>\n";

			html += "<td style=\"text-align: left;\">Parameter: <input maxlength=\"10\" size=\"8\" name=\"param" + String(ax + 5) + "\" type=\"text\" value=\"" + String(config.tlm0_PARM[ax + 5]) + "\" /></td>\n";
			html += "<td style=\"text-align: left;\">Unit: <input maxlength=\"8\" size=\"5\" name=\"unit" + String(ax + 5) + "\" type=\"text\" value=\"" + String(config.tlm0_UNIT[ax + 5]) + "\" /></td>\n";
			String LowFlag = "", HighFlag = "";
			if (config.tlm0_BITS_Active & b)
				HighFlag = "checked=\"checked\"";
			else
				LowFlag = "checked=\"checked\"";
			html += "<td style=\"text-align: left;\"> Active:<input type=\"radio\" name=\"bitact" + String(ax) + "\" value=\"0\" " + LowFlag + "/>LOW <input type=\"radio\" name=\"bitact" + String(ax) + "\" value=\"1\" " + HighFlag + "/>HIGH </td>\n";
			html += "</tr>\n";
			// html += "<tr><td style=\"text-align: right;\">EQNS:</td><td colspan=\"3\" style=\"text-align: left;\">a:<input min=\"-999\" max=\"999\" step=\"0.1\" name=\"eqns" + String(ax + 1) + "a\" type=\"number\" value=\"" + String(config.tlm0_EQNS[ax][0], 3) + "\" />  b:<input min=\"-999\" max=\"999\" step=\"0.1\" name=\"eqns" + String(ax + 1) + "b\" type=\"number\" value=\"" + String(config.tlm0_EQNS[ax][1], 3) + "\" /> c:<input min=\"-999\" max=\"999\" step=\"0.1\" name=\"eqns" + String(ax + 1) + "c\" type=\"number\" value=\"" + String(config.tlm0_EQNS[ax][2], 3) + "\" /> (av^2+bv+c)</td></tr>\n";
			html += "</table></td>";
			html += "</tr>\n";
			b <<= 1;
		}

		// html += "<tr><td align=\"right\"><b>Parameter Name:</b></td>\n";
		// html += "<td align=\"center\">\n";
		// html += "<table>";

		// // html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" name=\"PosLat\" type=\"number\" value=\"" + String(config.wx_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
		// // html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" name=\"PosLon\" type=\"number\" value=\"" + String(config.wx_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
		// // html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" name=\"PosAlt\" type=\"number\" value=\"" + String(config.wx_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
		// html += "</table></td>";
		// html += "</tr>\n";

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitTLM'  name=\"commitTLM\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitTLM\"/>\n";
		html += "</form><br />";
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

void handle_tracker()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	bool trakerEn = false;
	bool smartEn = false;
	bool compEn = false;

	bool posGPS = false;
	bool bcnEN = false;
	bool pos2RF = false;
	bool pos2INET = false;
	bool optCST = false;
	bool optAlt = false;
	bool optBat = false;
	bool optSat = false;
	bool timeStamp = false;

	if (server.hasArg("commitTRACKER"))
	{
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "trackerEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						trakerEn = true;
				}
			}
			if (server.argName(i) == "smartBcnEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						smartEn = true;
				}
			}
			if (server.argName(i) == "compressEnable")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						compEn = true;
				}
			}
			if (server.argName(i) == "trackerOptCST")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optCST = true;
				}
			}
			if (server.argName(i) == "trackerOptAlt")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optAlt = true;
				}
			}
			if (server.argName(i) == "trackerOptBat")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optBat = true;
				}
			}
			if (server.argName(i) == "trackerOptSat")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						optSat = true;
				}
			}
			if (server.argName(i) == "myCall")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					name.toUpperCase();
					strcpy(config.trk_mycall, name.c_str());				
				}
			}
			if (server.argName(i) == "trackerObject")
			{
				if (server.arg(i) != "")
				{
					String name=server.arg(i);
					name.trim();
					strcpy(config.trk_item, name.c_str());
				}
				else
				{
					memset(config.trk_item,0,sizeof(config.trk_item));
				}
			}
			if (server.argName(i) == "mySSID")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_ssid = server.arg(i).toInt();
					if (config.trk_ssid > 15)
						config.trk_ssid = 13;
				}
			}
			if (server.argName(i) == "trackerPosInv")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_interval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "trackerPosLat")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_lat = server.arg(i).toFloat();
				}
			}

			if (server.argName(i) == "trackerPosLon")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_lon = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "trackerPosAlt")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_alt = server.arg(i).toFloat();
				}
			}
			if (server.argName(i) == "trackerPosSel")
			{
				if (server.arg(i) != "")
				{
					if (server.arg(i).toInt() == 1)
						posGPS = true;
				}
			}
			if (server.argName(i) == "hspeed")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_hspeed = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "lspeed")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_lspeed = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "slowInterval")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_slowinterval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "maxInterval")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_maxinterval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "minInterval")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_mininterval = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "minAngle")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_minangle = server.arg(i).toInt();
				}
			}

			if (server.argName(i) == "trackerTable")
			{
				if (server.arg(i) != "")
				{
					config.trk_symbol[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "trackerSymbol")
			{
				if (server.arg(i) != "")
				{
					config.trk_symbol[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "moveTable")
			{
				if (server.arg(i) != "")
				{
					config.trk_symmove[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "moveSymbol")
			{
				if (server.arg(i) != "")
				{
					config.trk_symmove[1] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "stopTable")
			{
				if (server.arg(i) != "")
				{
					config.trk_symstop[0] = server.arg(i).charAt(0);
				}
			}
			if (server.argName(i) == "stopSymbol")
			{
				if (server.arg(i) != "")
				{
					config.trk_symstop[1] = server.arg(i).charAt(0);
				}
			}

			if (server.argName(i) == "trackerPath")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.trk_path = server.arg(i).toInt();
				}
			}
			if (server.argName(i) == "trackerComment")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.trk_comment, server.arg(i).c_str());
				}else{
					memset(config.trk_comment,0,sizeof(config.trk_comment));
				}
			}

			if (server.argName(i) == "trackerPos2RF")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2RF = true;
				}
			}
			if (server.argName(i) == "trackerPos2INET")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						pos2INET = true;
				}
			}
			if (server.argName(i) == "trackerTimeStamp")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
						timeStamp = true;
				}
			}
		}
		config.trk_en = trakerEn;
		config.trk_smartbeacon = smartEn;
		config.trk_compress = compEn;

		config.trk_gps = posGPS;
		config.trk_loc2rf = pos2RF;
		config.trk_loc2inet = pos2INET;

		config.trk_cst = optCST;
		config.trk_altitude = optAlt;
		config.trk_bat = optBat;
		config.trk_sat = optSat;
		config.trk_timestamp = timeStamp;

		saveEEPROM();
		initInterval=true;
		String html = "OK";
		server.send(200, "text/html", html);
	}

	String html = "<script type=\"text/javascript\">\n";
	html += "$('form').submit(function (e) {\n";
	html += "e.preventDefault();\n";
	html += "var data = new FormData(e.currentTarget);\n";
	html += "document.getElementById(\"submitTRACKER\").disabled=true;\n";
	html += "$.ajax({\n";
	html += "url: '/tracker',\n";
	html += "type: 'POST',\n";
	html += "data: data,\n";
	html += "contentType: false,\n";
	html += "processData: false,\n";
	html += "success: function (data) {\n";
	html += "alert(\"Submited Successfully\");\n";
	html += "},\n";
	html += "error: function (data) {\n";
	html += "alert(\"An error occurred.\");\n";
	html += "}\n";
	html += "});\n";
	html += "});\n";
	html += "</script>\n<script type=\"text/javascript\">\n";
	html += "function openWindowSymbol(sel) {\n";
	html += "var i, l, options = [{\n";
	html += "value: 'first',\n";
	html += "text: 'First'\n";
	html += "}, {\n";
	html += "value: 'second',\n";
	html += "text: 'Second'\n";
	html += "}],\n";
	html += "newWindow = window.open(\"\", null, \"height=400,width=400,status=no,toolbar=no,menubar=no,location=no\");\n";

	int i;

	html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
	html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '/'</th></tr><tr>\");\n";
	for (i = 33; i < 129; i++)
	{
		html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(\"+sel.toString()+\"," + String(i) + ",1);\\\" src=\\\"http://aprs.dprns.com/symbols/icons/" + String(i) + "-1.png\\\"></td>\");\n";
		if (((i % 16) == 0) && (i < 126))
			html += "newWindow.document.write(\"</tr><tr>\");\n";
	}
	html += "newWindow.document.write(\"</tr></table><br />\");\n";
	html += "newWindow.document.write(\"<table border=\\\"1\\\" align=\\\"center\\\">\");\n";
	html += "newWindow.document.write(\"<tr><th colspan=\\\"16\\\">Table '\\\'</th></tr><tr>\");\n";
	for (i = 33; i < 129; i++)
	{
		html += "newWindow.document.write(\"<td><img onclick=\\\"window.opener.setValue(\"+sel.toString()+\"," + String(i) + ",2);\\\" src=\\\"http://aprs.dprns.com/symbols/icons/" + String(i, DEC) + "-2.png\\\"></td>\");\n";
		if (((i % 16) == 0) && (i < 126))
			html += "newWindow.document.write(\"</tr><tr>\");\n";
	}
	html += "newWindow.document.write(\"</tr></table>\");\n";

	html += "}\n";

	html += "function setValue(sel,symbol,table) {\n";
	html += "var txtsymbol=document.getElementById('trackerSymbol');\n";
	html += "var txttable=document.getElementById('trackerTable');\n";
	html += "var imgicon=document.getElementById('trackerImgSymbol');\n";
	html += "if(sel==1){\n";
	html += "txtsymbol=document.getElementById('moveSymbol');\n";
	html += "txttable=document.getElementById('moveTable');\n";
	html += "imgicon= document.getElementById('moveImgSymbol');\n";
	html += "}else if(sel==2){\n";
	html += "txtsymbol=document.getElementById('stopSymbol');\n";
	html += "txttable=document.getElementById('stopTable');\n";
	html += "imgicon= document.getElementById('stopImgSymbol');\n";
	html += "}\n";
	html += "txtsymbol.value = String.fromCharCode(symbol);\n";
	html += "if(table==1){\n txttable.value='/';\n";
	html += "}else if(table==2){\n txttable.value='\\\\';\n}\n";
	html += "imgicon.src = \"http://aprs.dprns.com/symbols/icons/\"+symbol.toString()+'-'+table.toString()+'.png';\n";
	html += "\n}\n";
	html += "function onSmartCheck() {\n";
	html += "if (document.querySelector('#smartBcnEnable').checked) {\n";
	// Checkbox has been checked
	html += "document.getElementById(\"smartbcnGrp\").disabled=false;\n";
	html += "} else {\n";
	// Checkbox has been unchecked
	html += "document.getElementById(\"smartbcnGrp\").disabled=true;\n";
	html += "}\n}\n";

	html += "</script>\n";

	/************************ tracker Mode **************************/
	html += "<form id='formtracker' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
	// html += "<h2>[TRACKER] Tracker Position Mode</h2>\n";
	html += "<table>\n";
	// html += "<tr>\n";
	// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
	// html += "<th><span><b>Value</b></span></th>\n";
	// html += "</tr>\n";
	html += "<th colspan=\"2\"><span><b>[TRACKER] Tracker Position Mode</b></span></th>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Enable:</b></td>\n";
	String trackerEnFlag = "";
	if (config.trk_en)
		trackerEnFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"trackerEnable\" value=\"OK\" " + trackerEnFlag + "><span class=\"slider round\"></span></label></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Station Callsign:</b></td>\n";
	html += "<td style=\"text-align: left;\"><input maxlength=\"7\" size=\"6\" id=\"myCall\" name=\"myCall\" type=\"text\" value=\"" + String(config.trk_mycall) + "\" /></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Station SSID:</b></td>\n";
	html += "<td style=\"text-align: left;\">\n";
	html += "<select name=\"mySSID\" id=\"mySSID\">\n";
	for (uint8_t ssid = 0; ssid <= 15; ssid++)
	{
		if (config.trk_ssid == ssid)
		{
			html += "<option value=\"" + String(ssid) + "\" selected>" + String(ssid) + "</option>\n";
		}
		else
		{
			html += "<option value=\"" + String(ssid) + "\">" + String(ssid) + "</option>\n";
		}
	}
	html += "</select></td>\n";
	html += "</tr>\n";

	html += "<tr>\n";
	html += "<td align=\"right\"><b>Item/Obj Name:</b></td>\n";
	html += "<td style=\"text-align: left;\"><input maxlength=\"9\" size=\"9\" id=\"trackerObject\" name=\"trackerObject\" type=\"text\" value=\"" + String(config.trk_item) + "\" /><i> *If not used, leave it blank.In use 3-9 charactor</i></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>PATH:</b></td>\n";
	html += "<td style=\"text-align: left;\">\n";
	html += "<select name=\"trackerPath\" id=\"trackerPath\">\n";
	for (uint8_t pthIdx = 0; pthIdx < PATH_LEN; pthIdx++)
	{
		if (config.trk_path == pthIdx)
		{
			html += "<option value=\"" + String(pthIdx) + "\" selected>" + String(PATH_NAME[pthIdx]) + "</option>\n";
		}
		else
		{
			html += "<option value=\"" + String(pthIdx) + "\">" + String(PATH_NAME[pthIdx]) + "</option>\n";
		}
	}
	html += "</select></td>\n";
	//html += "<td style=\"text-align: left;\"><input maxlength=\"72\" size=\"72\" id=\"trackerPath\" name=\"trackerPath\" type=\"text\" value=\"" + String(config.trk_path) + "\" /></td>\n";
	html += "</tr>\n";

	html += "<tr>\n";
	html += "<td align=\"right\"><b>Text Comment:</b></td>\n";
	html += "<td style=\"text-align: left;\"><input maxlength=\"50\" size=\"50\" id=\"trackerComment\" name=\"trackerComment\" type=\"text\" value=\"" + String(config.trk_comment) + "\" /></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Smart Beacon:</b></td>\n";
	String smartBcnEnFlag = "";
	if (config.trk_smartbeacon)
		smartBcnEnFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" id=\"smartBcnEnable\" name=\"smartBcnEnable\" onclick=\"onSmartCheck()\" value=\"OK\" " + smartBcnEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch use to smart beacon mode</i></label></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Compress:</b></td>\n";
	String compressEnFlag = "";
	if (config.trk_compress)
		compressEnFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"compressEnable\" value=\"OK\" " + compressEnFlag + "><span class=\"slider round\"></span></label><label style=\"vertical-align: bottom;font-size: 8pt;\"><i> *Switch compress packet</i></label></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\"><b>Time Stamp:</b></td>\n";
	String timeStampFlag = "";
	if (config.trk_timestamp)
		timeStampFlag = "checked";
	html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"trackerTimeStamp\" value=\"OK\" " + timeStampFlag + "><span class=\"slider round\"></span></label></td>\n";
	html += "</tr>\n";
	String trackerPos2RFFlag = "";
	String trackerPos2INETFlag = "";
	if (config.trk_loc2rf)
		trackerPos2RFFlag = "checked";
	if (config.trk_loc2inet)
		trackerPos2INETFlag = "checked";
	html += "<tr><td style=\"text-align: right;\"><b>TX Channel:</b></td><td style=\"text-align: left;\"><input type=\"checkbox\" name=\"trackerPos2RF\" value=\"OK\" " + trackerPos2RFFlag + "/>RF <input type=\"checkbox\" name=\"trackerPos2INET\" value=\"OK\" " + trackerPos2INETFlag + "/>Internet </td></tr>\n";
	String trackerOptBatFlag = "";
	String trackerOptSatFlag = "";
	String trackerOptAltFlag = "";
	String trackerOptCSTFlag = "";
	if (config.trk_bat)
		trackerOptBatFlag = "checked";
	if (config.trk_sat)
		trackerOptSatFlag = "checked";
	if (config.trk_altitude)
		trackerOptAltFlag = "checked";
	if (config.trk_cst)
		trackerOptCSTFlag = "checked";
	html += "<tr><td style=\"text-align: right;\"><b>Option:</b></td><td style=\"text-align: left;\">";
	html += "<input type=\"checkbox\" name=\"trackerOptCST\" value=\"OK\" " + trackerOptCSTFlag + "/>Course/Speed ";
	html += "<input type=\"checkbox\" name=\"trackerOptAlt\" value=\"OK\" " + trackerOptAltFlag + "/>Altitude ";
	html += "<input type=\"checkbox\" name=\"trackerOptBat\" value=\"OK\" " + trackerOptBatFlag + "/>Battery ";
	html += "<input type=\"checkbox\" name=\"trackerOptSat\" value=\"OK\" " + trackerOptSatFlag + "/>Satellite";
	html += "</td></tr>\n";

	html += "<tr>";
	html += "<td align=\"right\"><b>POSITION:</b></td>\n";
	html += "<td align=\"center\">\n";
	html += "<table>";
	html += "<tr><td style=\"text-align: right;\">Interval:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"3600\" step=\"1\" id=\"trackerPosInv\" name=\"trackerPosInv\" type=\"number\" value=\"" + String(config.trk_interval) + "\" />Sec.</label></td></tr>";
	String trackerPosFixFlag = "";
	String trackerPosGPSFlag = "";

	if (config.trk_gps)
		trackerPosGPSFlag = "checked=\"checked\"";
	else
		trackerPosFixFlag = "checked=\"checked\"";

	html += "<tr><td style=\"text-align: right;\">Location:</td><td style=\"text-align: left;\"><input type=\"radio\" name=\"trackerPosSel\" value=\"0\" " + trackerPosFixFlag + "/>Fix <input type=\"radio\" name=\"trackerPosSel\" value=\"1\" " + trackerPosGPSFlag + "/>GPS </td></tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\">Symbol Icon:</td>\n";
	String table = "1";
	if (config.trk_symbol[0] == 47)
		table = "1";
	if (config.trk_symbol[0] == 92)
		table = "2";
	html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"trackerTable\" name=\"trackerTable\" type=\"text\" value=\"" + String(config.trk_symbol[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"trackerSymbol\" name=\"trackerSymbol\" type=\"text\" value=\"" + String(config.trk_symbol[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"trackerImgSymbol\" onclick=\"openWindowSymbol(0);\" src=\"http://aprs.dprns.com/symbols/icons/" + String((int)config.trk_symbol[1]) + "-" + table + ".png\"> <i>*Click icon for select symbol</i></td>\n";
	html += "</tr>\n";
	html += "<tr><td style=\"text-align: right;\">Latitude:</td><td style=\"text-align: left;\"><input min=\"-90\" max=\"90\" step=\"0.00001\" id=\"trackerPosLat\" name=\"trackerPosLat\" type=\"number\" value=\"" + String(config.trk_lat, 5) + "\" />degrees (positive for North, negative for South)</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Longitude:</td><td style=\"text-align: left;\"><input min=\"-180\" max=\"180\" step=\"0.00001\" id=\"trackerPosLon\" name=\"trackerPosLon\" type=\"number\" value=\"" + String(config.trk_lon, 5) + "\" />degrees (positive for East, negative for West)</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Altitude:</td><td style=\"text-align: left;\"><input min=\"0\" max=\"10000\" step=\"0.1\" id=\"trackerPosAlt\" name=\"trackerPosAlt\" type=\"number\" value=\"" + String(config.trk_alt, 2) + "\" /> meter. *Value 0 is not send height</td></tr>\n";
	html += "</table></td>";
	html += "</tr>\n";

	html += "<tr>\n";
	html += "<td align=\"right\"><b>Smart Beacon:</b></td>\n";
	html += "<td align=\"center\">\n";
	if (config.trk_smartbeacon)
		html += "<fieldset id=\"smartbcnGrp\">\n";
	else
		html += "<fieldset id=\"smartbcnGrp\" disabled>\n";
	html += "<legend>Smart beacon configuration</legend>\n<table>";
	html += "<tr>\n";
	html += "<td align=\"right\">Move Symbol:</td>\n";
	table = "1";
	if (config.trk_symmove[0] == 47)
		table = "1";
	if (config.trk_symmove[0] == 92)
		table = "2";
	html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"moveTable\" name=\"moveTable\" type=\"text\" value=\"" + String(config.trk_symmove[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"moveSymbol\" name=\"moveSymbol\" type=\"text\" value=\"" + String(config.trk_symmove[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"moveImgSymbol\" onclick=\"openWindowSymbol(1);\" src=\"http://aprs.dprns.com/symbols/icons/" + String((int)config.trk_symmove[1]) + "-" + table + ".png\"> <i>*Click icon for select MOVE symbol</i></td>\n";
	html += "</tr>\n";
	html += "<tr>\n";
	html += "<td align=\"right\">Stop Symbol:</td>\n";
	table = "1";
	if (config.trk_symstop[0] == 47)
		table = "1";
	if (config.trk_symstop[0] == 92)
		table = "2";
	html += "<td style=\"text-align: left;\">Table:<input maxlength=\"1\" size=\"1\" id=\"stopTable\" name=\"stopTable\" type=\"text\" value=\"" + String(config.trk_symstop[0]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> Symbol:<input maxlength=\"1\" size=\"1\" id=\"stopSymbol\" name=\"stopSymbol\" type=\"text\" value=\"" + String(config.trk_symstop[1]) + "\" style=\"background-color: rgb(97, 239, 170);\" /> <img border=\"1\" style=\"vertical-align: middle;\" id=\"stopImgSymbol\" onclick=\"openWindowSymbol(2);\" src=\"http://aprs.dprns.com/symbols/icons/" + String((int)config.trk_symstop[1]) + "-" + table + ".png\"> <i>*Click icon for select STOP symbol</i></td>\n";
	html += "</tr>\n";
	html += "<tr><td style=\"text-align: right;\">High Speed:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"10\" max=\"1000\" step=\"1\" id=\"hspeed\" name=\"hspeed\" type=\"number\" value=\"" + String(config.trk_hspeed) + "\" /> km/h</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Low Speed:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"1\" max=\"250\" step=\"1\" id=\"lspeed\" name=\"lspeed\" type=\"number\" value=\"" + String(config.trk_lspeed) + "\" /> km/h</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Slow Interval:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"60\" max=\"3600\" step=\"1\" id=\"slowInterval\" name=\"slowInterval\" type=\"number\" value=\"" + String(config.trk_slowinterval) + "\" /> Sec.</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Max Interval:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"10\" max=\"255\" step=\"1\" id=\"maxInterval\" name=\"maxInterval\" type=\"number\" value=\"" + String(config.trk_maxinterval) + "\" /> Sec.</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Min Interval:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"1\" max=\"100\" step=\"1\" id=\"minInterval\" name=\"minInterval\" type=\"number\" value=\"" + String(config.trk_mininterval) + "\" /> Sec.</td></tr>\n";
	html += "<tr><td style=\"text-align: right;\">Min Angle:</td><td style=\"text-align: left;\"><input size=\"3\" min=\"1\" max=\"359\" step=\"1\" id=\"minAngle\" name=\"minAngle\" type=\"number\" value=\"" + String(config.trk_minangle) + "\" /> Degree.</td></tr>\n";

	html += "</table></fieldset></tr></table><br />\n";
	html += "<div><button type='submit' id='submitTRACKER'  name=\"commitTRACKER\"> Apply Change </button></div>\n";
	html += "<input type=\"hidden\" name=\"commitTRACKER\"/>\n";
	html += "</form><br />";
	server.send(200, "text/html", html); // send to someones browser when asked
}

void handle_wireless()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	if (server.hasArg("commitWiFiAP"))
	{
		bool wifiAP = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "wifiAP")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						wifiAP = true;
					}
				}
			}

			if (server.argName(i) == "wifi_ssidAP")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wifi_ap_ssid, server.arg(i).c_str());
				}
			}
			if (server.argName(i) == "wifi_passAP")
			{
				if (server.arg(i) != "")
				{
					strcpy(config.wifi_ap_pass, server.arg(i).c_str());
				}
			}
		}
		if (wifiAP)
		{
			config.wifi_mode |= WIFI_AP_FIX;
		}
		else
		{
			config.wifi_mode &= ~WIFI_AP_FIX;
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
	}
	else if (server.hasArg("commitWiFiClient"))
	{
		bool wifiSTA = false;
		String nameSSID, namePASS;
		for (int n = 0; n < 5; n++)
			config.wifi_sta[n].enable = false;
		for (uint8_t i = 0; i < server.args(); i++)
		{
			if (server.argName(i) == "wificlient")
			{
				if (server.arg(i) != "")
				{
					if (String(server.arg(i)) == "OK")
					{
						wifiSTA = true;
					}
				}
			}

			for (int n = 0; n < 5; n++)
			{
				nameSSID = "wifiStation" + String(n);
				if (server.argName(i) == nameSSID)
				{
					if (server.arg(i) != "")
					{
						if (String(server.arg(i)) == "OK")
						{
							config.wifi_sta[n].enable = true;
						}
					}
				}
				nameSSID = "wifi_ssid" + String(n);
				if (server.argName(i) == nameSSID)
				{
					if (server.arg(i) != "")
					{
						strcpy(config.wifi_sta[n].wifi_ssid, server.arg(i).c_str());
					}
				}
				namePASS = "wifi_pass" + String(n);
				if (server.argName(i) == namePASS)
				{
					if (server.arg(i) != "")
					{
						strcpy(config.wifi_sta[n].wifi_pass, server.arg(i).c_str());
					}
				}
			}

			if (server.argName(i) == "wifi_pwr")
			{
				if (server.arg(i) != "")
				{
					if (isValidNumber(server.arg(i)))
						config.wifi_power = (int8_t)server.arg(i).toInt();
				}
			}
		}
		if (wifiSTA)
		{
			config.wifi_mode |= WIFI_STA_FIX;
		}
		else
		{
			config.wifi_mode &= ~WIFI_STA_FIX;
		}
		saveEEPROM();
		String html = "OK";
		server.send(200, "text/html", html);
		WiFi.setTxPower((wifi_power_t)config.wifi_power);
	}else
	{
		String html = "<script type=\"text/javascript\">\n";
		html += "$('form').submit(function (e) {\n";
		html += "e.preventDefault();\n";
		html += "var data = new FormData(e.currentTarget);\n";
		html += "if(e.currentTarget.id===\"formBluetooth\") document.getElementById(\"submitBluetooth\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formWiFiAP\") document.getElementById(\"submitWiFiAP\").disabled=true;\n";
		html += "if(e.currentTarget.id===\"formWiFiClient\") document.getElementById(\"submitWiFiClient\").disabled=true;\n";
		html += "$.ajax({\n";
		html += "url: '/wireless',\n";
		html += "type: 'POST',\n";
		html += "data: data,\n";
		html += "contentType: false,\n";
		html += "processData: false,\n";
		html += "success: function (data) {\n";
		html += "alert(\"Submited Successfully\");\n";
		html += "},\n";
		html += "error: function (data) {\n";
		html += "alert(\"An error occurred.\");\n";
		html += "}\n";
		html += "});\n";
		html += "});\n";
		html += "</script>\n";
		/************************ WiFi AP **************************/
		html += "<form id='formWiFiAP' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>WiFi Access Point</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>WiFi Access Point</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\" width=\"120\"><b>Enable:</b></td>\n";
		String wifiAPEnFlag = "";
		if (config.wifi_mode & WIFI_AP_FIX)
			wifiAPEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wifiAP\" value=\"OK\" " + wifiAPEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi AP SSID:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"32\" maxlength=\"32\" class=\"form-control\" id=\"wifi_ssidAP\" name=\"wifi_ssidAP\" type=\"text\" value=\"" + String(config.wifi_ap_ssid) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi AP PASSWORD:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input size=\"63\" maxlength=\"63\" class=\"form-control\" id=\"wifi_passAP\" name=\"wifi_passAP\" type=\"password\" value=\"" + String(config.wifi_ap_pass) + "\" /></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitWiFiAP'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWiFiAP\"/>\n";
		html += "</form><br />";
		/************************ WiFi Client **************************/
		html += "<br />\n";
		html += "<form id='formWiFiClient' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		html += "<table>\n";
		html += "<th colspan=\"2\"><span><b>WiFi Multi Station</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi STA Enable:</b></td>\n";
		String wifiClientEnFlag = "";
		if (config.wifi_mode & WIFI_STA_FIX)
			wifiClientEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wificlient\" value=\"OK\" " + wifiClientEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>WiFi RF Power:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"wifi_pwr\" id=\"wifi_pwr\">\n";
		for (int i = 0; i < 12; i++)
		{
			if (config.wifi_power == (int8_t)wifiPwr[i][0])
				html += "<option value=\"" + String((int8_t)wifiPwr[i][0]) + "\" selected>" + String(wifiPwr[i][1], 1) + " dBm</option>\n";
			else
				html += "<option value=\"" + String((int8_t)wifiPwr[i][0]) + "\" >" + String(wifiPwr[i][1], 1) + " dBm</option>\n";
		}
		html += "</select>\n";
		html += "</td>\n";
		html += "</tr>\n";
		for (int n = 0; n < 5; n++)
		{
			html += "<tr>\n";
			html += "<td align=\"right\"><b>Station #" + String(n + 1) + ":</b></td>\n";
			html += "<td align=\"center\">\n";
			html += "<fieldset id=\"filterDispGrp" + String(n + 1) + "\">\n";
			html += "<legend>WiFi Station #" + String(n + 1) + "</legend>\n<table style=\"text-align:unset;border-width:0px;background:unset\">";
			html += "<tr style=\"background:unset;\">";
			// html += "<tr>\n";
			html += "<td align=\"right\" width=\"120\"><b>Enable:</b></td>\n";
			String wifiClientEnFlag = "";
			if (config.wifi_sta[n].enable)
				wifiClientEnFlag = "checked";
			html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"wifiStation" + String(n) + "\" value=\"OK\" " + wifiClientEnFlag + "><span class=\"slider round\"></span></label></td>\n";
			html += "</tr>\n";
			html += "<tr>\n";
			html += "<td align=\"right\"><b>WiFi SSID:</b></td>\n";
			html += "<td style=\"text-align: left;\"><input size=\"32\" maxlength=\"32\" name=\"wifi_ssid" + String(n) + "\" type=\"text\" value=\"" + String(config.wifi_sta[n].wifi_ssid) + "\" /></td>\n";
			html += "</tr>\n";
			html += "<tr>\n";
			html += "<td align=\"right\"><b>WiFi PASSWORD:</b></td>\n";
			html += "<td style=\"text-align: left;\"><input size=\"63\" maxlength=\"63\" name=\"wifi_pass" + String(n) + "\" type=\"password\" value=\"" + String(config.wifi_sta[n].wifi_pass) + "\" /></td>\n";
			html += "</tr>\n";
			html += "</tr></table></fieldset>\n";
			html += "</td></tr>\n";
		}

		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitWiFiClient'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitWiFiClient\"/>\n";
		html += "</form><br />";
		/************************ Bluetooth **************************/
#ifdef BLUETOOTH
		html += "<br />\n";
		html += "<form id='formBluetooth' method=\"POST\" action='#' enctype='multipart/form-data'>\n";
		// html += "<h2>Bluetooth Master (BLE)</h2>\n";
		html += "<table>\n";
		// html += "<tr>\n";
		// html += "<th width=\"200\"><span><b>Setting</b></span></th>\n";
		// html += "<th><span><b>Value</b></span></th>\n";
		// html += "</tr>\n";
		html += "<th colspan=\"2\"><span><b>Bluetooth Master (BLE)</b></span></th>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>Enable:</b></td>\n";
		String btEnFlag = "";
		if (config.bt_master)
			btEnFlag = "checked";
		html += "<td style=\"text-align: left;\"><label class=\"switch\"><input type=\"checkbox\" name=\"btMaster\" value=\"OK\" " + btEnFlag + "><span class=\"slider round\"></span></label></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>NAME:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"20\" id=\"bt_name\" name=\"bt_name\" type=\"text\" value=\"" + String(config.bt_name) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>UUID:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid\" name=\"bt_uuid\" type=\"text\" value=\"" + String(config.bt_uuid) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>UUID RX:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid_rx\" name=\"bt_uuid_rx\" type=\"text\" value=\"" + String(config.bt_uuid_rx) + "\" /></td>\n";
		html += "</tr>\n";
		html += "<tr>\n";
		html += "<td align=\"right\"><b>UUID TX:</b></td>\n";
		html += "<td style=\"text-align: left;\"><input maxlength=\"37\" size=\"38\" id=\"bt_uuid_tx\" name=\"bt_uuid_tx\" type=\"text\" value=\"" + String(config.bt_uuid_tx) + "\" /></td>\n";
		html += "</tr>\n";

		html += "<td align=\"right\"><b>MODE:</b></td>\n";
		html += "<td style=\"text-align: left;\">\n";
		html += "<select name=\"bt_mode\" id=\"bt_mode\">\n";
		String btModeOff = "";
		String btModeTNC2 = "";
		String btModeKISS = "";
		if (config.bt_mode == 1)
		{
			btModeTNC2 = "selected";
		}
		else if (config.bt_mode == 2)
		{
			btModeKISS = "selected";
		}
		else
		{
			btModeOff = "selected";
		}
		html += "<option value=\"0\" " + btModeOff + ">NONE</option>\n";
		html += "<option value=\"1\" " + btModeTNC2 + ">TNC2</option>\n";
		html += "<option value=\"2\" " + btModeKISS + ">KISS</option>\n";
		html += "</select>\n";

		html += "<label style=\"font-size: 8pt;text-align: right;\">*See the following for generating UUIDs: <a href=\"https://www.uuidgenerator.net\" target=\"_blank\">https://www.uuidgenerator.net</a></label></td>\n";
		html += "</tr>\n";
		html += "</table><br />\n";
		html += "<div><button type='submit' id='submitBluetooth'  name=\"commit\"> Apply Change </button></div>\n";
		html += "<input type=\"hidden\" name=\"commitBluetooth\"/>\n";
		html += "</form>";
#endif
		server.send(200, "text/html", html); // send to someones browser when asked
	}
}

extern bool afskSync;
extern String lastPkgRaw;
extern float dBV;
extern int mVrms;
void handle_realtime()
{
	// char jsonMsg[1000];
	char *jsonMsg;
	time_t timeStamp;
	time(&timeStamp);

	if (afskSync && (lastPkgRaw.length() > 5))
	{
		int input_length = lastPkgRaw.length();
		jsonMsg = (char *)malloc((input_length * 2) + 70);
		char *input_buffer = (char *)malloc(input_length + 2);
		char *output_buffer = (char *)malloc(input_length * 2);
		if (output_buffer)
		{
			lastPkgRaw.toCharArray(input_buffer, lastPkgRaw.length(), 0);
			lastPkgRaw.clear();
			encode_base64((unsigned char *)input_buffer, input_length, (unsigned char *)output_buffer);
			// Serial.println(output_buffer);
			sprintf(jsonMsg, "{\"Active\":\"1\",\"mVrms\":\"%d\",\"RAW\":\"%s\",\"timeStamp\":\"%li\"}", mVrms, output_buffer, timeStamp);
			// Serial.println(jsonMsg);
			free(input_buffer);
			free(output_buffer);
		}
	}
	else
	{
		jsonMsg = (char *)malloc(100);
		if (afskSync)
			sprintf(jsonMsg, "{\"Active\":\"1\",\"mVrms\":\"%d\",\"RAW\":\"REVDT0RFIEZBSUwh\",\"timeStamp\":\"%li\"}", mVrms, timeStamp);
		else
			sprintf(jsonMsg, "{\"Active\":\"0\",\"mVrms\":\"0\",\"RAW\":\"\",\"timeStamp\":\"%li\"}", timeStamp);
	}
	afskSync = false;
	server.send(200, "text/html", String(jsonMsg));

	delay(100);
	free(jsonMsg);
}

void handle_ws()
{
	// char jsonMsg[1000];
	char *jsonMsg;
	time_t timeStamp;
	time(&timeStamp);

	if (afskSync && (lastPkgRaw.length() > 5))
	{
		int input_length = lastPkgRaw.length();
		jsonMsg = (char *)malloc((input_length * 2) + 70);
		char *input_buffer = (char *)malloc(input_length + 2);
		char *output_buffer = (char *)malloc(input_length * 2);
		if (output_buffer)
		{
			lastPkgRaw.toCharArray(input_buffer, lastPkgRaw.length(), 0);
			lastPkgRaw.clear();
			encode_base64((unsigned char *)input_buffer, input_length, (unsigned char *)output_buffer);
			// Serial.println(output_buffer);
			sprintf(jsonMsg, "{\"Active\":\"1\",\"mVrms\":\"%d\",\"RAW\":\"%s\",\"timeStamp\":\"%li\"}", mVrms, output_buffer, timeStamp);
			// Serial.println(jsonMsg);
			free(input_buffer);
			free(output_buffer);
		}
	}
	else
	{
		jsonMsg = (char *)malloc(100);
		if (afskSync)
			sprintf(jsonMsg, "{\"Active\":\"1\",\"mVrms\":\"%d\",\"RAW\":\"REVDT0RFIEZBSUwh\",\"timeStamp\":\"%li\"}", mVrms, timeStamp);
		else
			sprintf(jsonMsg, "{\"Active\":\"0\",\"mVrms\":\"0\",\"RAW\":\"\",\"timeStamp\":\"%li\"}", timeStamp);
	}
	afskSync = false;
	ws.textAll(jsonMsg);
	free(jsonMsg);
}

void handle_ws_gnss(char *nmea)
{
	char jsonMsg[700];
	char outp[500];
	//char *jsonMsg;
	time_t timeStamp;
	time(&timeStamp);

	int input_length = strlen(nmea);
	//jsonMsg = (char *)malloc((input_length * 2) + 70);
	//if (jsonMsg)
	//{
		//char *input_buffer = (char *)malloc(input_length + 2);
		//char *output_buffer = (char *)malloc(input_length * 2);
		//if (output_buffer)
		{
			// lastPkgRaw.toCharArray(input_buffer, lastPkgRaw.length(), 0);
			// lastPkgRaw.clear();
			//strncpy(input_buffer, nmea, input_length);
			//encode_base64((unsigned char *)input_buffer, input_length, (unsigned char *)output_buffer);
			memset(outp,0,sizeof(outp));
			encode_base64((unsigned char *)nmea, input_length, (unsigned char *)outp);
			// Serial.println(output_buffer);
			sprintf(jsonMsg, "{\"en\":\"%d\",\"lat\":\"%.5f\",\"lng\":\"%.5f\",\"alt\":\"%.2f\",\"spd\":\"%.2f\",\"csd\":\"%.1f\",\"hdop\":\"%.2f\",\"sat\":\"%d\",\"timeStamp\":\"%li\",\"RAW\":\"%s\"}",(int)config.gnss_enable, gps.location.lat(), gps.location.lng(),gps.altitude.meters(),gps.speed.kmph(),gps.course.deg(),gps.hdop.hdop(),gps.satellites.value(), timeStamp, outp);
			// Serial.println(jsonMsg);
			//free(input_buffer);
		//	free(output_buffer);
		}

		afskSync = false;
		//if(ws_gnss.getClients().isEmpty())
		ws_gnss.textAll(jsonMsg);
		//free(jsonMsg);
	//}
}

void handle_test()
{
	// if (server.hasArg("sendBeacon"))
	// {
	// 	String tnc2Raw = send_fix_location();
	// 	if (config.rf_en)
	// 		pkgTxPush(tnc2Raw.c_str(), tnc2Raw.length(), 0);
	// 	// APRS_sendTNC2Pkt(tnc2Raw); // Send packet to RF
	// }
	// else if (server.hasArg("sendRaw"))
	// {
	// 	for (uint8_t i = 0; i < server.args(); i++)
	// 	{
	// 		if (server.argName(i) == "raw")
	// 		{
	// 			if (server.arg(i) != "")
	// 			{
	// 				String tnc2Raw = server.arg(i);
	// 				if (config.rf_en)
	// 				{
	// 					pkgTxPush(tnc2Raw.c_str(), tnc2Raw.length(), 0);
	// 					// APRS_sendTNC2Pkt(server.arg(i)); // Send packet to RF
	// 					// Serial.println("Send RAW: " + tnc2Raw);
	// 				}
	// 			}
	// 			break;
	// 		}
	// 	}
	// }
	// setHTML(6);

	webString = "<html>\n<head>\n";
	webString += "<script src=\"https://apps.bdimg.com/libs/jquery/2.1.4/jquery.min.js\"></script>\n";
	webString += "<script src=\"https://code.highcharts.com/highcharts.js\"></script>\n";
	webString += "<script src=\"https://code.highcharts.com/highcharts-more.js\"></script>\n";
	webString += "<script language=\"JavaScript\">";
	webString += "$(document).ready(function() {\nvar chart = {\ntype: 'gauge',plotBorderWidth: 1,plotBackgroundColor: {linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },stops: [[0, '#FFFFC6'],[0.3, '#FFFFFF'],[1, '#FFF4C6']]},plotBackgroundImage: null,height: 200};\n";
	webString += "var credits = {enabled: false};\n";
	webString += "var title = {text: 'RX VU Meter'};\n";
	webString += "var pane = [{startAngle: -45,endAngle: 45,background: null,center: ['50%', '145%'],size: 300}];\n";
	webString += "var yAxis = [{min: -40,max: 1,minorTickPosition: 'outside',tickPosition: 'outside',labels: {rotation: 'auto',distance: 20},\n";
	webString += "plotBands: [{from: -10,to: 1,color: '#C02316',innerRadius: '100%',outerRadius: '105%'},{from: -20,to: -10,color: '#00C000',innerRadius: '100%',outerRadius: '105%'},{from: -30,to: -20,color: '#AFFF0F',innerRadius: '100%',outerRadius: '105%'},{from: -40,to: -30,color: '#C0A316',innerRadius: '100%',outerRadius: '105%'}],\n";
	webString += "pane: 0,title: {text: '<span style=\"font-size:12px\">dBV</span>',y: -40}}];\n";
	webString += "var plotOptions = {gauge: {dataLabels: {enabled: false},dial: {radius: '100%'}}};\n";
	webString += "var series= [{data: [-40],yAxis: 0}];\n";
	webString += "var json = {};\n json.chart = chart;\n json.credits = credits;\n json.title = title;\n json.pane = pane;\n json.yAxis = yAxis;\n json.plotOptions = plotOptions;\n json.series = series;\n";
	// Add some life
	webString += "var chartFunction = function (chart) { \n"; // the chart may be destroyed
	webString += "var Vrms=0;\nvar dBV=-40;\nvar active=0;var raw=\"\";var timeStamp;\n";
	webString += "if (chart.series) {\n";
	webString += "var left = chart.series[0].points[0];\n";
	webString += "var host='ws://'+location.hostname+':81/ws'\n";
	webString += "const ws = new WebSocket(host);\n";
	webString += "ws.onopen = function() { console.log('Connection opened');};\n ws.onclose = function() { console.log('Connection closed');};\n";
	webString += "ws.onmessage = function(event) {\n  console.log(event.data);\n";
	webString += "const jsonR=JSON.parse(event.data);\n";
	webString += "active=parseInt(jsonR.Active);\n";
	webString += "Vrms=parseFloat(jsonR.mVrms)/1000;\n";
	webString += "dBV=20.0*Math.log10(Vrms);\n";
	webString += "if(dBV<-40) dBV=-40;\n";
	webString += "raw=jsonR.RAW;\n";
	webString += "timeStamp=Number(jsonR.timeStamp);\n";
	webString += "if(active==1){\nleft.update(dBV,false);\nchart.redraw();\n";
	webString += "var date=new Date(timeStamp * 1000);\n";
	webString += "var head=date+\"[\"+Vrms.toFixed(3)+\"Vrms,\"+dBV.toFixed(1)+\"dBV]\\n\";\n";
	//webString += "document.getElementById(\"raw_txt\").value+=head+atob(raw)+\"\\n\";\n";
	webString += "var textArea=document.getElementById(\"raw_txt\");\n";
	webString += "textArea.value+=head+atob(raw)+\"\\n\";\n";
	webString += "textArea.scrollTop = textArea.scrollHeight;\n";
	webString += "}\n";
	webString += "}\n";
	webString += "}};\n";
	webString += "$('#vumeter').highcharts(json, chartFunction);\n";
	webString += "});\n</script>\n";
	webString += "</head><body>\n<table>\n";
	// webString += "<tr><td><form accept-charset=\"UTF-8\" action=\"/test\" class=\"form-horizontal\" id=\"test_form\" method=\"post\">\n";
	// webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-danger\" name=\"sendBeacon\" value='SEND BEACON'></div><br />\n";
	// webString += "<div style=\"margin-left: 20px;\">TNC2 RAW: <input id=\"raw\" name=\"raw\" type=\"text\" size=\"60\" value=\"" + String(config.aprs_mycall) + ">APE32I,WIDE1-1:>Test Status\"/></div>\n";
	// webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-primary\" name=\"sendRaw\" value='SEND RAW'></div> <br />\n";
	// webString += "</form></td></tr>\n";
	// webString += "<tr><td><hr width=\"80%\" /></td></tr>\n";
	webString += "<tr><td><div id=\"vumeter\" style=\"width: 300px; height: 200px; margin: 10px;\"></div></td>\n";
	webString += "<tr><td><div style=\"margin: 15px;\">Terminal<br /><textarea id=\"raw_txt\" name=\"raw_txt\" rows=\"50\" cols=\"80\" /></textarea></div></td></tr>\n";
	webString += "</table>\n";

	webString += "</body></html>\n";
	server.send(200, "text/html", webString); // send to someones browser when asked

	delay(100);
	webString.clear();
}

void handle_about()
{
	if (!server.authenticate(config.http_username, config.http_password))
	{
		return server.requestAuthentication();
	}
	char strCID[50];
	uint64_t chipid = ESP.getEfuseMac();
	sprintf(strCID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

	webString.clear();
	webString += "<table style=\"text-align:unset;border-width:0px;background:unset\"><tr style=\"background:unset;\"><td width=\"49%\" style=\"border:unset;\">";

	webString += "<table>";
	webString += "<th colspan=\"2\"><span><b>System Information</b></span></th>\n";
	// webString += "<tr><th width=\"200\"><span><b>Name</b></span></th><th><span><b>Information</b></span></th></tr>";
	webString += "<tr><td align=\"right\"><b>Hardware Version: </b></td><td align=\"left\"> ESP32DR Simple,ESP32DR_SA868,DIY </td></tr>";
	webString += "<tr><td align=\"right\"><b>Firmware Version: </b></td><td align=\"left\"> V" + String(VERSION) + String(VERSION_BUILD) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>RF Analog Module: </b></td><td align=\"left\"> MODEL: " + String(RF_TYPE[config.rf_type]) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>ESP32 Model: </b></td><td align=\"left\"> " + String(ESP.getChipModel()) + "</td></tr>";
	webString += "<tr><td align=\"right\"><b>Chip ID: </b></td><td align=\"left\"> " + String(strCID) + "</td></tr>";
	webString += "<tr><td align=\"right\"><b>Revision: </b></td><td align=\"left\"> " + String(ESP.getChipRevision()) + "</td></tr>";
	webString += "<tr><td align=\"right\"><b>Flash: </b></td><td align=\"left\">" + String(ESP.getFlashChipSize() / 1000) + " KByte</td></tr>";
	webString += "<tr><td align=\"right\"><b>PSRAM: </b></td><td align=\"left\">" + String(ESP.getPsramSize() / 1000) + " KByte</td></tr>";
	webString += "</table>";
	webString += "</td><td width=\"2%\" style=\"border:unset;\"></td>";
	webString += "<td width=\"49%\" style=\"border:unset;\">";

	webString += "<table>";
	webString += "<th colspan=\"2\"><span><b>Developer/Support Information</b></span></th>\n";
	webString += "<tr><td align=\"right\"><b>Author: </b></td><td align=\"left\">Mr.Somkiat Nakhonthai </td></tr>";
	webString += "<tr><td align=\"right\"><b>Callsign: </b></td><td align=\"left\">HS5TQA</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Country: </b></td><td align=\"left\">Bangkok,Thailand</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Github: </b></td><td align=\"left\"><a href=\"https://github.com/nakhonthai\" target=\"_github\">https://github.com/nakhonthai</a></td></tr>";
	webString += "<tr><td align=\"right\"><b>Youtube: </b></td><td align=\"left\"><a href=\"https://www.youtube.com/@HS5TQA\" target=\"_youtube\">https://www.youtube.com/@HS5TQA</a></td></tr>";
	webString += "<tr><td align=\"right\"><b>Facebook: </b></td><td align=\"left\"><a href=\"https://www.facebook.com/atten\" target=\"_facebook\">https://www.facebook.com/atten</a></td></tr>";
	webString += "<tr><td align=\"right\"><b>Chat LINE: </b></td><td align=\"left\"><a href=\"https://line.me/ti/p/Pw5MKwm6Vo\" target=\"_line\">nakhonline</a></td></tr>";
	webString += "<tr><td align=\"right\"><b>Donate: </b></td><td align=\"left\"><a href=\"https://www.paypal.com/paypalme/hs5tqa\" target=\"_sponsor\">https://www.paypal.com/paypalme/hs5tqa</a></td></tr>";

	webString += "</table>";
	webString += "</td></tr></table><br />";

	webString += "<table>\n";
	webString += "<th colspan=\"2\"><span><b>WiFi Status</b></span></th>\n";
	webString += "<tr><td align=\"right\"><b>Mode:</b></td>\n";
	webString += "<td align=\"left\">";
	if (config.wifi_mode == WIFI_AP_FIX)
	{
		webString += "AP";
	}
	else if (config.wifi_mode == WIFI_STA_FIX)
	{
		webString += "STA";
	}
	else if (config.wifi_mode == WIFI_AP_STA_FIX)
	{
		webString += "AP+STA";
	}
	else
	{
		webString += "OFF";
	}

	wifi_power_t wpr = WiFi.getTxPower();
	String wifipower = "";
	if (wpr < 8)
	{
		wifipower = "-1 dBm";
	}
	else if (wpr < 21)
	{
		wifipower = "2 dBm";
	}
	else if (wpr < 29)
	{
		wifipower = "5 dBm";
	}
	else if (wpr < 35)
	{
		wifipower = "8.5 dBm";
	}
	else if (wpr < 45)
	{
		wifipower = "11 dBm";
	}
	else if (wpr < 53)
	{
		wifipower = "13 dBm";
	}
	else if (wpr < 61)
	{
		wifipower = "15 dBm";
	}
	else if (wpr < 69)
	{
		wifipower = "17 dBm";
	}
	else if (wpr < 75)
	{
		wifipower = "18.5 dBm";
	}
	else if (wpr < 77)
	{
		wifipower = "19 dBm";
	}
	else if (wpr < 80)
	{
		wifipower = "19.5 dBm";
	}
	else
	{
		wifipower = "20 dBm";
	}

	webString += "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>MAC:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.macAddress()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Channel:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.channel()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>TX Power:</b></td>\n";
	webString += "<td align=\"left\">" + wifipower + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>SSID:</b></td>\n";
	webString += "<td align=\"left\">" + String(WiFi.SSID()) + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Local IP:</b></td>\n";
	webString += "<td align=\"left\">" + WiFi.localIP().toString() + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Gateway IP:</b></td>\n";
	webString += "<td align=\"left\">" + WiFi.gatewayIP().toString() + "</td></tr>\n";
	webString += "<tr><td align=\"right\"><b>DNS:</b></td>\n";
	webString += "<td align=\"left\">" + WiFi.dnsIP().toString() + "</td></tr>\n";
	webString += "</table><br /><br />\n";

	webString += "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form' class=\"form-horizontal\">\n";
	webString += "<table>";
	webString += "<th colspan=\"2\"><span><b>Firmware Update</b></span></th>\n";
	webString += "<tr><td align=\"right\"><b>File:</b></td><td align=\"left\"><input id=\"file\" name=\"update\" type=\"file\" onchange='sub(this)' /></td></tr>\n";
	webString += "<tr><td align=\"right\"><b>Progress:</b></td><td><div id='prgbar'><div id='bar' style=\"width: 0px;\"><label id='prg'></label></div></div></td></tr>\n";
	webString += "</table><br />\n";
	webString += "<div class=\"col-sm-3 col-xs-4\"><input type='submit' class=\"btn btn-danger\" id=\"update_sumbit\" value='Firmware Update'></div>\n";

	webString += "</form>\n";
	webString += "<script>"
				 "function sub(obj){"
				 "var fileName = obj.value.split('\\\\');"
				 "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
				 "};"
				 "$('form').submit(function(e){"
				 "e.preventDefault();"
				 "var form = $('#upload_form')[0];"
				 "var data = new FormData(form);"
				 "document.getElementById('update_sumbit').disabled = true;"
				 "$.ajax({"
				 "url: '/update',"
				 "type: 'POST',"
				 "data: data,"
				 "contentType: false,"
				 "processData:false,"
				 "xhr: function() {"
				 "var xhr = new window.XMLHttpRequest();"
				 "xhr.upload.addEventListener('progress', function(evt) {"
				 "if (evt.lengthComputable) {"
				 "var per = evt.loaded / evt.total;"
				 "$('#prg').html(Math.round(per*100) + '%');"
				 "$('#bar').css('width',Math.round(per*100) + '%');"
				 "}"
				 "}, false);"
				 "return xhr;"
				 "},"
				 "success:function(d, s) {"
				 "alert('Wait for system reboot 10sec') "
				 "},"
				 "error: function (a, b, c) {"
				 "}"
				 "});"
				 "});"
				 "</script>";

	webString += "</body></html>\n";
	server.send(200, "text/html", webString); // send to someones browser when asked
}

void handle_gnss()
{
	webString = "<html>\n<head>\n";
	webString += "<script src=\"https://apps.bdimg.com/libs/jquery/2.1.4/jquery.min.js\"></script>\n";
	webString += "<script src=\"https://code.highcharts.com/highcharts.js\"></script>\n";
	webString += "<script src=\"https://code.highcharts.com/highcharts-more.js\"></script>\n";
	webString += "<script language=\"JavaScript\">";

	// Add some life
	webString += "function gnss() { \n"; // the chart may be destroyed
	webString += "var Vrms=0;\nvar dBV=-40;\nvar active=0;var raw=\"\";var timeStamp;\n";
	// webString += "if (chart.series) {\n";
	// webString += "var left = chart.series[0].points[0];\n";
	//webString += "const ws = new WebSocket(\"ws://" + WiFi.localIP().toString() + ":81/ws_gnss\");\n";
	webString += "var host='ws://'+location.hostname+':81/ws_gnss'\n";
	webString += "const ws = new WebSocket(host);\n";
	webString += "ws.onopen = function() { console.log('Connection opened');};\n ws.onclose = function() { console.log('Connection closed');};\n";
	webString += "ws.onmessage = function(event) {\n  console.log(event.data);\n";
	webString += "const jsonR=JSON.parse(event.data);\n";
	webString += "document.getElementById(\"en\").innerHTML=parseInt(jsonR.en);\n";
	webString += "document.getElementById(\"lat\").innerHTML=parseFloat(jsonR.lat);\n";
	webString += "document.getElementById(\"lng\").innerHTML=parseFloat(jsonR.lng);\n";
	webString += "document.getElementById(\"alt\").innerHTML=parseFloat(jsonR.alt);\n";
	webString += "document.getElementById(\"spd\").innerHTML=parseFloat(jsonR.spd);\n";
	webString += "document.getElementById(\"csd\").innerHTML=parseFloat(jsonR.csd);\n";
	webString += "document.getElementById(\"hdop\").innerHTML=parseFloat(jsonR.hdop);\n";
	webString += "document.getElementById(\"sat\").innerHTML=parseInt(jsonR.sat);\n";
	// webString += "active=parseInt(jsonR.Active);\n";
	// webString += "Vrms=parseFloat(jsonR.mVrms)/1000;\n";
	// webString += "dBV=20.0*Math.log10(Vrms);\n";
	// webString += "if(dBV<-40) dBV=-40;\n";
	webString += "raw=jsonR.RAW;\n";
	webString += "timeStamp=Number(jsonR.timeStamp);\n";
	// webString += "if(active==1){\nleft.update(dBV,false);\nchart.redraw();\n";
	// webString += "var date=new Date(timeStamp * 1000);\n";
	// webString += "var head=date+\"[\"+Vrms.toFixed(3)+\"Vrms,\"+dBV.toFixed(1)+\"dBV]\\n\";\n";
	// webString += "document.getElementById(\"raw_txt\").value+=head+atob(raw)+\"\\n\";\n";
	webString += "var textArea=document.getElementById(\"raw_txt\");\n";
	webString += "textArea.value+=atob(raw)+\"\\n\";\n";
	webString += "textArea.scrollTop = textArea.scrollHeight;\n";
	webString += "}\n";
	// webString += "}};\n";
	// webString += "$('#vumeter').highcharts(json, chartFunction);\n";
	webString += "};\n</script>\n";
	webString += "</head><body onload=\"gnss()\">\n";

	webString += "<table width=\"200\" border=\"1\">";
	webString += "<th colspan=\"2\" style=\"background-color: #00BCD4;\"><span><b>GNSS Information</b></span></th>\n";
	// webString += "<tr><th width=\"200\"><span><b>Name</b></span></th><th><span><b>Information</b></span></th></tr>";
	webString += "<tr><td align=\"right\"><b>Enable: </b></td><td align=\"left\"> <label id=\"en\">"+String(config.gnss_enable)+"</label></td></tr>";
	webString += "<tr><td align=\"right\"><b>Latitude: </b></td><td align=\"left\"> <label id=\"lat\">"+String(gps.location.lat(),5)+"</label></td></tr>";
	webString += "<tr><td align=\"right\"><b>Longitude: </b></td><td align=\"left\"> <label id=\"lng\">"+String(gps.location.lng(),5)+"</label></td></tr>";
	webString += "<tr><td align=\"right\"><b>Altitude: </b></td><td align=\"left\"> <label id=\"alt\">"+String(gps.altitude.meters(),2)+"</label> m.</td></tr>";
	webString += "<tr><td align=\"right\"><b>Speed: </b></td><td align=\"left\"> <label id=\"spd\">"+String(gps.speed.kmph(),2)+"</label> km/h</td></tr>";
	webString += "<tr><td align=\"right\"><b>Course: </b></td><td align=\"left\"> <label id=\"csd\">"+String(gps.course.deg(),1)+"</label></td></tr>";	
	webString += "<tr><td align=\"right\"><b>HDOP: </b></td><td align=\"left\"> <label id=\"hdop\">"+String(gps.hdop.hdop(),2)+"</label> </td></tr>";
	webString += "<tr><td align=\"right\"><b>SAT: </b></td><td align=\"left\"> <label id=\"sat\">"+String(gps.satellites.value())+"</label> </td></tr>";
	webString += "</table><table>";
	// webString += "<tr><td><form accept-charset=\"UTF-8\" action=\"/test\" class=\"form-horizontal\" id=\"test_form\" method=\"post\">\n";
	// webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-danger\" name=\"sendBeacon\" value='SEND BEACON'></div><br />\n";
	// webString += "<div style=\"margin-left: 20px;\">TNC2 RAW: <input id=\"raw\" name=\"raw\" type=\"text\" size=\"60\" value=\"" + String(config.aprs_mycall) + ">APE32I,WIDE1-1:>Test Status\"/></div>\n";
	// webString += "<div style=\"margin-left: 20px;\"><input type='submit' class=\"btn btn-primary\" name=\"sendRaw\" value='SEND RAW'></div> <br />\n";
	// webString += "</form></td></tr>\n";
	// webString += "<tr><td><hr width=\"80%\" /></td></tr>\n";
	// webString += "<tr><td><div id=\"vumeter\" style=\"width: 300px; height: 200px; margin: 10px;\"></div></td>\n";
	webString += "<tr><td><b>Terminal:</b><br /><textarea id=\"raw_txt\" name=\"raw_txt\" rows=\"30\" cols=\"80\" /></textarea></td></tr>\n";
	webString += "</table>\n";

	webString += "</body></html>\n";
	server.send(200, "text/html", webString); // send to someones browser when asked

	delay(100);
	webString.clear();
}

void handle_default()
{
	defaultSetting = true;
	defaultConfig();
	defaultSetting = false;
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{

	if (type == WS_EVT_CONNECT)
	{

		log_d("Websocket client connection received");
	}
	else if (type == WS_EVT_DISCONNECT)
	{

		log_d("Client disconnected");
	}
}

bool webServiceBegin = true;
void webService()
{
	if (webServiceBegin)
	{
		webServiceBegin = false;
	}
	else
	{
		return;
	}
	server.close();
	ws.onEvent(onWsEvent);
	async_server.addHandler(&ws);
	async_server.addHandler(&ws_gnss);
	async_server.begin();
	// web client handlers
	server.on("/", setMainPage);
	server.on("/logout", handle_logout);
	server.on("/radio", handle_radio);
	server.on("/vpn", handle_vpn);
	server.on("/mod", handle_mod);
	server.on("/default", handle_default);
	server.on("/igate", handle_igate);
	server.on("/digi", handle_digi);
	server.on("/tracker", handle_tracker);
	server.on("/wx", handle_wx);
	server.on("/tlm", handle_tlm);
	server.on("/system", handle_system);
	server.on("/symbol", handle_symbol);
	server.on("/wireless", handle_wireless);
	server.on("/tnc2", handle_test);
	server.on("/gnss", handle_gnss);
	server.on("/realtime", handle_realtime);
	server.on("/about", handle_about);
	server.on("/dashboard", handle_dashboard);
	server.on("/sidebarInfo", handle_sidebar);
	server.on("/sysinfo", handle_sysinfo);
	server.on("/lastHeard", handle_lastHeard);
	server.on("/style.css", handle_css);
	server.on("/jquery-3.7.1.js", handle_jquery);
	/*handling uploading firmware file */
	server.on(
		"/update", HTTP_POST, []()
		{
			server.sendHeader("Connection", "close");
			server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
			ESP.restart(); },
		[]()
		{
			HTTPUpload &upload = server.upload();
			if (upload.status == UPLOAD_FILE_START)
			{
				Serial.printf("Firmware Update FILE: %s\n", upload.filename.c_str());
				if (!Update.begin(UPDATE_SIZE_UNKNOWN))
				{ // start with max available size
					Update.printError(Serial);
					delay(3);
				}
				else
				{
					// wdtDisplayTimer = millis();
					// wdtSensorTimer = millis();
					// disableCore0WDT();
					// disableCore1WDT();
					// disableLoopWDT();

					// aprsClient.stop();
					// vTaskSuspend(taskAPRSHandle);
					// vTaskSuspend(taskTNCHandle);
					// vTaskSuspend(taskGpsHandle);
					// vTaskSuspend(taskNetworkHandle);
					// config.igate_en = false;
					// config.rf_en = false;

					// delay(3);
				}
			}
			else if (upload.status == UPLOAD_FILE_WRITE)
			{
				/* flashing firmware to ESP*/
				if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
				{
					Update.printError(Serial);
					delay(3);
				}
			}
			else if (upload.status == UPLOAD_FILE_END)
			{
				if (Update.end(true))
				{ // true to set the size to the current progress
					delay(3);
				}
				else
				{
					Update.printError(Serial);
					delay(3);
				}
			}
		});
	server.begin();
}