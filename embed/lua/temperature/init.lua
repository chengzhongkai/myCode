--------------------------------------
-- read previous temperature data from
--   log file
--------------------------------------
function getOldTemp()
  local tmp = 0
  if file.open("tmp", "r") == true then
    tmp=file.read()
    file.close()
  else
    file.open("tmp","w")
    file.write(0)
    file.flush()
    file.close()
  end 
  return tmp
end

--------------------------------------
-- get temperature form adc interface
--------------------------------------
function getTemp()
  return -(adc.read(0)*100000/512-171400)/814
end

--------------------------------------
-- write temperature data to log file
--------------------------------------
function setTemp(tmp)
   file.open("tmp","w")
    file.write(tmp)
    file.flush()
    file.close()
end

--------------------------------------
-- write string to log file
--------------------------------------
function addLog(str)
   file.open("log","a+")
    file.writeline(str)
    file.close()
end

--------------------------------------
-- connect to wifi network
-- @ssid      network name
-- @pwd       password
-- @callback  the function is invoked after connected network
-- @timeout   the function is invoked when connect timeout
--------------------------------------
function connWifi(ssid,pwd,callback,timeout)
  wifi.setmode(wifi.STATION)
  
  
  --wifi.sta.setip(cfg)
  
  
  wifi.sta.config(ssid, pwd)
  local i=0
  tmr.alarm (1, 800, 1, function ( )
    if wifi.sta.getip ( ) == nil then
      --print ("Waiting for Wifi connection")
      i=i+1
      if i>30 then
        timeout()
      end
      
    else
      tmr.stop (1)
      --print ("Config done, IP is " .. wifi.sta.getip ( ))
      callback(wifi.sta.getip ( ))
    end
  end)
end

--------------------------------------
-- get datetime for google service
--------------------------------------
function getTime(fun)
  conn=net.createConnection(net.TCP, 0) 
 
  conn:on("connection",function(conn, payload)
            conn:send("HEAD / HTTP/1.1\r\n".. 
                      "Host: google.com\r\n"..
                      "Accept: */*\r\n"..
                      "User-Agent: Mozilla/4.0 (compatible; esp8266 Lua;)"..
                      "\r\n\r\n") 
            end)
            
  conn:on("receive", function(conn, payload)
      fun(string.sub(payload,string.find(payload,"Date: ")
           +6,string.find(payload,"Date: ")+35))
    conn:close()
    end) 
  t = tmr.now()    
  conn:connect(80,'google.com')
end

--------------------------------------
-- publish mqtt message
--------------------------------------
function sendMsg(msg,fun)
  m = mqtt.Client("clientid", 120, "chengzhongkai@github", "UmnfzA7UILkQMtzu")
  m:lwt("/lwt", "offline", 0, 0)
  m:on("connect", function(con) print("connected") end)
  
  m:connect("lite.mqtt.shiguredo.jp", 1883, 0, 0, function(conn) 
    m:publish("chengzhongkai@github/aa", msg, 0, 0, function(conn) 
      m:close()
      fun()
    end)
  end)
end

-- start programe ---------------------
print("start ...")

-- get real-time temperature
oldTemp = getOldTemp()
tmp = getTemp()

if oldTemp - tmp ~= 0 then
  -- if temperature changes
  print(oldTemp..'->'..tmp)
  setTemp(tmp)
  --  publish mqtt message to server
  connWifi("dd-wrt_vap", "chibamatsudo0411",function(aa)
    getTime(function(bb)
      msgStr = "temp001:"..tmp..":"..bb
      addLog(msgStr)
      sendMsg(msgStr,function()
        node.dsleep(60000000,4)
      end)
      
    end) 
  end,function()
    print("timeout ...")
    node.dsleep(60000000,4)
  end)
else
  -- continue next sampling
  node.dsleep(60000000,4)
end


