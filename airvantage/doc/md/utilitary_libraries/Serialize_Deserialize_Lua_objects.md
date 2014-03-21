Serialize, Deserialize Lua objects
==================================

Serializable Lua objects
========================

-   nil
-   boolean
-   numbers
-   string
-   function
-   table of serializable keys and values. Cycles and shared subtrees
    allowed.

API and usage
=============

The serializer, deserializer is written purely in C. The serializer
returns a string or a string buffer (list of strings); the deserializer
builds objects back from a string or a string buffer.

*Serializes*\
 serobj = luatobin.serialize(obj, totable)

**arguments**

-   obj: must be of an elligible lua object
-   totable: a boolean indicating (if true) that serobj must be
    returned as a string buffer (table)

**return**

-   serobj: either a buffer or a string buffer depending of the
    boolean value of totable. serobj strings/buffers can be
    concatenated together.

*Deserializes*\
 index, obj1, ..., = luatobin.deserialize(serobj, nobjs, offset)

**arguments**

-   serobj: either a buffer or a string buffer
-   nobjs: number of objects to deserialize in serobj
-   offset: index of the first object to deserialize

**return**

-   newoffset: offset of the next object to deserialize in serobj. if
    newoffset == \#serobj+1, all objects have been deserialized.
-   obj1, ...: objects deserialized

~~~~{.lua}
require "luatobin"
obj = {"is a test string", nil, 565482, false, function() print("dummyfunction") end}
obj = {content=obj, true}

-- ser1 is a table contening the serialized version of obj
ser1 = luatobin.serialize(obj)


-- ser2 is a string contening the serialized version of obj
ser2 = luatobin.serialize(obj, true)
print(ser1)
print(ser2)
_, deser1 = luatobin.deserialize(ser1)
_, deser2 = luatobin.deserialize(ser2)


-- deserialize multiple values
ser2 = luatobin.serialize(obj)
_, deser1, deser2 = luatobin.deserialize(ser1..ser2)
p(deser1)
p(deser2)
-- deserialize a specified number of valuesidx, deser1 = luatobin.deserialize(ser1..ser2, 1)
p(deser1)



-- reuse idx to deserialize in the middle of the serialized string
_, deser2 = luatobin.deserialize(ser1..ser2, idx)
p(deser2)
~~~~

Performance comparison with older flash module
==============================================

The new flash object lua API si built on this new serializer

Timing performances (in ms)

-----------------------------------------------------
operation                 old flash    luatobin flash
---------                 ---------    --------------
create + load (256)       40           260

set                       570          10

get                       40           30

reset                     720          1
-----------------------------------------------------

