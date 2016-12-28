
uci = uci.cursor()

m = Map("curlfile","")

s = m:section(TypedSection,"CURL","电桩配置文件")
s.addremove = false
s.anonymous = true

--conf1 = s:option(Value,"SURL","URL:")
--conf2 = s:option(Value,"code","Code:")
--conf3 = s:option(Value,"userkey","Key:")
conf4 = s:option(Value,"chargerid","CID:")

start = s:option(Button,"start","下载配置文件")
start.inputstyle = "apply"
start.write = function(self,section)
	if ( tonumber(uci:get("curlfile","CURL","chargerid")) ~= 0) then
		local s1 = uci:get("curlfile","curlconf","SURL")
		local s2 = uci:get("curlfile","curlconf","code")
		local s3 = uci:get("curlfile","curlconf","userkey")
		local s4 = uci:get("curlfile","curlconf","chargerid")
		
		local res = os.execute("curl -d ".."'".."code="..s2.."&userKey="..s3.."&chargerID="..s4.."' "..s1.." > /mnt/umemory/power_bar/CONFIG/"..s4)
	end
end

return m
