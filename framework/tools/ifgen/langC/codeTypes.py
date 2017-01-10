#
# Define types/classes for different code/message objects
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#

# Python libraries
import logging

# For reporting exceptions to interface parser
import pyparsing

# ifgen specific libraries
import common



# Global for storing the prefix used for auto-generated interface functions and types
NamePrefix = ""

# Set the global prefix variable
def SetNamePrefix(namePrefix):
    global NamePrefix
    NamePrefix = namePrefix

# Add the prefix to the given name
def AddNamePrefix(name):
    if NamePrefix:
        return "%s_%s" % (NamePrefix, name)
    else:
        return name



#
# Using commonTypes  ...
#
# TODO: started the transition to using definitions in commonTypes, instead of having them
#       here.  This is work in progress, and so only some of the definitions are used for now.
import commonTypes
from commonTypes import ( SetImportName,
                          AddInterfaceType,
                          AddDefinitionValue,
                          EvaluateDefinition,
                          HandlerTypes,
                          BaseTypeData,
                          ImportData,
                          PrintCode,
                          PrintDefinedTypes )



DefinedInterfaceTypes = dict(
    # unsigned ints
    uint8   = "uint8_t",
    uint16  = "uint16_t",
    uint32  = "uint32_t",
    uint64  = "uint64_t",
    uint    = "uint32_t",

    # signed ints
    int8    = "int8_t",
    int16   = "int16_t",
    int32   = "int32_t",
    int64   = "int64_t",
    int     = "int32_t",

    # boolean
    bool = "bool",

    # a single char -- this is not for string support
    char = "char",

    # double is sufficient -- no need to support float as well.
    double = "double",

    # for array sizes and indices, and things like that
    size = 'size_t',

    # TODO: This is temporary, so that errors are not generated for these types.
    #       The issues with these types will be resolved soon, at which point these
    #       lines will be removed.
    le_result_t = "le_result_t",
    le_onoff_t = 'le_onoff_t'
)

commonTypes.InitPredefinedInterfaces(DefinedInterfaceTypes)



# Convert from the interfaceType, as used in the interface files, to the corresponding C type.
def ConvertInterfaceType(interfaceType):
    if isinstance(interfaceType, RawType):
        return interfaceType.rawType
    else:
        return commonTypes.ConvertInterfaceType(interfaceType)



#
# Helper class used to wrap type names that don't need to be converted.  This is used for various
# internally specified functions and parameters.  A type specified in an API file will never be
# a RawType.
#
class RawType(object):

    def __init__(self, rawType):
        self.rawType = rawType


#--------------------------------------------------------------------------
# A descriptor that only implements the __get__ part of the descriptor protocol
# Thus, it is a non-data descriptor, and so can be over-written by instance variables
# in sub-classes.
# todo: Probably need to expand this comment ...
#--------------------------------------------------------------------------

class Getter(object):

    def __init__(self, getter):
        self.getter = getter

    def __get__(self, instance, owner):
        if instance is None:
            return self

        return self.getter(instance)


#--------------------------------------------------------------------------
# Parameter data related classes
#--------------------------------------------------------------------------

class BaseParmData(object):

    # These are the default templates for all the parameter data classes.
    # A particular class can over-ride these, by defining the corresponding
    # instance variable.
    #
    # todo: Should figure out how to best deal with the indentation of these
    #       definitions.  It might be worth having a FormatCodeLeft() function
    #       that will left adjust the template before using it.  That way, it
    #       doesn't matter how much indentation these templates have.

    clientParmList = """\
{parm.parmType} {parm.parmName}\
"""

    clientPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.address}, {parm.numBytes} );\
"""

    clientUnpack = """\
_msgBufPtr = UnpackData( _msgBufPtr, {parm.address}, {parm.numBytes} );\
"""

    serverParmList = """\
{parm.serverType} {parm.serverName};\
"""

    serverUnpack = """\
{parm.serverType} {parm.serverName};
_msgBufPtr = UnpackData( _msgBufPtr, {parm.serverAddr}, {parm.numBytes} );\
"""

    serverPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.serverAddr}, {parm.numBytes} );\
"""

    asyncServerParmList = """\
{parm.asyncServerParmType} {parm.asyncServerParmName}\
"""

    asyncServerPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.serverAddr}, {parm.numBytes} );\
