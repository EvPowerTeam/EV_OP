module("luci.controller.admin.evpower",package.seeall)

function index()
	entry({"admin","evpower"},alias("admin","evpower","overview"),_("Charger  Administration"),30).index = true
	entry({"admin","evpower","overview"},call("action_evsqlconf"),_("Overview"),1)
	entry({"admin","evpower","upgrade"},alias("admin","system","flashops"),_("Remote Update Firmware"),2)
	--entry({"admin","evpower","upgrade"},call("action_chargerfw"),_("Remote Update Firmware"),2)
	--entry({"admin","evpower","chargerfw"},form("evpower_charger/chargupg"))
	entry({"admin","evpower","reading"},form("evpower_charger/chaobiao"),_("Remote Meter Reading"),3)
	entry({"admin","evpower","balancing"},call("action_evsqlconf"),"Charger Load Balancing",4)
	entry({"admin","evpower","chargerdata"},call("action_evsqlconf"),"Charger Data Overview",5)
	entry({"admin","evpower","pushconf"},call("action_evsqlconf"),"Push Configuration",6)
end

function action_evsqlconf()
	if not nixio.fs.access("/etc/config/chargerinfo") then
		return
	end

	local info = nixio.fs.readfile("/etc/config/chargerinfo")
	luci.template.render("evpower_charger/evsqlconf",{info=info})
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

