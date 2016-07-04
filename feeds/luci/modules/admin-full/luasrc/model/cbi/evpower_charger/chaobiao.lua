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

require "luci.fs"
require "luci.sys"
require "luci.util"

local inits = { }

local cname = {}

local cinfo = luci.model.uci.cursor()

lccid = cinfo:get("chargerinfo","charger1","Model")
lcip  = cinfo:get("chargerinfo","charger1","IP")


m = SimpleForm("chargread", translate("Remote Meter Reading"), translate("You can Reading Meter"))
m.reset = false
m.submit = false


s = m:section(Table, cname)

i = s:option(DummyValue, "lccid", translate("Charger Name"))
n = s:option(DummyValue, "lcip", translate("Execute"))


e = s:option(Button, "endisable", translate("Success Reading Meter Time"))

e.render = function(self, section, scope)
	if inits[section].enabled then
		self.title = translate("Enabled")
		self.inputstyle = "save"
	else
		self.title = translate("Disabled")
		self.inputstyle = "reset"
	end

	Button.render(self, section, scope)
end

e.write = function(self, section)
	if inits[section].enabled then
		inits[section].enabled = false
		return luci.sys.init.disable(inits[section].name)
	else
		inits[section].enabled = true
		return luci.sys.init.enable(inits[section].name)
	end
end


start = s:option(Button, "start", translate("Record Number"))
start.inputstyle = "apply"
start.write = function(self, section)
	luci.sys.call("/etc/init.d/%s %s >/dev/null" %{ inits[section].name, self.option })
end

restart = s:option(Button, "restart", translate("Restart"))
restart.inputstyle = "reload"
restart.write = start.write

stop = s:option(Button, "stop", translate("Stop"))
stop.inputstyle = "remove"
stop.write = start.write

return m
