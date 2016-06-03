
# --------------------------------------------------------------------------------------------------
#
#  Generate the Java client and server code for a given API file.
#
#  Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
#
# --------------------------------------------------------------------------------------------------

import os
import cStringIO
import codeTypes




# The user defined types found in the API.
Types = dict(Result = codeTypes.EnumData("Result", []))

# The functions with their generated Indices.
FunctionIndicies = dict()
CurrentIndex = 0

# List of client event handler functions, mapped to their message ID.
ClientEventHandlers = dict()

# Map for keeping track of types that need to be boxed when used with a Ref.
BoxMap = dict(
        boolean = "Boolean",
        byte    = "Byte",
        char    = "Character",
        float   = "Float",
        int     = "Integer",
        long    = "Long",
        short   = "Short",
        double  = "Double"
    )

LiteralInitMap = dict(
        boolean = "false",
        byte = "0",
        short = "0",
        int = "0",
        long = "0L",
        float = "0.0f",
        double = "0.0d",
        char = "'\u0000'"
    )



# --------------------------------------------------------------------------------------------------
# Record a user defined type for later referral.
# --------------------------------------------------------------------------------------------------
def RecordType(name, codeItem):

    Types[name] = codeItem




# --------------------------------------------------------------------------------------------------
# Record all of the imported types for future lookup.
# --------------------------------------------------------------------------------------------------
def RecordImportedTypes(importedCode):

    for codeItem in importedCode:

        if (   isinstance(codeItem, codeTypes.ReferenceData)
            or isinstance(codeItem, codeTypes.EnumData)
            or isinstance(codeItem, codeTypes.BitMaskData)):

            RecordType(codeItem.name, codeItem)

        elif isinstance(codeItem, codeTypes.HandlerFuncData):

            RecordType(codeItem.funcName, codeItem)




# --------------------------------------------------------------------------------------------------
# Translate a type string from it's internal name to a Java name.
# --------------------------------------------------------------------------------------------------
def BufferSize(args):

    if args.interfaceFile.endswith( ("le_secStore.api", "secStoreAdmin.api") ):
        maxMsgSize = 8504
    elif args.interfaceFile.endswith("le_cfg.api"):
        maxMsgSize = 1604
    else:
        maxMsgSize = 1104

    return maxMsgSize





# --------------------------------------------------------------------------------------------------
# Translate a type string from it's internal name to a Java name.
# --------------------------------------------------------------------------------------------------
def TranslateTypeStr(typeStr):

    if typeStr == "":
        retVal = "void"
    elif typeStr == "rawRef":
        retVal = "long"
    else:
        retVal = typeStr

    return retVal




# --------------------------------------------------------------------------------------------------
# Return true if the type is defined within an API file, false if not.
# --------------------------------------------------------------------------------------------------
def IsCustomType(typeName):

    # Currently the Result type is defined in our JNI lib, so return false here.
    if typeName == "Result":

        return False

    # Otherwise, perform the check as normal.
    if typeName in Types:

        typeRef = Types[typeName]

        if (   isinstance(typeRef, codeTypes.EnumData)
            or isinstance(typeRef, codeTypes.BitMaskData)
            or isinstance(typeRef, codeTypes.ReferenceData)
            or isinstance(typeRef, codeTypes.HandlerFuncData)):

            return True


    return False



# --------------------------------------------------------------------------------------------------
# Get the code to pack a value based on it's type.
# --------------------------------------------------------------------------------------------------
def SetterForType(typeName, varName, varSize = None):
    setterMap = dict(
        byte = "buffer.writeByte(%s);",
        short = "buffer.writeShort(%s);",
        int = "buffer.writeInt(%s);",
        long = "buffer.writeLong(%s);",
        boolean = "buffer.writeBool(%s);",
        double = "buffer.writeDouble(%s);",
        String = "buffer.writeString(%s, %d);",
        FileDescriptor = "message.setFd(%s);",
        Result = "buffer.writeInt(%s);",
        ref = "buffer.writeLongRef(%s.getRef());",
        rawRef = "buffer.writeLongRef(%s);",
        enum = "buffer.writeInt(%s.getValue());",
        handler = "long ref%s = handlerMap.newRef(new HandlerMapper(%s, true));\n" + \
                  "        buffer.writeLongRef(ref%s);"
    )

    if typeName in Types:

        typeRef = Types[typeName]

        if isinstance(typeRef, codeTypes.EnumData) or isinstance(typeRef, codeTypes.BitMaskData):
            typeName = "enum"
        elif isinstance(typeRef, codeTypes.ReferenceData):
            typeName = "ref"
        elif isinstance(typeRef, codeTypes.HandlerFuncData):
            typeName = "handler"

            return setterMap[typeName] % ( varName, varName, varName )


    formats = varName

    if varSize is not None:

        formats = ( formats, varSize )

    return setterMap[typeName] % formats




# --------------------------------------------------------------------------------------------------
# Generate the code to pack a single value into a message buffer.
# --------------------------------------------------------------------------------------------------
def PackValue(outFile, nameStr, typeStr, size, isRef = False, prefixStr = ""):

    formatParams = prefixStr + nameStr

    if isRef:
        formatParams += ".getValue()"

    setSize = None

    if typeStr == "String":
        setSize = size

    setter = SetterForType(typeStr, formatParams, setSize)

    print >>outFile, "        " + setter