"""

    # Ensure that the array/string length is not greater than the maximum from the API definition.
    # This only applies to IN arrays/strings, because only IN arrays/strings have a maxValue.
    maxValueCheck = """\
if ( {parm.value} > {parm.maxValue} ) LE_FATAL("{parm.value} > {parm.maxValue}");\
"""


    def __init__(self, pName, pType):

        # TODO: This check is for backwards compatibility; should be removed at some point.
        if pType == 'handler':
            pType = pName
            pName = 'handler'

        self.baseName = pName
        self.baseType = pType

        self.name = pName

        # Check for handler type, followed by any other type
        if pType in HandlerTypes:
            self.type = HandlerTypes[pType]
            self.isHandler = True

        elif pType:
            self.type = ConvertInterfaceType(pType)

        else:
            # TODO: May want to do something more here, but at least get rid of the print
            self.type = ""
            print pName, repr(pType)

        # Always an input parameter
        self.direction = common.DIR_IN

        # Comment is initially empty
        self.commentLines = []


    # The following getter() functions allow determining the attribute value when accessed rather
    # than when the object is created.  This reduces to need for redefining some of the attributes
    # in the classes that inherit from this class.  If a class that inherits from this class wants
    # a different value, then it should explicitly define the attribute.

    # Name when used in parameter list
    @Getter
    def parmName(self):
        return self.name

    # Type when used in parameter list
    @Getter
    def parmType(self):
        return self.type

    @Getter
    def numBytes(self):
        return "sizeof(%s)" % self.type

    @Getter
    def address(self):
        return "&%s" % self.name

    @Getter
    def value(self):
        return self.name


    #
    # For server-side
    #

    @Getter
    def serverName(self):
        return self.name

    @Getter
    def serverType(self):
        return self.type

    @Getter
    def serverAddr(self):
        return self.address

    @Getter
    def serverCallName(self):
        return self.name

    #
    # For support of server-side async functions
    #

    @Getter
    def asyncServerParmName(self):
        return self.parmName

    @Getter
    def asyncServerParmType(self):
        return self.parmType

    @Getter
    def asyncServerCallName(self):
        return self.name


    def __str__(self):
        return "%s %s" % (self.type, self.name)


class PointerParmData(BaseParmData):

    def __init__(self, name, type, direction):
        super(PointerParmData, self).__init__(name, type)

        # todo: should probably verify that direction is one of the two
        #       allowable values: DIR_IN or DIR_OUT.
        self.direction = direction

        # Name when used in parameter list
        # todo: IN pointers should be "const"
        self.parmName = self.name+"Ptr"
        self.parmType = self.type+'*'

        self.address = self.parmName
        self.value = '*'+self.parmName

        self.serverAddr = "&%s" % self.name
        self.serverCallName = self.serverAddr

        # For support of server-side async functions
        self.asyncServerParmType = self.type
        self.asyncServerParmName = self.name



class FileInParmData(BaseParmData):

    def __init__(self, name):
        super(FileInParmData, self).__init__(name, RawType('int'))

        self.clientPack = """\
le_msg_SetFd(_msgRef, {parm.parmName});\
"""

        self.serverUnpack = """\
{parm.serverType} {parm.serverName};
{parm.serverName} = le_msg_GetFd(_msgRef);\
"""


class FileOutParmData(PointerParmData):

    def __init__(self, name):
        super(FileOutParmData, self).__init__(name, RawType('int'), common.DIR_OUT)

    clientUnpack = """\
{parm.value} = le_msg_GetFd(_responseMsgRef);\
"""

    serverPack = """\
le_msg_SetFd(_msgRef, {parm.serverName});\
"""

    asyncServerPack = """\
