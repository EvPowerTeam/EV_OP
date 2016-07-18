module("luci.controller.admin.evpower",package.seeall)

function index()
	entry({"admin","evpower"},alias("admin","evpower","overview"),_("电桩管理"),30).index = true
	entry({"admin","evpower","overview"},call("action_evsqlconf"),_("总览"),1)
	entry({"admin","evpower","overview","shuru"},call("action_chargerinput"))
	entry({"admin","evpower","upgrade"},alias("admin","system","flashops"),_("更新电桩固件"),2)
	entry({"admin","evpower","reading"},cbi("evpower_charger/chaobiao"),_("远程抄表"),3)
	entry({"admin","evpower","balancing"},alias("admin","evpower","overview"),"电桩负载均衡",4)
	entry({"admin","evpower","chargerdata"},alias("admin","evpower","overview"),"电桩数据总览",5)
	entry({"admin","evpower","pushconf"},alias("admin","evpower","overview"),"推送配置",6)
end

function action_evsqlconf()
	if not nixio.fs.access("/etc/config/chargerinfo") then
		return
	end

	local info = nixio.fs.readfile("/etc/config/chargerinfo")
	luci.template.render("evpower_charger/evsqlconf",{info=info})
end

function action_chargerinput()
	if luci.http.formvalue("rid_flag") == "1" then
		luci.sys.exec("echo 111 > qqq")
		local saverid=luci.http.formvalue("is_rid")
		local file=io.open("/root/ceshi","w")
		file:write(saverid)
		return
	end
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

			local path = "/tmp/"..field.file

			if not fd then
				fd = io.open(path,"w")
			end
			
			fd:write(chunk)

			if eof and fd then
				fd:close()
				fd = nil

				h.write("<br />Upload Success")
			end
		end
	)
	
	if h.formvalue("act") == "update" then
		return
	end
end

