#
# Define types/classes for different code/message objects
#
# Copyright (C) Sierra Wireless Inc.
#

# Python libraries
import sys

# For reporting exceptions to interface parser
import pyparsing

# ifgen specific libraries
import common


# Global for storing the name of the imported file. Empty if the file is not imported.
ImportName = ""

# Set the global import name variable
def SetImportName(name):
    global ImportName
    ImportName = name

# Add the import name prefix to the given name
# This would normally be called from the language specific codeTypes.
def AddImportNamePrefix(name):
    if ImportName:
        return "%s.%s" % (ImportName, name)
    else:
        return name


# Pre-defined interface types
#   - size is in bytes
#   - size of None means length should be calculated a different way, or is not valid to be used.
#   - value is the language specific translation
DefinedInterfaceTypes = dict(
    # unsigned ints
    uint8   = dict(size=1, value=None),
    uint16  = dict(size=2, value=None),
    uint32  = dict(size=4, value=None),
    uint64  = dict(size=8, value=None),
    uint    = dict(size=4, value=None),

    # signed ints
    int8    = dict(size=1, value=None),
    int16   = dict(size=2, value=None),
    int32   = dict(size=4, value=None),
    int64   = dict(size=8, value=None),
    int     = dict(size=4, value=None),

    # boolean
    bool = dict(size=1, value=None),

    # a single char -- this is not for string support
    char    = dict(size=1, value=None),

    # double is sufficient -- no need to support float as well.
    double = dict(size=8, value=None),

    # for array sizes and indices, and things like that
    size = dict(size=8, value=None),

    # these are not used for C, but may be needed for other languages
    string = dict(size=None, value=None),
    file = dict(size=4, value=None),

    # TODO: This is temporary, so that errors are not generated for these types.
    #       The issues with these types will be resolved soon, at which point these
    #       lines will be removed.
    le_result_t = dict(size=4, value=None),
    le_onoff_t = dict(size=4, value=None)
)



def InitPredefinedInterfaces(definitions):
    # 'definitions' maps from predefined types to language specific types.

    for k in definitions:
        if k not in DefinedInterfaceTypes:
            sys.exit("'%s' is not a predefined type" % k)

        DefinedInterfaceTypes[k]['value'] = definitions[k]



# Convert from the interfaceType, as used in the interface files, to the corresponding language type.
def ConvertInterfaceType(interfaceType):
    # If we are processing an imported file, then the types defined in that file will be prefixed
    # by the file name. First try the type as-is, and if that is not found, try with file name.
    # TODO: I'm wondering if there could be some issue here with trying both with and without the
    #       prefix. More testing and thought is needed.
    value = None
    if interfaceType in DefinedInterfaceTypes:
        value = DefinedInterfaceTypes[interfaceType]['value']
    elif ImportName:
        importedInterfaceType = "%s.%s" % (ImportName, interfaceType)
        if importedInterfaceType in DefinedInterfaceTypes:
            value = DefinedInterfaceTypes[importedInterfaceType]['value']

    if value is None:
        #print ("unknown type '%s'" % interfaceType)
        raise pyparsing.ParseException("unknown type '%s'" % interfaceType)

    return value



# Add a mapping from the interfaceType, as used in the .api files, to the language specific type.
def AddInterfaceType(interfaceType, langType, typeSize=None):
    # Prepend the import name, if defined
    if ImportName:
        interfaceType = "%s.%s" % (ImportName, interfaceType)

    # Do not allow multiple definitions for the same type
    if interfaceType not in DefinedInterfaceTypes:
        DefinedInterfaceTypes[interfaceType] = dict(size=typeSize, value=langType)
    else:
        # todo: Should this be a fatal error?
        print "ERROR: %s already defined as %s" % (interfaceType, langType)



# Holds the values of DEFINEd symbols
DefinedValues = dict()


# Holds the handler definitions
HandlerTypes = dict()


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
            importedValues = Empty()

            # First, add any previously imported value objects into this newly created object
            for k,v in DefinedValues.items():
                if isinstance(v, Empty):
                    setattr(importedValues, k, v)

            DefinedValues[ImportName] = importedValues

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



#---------------------------------------------------------------------------------------------------
# Parameter data related classes
#---------------------------------------------------------------------------------------------------

class BaseParmData(object):

    def __init__(self, pName, pType, direction=None, maxSize=None, minSize=None):
        self.baseName = pName
        self.baseType = pType

        self.pName = pName

        # Check for handler type, followed by any other type
        if pType in HandlerTypes:
            self.pType = HandlerTypes[pType]
            self.isHandler = True

        elif pType:
            self.pType = ConvertInterfaceType(pType)

        else:
            # TODO: May want to do something more here, but at least get rid of the print
            self.pType = ""
            print pName, repr(pType)

        if direction is not None:
            self.direction = direction
        else:
            # Not given, so default to DIR_IN
            self.direction = common.DIR_IN

        if maxSize is not None:
            self.maxSize = maxSize

        if minSize is not None:
            self.minSize = minSize

        # Comment is initially empty.  This attribute is set after the instance has been created,
        # and so is not passed to __init__().
        self.commentLines = []


    def __str__(self):
        return "%s %s" % (self.pType, self.pName)



