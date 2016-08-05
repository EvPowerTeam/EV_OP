n = Map("chargerinfo","电桩配置","")

----chargerset----

cur = n:section(TypedSection,"tab",translate(""))
cur.addremove = false
cur.anonymous = true

----current----

charcur = cur:option(Value,"maxcurrent",translate("最大电流值"))

----chargernum----

charnum = cur:option(Value,"chargernum",translate("电桩台数"))

return n
