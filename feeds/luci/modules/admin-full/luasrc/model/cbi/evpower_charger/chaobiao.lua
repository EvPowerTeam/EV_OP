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

local m

m = Map("chargerinfo",translate("远程抄表"),translate("你可以通过下面界面看到每台电桩的抄表状态、成功抄表时间和上传充电记录数量"))

----current----

cur = m:section(TypedSection,"tab",translate("电桩设定"))
cur.addremove = false
cur.anonymous = true

charcur = cur:option(Value,"maxcurrent",translate("电流值"))

----chargernum----

charnum = cur:option(Value,"chargernum",translate("电桩台数"))

----chargerversion----

charver = cur:option(Value,"chargerversion",translate("电桩版本"))

----charger1----

s1 = m:section(NamedSection,"charger1","1",translate("电桩1"))
s1.addremove = false
s1.anonymous = false
--s.template = "cbi/tblsection"


cid1 = s1:option(Value,"CID",translate("CID:"))

start1 = s1:option(Button, "start", translate("抄表"))
start1.inputstyle = "apply"
start1.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger1.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end1 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end1 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t1 = m:section(TypedSection,"issued","")
t1.addremove = false
t1.anonymous = true

time1 = t1:option(DummyValue,"END_TIME","成功抄表时间:")

num1 = t1:option(DummyValue,"chargernumber","上传充电记录数量:")

----charger2----

s2 = m:section(NamedSection,"charger2","1",translate("电桩2"))
s2.addremove = false
s2.anonymous = false

cid2 = s2:option(Value,"CID",translate("CID:"))

start2 = s2:option(Button, "start", translate("抄表"))
start2.inputstyle = "apply"
start2.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger2.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end2 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end2 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t2 = m:section(TypedSection,"issued","")
t2.addremove = false
t2.anonymous = true

time2 = t2:option(DummyValue,"END_TIME","成功抄表时间:")

num2 = t2:option(DummyValue,"chargernumber","上传充电记录数量:")


----charger3----

s3 = m:section(NamedSection,"charger3","1",translate("电桩3"))
s3.addremove = false
s3.anonymous = false

cid3 = s3:option(Value,"CID",translate("CID:"))

start3 = s3:option(Button, "start", translate("抄表"))
start3.inputstyle = "apply"
start3.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger3.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end3 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end3 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t3 = m:section(TypedSection,"issued","")
t3.addremove = false
t3.anonymous = true

time3 = t3:option(DummyValue,"END_TIME","成功抄表时间:")

num3 = t3:option(DummyValue,"chargernumber","上传充电记录数量:")

----charger4----

s4 = m:section(NamedSection,"charger4","1",translate("电桩4"))
s4.addremove = false
s4.anonymous = false

cid4 = s4:option(Value,"CID",translate("CID:"))

start4 = s4:option(Button, "start", translate("抄表"))
start4.inputstyle = "apply"
start4.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger4.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end4 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end4 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t4 = m:section(TypedSection,"issued","")
t4.addremove = false
t4.anonymous = true

time4 = t4:option(DummyValue,"END_TIME","成功抄表时间:")

num4 = t4:option(DummyValue,"chargernumber","上传充电记录数量:")

----charger5----

s5 = m:section(NamedSection,"charger5","1",translate("电桩5"))
s5.addremove = false
s5.anonymous = false

cid5 = s5:option(Value,"CID",translate("CID:"))

start5 = s5:option(Button, "start", translate("抄表"))
start5.inputstyle = "apply"
start5.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger5.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end5 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end5 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t5 = m:section(TypedSection,"issued","")
t5.addremove = false
t5.anonymous = true

time5 = t5:option(DummyValue,"END_TIME","成功抄表时间:")

num5 = t5:option(DummyValue,"chargernumber","上传充电记录数量:")


----charger6----

s6 = m:section(NamedSection,"charger6","1",translate("电桩6"))
s6.addremove = false
s6.anonymous = false

cid6 = s6:option(Value,"CID",translate("CID:"))

start6 = s6:option(Button, "start", translate("抄表"))
start6.inputstyle = "apply"
start6.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger6.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end6 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end6 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t6 = m:section(TypedSection,"issued","")
t6.addremove = false
t6.anonymous = true

time6 = t6:option(DummyValue,"END_TIME","成功抄表时间:")

num6 = t6:option(DummyValue,"chargernumber","上传充电记录数量:")


----charger7----

s7 = m:section(NamedSection,"charger7","1",translate("电桩7"))
s7.addremove = false
s7.anonymous = false

cid7 = s7:option(Value,"CID",translate("CID:"))

start7 = s7:option(Button, "start", translate("抄表"))
start7.inputstyle = "apply"
start7.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger7.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end7 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end7 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t7 = m:section(TypedSection,"issued","")
t7.addremove = false
t7.anonymous = true

time7 = t7:option(DummyValue,"END_TIME","成功抄表时间:")

num7 = t7:option(DummyValue,"chargernumber","上传充电记录数量:")


----charger8----

s8 = m:section(NamedSection,"charger8","1",translate("电桩8"))
s8.addremove = false
s8.anonymous = false

cid8 = s8:option(Value,"CID",translate("CID:"))

start8 = s8:option(Button, "start", translate("抄表"))
start8.inputstyle = "apply"
start8.write = function(self, section)
	luci.sys.call("uci set chargerinfo.SERVER.CID=`uci get chargerinfo.charger8.CID`; uci set chargerinfo.SERVER.CMD=1; uci commit chargerinfo")
	luci.sys.call("sh /root/abc")
end

local end8 = luci.sys.exec("uci get chargerinfo.CLIENT.STATUS")

if ( end8 == 1 )
then
	luci.sys.exec("uci set chargerinfo.SERVER.CMD=3;uci commit chargerinfo")
end

t8 = m:section(TypedSection,"issued","")
t8.addremove = false
t8.anonymous = true

time8 = t8:option(DummyValue,"END_TIME","成功抄表时间:")

num8 = t8:option(DummyValue,"chargernumber","上传充电记录数量:")


return m
