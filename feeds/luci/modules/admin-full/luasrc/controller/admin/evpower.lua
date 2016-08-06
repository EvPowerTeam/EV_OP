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
	entry({"admin","evpower","chargerconf"},call("action_chargerconf"))
	entry({"admin","evpower","confcomp"},call("action_confcomp"))
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
	if setconf ~= nil and setconf > 0 
	then
		x:set("chargerinfo","SERVER","CMD",'3');
		x:commit("chargerinfo");
		x:set("chargerinfo","SERVER","CID",setconf);
		x:commit("chargerinfo");
	end
	
end