class SimpleParmData(BaseParmData):
    pass



class FileParmData(BaseParmData):

    def __init__(self, name, direction):
        super(FileParmData, self).__init__(name, 'file', direction)



class ArrayParmData(BaseParmData):
    pass



class StringParmData(BaseParmData):

    def __init__(self, name, direction, maxSize=None, minSize=None):
        super(StringParmData, self).__init__(name, 'string', direction, maxSize, minSize)



#---------------------------------------------------------------------------------------------------
# Function data related classes
#---------------------------------------------------------------------------------------------------

class BaseFunctionData(object):

    def __init__(self, funcName, funcType, parmList, comment=""):
        self.baseName = funcName
        self.baseType = funcType

        self.funcName = funcName

        # If there is a return type, convert the interface type to the appropriate language type
        if funcType:
            self.funcType = ConvertInterfaceType(funcType)
        else:
            self.funcType = ""

        self.parmList = parmList

        # Comment is initially empty, unless explicitly given.
        self.comment = comment


    def __str__(self):
        resultList = [ self.funcType, self.funcName ]
        for p in self.parmList:
            resultList.append ( " "*4 + str(p) )

        return '\n'.join(resultList) + '\n'



class FunctionData(BaseFunctionData):
    pass



class HandlerFuncData(BaseFunctionData):

    def __init__(self, funcName, parmList, comment=""):

        # The funcName is actually used as the handler type
        super(HandlerFuncData, self).__init__(self.GetLangType(funcName), '', parmList, comment)

        # Store the full name, for mapping from API names to language specific names
        # TODO: Having a separate HandlerTypes dictionary is based on the C version.
        #       Does it make sense to do it this way for the general case ????
        HandlerTypes[funcName] = self.funcName


    # Get the fully qualified language specific name for this API type
    def GetLangType(self, typeName):
        return typeName



class EventFuncData(BaseFunctionData):

    def __init__(self, funcName, parmList, comment=""):
        super(EventFuncData, self).__init__(funcName, '', parmList, comment)




#--------------------------------------------------------------------------
# Type definition related classes
#--------------------------------------------------------------------------


# Base type for all type classes
class BaseTypeData(object):

    # Get the fully qualified language specific name for this API type
    def GetLangType(self, typeName):
        return typeName


class ReferenceData(BaseTypeData):

    def __init__(self, name, comment=''):
        self.baseName = name
        self.name = self.GetLangType(self.baseName)
        self.comment = comment

        AddInterfaceType(self.baseName, self.name, 8)



class DefineData(BaseTypeData):

    def __init__(self, name, value, comment=''):
        self.baseName = name
        self.name = self.GetLangType(self.baseName)
        self.value = value
        self.comment = comment

        AddInterfaceType(self.baseName, self.name)
        AddDefinitionValue(self.baseName, self.value)



# Class for a single enum member
class EnumMemberData(BaseTypeData):

    def __init__(self, name, value=None):
        self.baseName = name
        self.name = self.GetLangType(self.baseName)

        # the given value will be an integer or None
        self.value = value

        # Initially empty
        self.commentLines = []


    def __str__(self):
        return '%s = %s' % (self.name, self.value)



# Class for the complete enum definition
class EnumData(BaseTypeData):

    typeSize = 4

    # Default starting value for enumeration, matches C implementation
    defaultStartValue = 0

    # Calculate the next default value for ENUM
    getNextValue = lambda self, x: x+1


    def __init__(self, name, memberList, comment=''):
        self.baseName = name
        self.name = self.GetLangType(self.baseName)
        self.memberList = memberList
        self.comment = comment

        AddInterfaceType(self.baseName, self.name, self.typeSize)

        # Assign default values to each enum member
        self.assignValues()


    def assignValues(self):
        enumValue = self.defaultStartValue
        for m in self.memberList:
            if m.value is not None:
                # If the enum value was explicitly given then use it for calculating
                # the next default value
                enumValue = m.value
            else:
                # Assign the default value, since it was not explicitly given.
                m.value = enumValue

            enumValue = self.getNextValue(enumValue)


    def __str__(self):
        resultList = [ self.name ]
        for m in self.memberList:
            resultList.append ( " "*4 + str(m) )

        return '\n'.join(resultList) + '\n'



# A bit mask is a type of enum, but the default member values are bit shifted
class BitMaskData(EnumData):

    typeSize = 8

    # Default starting value for enumeration, matches C implementation
    defaultStartValue = 1

    # Calculate the next default value for BITMASK
    # If the current value is 0, it needs a little help
    getNextValue = lambda self, x: x<<1 if (x != 0) else 1


    def __init__(self, name, memberList, comment=''):
        super(BitMaskData, self).__init__(name, memberList, comment)



#---------------------------------------------------------------------------------------------------
# Import related classes
# (todo: Are these part of the type definition related classes??)
#---------------------------------------------------------------------------------------------------


class ImportData(object):

    def __init__(self, name):
        self.name = name



#---------------------------------------------------------------------------------------------------
# Misc functions mainly for testing/debugging
#---------------------------------------------------------------------------------------------------

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

    for k, v in HandlerTypes.items():
        print '%-20s => %s' % (k, v)
    print '-'*40


