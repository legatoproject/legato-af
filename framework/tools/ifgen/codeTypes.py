#
# Define types/classes for different code/message objects
#
# Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
#

import hashlib


#
# Direction for function parameters.  IN and OUT can be specified in the .api file, but INOUT
# is for internal auto-generated variables only.
#
# todo: Update checks for DIR_IN to not assume DIR_OUT in the else part
#
DIR_IN = "IN"
DIR_OUT = "OUT"
DIR_INOUT = "INOUT"   # for internal use only


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


# Global for storing the name of the imported file. Empty if the file is not imported.
ImportName = ""

# Set the global import name variable
def SetImportName(name):
    global ImportName
    ImportName = name


DefinedInterfaceTypes = dict(
    # unsigned ints
    uint8   = "uint8_t",
    uint16  = "uint16_t",
    uint32  = "uint32_t",
    uint64  = "uint64_t",

    # signed ints
    int8    = "int8_t",
    int16   = "int16_t",
    int32   = "int32_t",
    int64   = "int64_t",

    # string/char
    # todo: Maybe we should use string as the interface type, and map that into char for C.
    #       It may not matter that much, since it is only intended to be used internally.
    char    = "char"
)

# Convert from the interfaceType, as used in the interface files, to the corresponding C type.
def ConvertInterfaceType(interfaceType):
    if interfaceType in DefinedInterfaceTypes:
        return DefinedInterfaceTypes[interfaceType]

    # todo: Since user-defined interface types are not supported yet, map anything that is not
    #       pre-defined, back to the original value, which is hopefully a valid C type.  Once
    #       user-defined types are supported, this should probably raise an exception or something.
    return interfaceType


# Add a mapping from the interfaceType, as used in the interface files, to the corresponding C type.
def AddInterfaceType(interfaceType, cType):
    # Prepend the import name, if defined
    if ImportName:
        interfaceType = "%s.%s" % (ImportName, interfaceType)

    # Do not allow multiple definitions for the same type
    if interfaceType not in DefinedInterfaceTypes:
        DefinedInterfaceTypes[interfaceType] = cType
    else:
        # todo: Should this be a fatal error?
        print "ERROR: %s already defined as %s" % (interfaceType, cType)



# Holds the values of DEFINEd symbols
DefinedValues = dict()


# Empty class to hold attribute values
class Empty(object): pass


# Adds values to the DEFINEd symbols dictionary
def AddDefinitionValue(name, value):
    # todo: Should we check for redefinitions?

    # To support using imported values, create an empty object and put it in the DefinedValues
    # dictionary.  Then, set attributes for this object for each imported value.  Thus, the eval
    # will magically pick up those values.
    if ImportName:
        if ImportName not in DefinedValues:
            DefinedValues[ImportName] = Empty()

        setattr(DefinedValues[ImportName], name, value)
    else:
        DefinedValues[name] = value


# Evaluates the DEFINE expression using the DEFINEd symbols dictionary
def EvaluateDefinition(defn):

    # If evaluating a definition in an imported file, use the dictionary for that imported file.
    if ImportName and ImportName in DefinedValues:
        evalGlobals = DefinedValues[ImportName].__dict__
    else:
        evalGlobals = DefinedValues

    # Use a copy() of evalGlobals, so that eval doesn't put anything into our dictionaries
    return eval(defn, evalGlobals.copy())



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

class SimpleData(object):

    # These are the default templates for all the parameter data classes.
    # A particular class can over-ride these, by defining the corresponding
    # instance variable.
    #
    # todo: Maybe I should change the "handler" prefix to "server"
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

    handlerParmList = """\
{parm.unpackType} {parm.unpackName};\
"""

    handlerUnpack = """\
{parm.unpackType} {parm.unpackName};
_msgBufPtr = UnpackData( _msgBufPtr, {parm.unpackAddr}, {parm.numBytes} );\
"""

    handlerPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.unpackAddr}, {parm.numBytes} );\
