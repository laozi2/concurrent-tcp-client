local netutil = require "bit"
local netutil = require "netutil"

local htonl = netutil.htonl
local htons = netutil.htons
local unpackint32 = netutil.unpackint32
local unpackint16 = netutil.unpackint16
local packint32 = netutil.packint32
local packint16 = netutil.packint16

local test_data = {
	"/user/login2?user=Zmojianliang@163%2Ecom&password=d2dc0e04e5ee1740&client=Android-SM-A7000&client_id=4Q9DQ9Nge9WQ6DE1CFAf6bUd&client_app=CamScanner_AD_1_LITE@3%2E9%2E1%2E20151020_Market_360&type=email",
	"user/login2?user=Khalidmasood196710@gmail%2Ecom&password=f0ecde54720e5956&client=Android-GT-I9060I&client_id=FA76CV4995077h6Q7EV1CbHF&client_app=CamScanner_AD_1_LITE@3%2E9%2E4%2E20151118&type=email",
	"helloworld",
	"{sdfsdafdsafsdfasfd}",
}


local file = io.open("test_data.bin", "wb")
assert(file)

local arr_tb = {}
local arr_len = #test_data
table.insert(arr_tb, packint32(htonl(arr_len)))

for i=1,arr_len do
	local len = string.len(test_data[i]) + 4
	table.insert(arr_tb, packint32(htonl(len)))
	table.insert(arr_tb, test_data[i])
end

file:write(table.concat(arr_tb))

file:close()
