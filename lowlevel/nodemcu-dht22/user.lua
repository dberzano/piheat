--
-- Remember to connect GPIO16 and RST to enable deep sleep.
-- Configuration is in private.lua, please refer to the example.
--

--
-- Non-private configuration
--

-- B,G,N. B has the highest range (though the lowest rate)
wifi_signal_mode = wifi.PHYMODE_B

-- Leave blank for using DHCP
client_ip = ""
client_netmask = ""
client_gateway = ""

-- Deep sleep interval in us
dsleep_enabled = true
dsleep_interval_us = 60000000

-- Sleep between retries (and first attempt too)
sleep_tries_ms = 7000

-- Connections
gpio_dht    = 4
gpio_dhtpwr = 5
gpio_errled = 6

--
-- End of non-private configuration
--


temp = 0
humi = 0

-- Number of consecutive errors (HTTP, Wi-Fi, DHT22 reading)
nerr = 0
iserr = false

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
  -- Uncomment the following line to enable debug messages.
  --print("DEBUG: "..msg)
end

-- Blink led for signalling errors.
function siren(iters)
  lowdelay=0
  gpio.mode(gpio_errled, gpio.OUTPUT)
  gpio.write(gpio_errled, gpio.LOW)
  for i=0,iters,1
  do
    gpio.write(gpio_errled, gpio.HIGH)
    tmr.delay(50000)
    gpio.write(gpio_errled, gpio.LOW)
    if lowdelay > 0 then
      tmr.delay(lowdelay)
    end
    lowdelay=750000
  end
end

-- DHT22 sensor logic.
-- Works around DHT22 problems by turning it on only before reading.
function getsensor()
  print("DHT22: powering up")
  gpio.mode(gpio_dhtpwr, gpio.OUTPUT)
  gpio.write(gpio_dhtpwr, gpio.HIGH)
  tmr.delay(1100000)
  print("DHT22: reading")
  DHT = require("dht22_min")
  DHT.read(gpio_dht)
  temp = DHT.getTemperature()
  humi = DHT.getHumidity()
  gpio.write(gpio_dhtpwr, gpio.LOW)

  if humi == nil then
    print("DHT22: error reading")
  else
    print("DHT22: temp="..(temp / 10).."."..(temp % 10).."degC, "..
                 "humi="..(humi / 10).."."..(humi % 10).."%")
  end
  DHT = nil
  package.loaded["dht22_min"] = nil
end

function cond_dsleep()
  nerr = 0
  if dsleep_enabled then
    print("Enabling deep sleep for "..dsleep_interval_us.." us...")
    tmr.unregister(0)
    node.dsleep(dsleep_interval_us)
  else
    print("No deep sleep. Doing a normal sleep for "..dsleep_interval_us.." us...")
    tmr.unregister(0)
    tmr.alarm(0, dsleep_interval_us/1000, 1,
              function()
                print("End of sleep.")
                tmr.alarm(0, sleep_tries_ms, 1, function() loop() end)
              end)
  end
end

function loop()

  if iserr then
    nerr = nerr + 1
    print("We have had "..nerr.."/3 consecutive errors")
    if nerr == 3 then
      -- Just reboot.
      siren(5)
      cond_dsleep()
      return
    else
      siren(1)
    end
  end

  if wifi.sta.status() == 5 then

    iserr = true
    getsensor()
    if humi == nil then
      return
    end

    -- Split URL into host and path
    host = string.gsub(post_url, "^.*://", "")
    path = string.gsub(host, "^[^/]*", "")
    host = string.gsub(host, "/.*", "")

    -- Put data into the path string
    path = string.gsub(path, "@@TEMP@@", (temp/10).."."..(temp%10))
    path = string.gsub(path, "@@HUMI@@", (humi/10).."."..(humi%10))

    -- Send data
    print("Posting to URL http://"..host..path)
    iserr = true
    conn = net.createConnection(net.TCP, 0)
    conn:on("receive",
            function(conn, payload)
              dbg("=== RESPONSE ===\n"..payload.."\n=== END RESPONSE ===")
              -- We must wait for data to come back before deep sleeping.
              -- Otherwise we might do it too early and close the connection.
              iserr = false
              cond_dsleep()
            end)
    conn:dns(host,
             function(conn, ip)
               conn:connect(80, ip)
               req = "POST "..path.." HTTP/1.1\r\n"..
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
      iserr = true
    end
end

-- This loop will keep going in case of failures.
-- It is only killed on successful send.
tmr.alarm(0, sleep_tries_ms, 1, function() loop() end)