"""

    asyncServerParmList = """\
{parm.asyncServerParmType} {parm.asyncServerParmName}\
"""

    asyncServerPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.unpackAddr}, {parm.numBytes} );\
"""

    # Ensure that the array/string length is not greater than the maximum from the API definition.
    # This only applies to IN arrays/strings, because only IN arrays/strings have a maxValue.
    maxValueCheck = """\
if ( {parm.value} > {parm.maxValue} ) LE_FATAL("{parm.value} > {parm.maxValue}");\
"""


    def __init__(self, name, type):
        self.name = name
        self.type = ConvertInterfaceType(type)

        # Always an input parameter
        self.direction = DIR_IN

        # Name when used in parameter list
        self.parmName = self.name
        self.parmType = self.type

        self.numBytes = "sizeof(%s)" % self.type
        self.address = "&%s" % self.name
        self.value = self.name

        # todo: maybe should change the "unpack" prefix to "handler", since this
        #       seems to be more appropriate, and would also deal with the case
        #       of out parameters, which are packed by the handler.
        self.unpackName = self.name
        self.unpackType = self.type
        self.unpackAddr = self.address
        self.unpackCallName = self.name

        # For support of server-side async functions
        self.asyncServerCallName = self.name

        # Comment is initially empty
        self.comment = ""

    @Getter
    def asyncServerParmType(self):
        return self.parmType

    @Getter
    def asyncServerParmName(self):
        return self.parmName


    def __str__(self):
        return "%s %s" % (self.type, self.name)


    def getHashString(self):
        """
        Return a string with the relevant info for the variable, which can be used for computing
        the digest/hash for the code.
        """
        # todo: Does the variable name actually matter?
        resultList = [ self.type, self.direction ]

        # The baseMinValue takes precedence over minValue, since baseMinValue is the original value
        # specified in the interface, whereas minValue may have been modified.
        if hasattr(self, "baseMinValue"):
            resultList.append( str(self.baseMinValue) )
        elif hasattr(self, "minValue"):
            resultList.append( str(self.minValue) )

        if hasattr(self, "maxValue"):
            resultList.append( str(self.maxValue) )

        # Create a one line string.  Values are separated by spaces for readability
        result = ' '.join( resultList )

        return result


class PointerData(SimpleData):

    def __init__(self, name, type, direction):
        super(PointerData, self).__init__(name, type)

        # todo: should probably verify that direction is one of the two
        #       allowable values: DIR_IN or DIR_OUT.
        self.direction = direction

        # Name when used in parameter list
        # todo: IN pointers should be "const"
        self.parmName = self.name+"Ptr"
        self.parmType = self.type+'*'

        self.numBytes = "sizeof(%s)" % self.type
        self.address = self.parmName
        self.value = '*'+self.parmName

        self.unpackName = self.name
        self.unpackType = self.type
        self.unpackAddr = "&%s" % self.name
        self.unpackCallName = self.unpackAddr

        # For support of server-side async functions
        self.asyncServerParmType = self.type
        self.asyncServerParmName = self.name


class ArrayData(SimpleData):

    def __init__(self, name, type, direction, maxSize=None, minSize=None):
        super(ArrayData, self).__init__(name, type)

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

        self.unpackName = "%s[%s]" % (self.name, self.sizeVar)
        self.unpackType = self.type
        self.unpackAddr = self.name
        self.unpackCallName = self.name

        # Some details depend on direction
        if self.direction == DIR_IN:
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
            self.handlerPack = self.handlerPack.format(parm=self)

            # For the respond function, need a slightly different packing rule, and then go
            # ahead and pre-evaluate it, to ensure the correct numBytes is used.
            # todo: do we really need to pre-evaluate this?
            self.asyncServerPack = """\
_msgBufPtr = PackData( _msgBufPtr, {parm.unpackAddr}Ptr, {parm.numBytes} );\
""".format(parm=self)


