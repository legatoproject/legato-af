print "1..5"

local src_dir, build_dir = ...
package.path  = src_dir .. "?.lua;" .. package.path
package.cpath = build_dir .. "?.so;" .. package.cpath

local tap   = require("tap")
local yajl  = require("yajl")
local ok    = tap.ok

function main()
   test_simple()
   null_at_end_of_array()
   null_object_value()
   weird_numbers()
   number_in_string()
   test_generator()
   test_to_value()
end

function to_value(string)
   local result
   local stack = {
      function(val) result = val end
   }
   local obj_key
   local events = {
      value = function(_, val)
                 stack[#stack](val)
              end,
      open_array = function()
                      local arr = {}
                      local idx = 1
                      stack[#stack](arr)
                      table.insert(stack, function(val)
                                             arr[idx] = val
                                             idx = idx + 1
                                          end)
                   end,
      open_object = function()
                      local obj = {}
                      stack[#stack](obj)
                      table.insert(stack, function(val)
                                             obj[obj_key] = val
                                          end)
                   end,
      object_key = function(_, val)
                     obj_key = val
                  end,
      close = function()
                stack[#stack] = nil
             end,
   }

   yajl.parser({ events = events })(string)
   return result
end

function test_to_value()
   local json = '[["float",1.5,"integer",5,"string","hello","empty",[],"false",false,"custom","custom json serializer","ostr_key",{"key":"value"},"obool_key",{"true":true},"onull_key",{"null":null}],10,10.3,10.3,"a string",null,false,true]'
   local val = yajl.to_value(json)
   local got = yajl.to_string(val);

   ok(got == json,
      "yajl.to_value(" .. json .. ") -> " .. got)
end

function test_generator()
   local strings = {}
   local generator = yajl.generator {
      printer = function(string)
                  table.insert(strings, string)
               end
   }

   local custom = { __gen_json = function(self, gen)
                                    gen:string("custom json serializer")
                                 end
                 }
   setmetatable(custom, custom)

   generator:open_array()
   generator:value({  "float",  1.5,
                      "integer", 5,
                      "string", "hello",
                      "empty", {}, -- defaults to an empty array.
                      "false", false,
                      "custom", custom,
                      "ostr_key", { key = "value" },
                      "obool_key", { [true] = true },
                      "onull_key", { [yajl.null] = yajl.null },
                   })
   generator:integer(10.3)
   generator:double(10.3)
   generator:number(10.3)
   generator:string("a string")
   generator:null()
   generator:boolean(false)
   generator:boolean(true)
   generator:open_object()
   generator:close()
   generator:close()

   local expect = '[["float",1.5,"integer",5,"string","hello","empty",[],"false",false,"custom","custom json serializer","ostr_key",{"key":"value"},"obool_key",{"true":true},"onull_key",{"null":null}],10,10.300000000000000711,10.3,"a string",null,false,true,{}]'
   local got = table.concat(strings)
   ok(expect == got, expect .. " == " .. got)
end

function test_simple()
   local expect =
      '['..
      '"float",1.5,'..
      '"integer",5,'..
      '"true",true,'..
      '"false",false,'..
      '"null",null,'..
      '"string","hello",'..
      '"array",[1,2],'..
      '"object",{"key":"value"}'..
      ']'

   -- Input to to_value matches the output of to_string:
   local got = yajl.to_string(to_value(expect))
   ok(expect == got, expect .. " == " .. tostring(got))
end

function null_at_end_of_array()
   local expect = '["something",null]'
   local got = yajl.to_string(to_value(expect))
   ok(expect == got, expect .. " == " .. tostring(got))
end

function null_object_value()
   local expect = '{"something":null}'
   local got = yajl.to_string(to_value(expect))
   ok(expect == got, expect .. " == " .. tostring(got))
end

function weird_numbers()
   -- See note in README about how we are broken when it comes to big numbers.
   local expect = '[1e+666,-1e+666,-0]'
   local got = yajl.to_string(to_value(expect))
   ok(expect == got, expect .. " == " .. tostring(got))

   local nan = 0/0
   got = yajl.to_string { nan };
   ok("[-0]" == got, "NaN converts to -0 (got: " .. got .. ")")
end

function number_in_string()
   -- Thanks to agentzh for this bug report!
   local expect = '{"2":"two"}'
   local got = yajl.to_string {["2"]="two"}
   ok(expect == got, expect .. " == " .. tostring(got))
end

main()