le_msg_SetFd(_msgRef, {parm.serverName});
"""



class ArrayParmData(BaseParmData):

    def __init__(self, name, type, direction, maxSize=None, minSize=None):
        super(ArrayParmData, self).__init__(name, type)

        # IN arrays can have both maxSize and minSize. OUT arrays only have minSize.
        if maxSize is not None:
            self.maxSize = maxSize
        if minSize is not None:
            self.minSize = minSize

        # todo: should probably verify that direction is one of the two
        #       allowable values: DIR_IN or DIR_OUT.
        self.direction = direction

        # Name when used in parameter list
        self.parmName = self.name+"Ptr"
        self.parmType = self.type+'*'

        # Name of auto-generated size variable
        self.sizeVar = self.name+"NumElements"

        self.numBytes = "%s*sizeof(%s)" % (self.sizeVar, self.type)
        self.address = self.parmName

        self.serverName = "%s[%s]" % (self.name, self.sizeVar)
        self.serverAddr = self.name

        # Some details depend on direction
        if self.direction == common.DIR_IN:
            # IN arrays should be "const"
            self.parmType = "const " + self.parmType
        else:
            # OUT arrays have INOUT size parameters, and so the arrays have different expressions
            # for numBytes on the client and server side.  Rather than defining two different
            # numBytes instance variables, just pre-evaluate the templates here, with the two
            # different expressions for numBytes.

            # Definition for the client side -- size is a pointer variable
            self.numBytes = "%s*sizeof(%s)" % ('*%sPtr'%self.sizeVar, self.type)
            self.clientUnpack = self.clientUnpack.format(parm=self)

            # Definition for the server side -- size is not a pointer variable
            self.numBytes = "%s*sizeof(%s)" % (self.sizeVar, self.type)
            self.serverPack = self.serverPack.format(parm=self)

            # For the respond function, need a slightly different packing rule, and then go
            # ahead and pre-evaluate it, to ensure the correct numBytes is used.
            # todo: do we really need to pre-evaluate this?
            self.asyncServerPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.serverAddr}Ptr, {parm.numBytes} );\
""".format(parm=self)


class StringParmData(BaseParmData):

    def __init__(self, name, direction, maxSize=None, minSize=None):
        super(StringParmData, self).__init__(name, RawType('char'))

        # todo: should probably verify that direction is one of the two
        #       allowable values: DIR_IN or DIR_OUT.
        self.direction = direction

        if self.direction == common.DIR_IN:
            self.initInString(maxSize, minSize)
        else:
            self.initOutString(minSize)


    def initInString(self, maxSize=None, minSize=None):
        # For strings, the "value" of the parameter is actually the string length.
        # self.value is used when comparing against minValue/maxValue;
        self.value = "strlen(%s)" % self.name

        # IN strings do not have an explicit length parameter, so the minValue/maxValue checks
        # will be done against the length of the string parameter itself.
        if maxSize is not None:
            self.maxValue = maxSize
        if minSize is not None:
            self.minValue = minSize

        # todo: Should we add "Str" suffix here?
        #self.parmName = self.name
        self.parmType = self.type+'*'

        # IN strings should be "const"
        self.parmType = "const " + self.parmType

        # Need to add 1 to maxValue to account for the terminating NULL-character
        self.numBytes = self.maxValue+1
        self.serverName = "%s[%s]" % (self.name, self.numBytes )
        self.serverAddr = self.name

        self.clientPack = """\
_msgBufPtr = PackString( _msgBufPtr, {parm.parmName} );\
"""

        self.serverUnpack = """\
{parm.serverType} {parm.serverName};
_msgBufPtr = UnpackString( _msgBufPtr, {parm.serverAddr}, {parm.numBytes} );\
"""


    def initOutString(self, minSize=None):
        # todo: The code in this method was copied from ArrayParm; might need some cleanup ...

        # OUT strings only have a minSize, similar to Arrays, however, need to add 1 to account
        # for the terminating NULL-character.  Also save the original value for later use.
        if minSize is not None:
            self.baseMinSize = minSize
            self.minSize = eval('minSize+1')

        # Type when used in parameter list
        self.parmType = self.type+'*'

        # Name of auto-generated size variable
        self.sizeVar = self.name+"NumElements"

        self.numBytes = "%s*sizeof(%s)" % (self.sizeVar, self.type)
        self.address = self.parmName

        # Need to init the string to an empty string, in case the function that is called does not
        # actually return a value in this string. This ensures that a valid string is packed, even
        # if it is just empty.  The init has to be done in a separate statement, since the size
        # of the string is variable.
        #
        # todo:
        #  - If/when strings are packed/unpacked like other data, this init may no longer be needed.
        self.serverName = "%s[%s]; %s[0]=0" % (self.name, self.sizeVar, self.name)
        self.serverAddr = self.name

        self.asyncServerPack = """\
_msgBufPtr = PackString( _msgBufPtr, {parm.serverAddr} );\
"""

        self.serverPack = self.asyncServerPack

        self.clientUnpack = """\
_msgBufPtr = UnpackString( _msgBufPtr, {parm.address}, {parm.numBytes} );\
"""



