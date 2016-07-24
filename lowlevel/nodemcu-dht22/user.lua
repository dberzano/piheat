--
-- Remember to connect GPIO16 and RST to enable deep sleep.
-- Configuration is in private.lua, please refer to the example.
--

--
-- Non-private configuration
--

DEEPSLEEP_ON = true
DEEPSLEEP_US = 180000000
WATCHDOG_MS  = 45000
GPIO_DHT     = 4
GPIO_DHTPWR  = 5
GPIO_ERRLED  = 6

--
-- End of non-private configuration
--

starttime = 0
volt      = 0
temp      = 0
humi      = 0

function wificonn(callback)
  wifi.setmode(wifi.STATION)
  -- B,G,N. B has the highest range (though the lowest rate)
  wifi.setphymode(wifi.PHYMODE_B)
  wifi.sta.config(wifi_SSID, wifi_password)
  wifi.sta.connect()
  tmr.alarm(1, 1000, tmr.ALARM_AUTO,
            function()
              print("WiFi: waiting for IP address")
              if wifi.sta.status() == 5 then
                print("WiFi: got IP: "..wifi.sta.getip())
                tmr.unregister(1)
                callback()
              end
            end)
end

function wifidisconn(callback)
  wifi.setmode(wifi.STATION)
  wifi.sta.disconnect()
  tmr.alarm(1, 1000, tmr.ALARM_AUTO,
            function()
              print("WiFi: waiting for disconnection")
              if wifi.sta.status() == 0 then
                print("WiFi: disconnected")
                tmr.unregister(1)
                callback()
              end
            end)
end

function getvolt()
  volt = adc.readvdd33()
  print("Volt: read "..(volt/1000).."."..(volt%1000).."V")
end

function siren(times)
  lowdelay=0
  gpio.mode(GPIO_ERRLED, gpio.OUTPUT)
  gpio.write(GPIO_ERRLED, gpio.LOW)
  for i=0,times,1
  do
    gpio.write(GPIO_ERRLED, gpio.HIGH)
    tmr.delay(50000)
    gpio.write(GPIO_ERRLED, gpio.LOW)
    if lowdelay > 0 then
      tmr.delay(lowdelay)
    end
    lowdelay=750000
  end
end

function getsensor()
  print("DHT22: power up")
  gpio.mode(GPIO_DHTPWR, gpio.OUTPUT)
  gpio.write(GPIO_DHTPWR, gpio.HIGH)
  tmr.delay(1100000)
  print("DHT22: reading")
  DHT = require("dht22_min")
  DHT.read(GPIO_DHT)
  temp = DHT.getTemperature()
  humi = DHT.getHumidity()
  gpio.write(GPIO_DHTPWR, gpio.LOW)

  if humi == nil then
    print("DHT22: error reading")
  else
    print("DHT22: temp="..(temp / 10).."."..(temp % 10).."degC, "..
                 "humi="..(humi / 10).."."..(humi % 10).."%")
  end
  DHT = nil
  package.loaded["dht22_min"] = nil
end

function deepsleep(callback)
  for i=0,6,1 do
    tmr.unregister(i)
  end
  if DEEPSLEEP_ON then
    print("DeepSleep: deep sleeping for "..DEEPSLEEP_US.."us...")
    node.dsleep(DEEPSLEEP_US)
  else
    print("DeepSleep: no deep sleep, soft sleeping for "..DEEPSLEEP_US.."us...")
    tmr.alarm(0, DEEPSLEEP_US/1000, tmr.ALARM_SINGLE,
              function()
                print("DeepSleep: end of sleep")
                callback()
              end)
  end
end

function postdata(callback)

  -- Split URL into host and path
  host = string.gsub(post_url, "^.*://", "")
  path = string.gsub(host, "^[^/]*", "")
  host = string.gsub(host, "/.*", "")

  -- Put data into the path string
  path = string.gsub(path, "@@TEMP@@", (temp/10).."."..(temp%10))
  path = string.gsub(path, "@@HUMI@@", (humi/10).."."..(humi%10))
  path = string.gsub(path, "@@VOLT@@", (volt/1000).."."..(volt%1000))

  -- Send data
  print("Post: posting to URL http://"..host..path)
  conn = net.createConnection(net.TCP, 0)
  conn:on("receive",
          function(conn, payload)
            print("=== RESPONSE ===\n"..payload.."\n=== END RESPONSE ===")
            -- We must wait for data to come back before calling back.
            -- Otherwise we might do it too early and close the connection.
            print("Post: connection completed")
            callback()
          end)
    conn:dns(host,
             function(conn, ip)
               if ip == nil then
                 print("Post: cannot resolve ".. host)
                 return
               end
               print("Post: host "..host.." has IP "..ip)
               conn:connect(80, ip)
               req = "POST "..path.." HTTP/1.1\r\n"..
                     "Host: "..host.."\r\n"..
                     "Content-Type: application/x-www-form-urlencoded\r\n"..
                     "Connection: keep-alive\r\n"..
                     "Accept: */*\r\n\r\n"
               conn:send(req)
               print("=== REQUEST ===\n"..req.."\n=== END REQUEST ===")
             end)
    print("Post: waiting for connection to complete")
end

function entrypoint()
  starttime = tmr.now()
  tmr.unregister(2)
  tmr.alarm(2, WATCHDOG_MS, tmr.ALARM_SINGLE,
            function()
              print("Watchdog: not done in "..WATCHDOG_MS.."ms, restarting")
              siren(10)
              deepsleep(entrypoint)
            end)
  wifidisconn(function()
                getvolt()
                wificonn(function()
                           tmr.alarm(3, 1000, tmr.ALARM_SINGLE,
                           function()
                             getsensor()
                             if humi == nil then
                               print("humi is nil")
                               deepsleep(entrypoint)
                             end
                             postdata(function()
                                        print("Watchdog: completed in "..
                                              ((tmr.now()-starttime)/1000000)..
                                              "s")
                                        deepsleep(entrypoint)
                                      end)
                           end)
                         end)
              end)
end

entrypoint()