# --------------------------------------------------------------------------------------------------
# Generate the code to unpack a single value from a message buffer.
# --------------------------------------------------------------------------------------------------
def UnpackValue(outFile, nameStr, typeStr, isRef = False, prefixStr = "", apiName = "",
                needsClassName = False, funcName = ""):

    typePrefix = ""

    if IsCustomType(typeStr) and needsClassName:
        typePrefix = apiName + "."

    getterMap = dict(
        byte = "buffer.readByte()",
        short = "buffer.readShort()",
        int = "buffer.readInt()",
        long = "buffer.readLong()",
        boolean = "buffer.readBool()",
        double = "buffer.readDouble()",
        String = "buffer.readString()",
        FileDescriptor = "message.getFd()",
        Result = "Result.fromInt(buffer.readInt())",
        ref = "buffer.readLongRef()",
        rawRef = "buffer.readLongRef()",
        enum = typePrefix + typeStr + ".fromInt(buffer.readInt())"
    )

    fullName = prefixStr + nameStr
    getter = ""
    preamble = ""


    if typeStr in Types:
        typeRef = Types[typeStr]
        if isinstance(typeRef, codeTypes.EnumData) or isinstance(typeRef, codeTypes.BitMaskData):
            getter = getterMap["enum"]

        elif isinstance(typeRef, codeTypes.ReferenceData):
            getter = "new " + typePrefix + typeStr + "(" + getterMap["ref"] + ")"

        elif isinstance(typeRef, codeTypes.HandlerFuncData):

            contextName = nameStr + "Context"
            preamble = "long " + contextName + " = " + getterMap["ref"] + ";\n        "
            getter = GenerateEventHandler(apiName, contextName, funcName, typeRef)

    else:

        getter = getterMap[typeStr]




    if not isRef:

        print >>outFile, "        " + preamble + typePrefix + TranslateTypeStr(typeStr) + " " + \
                                      fullName + " = " + getter + ";"

    else:

        print >>outFile, "        " + fullName + ".setValue(" + getter + ");"




# --------------------------------------------------------------------------------------------------
# Write out a function parameter list for the given function definition.
# --------------------------------------------------------------------------------------------------
def WriteFunctionParameters(outputFile, paramList, indent = 1, prefixStr = ""):

    def FormatMember(param, indent):

        paramType = param.pType

        if paramType == "rawRef":
            paramType = "long"

        strValue = "    " + ("    " * indent)

        if param.direction == "OUT":
            refType = paramType

            if refType in BoxMap:
                refType = BoxMap[refType]

            strValue += "Ref<" + refType + "> " + prefixStr + param.pName
        else:
            strValue += paramType + " " + prefixStr + param.pName

        return strValue

    formattedParams = [ FormatMember(item, indent) for item in paramList ]
    valuesStr = ",\n".join(formattedParams)

    if valuesStr != "":
        print >>outputFile, valuesStr




# --------------------------------------------------------------------------------------------------
# Generate simple function definition objects for the add and remove functions.
# --------------------------------------------------------------------------------------------------
def CreateEventFuncs(funcName, paramList):

    class SimpleParmData(object):

        def __init__(self, pName, pType, direction = None):
            self.pName = pName
            self.pType = pType
            self.direction = direction


    class FunctionData(object):

        def __init__(self, funcName, funcType, parmList, comment = ""):
            self.funcName = funcName
            self.funcType = funcType
            self.parmList = parmList
            self.comment = comment

    addFunc = FunctionData("Add" + funcName + "Handler",
                           "rawRef",
                           paramList,
                           "Register a handler for the " + funcName + " event.")

    removeFunc = FunctionData("Remove" + funcName + "Handler",
                              "",
                              [ SimpleParmData("handlerRef", "rawRef", "IN") ],
                              "Remove the registered handler for the " + funcName + " event.")

    return ( addFunc, removeFunc )




# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------




# --------------------------------------------------------------------------------------------------
# Generate the interface header.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceHeader(interfaceFile, name):

    print >>interfaceFile, "\n" + \
                           "// Generated interface for API " + name + ".\n" + \
                           "// This is a generated file, do not edit.\n" + \
                           "\n" + \
                           "package io.legato.api;\n" + \
                           "\n" + \
                           "import io.legato.Ref;\n" + \
                           "import io.legato.Result;\n" + \
                           "import java.util.Map;\n" + \
                           "import java.util.HashMap;\n" + \
                           "import java.io.FileDescriptor;\n" + \
                           "\n" + \
                           "public interface " + name + "\n" + \
                           "{"




# --------------------------------------------------------------------------------------------------
# Generate a reference type.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceRef(interfaceFile, codeItem):

    print >>interfaceFile, "    public class " + codeItem.name + "\n" + \
                           "    {\n" + \
                           "        private long nativeRef;\n" + \
                           "\n" + \
                           "        public " + codeItem.name + "(long newRef)\n" + \
                           "        {\n" + \
                           "            nativeRef = newRef;\n" + \
                           "        }\n" + \
                           "\n" + \
                           "        public long getRef()\n" + \
                           "        {\n" + \
                           "            return nativeRef;\n" + \
                           "        }\n" + \
                           "    }"