class StringData(SimpleData):

    def __init__(self, name, direction, maxSize=None, minSize=None):
        super(StringData, self).__init__(name, 'char')

        # todo: should probably verify that direction is one of the two
        #       allowable values: DIR_IN or DIR_OUT.
        self.direction = direction

        if self.direction == DIR_IN:
            self.initInString(name, maxSize, minSize)
        else:
            self.initOutString(name, minSize)


    def initInString(self, name, maxSize=None, minSize=None):
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
        self.parmName = self.name
        self.parmType = self.type+'*'

        # IN strings should be "const"
        self.parmType = "const " + self.parmType

        #self.numBytes = "%s*sizeof(%s)" % (size, self.type.rstrip('*'))
        #self.address = self.name
        #self.unpackName = "%s[%s]" % (self.name, size)
        #self.unpackType = self.type.rstrip('*')
        #self.unpackAddr = self.address
        self.unpackCallName = self.name

        self.clientPack = """\
_msgBufPtr = PackString( _msgBufPtr, {parm.parmName} );\
"""

        self.handlerUnpack = """\
{parm.parmType} {parm.parmName};
_msgBufPtr = UnpackString( _msgBufPtr, &{parm.name} );\
"""


    def initOutString(self, name, minSize=None):
        # todo: The code in this method was copied from ArrayParm; might need some cleanup ...

        # OUT strings only have a minSize, similar to Arrays, however, need to add 1 to account
        # for the terminating NULL-character.  Also save the original value for later use.
        if minSize is not None:
            self.baseMinSize = minSize
            self.minSize = eval('minSize+1')

        # Name when used in parameter list
        self.parmName = self.name
        self.parmType = self.type+'*'

        # Name of auto-generated size variable
        self.sizeVar = self.name+"NumElements"

        self.numBytes = "%s*sizeof(%s)" % (self.sizeVar, self.type)
        self.address = self.parmName

        self.unpackName = "%s[%s]" % (self.name, self.sizeVar)
        self.unpackType = self.type
        self.unpackAddr = self.name
        self.unpackCallName = self.name

        # todo:
        # Originally OUT strings were handled the same as any other data, and used the regular
        # PackData and UnpackData functions, and thus these definitions were not necessary.
        #
        # However, this will not work for server-side async function support, since the Respond
        # function does not (easily) have the necessary data size.  The solution is to use
        # PackString() on the server side, and UnpackDataString() on the client side.  The one
        # problem with this solution is if the server returns a string that is too long for the
        # client buffer, the string will get truncated.  TBD whether it is acceptable to truncate
        # the string in this case, or whether something else should be done.
        self.asyncServerPack = """\
_msgBufPtr = PackString( _msgBufPtr, {parm.unpackAddr} );\
"""

        # This is not strictly necessary, but ensures that strings get packed the same way
        # regardless of whether in the regular server-side function or in the respond function.
        self.handlerPack = self.asyncServerPack

        self.clientUnpack = """\
_msgBufPtr = UnpackDataString( _msgBufPtr, {parm.address}, {parm.numBytes} );\
"""



class HandlerParmData(SimpleData):

    def __init__(self, name, type):
        super(HandlerParmData, self).__init__(name, type)

        # Always an input parameter
        self.direction = DIR_IN

        self.clientPack = """\
        // This parameter is stored locally, rather than passed down.  Eventually
        // the handler will be registered with the event loop, rather than being
        // stored locally.  This registration will return a result, but until then
        // just hard-code a fake result.
        HandlerRef_{parm.funcName} = {parm.name};
        _result=({parm.funcType})1;\
        """

        self.handlerUnpack = ""


    def setFuncName(self, funcName, funcType):
        self.funcName = funcName
        self.funcType = funcType
        self.unpackCallName = "AsyncResponse_%s" % self.funcName



class VoidData(SimpleData):

    def __init__(self):
        super(VoidData, self).__init__('', 'void')

        # Nothing to pack or unpack
        self.clientPack = ""
        self.handlerUnpack = ""



#--------------------------------------------------------------------------
# Function data related classes
#--------------------------------------------------------------------------

