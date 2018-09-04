--print("Hello World!")
--log_debug("Hello World!");

key = ""
function PrintTable(table, level)
	level = level or 1
	local indent = ""
	for i = 1, level do
		indent = indent.."  "
	end

	if key ~= "" then
		print(indent..key.." ".."=".." ".."{")
	else
		print(indent .. "{")
	end

	key = ""
	for k,v in pairs(table) do
		if type(v) == "table" then
			key = k
			PrintTable(v, level + 1)
		else
			local context = string.format("%s%s = %s", indent .. "  ", tostring(k), tostring(v))
		print(context)
		end
	end
	print(indent .. "}")
end
--[[
mysql_wrapper.connect("127.0.0.1", 3306, "class_data", "root", "null980311", function(err, context)
	log_debug("mysql cmd call event");
	if(err) then
		print(err)
		return
	end

	mysql_wrapper.query(context, "select * from studentinfo", function(err, ret)
		if err then
			print(err)
			return;
		end
		print("success")
		PrintTable(ret)
	end)

	mysql_wrapper.query(context, "insert into from student", function(err, ret)
		if err then
			print(err)
			return;
		end
		print("success")
	end)
	--mysql_wrapper.close_mysql(context);
end)
]]
redis_wrapper.connect("127.0.0.1", 6379, function(err, context)
	print("redis cmd call event")
	if err then
		print(err)
		return
	end

	print("redis db start success")
--[[
	redis_wrapper.query(context, "hamset 001001 name \"blake\" age \"34\"", function (err, result)
   		if err then 
   			print(err)
    		return
    	end
    	print(result)
	  end);
	  ]]

	redis_wrapper.query(context, "hgetall 001001", function(err, result)
		if err then
			print(err)
			return
		end
		print(result)
		PrintTable(result)
	end)
	--redis_wrapper.close_redis(context)
end)