# --------------------------------------------------------------------------------------------------
# Generate a Java enumeration for the enum found in the API file.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceBitmask(interfaceFile, bitmaskItem):

    def FormatItem(maskItem):

        return "        public static final int %s = %s;" % ( maskItem.name, maskItem.value )

    valueList = [ FormatItem(maskItem) for maskItem in bitmaskItem.memberList ]
    maskValuesStr = "\n".join(valueList) + "\n"

    print >>interfaceFile, "    public static class " + bitmaskItem.name + "\n" + \
                           "    {\n" + \
                           maskValuesStr + \
                           "\n" + \
                           "        private int value;\n" + \
                           "\n" + \
                           "        public " + bitmaskItem.name + "(int newValue)\n" + \
                           "        {\n" + \
                           "            value = newValue;\n" + \
                           "        }\n" + \
                           "\n" + \
                           "        public static " + bitmaskItem.name + " fromInt(int newValue)\n" + \
                           "        {\n" + \
                           "            return new " + bitmaskItem.name + "(newValue);\n" + \
                           "        }\n" + \
                           "\n" + \
                           "        public int getValue()\n" + \
                           "        {\n" + \
                           "            return value;\n" + \
                           "        }\n" + \
                           "    }"




# --------------------------------------------------------------------------------------------------
# Generate a Java enumeration for the enum found in the API file.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceEnum(interfaceFile, enumItem):

    def FormatMember(enumValue, isLast):
        strValue = "        " + enumValue.name

        if enumValue.value is not None:
            strValue += "(" + str(enumValue.value) + ")"
        else:
            strValue += "(0)"

        if isLast == False:
            strValue += ","
        else:
            strValue += ";"

        return strValue


    enumName = enumItem.name

    print >>interfaceFile, "    public enum " + enumName + "\n" + \
                           "    {"

    count = len(enumItem.memberList)

    if count == 1:
         print >>interfaceFile, FormatMember(enumItem.memberList[0], True) + "\n"
    elif count > 1:
        memberList = [ FormatMember(item, False) for item in enumItem.memberList[: -1] ]
        memberList += [ FormatMember(enumItem.memberList[-1], True) ]

        valuesStr = '\n'.join(memberList) + "\n"

        print >>interfaceFile, valuesStr

    print >>interfaceFile, "        private int value;\n" + \
                           "\n" + \
                           "        " + enumName + "(int newValue)\n" + \
                           "        {\n" + \
                           "            value = newValue;\n" + \
                           "        }\n" + \
                           "\n" + \
                           "        public int getValue()\n" + \
                           "        {\n" + \
                           "            return value;\n" + \
                           "        }\n" + \
                           "\n" + \
                           "        private static final Map<Integer, " + enumName + \
                                      "> valueMap = new HashMap<Integer, " + enumName + ">();\n" + \
                           "\n" + \
                           "        static\n" + \
                           "        {\n" + \
                           "            for (" + enumName + " item : " + enumName + ".values())\n" + \
                           "            {\n" + \
                           "                valueMap.put(item.value, item);\n" + \
                           "            }\n" + \
                           "        }\n" + \
                           "\n" + \
                           "        public static " + enumName + " fromInt(int newValue)\n" + \
                           "        {\n" + \
                           "            return valueMap.get(newValue);\n" + \
                           "        }\n" + \
                           "    }"




# --------------------------------------------------------------------------------------------------
# Write a function definition for the API interface.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceFunction(interfaceFile, functionDef):


    print >>interfaceFile, "    public " + TranslateTypeStr(functionDef.funcType) + " " + \
                                                                   functionDef.funcName + \
                         "\n    ("

    WriteFunctionParameters(interfaceFile, functionDef.parmList)

    print >>interfaceFile, "    );"




# --------------------------------------------------------------------------------------------------
# Generate an interface for a given event, so that later event handlers can use this interface.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceEventType(interfaceFile, eventFunction):

    print >>interfaceFile, "    public interface " + eventFunction.funcName + "\n" + \
                           "    {\n" + \
                           "        public void handle\n" + \
                           "        ("

    WriteFunctionParameters(interfaceFile, eventFunction.parmList, 2)

    print >>interfaceFile, "        );\n" + \
                           "    }"




# --------------------------------------------------------------------------------------------------
# Generate definitions for the add/remove event handling functions.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceEventFuncs(interfaceFile, eventDef):

    ( addFunc, removeFunc ) = CreateEventFuncs(eventDef.funcName, eventDef.parmList)

    WriteInterfaceFunction(interfaceFile, addFunc)
    WriteInterfaceFunction(interfaceFile, removeFunc)




# --------------------------------------------------------------------------------------------------
# Record a function with it's message ID.
# --------------------------------------------------------------------------------------------------
def RecordFunction(funcName):

    global CurrentIndex
    global FunctionIndicies

    FunctionIndicies[funcName] = CurrentIndex
    CurrentIndex += 1




# --------------------------------------------------------------------------------------------------
# Record the event functions and their IDs for later Id generation.
# --------------------------------------------------------------------------------------------------
def RecordEventFunctions(eventDef):

    ( addFunc, removeFunc ) = CreateEventFuncs(eventDef.funcName, eventDef.parmList)

    RecordFunction(addFunc.funcName)
    RecordFunction(removeFunc.funcName)




# --------------------------------------------------------------------------------------------------
# Write out the interfaces structures for the various API elements.
#
# This function also has the side effect of building a lists of interface functions and types for
# later reference by the server/client code generators.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceItem(interfaceFile, codeItem):

    if isinstance(codeItem, codeTypes.ReferenceData):

        WriteInterfaceRef(interfaceFile, codeItem)
        RecordType(codeItem.name, codeItem)

    elif isinstance(codeItem, codeTypes.BitMaskData):

        WriteInterfaceBitmask(interfaceFile, codeItem)
        RecordType(codeItem.name, codeItem)

    elif isinstance(codeItem, codeTypes.EnumData):

        WriteInterfaceEnum(interfaceFile, codeItem)
        RecordType(codeItem.name, codeItem)

    elif isinstance(codeItem, codeTypes.FunctionData):

        WriteInterfaceFunction(interfaceFile, codeItem)
        RecordFunction(codeItem.funcName)

    elif isinstance(codeItem, codeTypes.HandlerFuncData):

        WriteInterfaceEventType(interfaceFile, codeItem)
        RecordType(codeItem.funcName, codeItem)

    elif isinstance(codeItem, codeTypes.EventFuncData):

        WriteInterfaceEventFuncs(interfaceFile, codeItem)
        RecordEventFunctions(codeItem)