class HandlerPointerParmData(BaseParmData):

    def __init__(self, name, type):

        # The handler variable name should end in 'Ptr'
        if not name.endswith('Ptr'):
            name += 'Ptr'

        super(HandlerPointerParmData, self).__init__(name, type)

        # Always an input parameter
        self.direction = common.DIR_IN

        # TODO: There is additional code in codeGen.FuncImplTemplate, that uses the client data
        #       block. Should the code below also go there, to be symmetric.
        self.clientPack = """\
// The handlerPtr and contextPtr input parameters are stored in the client data object, and it is
// a safe reference to this object that is passed down as the context pointer.  The handlerPtr is
// not passed down.
// Create a new client data object and fill it in
_ClientData_t* _clientDataPtr = le_mem_ForceAlloc(_ClientDataPool);
_clientDataPtr->handlerPtr = (le_event_HandlerFunc_t){parm.parmName};
_clientDataPtr->contextPtr = contextPtr;
_clientDataPtr->callersThreadRef = le_thread_GetCurrent();
// Create a safeRef to be passed down as the contextPtr
_LOCK
contextPtr = le_ref_CreateRef(_HandlerRefMap, _clientDataPtr);
_UNLOCK\
"""

        # Nothing to do in this case
        self.serverUnpack = ""


    def setFuncName(self, funcName, funcType):
        self.funcName = funcName
        self.funcType = funcType
        self.serverCallName = "AsyncResponse_%s" % self.funcName



#---------------------------------------------------------------------------------------------------
# Factory functions for parameter data
#
# In some cases, factory functions are used to generate parameter data instances; in other cases,
# the instances are directly created from the class definition.
#
# TODO: This may need to be revisited, as part of cleaning up the class definitions.
#
#---------------------------------------------------------------------------------------------------

def SimpleParmData(pName, pType, direction=common.DIR_IN):

    if direction == common.DIR_IN:
        return BaseParmData( pName, pType )
    elif direction == common.DIR_OUT:
        return PointerParmData( pName, pType, common.DIR_OUT )


def FileParmData(pName, direction):

    if direction == common.DIR_IN:
        return FileInParmData( pName )
    elif direction == common.DIR_OUT:
        return FileOutParmData( pName )



#--------------------------------------------------------------------------
# Function data related classes
#--------------------------------------------------------------------------

class BaseFunctionData(object):

    def __init__(self, funcName, funcType, parmList, comment=""):
        # Keep the base name and type, since the "name" and "type" may be modified later
        self.baseName = funcName
        self.baseType = funcType

        # Add the prefix to all function names
        self.name = AddNamePrefix(funcName)

        # If there is a return type, convert the interface type to the approrpriate C type
        if funcType:
            self.type = ConvertInterfaceType(funcType)
        else:
            self.type = ""

        self.parmList = parmList

        # Comment is initially empty, unless explicitly given.
        self.comment = comment


    def __str__(self):
        resultList = [ self.type, self.name ]
        for p in self.parmList:
            resultList.append ( " "*4 + str(p) )

        return '\n'.join(resultList) + '\n'



