module("luci.controller.admin.evpower",package.seeall)

function index()
	entry({"admin","evpower"},alias("admin","evpower","overview"),_("电桩管理"),30).index = true
	entry({"admin","evpower","overview"},call("action_evsqlconf"),_("总览"),1)
	entry({"admin","evpower","upgrade"},template("view_evpower_charger/chargerupgr"),_("更新电桩固件"),2)
	entry({"admin","evpower","chargerfw"},call("action_chargerexist"))
	entry({"admin","evpower","fwuptime"},call("action_fwuptime"))
	entry({"admin","evpower","reading"},cbi("cbi_evpower_charger/chaobiao"),_("远程抄表"),3)
	entry({"admin","evpower","balancing"},cbi("cbi_evpower_charger/config"),"电桩负载均衡",4)
	entry({"admin","evpower","chargerdata"},alias("admin","evpower","overview"),"电桩充电数据总览",5)
	entry({"admin","evpower","pushconf"},template("view_evpower_charger/chargerconf"),"推送配置",6)
	entry({"admin","evpower","vpninfo"},template("view_evpower_charger/vpn_info"),_("VPN账号/密码"),7)
	entry({"admin","evpower","vpnlog"}, call("action_vpnlog"), _("VPN Log"), 8)
	entry({"admin","evpower","curlfile"},cbi("cbi_evpower_charger/curlfile"),"下载电桩配置文件",9)
	entry({"admin","evpower","vpninfo_button"},call("action_vpninfo_button"))
	entry({"admin","evpower","vpn_status"},call("action_vpnstatus"))
	entry({"admin","evpower","vpn_status1"},call("action_vpnstatus1"))
	entry({"admin","evpower","chargerconf"},call("action_chargerconf"))
	entry({"admin","evpower","confcomp"},call("action_confcomp"))
end

function action_vpninfo_button()
	if luci.http.formvalue("vpninfo_flag") == "1" then
		luci.sys.exec("/bin/sh /root/setup_9qu.sh > /tmp/vpn.log 2>&1")
	end
end

function action_vpnlog()
	local vpnlog = luci.sys.vpnlog()
	luci.template.render("view_evpower_charger/vpninfo",{vpnlog=vpnlog})
end

function action_vpnstatus()
	local vpnval = {}
	vpnval.vpnname = luci.sys.exec("sed -n \"/name/p\" /root/9qu.xl2tpd | awk '{ print $2 }' | cut -d '\"' -f 2 ")
	vpnval.vpnpasswd = luci.sys.exec("sed -n \"/password/p\" /root/9qu.xl2tpd | awk '{ print $2 }' | cut -d '\"' -f 2 ")
	luci.http.prepare_content("application/json")
	luci.http.write_json(vpnval)
end

function action_vpnstatus1()
	if luci.http.formvalue("vpn_flag") == "1" then
		local savevpn=luci.http.formvalue("is_vpn")

		local file = io.open("/root/9qu.xl2tpd","r")
		local str = file:read("*a")
		local v1,v2 = string.find(str,"name \"")
		local v3,v4 = string.find(str,"password")
		local str1 = string.sub(str,v2+1,v3-3)
		local newStr = string.gsub(str,str1,savevpn)

		file:close()

		local file1 = io.open("/root/9qu.xl2tpd","w")
		file1:write(newStr)
		local file2 = io.open("/etc/ppp/9qu.xl2tpd","w")
		file2:write(newStr)
		
	end
end

function action_evsqlconf()
	if not nixio.fs.access("/etc/config/chargerinfo") then
		return
	end

	local info = nixio.fs.readfile("/etc/config/chargerinfo")
	luci.template.render("view_evpower_charger/evsqlconf",{info=info})
end

function action_chargerexist()
	luci.sys.call("rm /mnt/umemory/power_bar/UPDATE/*")
	action_chargerfw();
end

function action_chargerfw()
	local h    = require"luci.http"
	local io   = require"nixio"
	local flag = true
	local run  = true
	local fd   = nil	
	
	h.setfilehandler(
		function(field,chunk,eof)
			if not field or not run then return end

			if flag then
				h.write("Uploading")
				flag = false
			end

			local path = "/mnt/umemory/power_bar/UPDATE/"..field.file

			if not fd then
				fd = io.open(path,"w")
			end
			
			fd:write(chunk)

			if eof and fd then
				fd:close()
				fd = nil

				h.write("<br />Upload Success")
				h.write("<script>location.href='upgrade'</script>")
			end
		end
	)
	
	if h.formvalue("act") == "update" then
		return (0 == luci.sys.call("uci set chargerinfo.SERVER.CMD=5;uci commit chargerinfo"))
	end
end

function action_fwuptime()
	local is_time = tonumber(luci.http.formvalue("is_time"))
	local h       = require"luci.http"
	if is_time ~= nil and is_time > 0
	then
		if ( 0 ~= luci.sys.call("cat /root/is_uptime") )
		then
			luci.sys.exec("touch /root/is_uptime")
		end
		local timefile=io.open("/root/is_uptime","w")
		timefile:write(is_time)
		
		local cnum = luci.sys.exec("uci get chargerinfo.TABS.tables | tr -cd '[0-9]' | wc -m")
		if ( 1 == cnum)
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 2 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 3 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger3.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 4 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger3.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger4.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 5 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger3.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger4.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger5.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 6 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger3.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger4.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger5.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger6.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 7 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger3.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger4.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger5.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger6.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger7.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		elseif ( 8 == cnum )
		then		
			luci.sys.exec("uci set chargerinfo.charger1.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger2.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger3.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger4.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger5.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger6.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger7.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
			luci.sys.exec("uci set chargerinfo.charger8.FW_END_TIME=`cat /root/is_uptime`;uci commit chargerinfo")
		end		
	end
end

function action_chargerconf()
	local h    = require"luci.http"
	local io   = require"nixio"
	local flag = true
	local run  = true
	local fd   = nil
	
	h.setfilehandler(
		function(field,chunk,eof)
			if not field or not run then return end
			
			if flag then
				h.write("Uploading")
				flag = false
			end

			local path = "/mnt/umemory/power_bar/CONFIG/"..field.file
			
			if not fd then
				fd = io.open(path,"w")
			end

			fd:write(chunk)

			if eof and fd then
				fd:close()
				fd = nil

				h.write("<br />Upload Success")
				h.write("<script>location.href='pushconf?uploaded=1'</script>")
			end
		end
	)

	if h.formvalue("act") == "upconf" 
	then
		return
	end
end

function action_confcomp()
	require("uci");
	x = uci.cursor()
	local setconf = tonumber(luci.http.formvalue("is_cid"))
	luci.sys.exec("/bin/sh /root/curlfile"..setconf)
	if setconf ~= nil and setconf > 0 
	then
		x:set("chargerinfo","SERVER","CMD",'3');
		x:commit("chargerinfo");
		x:set("chargerinfo","SERVER","CID",setconf);
		x:commit("chargerinfo");
	end
	
end