class BaseFunctionData(object):

    # Short name for the class, used when printing out the hash string.
    ClassName = "BASE"

    def __init__(self, funcName, funcType, parmList, comment=""):
        self.name = funcName
        self.type = ConvertInterfaceType(funcType)
        self.parmList = parmList

        # Keep the base name and type, since the "name" and "type" may be modified later
        self.baseName = self.name
        self.baseType = self.type

        # Add the prefix to all function names
        self.name = AddNamePrefix(self.name)

        # Comment is initially empty, unless explicitly given.
        self.comment = comment


    def __str__(self):
        resultList = [ self.type, self.name ]
        for p in self.parmList:
            resultList.append ( " "*4 + str(p) )

        return '\n'.join(resultList) + '\n'


    def getHashString(self):
        """
        Return a string with the relevant info for the function, which can be used for computing
        the digest/hash for the code.
        """
        resultList = [ self.ClassName ]

        # Some classes only have a type or a name, but not both.  It will be an empty string,
        # if not available
        if self.baseType:
            resultList.append(self.baseType)
        if self.baseName:
            resultList.append(self.baseName)

        # Add in all the parameters, and create a one line string.  Values are separated by spaces
        # for readability
        result = " ".join( resultList + [ p.getHashString() for p in self.parmList ] )

        return result



class FunctionData(BaseFunctionData):

    ClassName = "FUNCTION"

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

        # Extra info is needed if this function is an Add or Remove handler function.
        # todo: maybe this would be better done through separate classes and isinstance()
        self.addHandlerName = None
        self.isRemoveHandler = False

        if self.type != 'void':
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
            if p.direction == DIR_IN:
                if isinstance( p, ArrayData ):

                    # Insert size parameter into newParmList as well as inList.
                    #
                    # Normal usage seems to be that the size parameter comes after the data
                    # parameter, but when packing/unpacking it has to come first.
                    #
                    # todo: It is probably wrong to use size_t here and below.  What should be
                    #       used instead? uint32_t?
                    sizeParm = SimpleData( p.sizeVar, "size_t" )

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

                if isinstance( p, ArrayData ):

                    # Insert size parameter into newParmList as well as inList.
                    #
                    # Normal usage seems to be that the size parameter comes after the data
                    # parameter, but when packing/unpacking it has to come first.
                    #
                    # This parm is the only INOUT parameter currently used.
                    sizeParm = PointerData( p.sizeVar, "size_t", DIR_INOUT )

                    # todo: This should be done somewhere else, but do it here for now.
                    #       This is needed so that the variable is not defined two, since it is in both
                    #       the IN list and OUT list.
                    sizeParm.handlerParmList = ""

                    # todo: Setting maxValue/minValue needs to be done somewhere else, but do it
                    #       here for now.
                    # todo: What does minSize/maxSize actually mean for an OUT array.  Isn't maxSize
                    #       really minValue, i.e. the provided buffer must be at least large enough
                    #       to hold maxSize.  In addition, does it really make sense to specify a
                    #       minSize, i.e. what would it be used for; how does it differ from maxSize.
                    #sizeParm.maxValue = p.maxSize
                    if hasattr(p, 'minSize'):
                        sizeParm.minValue = p.minSize


                    # Add the parameter to the appropriate lists.  Note that it goes into both
                    # the IN and OUT lists, since it is an INOUT parameter.
                    newParmList.append(sizeParm)

                    inList.append(sizeParm)
                    inCallList.append(sizeParm)
                    outList.append(sizeParm)
                    outList.append(p)

                elif isinstance( p, StringData ):
                    # OUT strings are different from OUT arrays in that the size parameter is IN
                    # rather than INOUT.

                    # Insert size parameter into newParmList as well as inList.
                    #
                    # Normal usage seems to be that the size parameter comes after the data
                    # parameter, but when packing/unpacking it has to come first.
                    sizeParm = SimpleData( p.sizeVar, "size_t" )

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