# --------------------------------------------------------------------------------------------------
# Finish up the interface footer.
# --------------------------------------------------------------------------------------------------
def WriteInterfaceFooter(interfaceFile):

    print >>interfaceFile, "}"




# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------




# --------------------------------------------------------------------------------------------------
# Generate the server code header.
# --------------------------------------------------------------------------------------------------
def WriteServerHeader(interfaceFile, importedApis, bufferSize, serviceName, hashValue):

    importList = [ ("import io.legato.api.%s;" % item) for item in importedApis ]
    importedApiStr = "\n".join(importList) + "\n"

    print >>interfaceFile, \
          "\n" + \
          "// Generated server implementation of API " + serviceName + ".\n" + \
          "// This is a generated file, do not edit.\n" + \
          "\n" + \
          "package io.legato.api.implementation;\n" + \
          "\n" + \
          "import java.io.FileDescriptor;\n" + \
          "import java.lang.AutoCloseable;\n" + \
          "import io.legato.Ref;\n" + \
          "import io.legato.Result;\n" + \
          "import io.legato.Message;\n" + \
          "import io.legato.MessageBuffer;\n" + \
          "import io.legato.MessageEvent;\n" + \
          "import io.legato.Service;\n" + \
          "import io.legato.Protocol;\n" + \
          "import io.legato.ServerSession;\n" + \
          "import io.legato.SessionEvent;\n" + \
          importedApiStr + \
          "import io.legato.api." + serviceName + ";\n" + \
          "\n" + \
          "public class " + serviceName + "Server implements AutoCloseable\n" + \
          "{\n" + \
          "    private static final String protocolIdStr = \"" + hashValue + "\";\n" + \
          "    private static final String serviceInstanceName = \"" + serviceName + "\";\n" + \
          "    private static final int maxMsgSize = " + str(bufferSize) + ";\n" + \
          "\n" + \
          "    private Service service;\n" + \
          "    private ServerSession currentSession;\n" + \
          "    private " + serviceName + " serverImpl;\n" + \
          "\n" + \
          "    public " + serviceName + "Server(" + serviceName + " newServerImpl)\n" + \
          "    {\n" + \
          "        service = null;\n" + \
          "        currentSession = null;\n" + \
          "        serverImpl = newServerImpl;\n" + \
          "    }\n" + \
          "\n" + \
          "    public void open()\n" + \
          "    {\n" + \
          "        open(serviceInstanceName);\n" + \
          "    }\n" + \
          "\n" + \
          "    public void open(String serviceName)\n" + \
          "    {\n" + \
          "        currentSession = null;\n" + \
          "        service = new Service(new Protocol(protocolIdStr, maxMsgSize), serviceName);\n" + \
          "\n" + \
          "        service.setReceiveHandler(new MessageEvent()\n" + \
          "            {\n" + \
          "                public void handle(Message message)\n" + \
          "                {\n" + \
          "                    OnClientMessageReceived(message);\n" + \
          "                }\n" + \
          "            });\n" + \
          "\n" + \
          "        service.addCloseHandler(new SessionEvent<ServerSession>()\n" + \
          "            {\n" + \
          "                public void handle(ServerSession session)\n" + \
          "                {\n" + \
          "                    // TODO: Clean up...\n" + \
          "                }\n" + \
          "            });\n" + \
          "\n" + \
          "        service.advertiseService();\n" + \
          "    }\n" + \
          "\n" + \
          "    @Override\n" + \
          "    public void close()\n" + \
          "    {\n" + \
          "        if (service != null)\n" + \
          "        {\n" + \
          "            currentSession = null;\n" + \
          "            service.close();\n" + \
          "            service = null;\n" + \
          "        }\n" + \
          "    }\n"




# --------------------------------------------------------------------------------------------------
# This will generate the server side intermediate event handler.  This event handler object will
# know how to pack up the events and send the event message to the client.
# --------------------------------------------------------------------------------------------------
def GenerateEventHandler(apiName, contextName, funcName, handlerRef):

    extraParams = ""
    extraParamPacking = ""

    for param in handlerRef.parmList:

        paramName = "_" + param.pName

        if extraParams:
            extraParams += ", "

        extraParams += param.pType + " " + paramName
        extraParamPacking += "                    " + SetterForType(param.pType, paramName) + "\n"

    fullName = apiName + "." + handlerRef.funcName

    handlerStr = "new " + fullName + "()\n" + \
                 "            {\n" + \
                 "                private long context;\n" + \
                 "                private ServerSession session;\n" + \
                 "\n" + \
                 "                @Override\n" + \
                 "                public void handle(" + extraParams + ")\n" + \
                 "                {\n" + \
                 "                    Message message = session.createMessage();\n" + \
                 "                    MessageBuffer buffer = message.getBuffer();\n" + \
                 "\n" + \
                 "                    buffer.writeInt(MessageID_" + funcName + ");\n" + \
                 "                    buffer.writeLongRef(context);\n" + \
                 extraParamPacking + \
                 "\n" + \
                 "                    message.send();\n" + \
                 "                }\n" + \
                 "\n" + \
                 "                public " + fullName + " setContext(long newContext, ServerSession newSession)\n" + \
                 "                {\n" + \
                 "                    context = newContext;\n" + \
                 "                    session = newSession;\n" + \
                 "                    return this;\n" + \
                 "                }\n" + \
                 "            }.setContext(" + contextName + ", currentSession)"

    return handlerStr




