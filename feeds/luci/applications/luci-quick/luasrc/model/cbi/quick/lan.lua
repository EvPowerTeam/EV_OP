local fs = require "nixio.fs"
local ut = require "luci.util"
local pt = require "luci.tools.proto"
local nw = require "luci.model.network"

m = Map("network", translate("LAN"), translate("Configure Local Area Network"))

nw.init(m.uci)

arg = {}
arg[1] = "lan"
local net = nw:get_network(arg[1])

s = m:section(NamedSection, "lan", "interface", "")

p = s:option(ListValue, "proto", translate("Protocol"))
local _, pr                                                                                             
for _, pr in ipairs(nw:get_protocols()) do                                           
        local strt, ed                                                                
        strt, ed = string.find(tostring(pr:proto()), "static")                                    
        if strt and ed then                                                                             
                p:value(pr:proto(), pr:get_i18n())                                                      
        end                                                                          
                                                                                                                                           
--      p:value(pr:proto(), pr:get_i18n().."("..tostring(pr:proto())..")")                              
--      if pr:proto() ~= net:proto() then                                                               
--              --p_switch:depends("proto", pr:proto())                                                                                    
--      end                                                                           
end     


ipaddr = s:option(Value, "ipaddr", translate("IPv4 address"))
ipaddr.datatype = "ip4addr"
ipaddr.rmempty = false


netmask = s:option(Value, "netmask", translate("IPv4 netmask"))  
netmask.datatype = "ip4addr"
netmask:value("255.255.255.0")
netmask:value("255.255.0.0")
netmask:value("255.0.0.0")



--
-- Display DNS settings if dnsmasq is available
--
local has_dnsmasq  = fs.access("/etc/config/dhcp")
if has_dnsmasq and net:proto() == "static" then
	m2 = Map("dhcp", "", "")

	local has_section = false

	m2.uci:foreach("dhcp", "dhcp", function(s)
		if s.interface == arg[1] then
			has_section = true
			return false
		end
	end)

	if not has_section and has_dnsmasq then

		s = m2:section(TypedSection, "dhcp", translate("DHCP Server"))
		s.anonymous   = true
		s.cfgsections = function() return { "_enable" } end

		x = s:option(Button, "_enable")
		x.title      = translate("No DHCP Server configured for this interface")
		x.inputtitle = translate("Setup DHCP Server")
		x.inputstyle = "apply"

	elseif has_section then

		s = m2:section(TypedSection, "dhcp", translate("DHCP Server"))
		s.addremove = false
		s.anonymous = true

		function s.filter(self, section)
			return m2.uci:get("dhcp", section, "interface") == arg[1]
		end

		local ignore = s:option(Flag, "ignore",
			translate("Ignore interface"),
			translate("Disable <abbr title=\"Dynamic Host Configuration Protocol\">DHCP</abbr> for " ..
				"this interface."))

		local start = s:option(Value, "start", translate("Start"),
			translate("Lowest leased address as offset from the network address."))
		start.optional = true
		start.datatype = "or(uinteger,ip4addr)"
		start.default = "100"

		local limit = s:option(Value, "limit", translate("Limit"),
			translate("Maximum number of leased addresses."))
		limit.optional = true
		limit.datatype = "uinteger"
		limit.default = "150"

		local ltime = s:option(Value, "leasetime", translate("Leasetime"),
			translate("Expiry time of leased addresses, minimum is 2 minutes (<code>2m</code>)."))
		ltime.rmempty = true
		ltime.default = "12h"

		local dd = s:option(Flag, "dynamicdhcp",
			translate("Dynamic <abbr title=\"Dynamic Host Configuration Protocol\">DHCP</abbr>"),
			translate("Dynamically allocate DHCP addresses for clients. If disabled, only " ..
				"clients having static leases will be served."))
		dd.default = dd.enabled

		--s:option(Flag, "force", translate("Force"),
		--	translate("Force DHCP on this network even if another server is detected."))

		-- XXX: is this actually useful?
		--s:taboption("advanced", Value, "name", translate("Name"),
		--	translate("Define a name for this network."))


		for i, n in ipairs(s.children) do
			if n ~= ignore then
				n:depends("ignore", "")
			end
		end

	else
		m2 = nil
	end
end






return m, m2
