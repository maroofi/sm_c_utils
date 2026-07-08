local requests = require("libcrequests")
assert(requests)
local inspect = require("inspect")
assert(inspect)

function getme()
    local to_send={
        url="https://ipinfo.io/json",
        timeout=10
    }
    data, msg, err = requests.get(to_send)
    print(inspect(data))
    print(inspect(msg))
    print(inspect(err))
end

getme()