class FunctionData(BaseFunctionData):

    def __init__(self, funcName, funcType, parmList, comment=""):

        # Process the parameter list before using it to init the instance.
        #
        # Note that inCallList is the same as inList, but the parameters are ordered in function
        # call order, rather than packing/unpacking order.
        parmList, inList, inCallList, outList = self.processParmList(parmList)
        super(FunctionData, self).__init__(funcName, funcType, parmList, comment)

        self.parmListIn = inList
        self.parmListInCall = inCallList
        self.parmListOut = outList

        # Extra info is needed if this function is an Add or Remove handler function, or if it
        # has a handler as a parameter (which includes Add handler functions).  If the function
        # has a handler parameter, also need to know the handler name.  If the name is None, then
        # the function does not have a handler parameter.
        #
        # todo: maybe this would be better done through separate classes and isinstance()
        self.handlerName = None
        self.isAddHandler = False
        self.isRemoveHandler = False

        if self.type:
            self.resultStorage = "%s _result;" % self.type
        else:
            self.resultStorage = ""


    def processParmList(self, parmList):
        #
        # todo: Update this comment
        #
        # Create the list of input parameters, which is used by the packing
        # and unpacking code.  All the array parameters should be put at the
        # end of the list, so that their corresponding length parameters get
        # packed and unpacked first.  This is necessary to know how much big
        # the unpacked arrrays will be.
        # Also create the list of output parameters, which is also used for
        # packing and unpacking in the opposite direction.  The length for
        # these output parameters are themselves input parameters, so there
        # is no need to re-order the output parameter list.
        #

        newParmList = []
        inList = []
        outList = []
        inCallList = []

        for p in parmList:

            # Add the parameter to the new list.  Additional parameters may get added below.
            newParmList.append(p)

            # Put the parameter in either the inList or outList.  Array parameters are handled
            # specially because additional parameters are added.
            if p.direction == common.DIR_IN:
                if isinstance( p, ArrayParmData ):

                    # Insert size parameter into newParmList as well as inList.
                    #
                    # Normal usage seems to be that the size parameter comes after the data
                    # parameter, but when packing/unpacking it has to come first.
                    sizeParm = BaseParmData( p.sizeVar, RawType("size_t") )

                    # todo: Setting maxValue/minValue needs to be done somewhere else, but do it
                    #       here for now.
                    sizeParm.maxValue = p.maxSize
                    if hasattr(p, 'minSize'):
                        sizeParm.minValue = p.minSize


                    newParmList.append(sizeParm)

                    inList.append(sizeParm)
                    inList.append(p)

                    inCallList.append(p)
                    inCallList.append(sizeParm)

                else:
                    inList.append(p)
                    inCallList.append(p)

            else:
                # todo: Handle length parameters for OUT arrays. I wonder if these length parameters
                #       would then be INOUT parameters.  Perhaps if the size was fixed, then it
                #       would just be an IN parameter to give the buffer size.  Hmmm, have to think
                #       about what max size means for an OUT parameter.
                # must be an output parameter
                #outList.append(p)

                if isinstance( p, ArrayParmData ):

                    # Insert size parameter into newParmList as well as inList.
                    #
                    # Normal usage seems to be that the size parameter comes after the data
                    # parameter, but when packing/unpacking it has to come first.
                    #
                    # This parm is the only INOUT parameter currently used.
                    sizeParm = PointerParmData( p.sizeVar, RawType("size_t"), common.DIR_INOUT )

                    # todo: This should be done somewhere else, but do it here for now.
                    #       This is needed so that the variable is not defined two, since it is in both
                    #       the IN list and OUT list.
                    sizeParm.serverParmList = ""

                    # todo: Setting maxValue/minValue needs to be done somewhere else, but do it
                    #       here for now.
                    # todo: What does minSize/maxSize actually mean for an OUT array.  Isn't maxSize
                    #       really minValue, i.e. the provided buffer must be at least large enough
                    #       to hold maxSize.  In addition, does it really make sense to specify a
                    #       minSize, i.e. what would it be used for; how does it differ from maxSize.
                    #sizeParm.maxValue = p.maxSize
                    if hasattr(p, 'minSize'):
                        sizeParm.minValue = p.minSize

                        # After unpacking the value, range check and limit the value on the server
                        # side, in case the client passed in a value larger than specified, which
                        # is actually a valid thing to do.
                        sizeParm.serverUnpack +=  """
if ( {parm.serverName} > {parm.minValue} )
{{
    LE_DEBUG("Adjusting {parm.serverName} from %zd to {parm.minValue}", {parm.serverName});
    {parm.serverName} = {parm.minValue};
}}\
"""

                    # Add the parameter to the appropriate lists.  Note that it goes into both
                    # the IN and OUT lists, since it is an INOUT parameter.
                    newParmList.append(sizeParm)

                    inList.append(sizeParm)
                    inCallList.append(sizeParm)
                    outList.append(sizeParm)
                    outList.append(p)

                elif isinstance( p, StringParmData ):
                    # OUT strings are different from OUT arrays in that the size parameter is IN
                    # rather than INOUT.

                    # Insert size parameter into newParmList as well as inList.
                    #
                    # Normal usage seems to be that the size parameter comes after the data
                    # parameter, but when packing/unpacking it has to come first.
                    sizeParm = BaseParmData( p.sizeVar, RawType("size_t") )

                    # todo: Setting maxValue/minValue needs to be done somewhere else, but do it
                    #       here for now.
                    # todo: Similar to OUT arrays above, there is only a minSize.
                    #sizeParm.maxValue = p.maxSize
                    if hasattr(p, 'minSize'):
                        sizeParm.minValue = p.minSize

                    # Get the original size specified to the OUT string, before any adjustments.
                    if hasattr(p, 'baseMinSize'):
                        sizeParm.baseMinValue = p.baseMinSize

                    newParmList.append(sizeParm)

                    inList.append(sizeParm)
                    inCallList.append(sizeParm)
                    outList.append(p)

                else:
                    outList.append(p)

        return newParmList, inList, inCallList, outList



