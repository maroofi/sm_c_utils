local requests = require("libcrequests")
assert(requests)
local inspect = require("inspect")
assert(inspect)


params = { name="John", lastname="Doe", age=32 }
timeout=3
headers={ ["Auth-mode"]="test", ["Auth-token"]="my imaginary token"}
url="http://127.0.0.1:8080/get_data"

to_send = {timeout=timeout, headers=headers, url=url, params=params}
data, err, msg = requests.get(to_send)
print(inspect(data))
print(inspect(err))
print(inspect(msg))
