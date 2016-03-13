
local nw = require "luci.model.network"

m = Map("network", translate("VPN"), translate("Virtual Private Network"))

nw.init(m.uci)

local network = "vpn"
local net = nw:get_network(network)
if not net then return m; end

s = m:section(NamedSection, "vpn", "interface", "VPN")



proto = s:option(ListValue, "proto", translate("Protocol"))

local _, pr  
for _, pr in ipairs(nw:get_protocols()) do    
        local strt, ed     
        strt, ed = string.find(tostring(pr:proto()), "pptp")      
        if strt and ed then         
                proto:value(pr:proto(), pr:get_i18n())      
        end          
end


server = s:option(Value, "server", translate("VPN Server"))
server.datatype = "host"
username = s:option(Value, "username", translate("PAP/CHAP username"))
password = s:option(Value, "password", translate("PAP/CHAP password"))
password.password = true



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
		st.network  = network
		st.value    = nil
	end
end

m.on_init = set_status
m.on_after_save = set_status



return m
