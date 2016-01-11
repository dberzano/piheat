--
-- Remember to connect GPIO16 and RST to enable deep sleep
--

--wifi_SSID = "@WIFI_SSID@"
--wifi_password = "@WIFI_PASSWORD@"
-- B,G,N. B has the highest range (though the lowest rate)
wifi_signal_mode = wifi.PHYMODE_B

-- Leave blank for using DHCP
client_ip = ""
client_netmask = ""
client_gateway = ""

-- Deep sleep interval in us
dsleep_interval_us = 60000000

-- dweet.io thing id
--dweet_thing_id = "@DWEET_THING_ID@"

temp = 0
humi = 0

-- Connect to the wifi network
wifi.setmode(wifi.STATION)
wifi.setphymode(wifi_signal_mode)
wifi.sta.config(wifi_SSID, wifi_password)
wifi.sta.connect()
if client_ip ~= "" then
  wifi.sta.setip({ip=client_ip,netmask=client_netmask,gateway=client_gateway})
end

-- Debug print
function dbg(msg)
  --print("DEBUG: "..msg)
end

-- DHT22 sensor logic
function getsensor()
  DHT = require("dht22_min")
  DHT.read(4)
  temp = DHT.getTemperature()
  humi = DHT.getHumidity()

  if humi == nil then
    print("DHT22: error reading")
  else
    print("DHT22: temp="..(temp / 10).."."..(temp % 10).." degC "..
                 "humi="..(humi / 10).."."..(humi % 10).."%")
  end
  DHT = nil
  package.loaded["dht22_min"] = nil
end

function loop()
  if wifi.sta.status() == 5 then

    getsensor()
    if humi == nil then
      return
    end

    host = "dweet.io"
    path = "/dweet/for/"..dweet_thing_id

    -- Send data to dweet.io
    print("Sending data for thing "..dweet_thing_id)
    conn = net.createConnection(net.TCP, 0)
    conn:on("receive",
            function(conn, payload)
              dbg("=== RESPONSE ===\n"..payload.."\n=== END RESPONSE ===")
              -- We must wait for data to come back before deep sleeping.
              -- Otherwise we might do it too early and close the connection.
              print("Enabling deep sleep for "..dsleep_interval_us.." us...")
              tmr.stop(0)
              node.dsleep(dsleep_interval_us)
            end)
    conn:dns(host,
             function(conn, ip)
               conn:connect(80, ip)
               req = "POST "..path.."?"..
                     "temp="..(temp/10).."."..(temp%10).."&"..
                     "humi="..(humi/10).."."..(humi%10).."&"..
                     " HTTP/1.1\r\n"..
                     "Host: "..host.."\r\n"..
                     "Content-Type: application/x-www-form-urlencoded\r\n"..
                     "Connection: keep-alive\r\n"..
                     "Accept: */*\r\n\r\n"
               conn:send(req)      
               dbg("=== REQUEST ===\n"..req.."\n=== END REQUEST ===")
             end)

    print("Waiting for connection to complete")

    else
      print("Connecting to Wi-Fi")
    end
end

tmr.alarm(0, 7000, 1, function() loop() end)
