Time services
=============

#### NTP time synchronization

The time synchronization client is based on Simple Network Time Protocol
(SNTP) Version 4, RFC 4330.

##### Main algorithm

On each synchronization, the Time module does up to 8 exchanges
request/response with NTP server (Any potential NTP packet sanity check
failures or UDP errors are included in this number exchanges)\
 For each successful exchange, the Time module computes the roundtrip
time of the exchange, and the offset to apply to local clock to set time
the more accurately.\
 Finally, the offset computed from the packet with the minimum roundtrip
time, is used to change the local clock.

Notes

- The precision of the time sync is about 0.01 sec.

##### NTP API

~~~~{.lua}
synchronize()
~~~~

Run NTP time synchronization **asynchronously**.

###### Time update broadcast / notification

-   Internal notification (within the Agent Lua VM):\
    -\> Time updated given the offset computed from NTP packet (local
    clock updated)

~~~~{.lua}
Lua Event : emitter="TIME", event="TIME_UPDATED", params=applied_offset(LuaNumber)
~~~~

- External notification:\
  Not available for now.

###### Clock Update Vs timers

> **INFO**
>
> The local clock update that can occur during time synchronization can
> have some impacts on timers and other time-related actions.\
> This can be quite important when the clock is updated with a big
> positive offset (i.e clock was way in the past), as scheduled events
> will be delayed by this big offset.

Example:

~~~~{.lua}
function test_ntp_timer ()
    local sys = require"system"
    local timer = require"timer"
    local string = require "string"
    local now = os.time()
    local t1 = timer.new(10, function() print(string.format("now = %d, one shot 10, should arrive %d", os.time(), now+10)) end)
    local t2 = timer.new(-10, function()print(string.format("now = %d, period 10, should arrive first %d", os.time(), now+10))  end)
    sys.settime(os.time()+ TIME_OFFSET)
end
~~~~

-   if TIME\_OFFSET is positive (e.g. 20), then:
    -   on Linux devices, any event that should happen with a delay <
        TIME\_OFFSET will trigger immediately (here, t1 and t2 will
        trigger just after the time update, then t2 will trigger each
        10s)
    -   on Open AT devices, pending events will be done after the
        correct delay

-   if TIME\_OFFSET is negative(e.g. -20), then:\
     on any device, the events will trigger after delay+TIME\_OFFSET
    time (here t1 and t2 will trigger after 10+20=30s, then t2 will
    trigger each 10s)

#### Time zone and Daylight Saving Time (DST)

Time zone and Daylight Saving Time parameters are defined in the Agent's
Config table.

> **WARNING**
>
> Those parameters are here just to provide a way to read those values for
> applicative logic purpose and to be set/read by server commands.\
> All timestamps exchanged with the server, or used internally, are in
> **UTC**.

#### Useful documentation

SNTP RFC:
[http://www.faqs.org/rfcs/rfc4330.html](http://www.faqs.org/rfcs/rfc4330.html)\
 NTP papers, best practices, ... :
[http://www.ece.udel.edu/\~mills/ntp.html](http://www.ece.udel.edu/~mills/ntp.html)