# --------------------------------------------------------------------------------------------------
# Generate the message unpack and dispatch code.
# --------------------------------------------------------------------------------------------------
def WriteServerFunction(serverFile, apiName, functionDef):

    print >>serverFile, "    private void handle" + functionDef.funcName + \
                        "(MessageBuffer buffer)\n" + \
                        "    {"

    paramStr = ""

    hasOuts = False

    # Prepare all of the function parameters.

    for param in functionDef.parmList:

        if paramStr != "":
            paramStr += ", "

        paramStr += "_" + param.pName


        if param.direction == "IN":
            UnpackValue(serverFile, param.pName, param.pType,
                        prefixStr = "_",
                        apiName = apiName,
                        needsClassName = True,
                        funcName = functionDef.funcName)
        else:
            hasOuts = True
            refType = param.pType

            if param.pType in LiteralInitMap:
                initStr = LiteralInitMap[param.pType]
            else:
                initStr = "null";

            if refType in BoxMap:
                refType = BoxMap[refType]

            if IsCustomType(refType):

                refType = apiName + "." + refType

            print >>serverFile, "        Ref<" + refType + "> _" + param.pName + \
                                " = new Ref<" + refType + ">(" + initStr + ");"

    # Now call the class that actually implements the functionality.
    if len(functionDef.parmList) != 0:
        print >>serverFile, ""

    resultStr = "        "

    if functionDef.funcType != "":
        hasOuts = True
        typeStr = TranslateTypeStr(functionDef.funcType)

        if IsCustomType(typeStr):
            typeStr = apiName + "." + typeStr

        resultStr += typeStr + " result = "

    print >>serverFile, resultStr + "serverImpl." + functionDef.funcName + "(" + paramStr + ");"

    print >>serverFile, "        buffer.resetPosition();\n" + \
                        "        buffer.writeInt(MessageID_" + functionDef.funcName + ");"

    # If there was a return value, pack that first.
    if functionDef.funcType != "":

        setter = SetterForType(functionDef.funcType, "result")
        print >>serverFile, "\n        " + setter


    # Now go through and pack all of the OUT params.
    hasOuts = False

    for param in functionDef.parmList:

        if param.direction == "OUT":
            if hasOuts == False:
                print >>serverFile, ""
                hasOuts = True

            size = 0

            if hasattr(param, 'minSize'):
                size = param.minSize

            PackValue(serverFile, param.pName, param.pType, size, isRef = True, prefixStr = "_")

    print >>serverFile, "    }" + \
                        "\n"




# --------------------------------------------------------------------------------------------------
# Write out the add/remove event registration functions.
# --------------------------------------------------------------------------------------------------
def WriteServerEventFuncs(serverFile, apiName, eventDef):

    ( addFunc, removeFunc ) = CreateEventFuncs(eventDef.funcName, eventDef.parmList)

    WriteServerFunction(serverFile, apiName, addFunc)
    WriteServerFunction(serverFile, apiName, removeFunc)




# --------------------------------------------------------------------------------------------------
# Write out all of the server message handling functions.
# --------------------------------------------------------------------------------------------------
def WriteServerItem(serverFile, apiName, codeItem):

    if isinstance(codeItem, codeTypes.FunctionData):

        WriteServerFunction(serverFile, apiName, codeItem)

    elif isinstance(codeItem, codeTypes.EventFuncData):

        WriteServerEventFuncs(serverFile, apiName, codeItem)




# --------------------------------------------------------------------------------------------------
# Finish up the server class definition with the core message dispatching code.
# --------------------------------------------------------------------------------------------------
def WriteServerFooter(serverFile):

    for func in FunctionIndicies.items():
        print >>serverFile, "    private static final int MessageID_" + func[0] + " = " + \
                            str(func[1]) + ";"

    print >>serverFile, "\n" + \
                        "    private void OnClientMessageReceived(Message clientMessage)\n" + \
                        "    {\n" + \
                        "        MessageBuffer buffer = null;\n" + \
                        "\n" + \
                        "        try\n" + \
                        "        {\n" + \
                        "            buffer = clientMessage.getBuffer();\n" + \
                        "            currentSession = clientMessage.getServerSession();\n" + \
                        "            int messageId = buffer.readInt();\n" + \
                        "\n" + \
                        "            switch (messageId)\n" + \
                        "            {"

    for func in FunctionIndicies.items():
        name = func[0]

        print >>serverFile, "                case MessageID_" + name + ":\n" + \
                            "                    handle" + name + "(buffer);\n" + \
                            "                    break;"

    print >>serverFile, "            }\n" + \
                        "\n" + \
                        "            clientMessage.respond();\n" + \
                        "        }\n" + \
                        "        finally\n" + \
                        "        {\n" + \
                        "            currentSession = null;\n" + \
                        "\n" + \
                        "            if (buffer != null)\n" + \
                        "            {\n" + \
                        "                buffer.close();\n" + \
                        "            }\n" + \
                        "        }\n" + \
                        "    }\n" + \
                        "}"




# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------




