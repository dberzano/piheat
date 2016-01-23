user_script = "user.lua"
private_script = "private.lua"
l = file.list()

for k,v in pairs(l) do
  if k == private_script then
    print("Loading private script...")
    dofile(private_script)
  end
end

for k,v in pairs(l) do
  if k == user_script then
    print("*** About to load user script. Abort within 3 sec with tmr.unregister(0) ***")
    tmr.alarm(0, 3000, 0, function()
      print("Loading user script...")
      dofile(user_script)
    end)
  end
end
