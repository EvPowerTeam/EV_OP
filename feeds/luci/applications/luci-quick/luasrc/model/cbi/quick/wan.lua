
local fs = require "nixio.fs"
local ut = require "luci.util"
local pt = require "luci.tools.proto"
local nw = require "luci.model.network"
local fw = require "luci.model.firewall"


m = Map("network", translate("WAN"), translate("Configure Wide Area Network"))

nw.init(m.uci)

arg = {}
arg[1] = "wan"
local net = nw:get_network(arg[1])


--s = m:section(NamedSection, "wan", "interface", translate("WAN Configuration"))
s = m:section(NamedSection, "wan", "interface", "")



--------------------Protocol---------------------------
p = s:option(ListValue, "proto", translate("Protocol")) 
--p.default = net:proto()   
    
local _, pr  
for _, pr in ipairs(nw:get_protocols()) do    
        local strt, ed     
        strt, ed = string.find(tostring(pr:proto()), "dhcp")      
        if strt and ed then         
                p:value(pr:proto(), pr:get_i18n())      
        end  
        strt, ed = string.find(tostring(pr:proto()), "pppoe")     
        if strt and ed then  
                p:value(pr:proto(), pr:get_i18n())    
     end      
              
            
--      p:value(pr:proto(), pr:get_i18n().."("..tostring(pr:proto())..")")   
--      if pr:proto() ~= net:proto() then  
--              --p_switch:depends("proto", pr:proto())  
--      end                        
end  

ifname = s:option(ListValue, "ifname", translate("Interface"))
ifname:value("eth1", "eth1(wan)")
-----------------protocol-dhcp------------------------
--ifname:depends("proto", "dhcp")
--ifname = s:option(ListValue, "_orig_ifname", translate("Interface"))
--bridge = s:option(ListValue, "_orig_bridge", translate("Bridge interfaces"))
--ifname:value("eth1", "eth1")
--bridge:value("false", "false")
--ifname:depends("proto", "dhcp")
--bridge:depends("proto", "dhcp")
----------------protocol--pppoe------------------------------

local username, password, ac, service
local ipv6, defaultroute, metric, peerdns, dns,
      keepalive_failure, keepalive_interval, demand, mtu

username = s:option(Value, "username", translate("PAP/CHAP username"))
password = s:option(Value, "password", translate("PAP/CHAP password"))
password.password = true
username:depends("proto", "pppoe")
password:depends("proto", "pppoe")


keepalive_failure = s:option(Value, "_keepalive_failure",
	translate("LCP echo failure threshold"),
	translate("Presume peer to be dead after given amount of LCP echo failures, use 0 to ignore failures"))

function keepalive_failure.cfgvalue(self, s)
	local v = m:get(s, "keepalive")
	if v and #v > 0 then
		return tonumber(v:match("^(%d+)[ ,]+%d+") or v)
	end
end

function keepalive_failure.write() end
function keepalive_failure.remove() end

keepalive_failure.placeholder = "0"
keepalive_failure.datatype    = "uinteger"

keepalive_failure:depends("proto", "pppoe") 



keepalive_interval = s:option(Value, "_keepalive_interval",
	translate("LCP echo interval"),
	translate("Send LCP echo requests at the given interval in seconds, only effective in conjunction with failure threshold"))

function keepalive_interval.cfgvalue(self, s)
	local v = m:get(s, "keepalive")
	if v and #v > 0 then
		return tonumber(v:match("^%d+[ ,]+(%d+)"))
	end
end

function keepalive_interval.write(self, s, value)
	local f = tonumber(keepalive_failure:formvalue(s)) or 0
	local i = tonumber(value) or 5
	if i < 1 then i = 1 end
	if f > 0 then
		m:set(s, "keepalive", "%d %d" %{ f, i })
	else
		m:del(s, "keepalive")
	end
end

keepalive_interval.remove      = keepalive_interval.write
keepalive_interval.placeholder = "5"
keepalive_interval.datatype    = "min(1)"

keepalive_interval:depends("proto", "pppoe") 



demand = s:option(Value, "demand",
	translate("Inactivity timeout"),
	translate("Close inactive connection after the given amount of seconds, use 0 to persist connection"))
demand.placeholder = "0"
demand.datatype    = "uinteger"
demand:depends("proto", "pppoe") 


mtu = s:option(Value, "mtu", translate("Override MTU"))
mtu.placeholder = "1500"
mtu.datatype    = "max(9200)"
mtu:depends("proto", "pppoe") 







-------------------------------Status------------------------------
st = s:option(DummyValue, "__status", translate("Status"))

local function set_status()
	-- if current network is empty, print a warning
	if not net:is_floating() and net:is_empty() then
		st.template = "cbi/dvalue"
		st.network  = nil
		st.value    = translate("There is no device assigned yet, please attach a network device in the \"Physical Settings\" tab")
	else
		st.template = "admin_network/iface_status"
		st.network  = arg[1]
		st.value    = nil
	end
end

m.on_init = set_status
m.on_after_save = set_status









return m