# --------------------------------------------------------------------------------------------------
# Write out the start of the class definition and all of the code that is standard for all client
# implementations.
# --------------------------------------------------------------------------------------------------
def WriteClientHeader(clientFile, importedApis, bufferSize, serviceName, hashValue):

    importList = [ ("import io.legato.api.%s;" % item) for item in importedApis ]
    importedApiStr = "\n".join(importList) + "\n"

    print >>clientFile, "\n" + \
                        "// Generated client implementation of the API " + serviceName + ".\n" + \
                        "// This is a generated file, do not edit.\n" + \
                        "\n" + \
                        "package io.legato.api.implementation;\n" + \
                        "\n" + \
                        "import java.io.FileDescriptor;\n" + \
                        "import java.lang.AutoCloseable;\n" + \
                        "import io.legato.Ref;\n" + \
                        "import io.legato.Result;\n" + \
                        "import io.legato.SafeRef;\n" + \
                        "import io.legato.Message;\n" + \
                        "import io.legato.MessageEvent;\n" + \
                        "import io.legato.Protocol;\n" + \
                        "import io.legato.MessageBuffer;\n" + \
                        "import io.legato.ClientSession;\n" + \
                        importedApiStr + \
                        "import io.legato.api." + serviceName + ";\n" + \
                        "\n" + \
                        "public class " + serviceName + "Client implements AutoCloseable, " + serviceName + "\n" + \
                        "{\n" + \
                        "    private static final String protocolIdStr = \"" + hashValue + "\";\n" + \
                        "    private static final String serviceInstanceName = \"" + serviceName + "\";\n" + \
                        "    private static final int maxMsgSize = " + str(bufferSize) + ";\n" + \
                        "\n" + \
                        "    private class HandlerMapper\n" + \
                        "    {\n" + \
                        "        public Object handler;\n" + \
                        "        public long serverRef;\n" + \
                        "        public boolean isOneShot;\n" + \
                        "\n" + \
                        "        public HandlerMapper(Object newHandler, boolean newIsOneShot)\n" + \
                        "        {\n" + \
                        "            handler = newHandler;\n" + \
                        "            isOneShot = newIsOneShot;\n" + \
                        "            serverRef = 0;\n" + \
                        "        }\n" + \
                        "    }\n" + \
                        "\n" + \
                        "    private ClientSession session;\n" + \
                        "    private SafeRef<HandlerMapper> handlerMap;\n" + \
                        "\n" + \
                        "    public " + serviceName + "Client()\n" + \
                        "    {\n" + \
                        "        session = null;\n" + \
                        "        handlerMap = new SafeRef<HandlerMapper>();\n" + \
                        "    }\n" + \
                        "\n" + \
                        "    public void open()\n" + \
                        "    {\n" + \
                        "        open(serviceInstanceName);\n" + \
                        "    }\n" + \
                        "\n" + \
                        "    public void open(String serviceName)\n" + \
                        "    {\n" + \
                        "        session = new ClientSession(new Protocol(protocolIdStr, maxMsgSize), serviceName);\n" + \
                        "\n" + \
                        "        session.setReceiveHandler(\n" + \
                        "            new MessageEvent()\n" + \
                        "            {\n" + \
                        "                public void handle(Message message)\n" + \
                        "                {\n" + \
                        "                    OnServerMessageReceived(message);\n" + \
                        "                }\n" + \
                        "            });\n" + \
                        "\n" + \
                        "        session.open();\n" + \
                        "    }\n" + \
                        "\n" + \
                        "    @Override\n" + \
                        "    public void close()\n" + \
                        "    {\n" + \
                        "        if (session != null)\n" + \
                        "        {\n" + \
                        "            session.close();\n" + \
                        "            session = null;\n" + \
                        "        }\n" + \
                        "    }\n"




# --------------------------------------------------------------------------------------------------
# Generic function header and message creation.
# --------------------------------------------------------------------------------------------------
def WriteClientFunctionPreamble(clientFile, functionDef):

    retTypeStr = TranslateTypeStr(functionDef.funcType)

    print >>clientFile, "    @Override\n" + \
                        "    public " + retTypeStr + " " + functionDef.funcName +"\n" + \
                        "    ("

    WriteFunctionParameters(clientFile, functionDef.parmList, prefixStr = "_")

    print >>clientFile, "    )\n" + \
                        "    {"
    # Create the message and pack the message ID.
    print >>clientFile, "        Message message = session.createMessage();\n" + \
                        "        MessageBuffer buffer = message.getBuffer();\n" + \
                        "\n" + \
                        "        buffer.writeInt(MessageID_" + functionDef.funcName + ");"