class HandlerFuncData(BaseFunctionData):

    def __init__(self, funcName, parmList, comment=""):
        def addArrayLengthParameters(params):
            callParams = list()
            transferParams = list()
            for p in params:
                callParams.append(p)
                if isinstance(p, ArrayParmData):
                    sizeParam = BaseParmData(p.sizeVar, RawType("size_t"))
                    sizeParam.maxValue = p.maxSize
                    if hasattr(p, 'minSize'):
                        sizeParam.minValue = p.minSize
                    # When transferring on the wire, the size is sent before the array.  When
                    # calling the function, the size parameter comes after the array.
                    callParams.append(sizeParam)
                    transferParams.append(sizeParam)
                transferParams.append(p)
            return (callParams, transferParams)

        # Add the contextPtr variable to the parameter list before using it to init the instance.
        # Note that parmList may be a pyparsing type, so always convert to a real list.
        #
        # todo: There might be a better way to handle this rather than using BaseParmData(), but in
        #       this case, we actually want to pass the pointer value, rather than dereferencing it.
        contextParm = BaseParmData( "contextPtr", RawType("void*") )

        # The contextPtr is always explicitly packed and unpacked, so don't need to do it with
        # the rest of the parameters for the handler.
        contextParm.serverUnpack = ""
        contextParm.clientPack = ""
        parmList = list(parmList) + [ contextParm ]

        # Add the array length parameters into the parameter list
        (parmList, self.transferParams) = addArrayLengthParameters(parmList)

        # Init the instance
        # The funcName is actually used as the handler type (todo: maybe change this), and the
        # convention for the type is to add the 'Func_t' suffix.
        super(HandlerFuncData, self).__init__(funcName+'Func_t', '', parmList, comment)

        # Store the full name, for mapping from API names to C names
        HandlerTypes[funcName] = self.name


class EventFuncData(BaseFunctionData):

    def __init__(self, funcName, parmList, comment=""):
        super(EventFuncData, self).__init__(funcName, '', parmList, comment)

        # Add the prefix to the type.
        self.type = AddNamePrefix(self.type)



#--------------------------------------------------------------------------
# Type definition related classes
#--------------------------------------------------------------------------


class ReferenceData(BaseTypeData):

    definitionTemplate = """\
typedef struct {self.name}* {self.refName};\
"""

    def __init__(self, name, comment=''):
        self.baseName = name
        self.name = AddNamePrefix(self.baseName)
        self.refName = self.name+"Ref_t"
        self.comment = comment

        AddInterfaceType(self.baseName, self.refName)

        # Pre-evaluate the type definition
        self.definition = self.definitionTemplate.format(self=self)



class DefineData(BaseTypeData):

    definitionTemplate = """\
#define {self.name} {self.value}\
"""

    def __init__(self, name, value, comment=''):
        self.baseName = name
        self.name = AddNamePrefix(self.baseName).upper()
        self.value = value
        self.comment = comment

        AddInterfaceType(self.baseName, self.name)
        AddDefinitionValue(self.baseName, self.value)

        # Pre-evaluate the type definition
        self.definition = self.definitionTemplate.format(self=self)


#
# Remaining classes below use commonTypes ...
#

# Class for a single enum member
class EnumMemberData(commonTypes.EnumMemberData):

    def GetLangType(self, typeName):
        return AddNamePrefix(typeName).upper()



# Class for the complete enum definition
class EnumData(commonTypes.EnumData):

    def GetLangType(self, typeName):
        return AddNamePrefix(typeName+"_t")



# A bit mask is a type of enum, but the default member values are bit shifted
class BitMaskData(commonTypes.BitMaskData):

    # Want the output value to be in hex, instead of decimal
    valueFormatFunc = hex

    def GetLangType(self, typeName):
        return AddNamePrefix(typeName+"_t")


