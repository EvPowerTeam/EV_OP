--[[
LuCI - Lua Configuration Interface

Copyright 2008 Steven Barth <steven@midlink.org>
Copyright 2010-2012 Jo-Philipp Wich <xm@subsignal.org>
Copyright 2010 Manuel Munz <freifunk at somakoma dot de>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local m,xuci

xuci = uci.cursor()

----Reading charger----

m = Map("chargerinfo",translate("远程抄表"),translate("你可以通过下面界面看到每台电桩的抄表状态、成功抄表时间和上传充电记录数量"))

----first reading chargertime---

local timetab = {year=2010,month=01,day=01,hour=0,min=0,sec=0,isdst=false}

local firsttime = os.time(timetab);

----charger cid----

charger1cid = xuci:get("chargerinfo","charger1","CID")
charger2cid = xuci:get("chargerinfo","charger2","CID")
charger3cid = xuci:get("chargerinfo","charger3","CID")
charger4cid = xuci:get("chargerinfo","charger4","CID")
charger5cid = xuci:get("chargerinfo","charger5","CID")
charger6cid = xuci:get("chargerinfo","charger6","CID")
charger7cid = xuci:get("chargerinfo","charger7","CID")
charger8cid = xuci:get("chargerinfo","charger8","CID")

----charger1----

    s1 = m:section(NamedSection,"charger1","1",translate("电桩1"))
    s1.addremove = false
    s1.anonymous = false

    cid1 = s1:option(Value,"CID",translate("CID:"))

    start1 = s1:option(Button, "start", translate("抄表"))
    start1.inputstyle = "apply"
    start1.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
	xuci:set("chargerinfo","SERVER","CID",charger1cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time1 = s1:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time1.cfgvalue = function(self,section)
	local t1
	t1 = xuci:get("chargerinfo","charger1","CB_END_TIME")
	return os.date("%c",t1)
    end
    num1 = s1:option(DummyValue,"CB_NUM","上传充电记录数量:")
    
----charger2----

s2 = m:section(NamedSection,"charger2","1",translate("电桩2"))
s2.addremove = false
s2.anonymous = false

    cid2 = s2:option(Value,"CID",translate("CID:"))

    start2 = s2:option(Button, "start", translate("抄表"))
    start2.inputstyle = "apply"
    start2.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
	xuci:set("chargerinfo","SERVER","CID",charger2cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end
    time2 = s2:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time2.cfgvalue = function(self,section)
	local t2
	t2 = xuci:get("chargerinfo","charger2","CB_END_TIME")
	return os.date("%c",t2)
    end
  
    num2 = s2:option(DummyValue,"CB_NUM","上传充电记录数量:")

----charger3----

s3 = m:section(NamedSection,"charger3","1",translate("电桩3"))
s3.addremove = false
s3.anonymous = false

    cid3 = s3:option(Value,"CID",translate("CID:"))

    start3 = s3:option(Button, "start", translate("抄表"))
    start3.inputstyle = "apply"
    start3.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
    	xuci:set("chargerinfo","SERVER","CID",charger3cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time3 = s3:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time3.cfgvalue = function(self,section)
	local t3
	t3 = xuci:get("chargerinfo","charger3","CB_END_TIME")
	return os.date("%c",t3)
    end

    num3 = s3:option(DummyValue,"CB_NUM","上传充电记录数量:")

----charger4----

    s4 = m:section(NamedSection,"charger4","1",translate("电桩4"))
    s4.addremove = false
    s4.anonymous = false

    cid4 = s4:option(Value,"CID",translate("CID:"))

    start4 = s4:option(Button, "start", translate("抄表"))
    start4.inputstyle = "apply"
    start4.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
    	xuci:set("chargerinfo","SERVER","CID",charger4cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time4 = s4:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time4.cfgvalue = function(self,section)
	local t4
	t4 = xuci:get("chargerinfo","charger4","CB_END_TIME")
	return os.date("%c",t4)
    end
    num4 = s4:option(DummyValue,"CB_NUM","上传充电记录数量:")
----charger5----

s5 = m:section(NamedSection,"charger5","1",translate("电桩5"))
s5.addremove = false
s5.anonymous = false

    cid5 = s5:option(Value,"CID",translate("CID:"))

    start5 = s5:option(Button, "start", translate("抄表"))
    start5.inputstyle = "apply"
    start5.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
    	xuci:set("chargerinfo","SERVER","CID",charger5cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time5 = s5:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time5.cfgvalue = function(self,section)
	local t5
	t5 = xuci:get("chargerinfo","charger5","CB_END_TIME")
	return os.date("%c",t5)
    end

    num5 = s5:option(DummyValue,"CB_NUM","上传充电记录数量:")

----charger6----

s6 = m:section(NamedSection,"charger6","1",translate("电桩6"))
s6.addremove = false
s6.anonymous = false

    cid6 = s6:option(Value,"CID",translate("CID:"))

    start6 = s6:option(Button, "start", translate("抄表"))
    start6.inputstyle = "apply"
    start6.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
    	xuci:set("chargerinfo","SERVER","CID",charger6cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time6 = s6:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time6.cfgvalue = function(self,section)
	local t6
	t6 = xuci:get("chargerinfo","charger6","CB_END_TIME")
	return os.date("%c",t6)
    end

    num6 = s6:option(DummyValue,"CB_NUM","上传充电记录数量:")

----charger7----

s7 = m:section(NamedSection,"charger7","1",translate("电桩7"))
s7.addremove = false
s7.anonymous = false

    cid7 = s7:option(Value,"CID",translate("CID:"))

    start7 = s7:option(Button, "start", translate("抄表"))
    start7.inputstyle = "apply"
    start7.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
    	xuci:set("chargerinfo","SERVER","CID",charger7cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time7 = s7:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time7.cfgvalue = function(self,section)
	local t7
	t7 = xuci:get("chargerinfo","charger7","CB_END_TIME")
	return os.date("%c",t7)
    end

    num7 = s7:option(DummyValue,"CB_NUM","上传充电记录数量:")

----charger8----

s8 = m:section(NamedSection,"charger8","1",translate("电桩8"))
s8.addremove = false
s8.anonymous = false
    cid8 = s8:option(Value,"CID",translate("CID:"))

    start8 = s8:option(Button, "start", translate("抄表"))
    start8.inputstyle = "apply"
    start8.write = function(self, section)
	if ( tonumber(xuci:get("chargerinfo","SERVER","START_TIME")) ==0 )then
		xuci:set("chargerinfo","SERVER","START_TIME",firsttime)
		xuci:commit("chargerinfo")
	end
    	xuci:set("chargerinfo","SERVER","CID",charger8cid)
	xuci:set("chargerinfo","SERVER","CMD",1)
	xuci:commit("chargerinfo")
    end

    time8 = s8:option(DummyValue,"CB_END_TIME","成功抄表时间:")
    time8.cfgvalue = function(self,section)
	local t8
	t8 = xuci:get("chargerinfo","charger8","CB_END_TIME")
	return os.date("%c",t8)
    end

    num8 = s8:option(DummyValue,"CB_NUM","上传充电记录数量:")

return m
