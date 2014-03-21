local print = print

module(...)
local printlevel = 0

function testprint(...)
  if printlevel==0 then
    print(...)
  else
    local strPrefix = ""
    for i=2,printlevel do strPrefix = printlevel .. "	" end
    print(strPrefix, ...)
  end
end

function pushprint()
  printlevel = printlevel + 1
end

function popprint()
  printlevel = printlevel - 1
end

-- Function: loadtestsontarget
-- Description: Load all tests corresponding to "environment" on the target. rpcclient must not be a valid object.
-- Return: success if no error occured, raise an error otherwise
function loadtestsontarget(tests, environment, rpcclient)
  assert(tests, "Tests list has to be a valid table")
  assert(environment, "environment has to be specified")
  assert(rpcclient, "RPC client has to be valid")

  if not tests[environment] then
    print('No tests defined for environment '..environment)
  else
    for key, value in pairs(tests[environment]) do
      rpcclient:call('require', 'tests.'..value)
      print(value .." loaded")
    end
  end

  return "success"
end

--M = {testprint, pushprint, popprint}
--return M;