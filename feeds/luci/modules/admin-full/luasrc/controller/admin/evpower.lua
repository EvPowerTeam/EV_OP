module("luci.controller.admin.evpower",package.seeall)

function index()
		entry({"admin","evpower"},firstchild(),"EV SQL",30).dependent=false
		entry({"admin","evpower","tab_from_cbi"},call("action_evsqlconf"),translate("Charging Station"),1).leaf=true
		end

function action_evsqlconf()
		if not nixio.fs.access("/etc/config/chargerinfo") then
		return
		end

		local info = nixio.fs.readfile("/etc/config/chargerinfo")
		luci.template.render("evsqlconf",{info=info})
end

