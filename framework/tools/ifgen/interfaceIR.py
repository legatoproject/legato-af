#
# Interface AST
#
# Copyright (C) Sierra Wireless Inc.
#

import collections
import copy
import os

#---------------------------------------------------------------------------------------------------
# Constants
#---------------------------------------------------------------------------------------------------
DIR_IN  = 1
DIR_OUT = 2
DIR_INOUT = (DIR_IN | DIR_OUT)

#---------------------------------------------------------------------------------------------------
# Named values
#---------------------------------------------------------------------------------------------------
class NamedValue(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value
        self.comments = []

    def __repr__(self):
        return "NamedValue({},{})".format(repr(self.name), self.value)

#---------------------------------------------------------------------------------------------------
# Types
#---------------------------------------------------------------------------------------------------
class Type(object):
    def __init__(self, name, size):
        self.iface = None
        self.name = name
        if (os.environ.get('LE_CONFIG_RPC') == "y"):
            # Include a 1-byte TagID
            self.size = size + 1
        else:
            self.size = size
        self.comment = ""

    def __str__(self):
        return "Type<%d> %s" % (self.size, self.name)

    def __repr__(self):
        return "Type({},{})".format(repr(self.name), self.size)

class BasicType(Type):
    """Basic type represents a built-in type"""
    def __init__(self, name, size):
        super(BasicType, self).__init__(name, size)

    def __str__(self):
        return "BasicType<%d> %s" % (self.size, self.name)

    def __repr__(self):
        return "BasicType({},{})".format(repr(self.name), self.size)

class EnumType(Type):
    def __init__(self, name, elements=[]):
        nextValue = 0
        size = 4
        resolvedElements = []
        for element in elements:
            resolvedElement = copy.copy(element)
            if resolvedElement.value == None:
                resolvedElement.value = nextValue
                nextValue += 1
            else:
                nextValue = resolvedElement.value + 1

            if resolvedElement.value >= (1 << (size*8)):
                size *= 2
                if size > 8:
                    raise Exception("Enum '%s' is larger than 64-bits." %
                                    (name,))
            resolvedElements.append(resolvedElement)

        super(EnumType, self).__init__(name, size)

        self.elements = resolvedElements

        # Check for duplicate elements
        values = [ element.value for element in self.elements ]
        if len(values) != len(set(values)):
            raise Exception('Enum %s has duplicate values' % (name,))

    def __str__(self):
        return "Enum<%d> %s (%d elements)" % (self.size, self.name, len(self.elements))

    def __repr__(self):
        return "EnumType({},{})".format(repr(self.name), repr(self.elements))


class BitmaskType(Type):
    def __init__(self, name, elements=[]):
        nextValue = 1
        size = 4
        resolvedElements = []
        for element in elements:
            resolvedElement = copy.copy(element)
            if resolvedElement.value == None:
                resolvedElement.value = nextValue
                nextValue <<= 1
            else:
                nextValue = resolvedElement.value << 1
                if nextValue == 0:
                    nextValue = 1

            if resolvedElement.value & (resolvedElement.value -1):
                raise Exception('Value %s of bitmask %s is not a power of 2' %
                                (resolvedElement.name, name))
            if resolvedElement.value >= (1 << (size*8)):
                size *= 2
                if size > 8:
                    raise Exception("Bitmask '%s' is larger than 64-bits." %
                                    (name,))
            resolvedElements.append(resolvedElement)

        super(BitmaskType, self).__init__(name, size)

        self.elements = resolvedElements

        # Check for duplicate elements
        values = [ element.value for element in self.elements ]
        if len(values) != len(set(values)):
            raise Exception('Enum %s has duplicate values' % (name,))

    def __str__(self):
        return "BitMask<%d> %s (%d elements)" % (self.size, self.name, len(self.elements))

    def __repr__(self):
        return "EnumType({},{})".format(repr(self.name), repr(self.elements))

class ReferenceType(Type):
    def __init__(self, name):
        super(ReferenceType, self).__init__(name, 4)

    def __str__(self):
        return "Reference %s" % (self.name)

    def __repr__(self):
        return "ReferenceType({})".format(self.name)

class HandlerReferenceType(ReferenceType):
    """
    Add/Remove functions for events use a reference to track which handler is being added/removed
    from the event.

    A special type is used for this so the event it's associated with can be tracked.
    """
    def __init__(self, eventObj):
        super(HandlerReferenceType, self).__init__(eventObj.name + "Handler")
        self.event = eventObj

    def __str__(self):
        return "HandlerRef %s" % (self.name, )

class HandlerType(Type):
    def __init__(self, name, parameters):
        # Handlers are the size of a reference
        super(HandlerType, self).__init__(name, 4)
        self.parameters = parameters

        if any([isinstance(parameter.apiType, HandlerType) for parameter in self.parameters]):
            raise Exception("Handlers cannot have handler parameters")

    def __str__(self):
        return "Handler %s(%s)" \
            % (self.name,
               ", ".join([str(param) for param in self.parameters]))

    def GetMessageSize(self):
        return sum([parameter.GetMaxSize(DIR_IN) for parameter in self.parameters])

class OldHandlerType(Type):
    """
    Magic type for old-style handlers.  When used as a parameter, will be converted to a handler
    with type equal to the name of the parameter.  Error when used in other contexts.
    """
    def __init__(self):
        super(OldHandlerType, self).__init__("handler", 4)

    def __str__(self):
        return "OldHandler"

class StructMember(object):
    def __init__(self, apiType, name):
        assert not isinstance(apiType, HandlerReferenceType)
        assert not isinstance(apiType, HandlerType)
        assert not isinstance(apiType, OldHandlerType)

        self.apiType = apiType
        self.name = name

    def MaxSize(self):
        return self.apiType.size

    def __string__(self):
        return "{} {}".format(apiType, name)

class StructStringMember(StructMember):
    def __init__(self, name, maxCount):
        super(StructStringMember, self).__init__(STRING_TYPE, name)
        self.maxCount = maxCount

    def MaxSize(self):
        return self.maxCount * self.apiType.size

    def __str__(self):
        return "{} {}[{}]".format(self.apiType, self.name, self.maxCount)

class StructArrayMember(StructMember):
    def __init__(self, apiType, name, maxCount):
        super(StructArrayMember, self).__init__(apiType, name)
        self.maxCount = maxCount

    def MaxSize(self):
        return self.maxCount * self.apiType.size

    def __str__(self):
        return "{} {}[{}]".format(self.apiType, self.name, self.maxCount)

class StructType(Type):
    """
    Structure type.  Contains a list of other types and names (similar to C structures).
    """
    def __init__(self, name, members=[]):
        size = 0
        for member in members:
            assert isinstance(member, StructMember)
            size += member.MaxSize()

        super(StructType, self).__init__(name, size)
        self.members = list(members)

    def __str__(self):
        return "Struct {} { {} }".format(self.name,
                                         ["{};".format(member)
                                          for member in self.members].join(" "))

def MakeStructMember(interface, typeObj, name, arraySize):
    """Helper to make a parameter object"""
    if isinstance(typeObj, BasicType) and typeObj.name == "string":
        # Strings are special
        if arraySize == None:
            raise Exception("String needs a size limit")
        return StructStringMember(name, arraySize)
    elif arraySize != None:
        # If a range is provided, it's an array
        return StructArrayMember(typeObj, name, arraySize)
    else:
        # Simple structure member
        return StructMember(typeObj, name)


#---------------------------------------------------------------------------------------------------
# Basic types
#---------------------------------------------------------------------------------------------------
UINT8_TYPE  = BasicType('uint8', 1)
UINT16_TYPE = BasicType('uint16', 2)
UINT32_TYPE = BasicType('uint32', 4)
UINT64_TYPE = BasicType('uint64', 8)
INT8_TYPE   = BasicType('int8', 1)
INT16_TYPE  = BasicType('int16', 2)
INT32_TYPE  = BasicType('int32', 4)
INT64_TYPE  = BasicType('int64', 8)
BOOL_TYPE   = BasicType('bool', 1)
CHAR_TYPE   = BasicType('char', 1)
DOUBLE_TYPE = BasicType('double', 8)
SIZE_TYPE   = BasicType('size', 4)
STRING_TYPE = BasicType('string', 1)
FILE_TYPE   = BasicType('file', 0)
RESULT_TYPE = BasicType('le_result_t', 4)
ONOFF_TYPE  = BasicType('le_onoff_t', 4)
# Indicates an error occurred parsing a type -- e.g. reference to type that doesn't exist
ERROR_TYPE  = BasicType('**ERROR**', 0)
# Magic old-handler type
OLD_HANDLER_TYPE = OldHandlerType()

#---------------------------------------------------------------------------------------------------
# Formal parameters
#---------------------------------------------------------------------------------------------------
class Parameter(object):
    def __init__(self, apiType, name, direction=DIR_IN):
        self.apiType = apiType
        self.name = name
        self.direction = direction
        self.comments = []

    def GetMaxSize(self, direction):
        if direction & self.direction != 0:
            return self.apiType.size
        else:
            return 0

    def __str__(self):
        result = "%s %s " % (self.apiType.name, self.name)
        if self.direction & DIR_IN:
            result += "IN"
        if self.direction & DIR_OUT:
            result += "OUT"

        return result

    def __repr__(self):
        return "<Parameter {}>".format(str(self))

class ArrayParameter(Parameter):
    def __init__(self, apiType, name, maxCount, direction=DIR_IN):
        super(ArrayParameter, self).__init__(apiType, name, direction)
        self.maxCount = maxCount

    def GetMaxSize(self, direction):
        if direction == DIR_IN:
            if self.direction & DIR_IN != 0:
                return UINT32_TYPE.size + self.apiType.size * self.maxCount
            else:
                return UINT32_TYPE.size
        elif direction == DIR_OUT:
            if self.direction & DIR_OUT:
                return UINT32_TYPE.size + self.apiType.size * self.maxCount
            else:
                return 0

    def __str__(self):
        result = "%s %s[%d] " % (self.apiType.name, self.name, self.maxCount)
        if self.direction & DIR_IN:
            result += "IN"
        if self.direction & DIR_OUT:
            result += "OUT"
        return result

    def __repr__(self):
        return "<ArrayParameter {}>".format(str(self))

class StringParameter(Parameter):
    def __init__(self, name, maxCount, direction=DIR_IN):
        super(StringParameter, self).__init__(STRING_TYPE, name, direction)
        self.maxCount = maxCount

    def GetMaxSize(self, direction):
        # Size of a string element is always 1.
        if direction == DIR_IN:
            if self.direction & DIR_IN:
                return UINT32_TYPE.size + self.maxCount
            else:
                return UINT32_TYPE.size
        elif direction == DIR_OUT:
            if self.direction & DIR_OUT:
                return UINT32_TYPE.size + self.maxCount
            else:
                return 0

    def __str__(self):
        result = "%s %s[%d] " % (self.apiType.name, self.name, self.maxCount)
        if self.direction & DIR_IN:
            result += "IN"
        if self.direction & DIR_OUT:
            result += "OUT"
        return result

    def __repr__(self):
        return "<StringParameter {}>".format(str(self))

def MakeParameter(interface, typeObj, name, arraySize, direction=DIR_IN):
    """Helper to make a parameter object"""
    if direction == None:
        direction = DIR_IN

    if typeObj == OLD_HANDLER_TYPE:
        # Convert old handler parameters to parameter type here.
        typeObj = interface.findType(name)
        name = "handler"

    if isinstance(typeObj, BasicType) and typeObj.name == "string":
        # Strings are special
        if arraySize == None:
            raise Exception("String needs a size limit")
        return StringParameter(name, arraySize, direction)
    elif arraySize != None:
        if isinstance(typeObj, HandlerType):
            raise Exception("Cannot have arrays of handlers")
        # If a range is provided, it's an array
        return ArrayParameter(typeObj, name, arraySize, direction)
    else:
        # Simple parameter
        return Parameter(typeObj, name, direction)

#---------------------------------------------------------------------------------------------------
# Declarations
#---------------------------------------------------------------------------------------------------
class Definition(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value
        self.comment = ""

    def __str__(self):
        return "%s = %s" % (self.name, repr(self.value))

    def __repr__(self):
        return "Definition({},{})".format(repr(self.name), repr(self.value))

class Function(object):
    def __init__(self, returnType, name, parameters):
        self.returnType = returnType
        self.name = name
        self.parameters = parameters

        if returnType == OLD_HANDLER_TYPE or isinstance(returnType, HandlerType):
            raise Exception ('Functions cannot return handlers')

        # All functions can have exactly one parameter.
        handlers = [ (index, handler) for (index, handler) in enumerate(parameters)
                     if isinstance(handler.apiType, HandlerType) ]
        if len(handlers) > 1:
            raise Exception('A function can only have one handler parameter')

        self.comment = ""

    def __str__(self):
        if self.returnType == None:
            return "FUNCTION %s(%s)" \
                % (self.name,
                   ", ".join([str(param) for param in self.parameters]))
        else:
            return "FUNCTION %s %s(%s)" \
                % (self.returnType.name, self.name,
                   ", ".join([str(param) for param in self.parameters]))

    def __repr__(self):
        return "<Function {} {}({})>".format(self.returnType.name if self.returnType else "None",
                                             self.name,
                                             ",".join([repr(param) for param in self.parameters]))

    def GetMessageSize(self):
        return max(sum([parameter.GetMaxSize(DIR_IN) for parameter in self.parameters]),
                   sum([self.returnType.size if self.returnType else 0] +
                       [parameter.GetMaxSize(DIR_OUT) for parameter in self.parameters]))

class Event(object):
    def __init__(self, name, parameters):
        self.name = name
        self.parameters = parameters
        self.comment = ""

        if len([handler for handler in self.parameters
                if isinstance(handler.apiType, HandlerType)]) != 1:
            raise Exception("Events must have exactly one handler")


    def __str__(self):
        return "EVENT %s(%s)" \
            % (self.name,
               ", ".join([str(param) for param in self.parameters]))

    def __repr__(self):
        return "<Event {}({})>".format(self.name,
                                       ",".join([repr(param) for param in self.parameters]))


class EventFunction(Function):
    """
    Events add two functions to the API: one to add handlers, one to remove them.

    These functions get a special type so they can track which events they're associated with.
    """
    def __init__(self, eventObj, returnType, name, parameters):
        super(EventFunction, self).__init__(returnType, name, parameters)
        self.event = eventObj


#---------------------------------------------------------------------------------------------------
# AST definition
#---------------------------------------------------------------------------------------------------
class Interface(object):
    """Interface definition.  Represents a single API file"""
    _basicTypes = { 'uint8':  UINT8_TYPE,
                    'uint16': UINT16_TYPE,
                    'uint32': UINT32_TYPE,
                    'uint64': UINT64_TYPE,
                    'int8':   INT8_TYPE,
                    'int16':  INT16_TYPE,
                    'int32':  INT32_TYPE,
                    'int64':  INT64_TYPE,
                    'bool':   BOOL_TYPE,
                    'char':   CHAR_TYPE,
                    'double': DOUBLE_TYPE,
                    'size':   SIZE_TYPE,
                    'string': STRING_TYPE,
                    'file':   FILE_TYPE,
                    'le_result_t': RESULT_TYPE,
                    'le_onoff_t': ONOFF_TYPE }

    def __init__(self):
        # Use ordered dictionaries here so these get emitted in the generated files in the
        # order they were declared.
        self.name = None
        self.baseName = None
        self.path = None
        self.imports = collections.OrderedDict()
        self.types = collections.OrderedDict()
        self.definitions = collections.OrderedDict()
        self.functions = collections.OrderedDict()
        self.events = collections.OrderedDict()
        self.comments = []
        self.text = None

    def isTypeNameUsed(self, name):
        return ((name in Interface._basicTypes) or
                (name in self.types))

    def _findLocalType(self, typeName):
        """Look up a type in this interface, without considering basic types"""
        splitType = typeName.split('.')
        # Dotted types are imported
        if len(splitType) == 2:
            return self.imports[splitType[0]]._findLocalType(splitType[1])
        elif len(splitType) == 1:
            return self.types[splitType[0]]
        else:
            # Multiple dots are illegal
            raise KeyError(typeName)

    def findType(self, typeName):
        """Look up a type"""
        if typeName == "handler":
            # Magic handler type
            return OLD_HANDLER_TYPE
        elif typeName in Interface._basicTypes:
            # Always consider basic types first
            return Interface._basicTypes[typeName]
        else:
            return self._findLocalType(typeName)

    @classmethod
    def findBasicType(cls, typeName):
        """Look up a basic type"""
        return cls._basicTypes[typeName]

    def findDefinition(self, definitionName):
        """Look up a definition"""
        splitDefinition = definitionName.split('.')
        # Dotted definitions are imported
        if len(splitDefinition) == 2:
            return self.imports[splitDefinition[0]].findDefinition(splitDefinition[1])
        elif len(splitDefinition) == 1:
            return self.definitions[splitDefinition[0]]
        else:
            # Multiple dots are illegal
            raise Exception('Invalid definition %s' % (definitionName))

    def addType(self, typeObj):
        """Add a type"""
        if self.isTypeNameUsed(typeObj.name):
            raise Exception("Redefining %s" % typeObj.name)

        typeObj.iface = self
        self.types[typeObj.name] = typeObj

    def addDefinition(self, definitionObj):
        """ Add a value definition"""
        if definitionObj.name in self.definitions:
            raise Exception("Redefining %s" % definitionObj.name)

        self.definitions[definitionObj.name] = definitionObj

    def addFunction(self, functionObj):
        """Add a function"""
        if functionObj.name in self.functions:
            raise Exception("Redefining %s" % functionObj.name)

        self.functions[functionObj.name] = functionObj

    def addEvent(self, eventObj):
        """Add an event"""
        if eventObj.name in self.events:
            raise Exception("Redefining %s" % eventObj.name)

        self.events[eventObj.name] = eventObj

        # Events also need a reference and functions for add/remove handlers.  Add these here.

        eventRefType = HandlerReferenceType(eventObj)
        eventRefType.iface = self
        eventRefType.comment = \
            "\n Reference type used by Add/Remove functions for EVENT '%s_%s'\n" % (self.name,
                                                                                    eventObj.name)

        # This type cannot be referred to in an API file, so decorate it with "@event@" so it
        # will not conflict with any types declared in the API file
        self.types["@event@" + eventObj.name] = eventRefType

        eventAddFunc = EventFunction(eventObj,
                                     eventRefType,
                                     "Add%sHandler" % (eventObj.name,),
                                     eventObj.parameters)
        eventAddFunc.comment = \
            "\n Add handler function for EVENT '%s_%s'\n%s" % (self.name,
                                                               eventObj.name,
                                                               eventObj.comment)
        self.addFunction(eventAddFunc)

        eventRemoveFunc = EventFunction(eventObj,
                                        None,
                                        "Remove%sHandler" % (eventObj.name,),
                                        [ Parameter(eventRefType, u"handlerRef") ])
        eventRemoveFunc.comment = \
            "\n Remove handler function for EVENT '%s_%s'\n" % (self.name,
                                                                eventObj.name)
        self.addFunction(eventRemoveFunc)

    def addDeclaration(self, declObj):
        if isinstance(declObj, Type):
            self.addType(declObj)
        elif isinstance(declObj, Definition):
            self.addDefinition(declObj)
        elif isinstance(declObj, Function):
            self.addFunction(declObj)
        elif isinstance(declObj, Event):
            self.addEvent(declObj)
        else:
            raise Exception("Unknown declaration object type")

    def getMessageSize(self):
        """
        Get size of largest possible message to a function or handler.

        A message is 4-bytes for message ID, optional 4
        bytes for required output parameters, optional 1 byte for TagID,
        and a variable number of bytes to pack
        the return value (if the function has one), and all input and output parameters.
        """
        padding = 8
        if (os.environ.get('LE_CONFIG_RPC') == "y"):
            # Include a 1-byte TagID
            padding = padding + 1
        return padding + max([1] +
                       [function.GetMessageSize() for function in self.functions.values()] +
                       [handler.GetMessageSize()
                        for handler in self.types.values() if isinstance(handler, HandlerType)])


    def __str__(self):
        resultStr  = "=== Interface ===\n"
        resultStr += "Imports:\n"
        for importKey, importValue in self.imports.iteritems():
            resultStr += "    %s\n" % (importKey,)
        resultStr += "\n";
        resultStr += "Definitions:\n"
        for defnKey, defnValue in self.definitions.iteritems():
            resultStr += "    %s\n" % (str(defnValue))
        resultStr += "\n";
        resultStr += "Types:\n"
        for typeKey, typeValue in self.types.iteritems():
            resultStr += "    %s\n" % (str(typeValue))
        resultStr += "\n";
        resultStr += "Functions:\n"
        for functionKey, functionValue in self.functions.iteritems():
            resultStr += "    %s\n" % (str(functionValue))
        resultStr += "\n";
        resultStr += "Events:\n"
        for eventKey, eventValue in self.events.iteritems():
            resultStr += "    %s\n" % (str(eventValue))
        return resultStr

    def __repr__(self):
        return "<Interface Imports:[{}] Definitions:[{}] Types:[{}] Functions:[{}] Events:[{}]>"\
            .format(
                ','.join([repr(apiImport) for apiImport in self.imports.itervalues()]),
                ','.join([repr(definition) for definition in self.definitions.itervalues()]),
                ','.join([repr(apiType) for apiType in self.types.itervalues()]),
                ','.join([repr(function) for function in self.functions.itervalues()]),
                ','.join([repr(event) for event in self.events.itervalues()]))