# --------------------------------------------------------------------------------------------------
# Called for all client functions, this will take care of the message packing and unpacking.
# --------------------------------------------------------------------------------------------------
def WriteClientFunction(clientFile, apiName, functionDef, isEvent = False):

    WriteClientFunctionPreamble(clientFile, functionDef)

    retTypeStr = TranslateTypeStr(functionDef.funcType)
    hasOuts = retTypeStr != "void"

    # Pack the parameters.
    for param in functionDef.parmList:
        if param.direction == "IN":
            size = 0

            if param.pType == "String":
                size = param.maxSize

            PackValue(clientFile, param.pName, param.pType, size, prefixStr = "_")

        elif param.direction == "OUT":

            if param.pType == "String":

                print >>clientFile, "        buffer.writeLongRef(" + str(param.minSize) + ");"

            hasOuts = True



    # Send the message and wait.
    preamble = ""

    if hasOuts:
        preamble = "Message response = "

    print >>clientFile, "\n" + \
                        "        " + preamble + "message.requestResponse();"

    # Unpack the response(s).
    if hasOuts:
        print >>clientFile, "        buffer = response.getBuffer();\n" + \
                            "        buffer.readInt();\n"


    if retTypeStr != "void":
        # Unpack value needs to know the original unfiltered type string.
        UnpackValue(clientFile, "result", functionDef.funcType)

    for param in functionDef.parmList:

        if param.direction == "OUT":

            UnpackValue(clientFile, param.pName, param.pType,
                        isRef = True, prefixStr = "_")

        elif param.pType in Types:

            typeRef = Types[param.pType]

            if isinstance(typeRef, codeTypes.HandlerFuncData):

                handlerName = "handle" + typeRef.funcName
                ClientEventHandlers["MessageID_" + functionDef.funcName] = handlerName

                #print >>clientFile, "\n        handlerMap.remove(ref_%s);" % param.pName


    if retTypeStr != "void":
        print >>clientFile, "\n" + \
                            "        return result;"

    # Return a result and exit.
    print >>clientFile, "    }\n"




# --------------------------------------------------------------------------------------------------
# .
# --------------------------------------------------------------------------------------------------
def WriteClientHandlerFunction(clientFile, apiName, handler):

    print >>clientFile, "    private void handle" + handler.funcName + "(MessageBuffer buffer)\n" + \
                        "    {\n" + \
                        "        long handlerId = buffer.readLongRef();\n" + \
                        "        HandlerMapper mapper = handlerMap.get(handlerId);\n" + \
                        "        %s handler = (%s)mapper.handler;" % \
                        ( handler.funcName, handler.funcName )

    paramStr = ""

    for param in handler.parmList:

        if paramStr != "":
            paramStr += ", "

        paramStr += "_" + param.pName

        UnpackValue(clientFile, param.pName, param.pType, prefixStr = "_", apiName = apiName)


    print >>clientFile, "\n" + \
                        "        handler.handle(" + paramStr + ");\n" + \
                        "\n" + \
                        "        if (mapper.isOneShot)\n" + \
                        "        {\n" + \
                        "            handlerMap.remove(handlerId);\n" + \
                        "        }\n" + \
                        "    }\n"




# --------------------------------------------------------------------------------------------------
# This will generate the code to request the server to register a client's event handler.
# --------------------------------------------------------------------------------------------------
def WriteClientAddEventFunction(clientFile, apiName, addFunc):

    WriteClientFunctionPreamble(clientFile, addFunc)

    # First off, pack up the normal parameters for this function.  We separate out the last one
    # because we deal with the event handler object specially.
    basicParams = addFunc.parmList[: -1]
    handlerParam = addFunc.parmList[-1]

    for param in basicParams:
        if param.direction == "IN":
            size = 0

            if param.pType == "String":
                size = param.maxSize

            PackValue(clientFile, param.pName, param.pType, size, prefixStr = "_")

    handlerName = "handle" + Types[handlerParam.pType].funcName
    ClientEventHandlers["MessageID_" + addFunc.funcName] = handlerName

    #print handlerName, ClientEventHandlers[handlerName]

    # Now, lets handle the handler parameter specially, then generate the rest of the function.
    print >>clientFile, "\n" + \
                        "        HandlerMapper mapper = new HandlerMapper(_" + handlerParam.pName + ", false);\n" + \
                        "        long newRef = handlerMap.newRef(mapper);\n" + \
                        "        buffer.writeLongRef(newRef);\n" + \
                        "\n" + \
                        "        Message response = message.requestResponse();\n" + \
                        "        buffer = response.getBuffer();\n" + \
                        "\n" + \
                        "        mapper.serverRef = buffer.readLongRef();\n" + \
                        "\n" + \
                        "        return newRef;\n" + \
                        "    }\n"




# --------------------------------------------------------------------------------------------------
# This will generate the event remove function.  It will take care of removing the client handler
# from our internal map.
# --------------------------------------------------------------------------------------------------
def WriteClientRemoveEventFunction(clientFile, apiName, removeFunc):

    WriteClientFunctionPreamble(clientFile, removeFunc)

    # The remove function only ever has the one parameter, so grab it's name and generate our
    # removal code.
    paramName = "_" + removeFunc.parmList[0].pName

    print >>clientFile, "\n" + \
                        "        HandlerMapper handler = handlerMap.get(" + paramName + ");\n" + \
                        "        buffer.writeLongRef(handler.serverRef);\n" + \
                        "        handlerMap.remove(" + paramName + ");\n" + \
                        "\n" + \
                        "        message.requestResponse();\n" + \
                        "    }\n"




# --------------------------------------------------------------------------------------------------
# Generate the function that receives events from the server.
# --------------------------------------------------------------------------------------------------
def WriteClientEventHandleFunction(clientFile, eventDef):

    eventName = eventDef.funcName
    typeName = eventName + "Handler"
    eventType = Types[typeName]

    eventUnpack = cStringIO.StringIO()
    eventParamsStr = ""

    for param in eventType.parmList:

        if eventParamsStr != "":
            eventParamsStr += ", "

        eventParamsStr += "_" + param.pName
        UnpackValue(eventUnpack, param.pName, param.pType, prefixStr = "_")


    eventHandlerName = "receive" + eventDef.funcName + "Event"
    ClientEventHandlers[eventHandlerName] = "MessageID_Add" + eventDef.funcName + "Handler"


    print >>clientFile, "    private void " + eventHandlerName + "\n" + \
                        "    (\n" + \
                        "        MessageBuffer buffer\n" + \
                        "    )\n" + \
                        "    {\n" + \
                        "        long _handlerRef = buffer.readLongRef();\n" + \
                        "        HandlerMapper mapper = handlerMap.get(_handlerRef);\n" + \
                        "        " + typeName + " handler = (" + typeName + ")mapper.handler;\n" + \
                        "\n" + \
                        eventUnpack.getvalue() + \
                        "        handler.handle(" + eventParamsStr + ");\n" + \
                        "    }\n"




