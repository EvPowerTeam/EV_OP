
m = Map("network", translate("3G/4G"), translate("Configure 3G/4G Modems"))

local nw = require "luci.model.network"  
local ntm = nw.init(m.uci)
local fwm = require "luci.model.firewall".init(m.uci)

local net
local ifaces = { }
local netlist = { }
for _, net in ipairs(ntm:get_networks()) do
	if net:proto() == "3g" then
		local z = fwm:get_zone_by_network(net:name())
		ifaces[#ifaces+1] = net:name()
		netlist[#netlist+1] = {
			net:name(), z and z:name() or "-", z
		}
	end
end



---------------status---------------------
--m.pageaction = false
--m:section(SimpleSection).template = "admin_network/iface_overview"
m:section(SimpleSection).template = "quick/iface3g_overview"         

--[[
m2 = Map("dhcp", translate("Status"), translate("Configure 3G/4G Modems")) 
sc = m2:section(TypedSection, "dhcp", translate("Status"), translate("all modems status")) 
sc.anonymous = true
st = sc:option(DummyValue, "__status", "")                                                                                   
local function set_status()                                                                                                                     
                st.template = "quick/iface_overview"           
	                                                                                 
end                                                                                                                                             

m2.on_init = set_status
m2.on_after_save = set_status
--]]



local str = luci.sys.exec("ls -l /sys/class/tty") 

s = {}
for i, net in ipairs(netlist) do  
--        proto:value(tostring(net[1]), tostring(net[1]:upper())) 
	s[#s+1] = m:section(NamedSection, net[1], "interface", net[1]:upper())
		
	proto = s[#s]:option(ListValue, "proto", translate("Protocol"))
	proto:value("3g", "3G")

	dev = s[#s]:option(Value, "device", translate("Modem device"))
	dev.rmempty = false

	cdev = s[#s]:option(Value, "atdevice", translate("AT device"), translate("to receive modem's additional information"))
        cdev.rmempty = false

	local device_suggestions = nixio.fs.glob("/dev/ttyUSB*")
	or nixio.fs.glob("/dev/tty[A-Z]*")
	or nixio.fs.glob("/dev/tts/*")

	if device_suggestions then          
                local node   
                local str1    
                for node in device_suggestions do   
                        str1 = string.sub(node, 6, -1) or "node"                   
                        str1 = string.format("%%s%s%%s.-%s", str1, str1) 
                        str1 = string.match(str, str1) or "str"        
                        --str1 = string.match(str1, "%d%-%d[^/]-/tty") or "str1"   
                        str1 = string.match(str1, "%d%-%d[^/]-:") or "str1"    
                        str1 = string.format("%s -> %s", node, string.sub(str1, 1, -2) or "")    
                        dev:value(node, str1)  
                        cdev:value(node, str1)  
                end   
        end    

--[[	if device_suggestions then
		local node
		for node in device_suggestions do
			dev:value(node)
		end
	end
--]]
	service = s[#s]:option(Value, "service", translate("Service Type"))
	service:value("umts", "UMTS/GPRS")
	service:value("umts_only", translate("UMTS only"))
	service:value("gprs_only", translate("GPRS only"))
	--service:value("evdo", "CDMA/EV-DO")

	apn = s[#s]:option(Value, "apn", translate("APN"))
	apn:value("3gnet", translate("China unicom").." - 3gnet")
	apn:value("cmnet", translate("China mobile").." - cmnet")
	apn:value("cmwap", translate("China mobile").." - cmwap")
	apn:value("ctnet", translate("China telecom").." - ctnet")

	dial = s[#s]:option(Value, "dialnumber", translate("Dial number"))
	dial:value("*99#", translate("China unicom"))
	dial:value("*99***1#", translate("China mobile"))
	dial:value("#777", translate("China telecom"))
	

	pincode = s[#s]:option(Value, "pincode", translate("PIN"))

	--username = s:option(Value, "username", translate("PAP/CHAP username"))

	--password = s:option(Value, "password", translate("PAP/CHAP password"))
	--password.password = true

end  


return m
