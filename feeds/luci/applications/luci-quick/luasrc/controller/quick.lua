module("luci.controller.quick", package.seeall)

function index()

--entry({"admin", "quick"}, call("preview"), _("Quick"), 20)
--arcombine(cbi("admin_network/network"), cbi("admin_network/ifaces"))



--entry({"admin", "quick", "wan"}, cbi("quick/wan"), _("WAN"), 1).dependent = false
--entry({"admin", "quick", "lan"}, cbi("quick/lan"), _("LAN"), 2).dependent = false
--entry({"admin", "quick", "wifi"}, cbi("quick/wifi"), _("WiFi"), 3).dependent = false

--entry({"admin", "quick"}, cbi("quick/overview"), _("Quick"), 20).dependent = true
entry({"admin", "quick"}, cbi("quick/overview"), _("Quick"), 20).index = true
entry({"admin", "quick", "wan"}, cbi("quick/wan"), _("WAN Configuration"), 1)
entry({"admin", "quick", "lan"}, cbi("quick/lan"), _("LAN Configuration"), 2)
entry({"admin", "quick", "wifi"}, cbi("quick/wifi"), _("WiFi Configuration"), 3)
entry({"admin", "quick", "wwan"}, cbi("quick/wwan"), _("3G/4G Configuration"), 4)

entry({"admin", "quick", "vpn"}, cbi("quick/vpn"), _("VPN Configuration"), 5)

page = entry({"admin","quick","wwan","evlink"},call("evlink_status"))
page.leaf = true

page = entry({"admin","quick","wwan","evlink1"},call("evlinkrid"))
page.leaf = true

page = entry({"admin", "quick", "wwan", "status"}, call("info_3gnet"))
page.leaf = true
page = entry({"admin", "quick", "wwan", "reset"}, call("g3net_reset"))
page.leaf = true


end

function evlink_status()
	local stat = {}
	stat.heartbeat = luci.sys.exec("cat /tmp/saveHB")
	stat.id = luci.sys.exec("cat /bin/saveRID")
	luci.http.prepare_content("application/json")
	luci.http.write_json(stat)
end

function evlinkrid()

	--change /bin/saveRID
	if luci.http.formvalue("rid_flag") == "1" then
		local saverid=luci.http.formvalue("is_rid")
		local file=io.open("/bin/saveRID","w")
		file:write(saverid)
		return
	end
end

function g3net_reset(iface)
	if iface then 
		luci.sys.exec(string.format("sh /usr/sbin/g3monitor -reset %s", iface))
	end
end

function info_3gnet()
	
	local ntm = require "luci.model.network".init()
	local fwm = require "luci.model.firewall".init()

	local net, i
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

	table.sort(netlist,
		function(a, b)
			if a[2] ~= b[2] then
				return a[2] < b[2]
			else
				return a[1] < b[1]
			end
		end)

	local rv = {	}
	rv.wwans = { }
	for i, net in ipairs(netlist) do 
		rv.wwans[#rv.wwans+1] = get_info3g(net[1])
		local nett = ntm:get_network(net[1])     
                local device = nett and nett:get_interface() 
                if device then 
                        rv.wwans[#rv.wwans].isup = device:is_up()          
                end
	end


	--rv.wwan1 = get_info3g(netlist[1], "/dev/ttyUSB2")
	--rv.wwan2 = get_info3g(netlist[2], "/dev/ttyUSB6")

	luci.http.prepare_content("application/json")
	luci.http.write_json(rv)
end


function get_info3g(intfce3g, defdev)

        --defdev = defdev or "/dev/ttyUSB2"
        local networkConfig = "/etc/config/network"                                                                  
        local readf = nixio.fs.readfile(networkConfig) or "" 
	if string.match(readf, intfce3g..".-option%sdevice%s\'[^%s]-\'") == "/dev/ttyUSB0" then
		defdev = defdev or "/dev/ttyUSB2"
	end
        local tmp = string.match(readf, intfce3g..".-option%satdevice%s\'[^%s]-\'") or string.format("option atdevice \'%s\'", defdev)
        tmp = string.match(tmp, "option%satdevice%s\'[^%s]-\'") --or "option atdevice \'..defdev..\'"  
        tmp = string.match(tmp, "\'[^%s]-\'") --or string.format("\'%s\'", defdev)                                                      
        local atdev = string.sub(tmp, 2, -2) or defdev   
        
	local wan3g = { ifname = intfce3g }
	local info3g = ""; 
	local extra = "";
	if defdev == atdev then
		extra = string.format("%s\"AT device\" is null\n", extra)
	else
		if ( string.len(luci.sys.exec("cat /usr/sbin/g3command")) > 0 ) then
			--luci.sys.exec(string.format("sh /usr/sbin/g3command %s -signal -sim -regist -operator", atdev)) 
			luci.sys.exec(string.format("sh /usr/sbin/g3command %s -all",atdev)) 
			--fork_exec(string.format("sh /usr/sbin/g3command %s", atdev)) 
			local path = string.format("/var/g3modem/g3info%s", string.match(atdev, "tty.+"))

			local simid = luci.sys.exec(string.format("awk -F: \'/SIMID:.*/{print $2}\' %s", path))
			local signal = luci.sys.exec(string.format("awk -F: \'/SIGNAL:.*/{print $2}\' %s", path))
			local regnet = luci.sys.exec(string.format("awk -F: \'/REGNET:.*/{print $2}\' %s", path))
			local cops = luci.sys.exec(string.format("awk -F: \'/COPS:.*/{print $2}\' %s", path))
			local hcsq = luci.sys.exec(string.format("awk -F: \'/HCSQ:.*/{print $2}\' %s", path))
			if string.len(simid) >= 15 then --have a blank space
				wan3g.sim = "OK"
			else
				wan3g.sim = "ERROR"
			end
			wan3g.signal = signal
			wan3g.network = regnet
			wan3g.operator = cops
			wan3g.hsignal = hcsq
		else 

		info3g = luci.sys.exec(string.format("comgt -d %s -s %s", atdev, "/etc/gcom/getsimid.gcom")) 
		wan3g.sim = string.match(info3g, "OK") or "ERROR"
		info3g = luci.sys.exec(string.format("comgt -d %s -s %s", atdev, "/etc/gcom/getstrength.gcom"))
		local signal = string.match(info3g, "+CSQ: [0-9,]+") --or "+CSQ: 0,0"
		if signal then 
			wan3g.signal = string.sub(signal, 7, -1) 
			info3g = luci.sys.exec(string.format("comgt -d %s -s %s", atdev, "/etc/gcom/getregistcode.gcom"))             
			local network = string.match(info3g, "+CREG: [a-zA-Z0-9,]+") --or "+CREG: 0,0" 
			if network then
				wan3g.network = string.sub(network, 8, -1) 
			else
				extra = string.format("%s%s\n", extra, info3g)
			end 
		else
			extra = string.format("%s%s\n", extra, info3g)
		end
		
		end
	end
	if string.len(extra)>0 then
		wan3g.extra = extra
		--wan3g.extra = string.nl2br(string.htmlspecialchars htmlspecialchars(extra))
	end
	return wan3g
end



function preview()
	luci.http.prepare_content("text/plain")
	luci.http.write("--Show network overwiew---")
end