# --------------------------------------------------------------------------------------------------
# Generate our event handling code.
# --------------------------------------------------------------------------------------------------
def WriteClientEventFuncs(clientFile, apiName, eventDef):

    ( addFunc, removeFunc ) = CreateEventFuncs(eventDef.funcName, eventDef.parmList)

    WriteClientAddEventFunction(clientFile, apiName, addFunc)
    WriteClientRemoveEventFunction(clientFile, apiName, removeFunc)




# --------------------------------------------------------------------------------------------------
# Generate client code for the various interface elements.
# --------------------------------------------------------------------------------------------------
def WriteClientItem(clientFile, apiName, codeItem):

    if isinstance(codeItem, codeTypes.FunctionData):

        WriteClientFunction(clientFile, apiName, codeItem)

    elif isinstance(codeItem, codeTypes.HandlerFuncData):

        WriteClientHandlerFunction(clientFile, apiName, codeItem)

    elif isinstance(codeItem, codeTypes.EventFuncData):

        WriteClientEventFuncs(clientFile, apiName, codeItem)





# --------------------------------------------------------------------------------------------------
# Write out the end of the client file.
# --------------------------------------------------------------------------------------------------
def WriteClientFooter(clientFile):

    # Dump out all of the message ids.
    for func in FunctionIndicies.items():
        print >>clientFile, "    private static final int MessageID_" + func[0] + " = " + \
                            str(func[1]) + ";"


    # Generate the message dispatching for any events receivable from the server.
    handlerStr = ""

    for handler in ClientEventHandlers.items():

        handlerStr += "                case " + handler[0] + ":\n" + \
                      "                    " + handler[1] + "(buffer);\n" + \
                      "                    break;\n"


    # Now generate the server message handler.
    print >>clientFile, "    \n" + \
                        "    private void OnServerMessageReceived(Message serverMessage)\n" + \
                        "    {\n"

    if len(ClientEventHandlers) > 0:
        print >>clientFile, "        try (MessageBuffer buffer = serverMessage.getBuffer())\n" + \
                            "        {\n" + \
                            "            int messageId = buffer.readInt();\n" + \
                            "\n" + \
                            "            switch (messageId)\n" + \
                            "            {\n" + \
                            handlerStr + \
                            "            }\n" + \
                            "\n" + \
                            "            buffer.close();\n" + \
                            "        }"

    # Close up the function and the class.
    print >>clientFile, "    }\n" + \
                        "}"




# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------
# --------------------------------------------------------------------------------------------------




# --------------------------------------------------------------------------------------------------
# Called to generate the Java classes and interfaces.
# --------------------------------------------------------------------------------------------------
def WriteAllCode(commandArgs, importedCode, importedApis, parsedData, hashValue):

    RecordImportedTypes(importedCode)

    interfaceFile = cStringIO.StringIO()
    serverFile = cStringIO.StringIO()
    clientFile = cStringIO.StringIO()

    # Extract the different types of data
    headerComments = parsedData['headerList']
    parsedCode = parsedData['codeList']
    importList = parsedData['importList']

    bufferSize = BufferSize(commandArgs)

    for line in headerComments:
        print >>interfaceFile, line

    # Generate file headers.
    WriteInterfaceHeader(interfaceFile, commandArgs.serviceName)

    if commandArgs.genServer:
        WriteServerHeader(serverFile, importedApis, bufferSize, commandArgs.serviceName, hashValue)

    if commandArgs.genClient:
        WriteClientHeader(clientFile, importedApis, bufferSize, commandArgs.serviceName, hashValue)

    # Generate all of the interface functions.
    for codeItem in parsedCode:

        WriteInterfaceItem(interfaceFile, codeItem)
        if commandArgs.genServer:
            WriteServerItem(serverFile, commandArgs.serviceName, codeItem)
        if commandArgs.genClient:
            WriteClientItem(clientFile, commandArgs.serviceName, codeItem)

    # Finally finish up the handler code.
    WriteInterfaceFooter(interfaceFile)
    if commandArgs.genServer:
        WriteServerFooter(serverFile)
    if commandArgs.genClient:
        WriteClientFooter(clientFile)


    # Make sure our output directories exist.
    interfaceDir = os.path.abspath(os.path.join(commandArgs.outputDir, "io/legato/api"))

    if not os.path.isdir(interfaceDir):
        os.makedirs(interfaceDir)

    implementationDir = os.path.join(interfaceDir, "implementation")

    if not os.path.isdir(implementationDir):
        os.makedirs(implementationDir)


    # Now write out the results to the different handler files.
    interfacePath = os.path.join(interfaceDir, commandArgs.serviceName + ".java")
    open(interfacePath, 'w').write(interfaceFile.getvalue())

    if commandArgs.genServer:
        serverPath = os.path.join(implementationDir, commandArgs.serviceName + "Server.java")
        open(serverPath, 'w').write(serverFile.getvalue())

    if commandArgs.genClient:
        clientPath = os.path.join(implementationDir, commandArgs.serviceName + "Client.java")
        open(clientPath, 'w').write(clientFile.getvalue())