class HandlerFunctionData(BaseFunctionData):

    ClassName = "HANDLER"

    def __init__(self, funcName, funcType, parmList, comment=""):

        # Add the contextPtr variable to the parameter list before using it to init the instance.
        # If the handler does not have any parameters, the parameter list will only contain VoidData.
        # Note that parmList may be a pyparsing type, so always convert to a real list.
        #
        # todo: There might be a better way to handle this rather than using SimpleData(), but in
        #       this case, we actually want to pass the pointer value, rather than dereferencing it.
        contextParm = SimpleData( "contextPtr", "void*" )
        if (len(parmList) == 1) and isinstance(parmList[0], VoidData):
            parmList = [ contextParm ]
        else:
            parmList = list(parmList) + [ contextParm ]

        # The contextPtr is always explicitly packed and unpacked, so don't need to do it with
        # the rest of the parameters for the handler.
        contextParm.handlerUnpack = ""
        contextParm.clientPack = ""

        # Init the instance
        super(HandlerFunctionData, self).__init__(funcName, funcType, parmList, comment)


class AddHandlerFunctionData(BaseFunctionData):

    ClassName = "ADD_HANDLER"

    def __init__(self, funcName, funcType, parmList, comment=""):
        super(AddHandlerFunctionData, self).__init__(funcName, funcType, parmList, comment)

        # Add the prefix to the type.
        self.type = AddNamePrefix(self.type)



#--------------------------------------------------------------------------
# Type definition related classes
#--------------------------------------------------------------------------


# Base type for all type classes
class BaseTypeData(object):

    # todo: Should I use the same format as the functions, and have the keyword as the first item?
    #       Perhaps the other classes that define this method could super() call this method to get
    #       the common prefix, and then add anything else that they wanted.
    def getHashString(self):
        return self.baseName


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

    def getHashString(self):
        return "%s %s" % (self.baseName, self.value)


# Class for a single enum item/value
class EnumValue(BaseTypeData):

    def __init__(self, name, comment=''):
        self.baseName = name
        self.name = AddNamePrefix(self.baseName).upper()
        self.comment = comment


# Class for the complete enum definition
class EnumData(BaseTypeData):

    def __init__(self, name, valueList, comment=''):
        self.baseName = name
        self.name = AddNamePrefix(self.baseName)
        self.typeName = self.name+"_t"
        self.valueList = valueList
        self.comment = comment

        AddInterfaceType(self.baseName, self.typeName)

    def getHashString(self):
        return self.baseName + ' ' + ' '.join( v.getHashString() for v in self.valueList )



#--------------------------------------------------------------------------
# Import related classes
# (todo: Are these part of the type definition related classes??)
#--------------------------------------------------------------------------


class ImportData(object):

    def __init__(self, name):
        self.name = name



#--------------------------------------------------------------------------
# Hash related functions
#--------------------------------------------------------------------------

#
# Generate a string representation of the interface, suitable for generating a hash/digest
#
def GetHashString(codeList):
    hashString = '\n'.join ( c.getHashString() for c in codeList )
    return hashString


#
# Get the hash/digest value for the interface
#
def GetHash(codeList):

    h = hashlib.sha256()
    h.update( GetHashString(codeList) )

    return h.hexdigest()



#--------------------------------------------------------------------------
# Misc functions mainly for testing/debugging
#--------------------------------------------------------------------------

#
# Print out the code list in a simple text format
#
def PrintCode(codeList):
    #print codeList

    for c in codeList:
        print '-'*10
        print c


#
# Print out the current mapping from .api types to C types.
#
def PrintDefinedTypes():
    for k, v in DefinedInterfaceTypes.items():
        print '%-20s => %s' % (k, v)
    print '-'*40

    for k, v in DefinedValues.items():
        if isinstance(v, Empty):
            for k1, v1 in v.__dict__.items():
                print '%-20s => %-20s => %s' % (k, k1, v1)
        else:
            print '%-20s => %s' % (k, v)
    print '-'*40


