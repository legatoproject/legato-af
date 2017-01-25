#
# Implement the Sample language specific parts of the ifgen command line tool
#
# Copyright (C) Sierra Wireless Inc.
#

# System libraries
import os
import sys

# Language independent libraries
import interfaceParser

# Language dependent libraries
import codeTypes
import codeGen


def AddLangArgumentGroup(parentParser):
    # Define the command line arguments/options
    parser = parentParser.add_argument_group("'Sample' language specific")

    parser.add_argument('--print-header',
                        dest="printHeader",
                        action='store_true',
                        default=False,
                        help='print out documentation header')

    parser.add_argument('--print-functions',
                        dest="printFunctions",
                        action='store_true',
                        default=False,
                        help='print out all functions')

    parser.add_argument('--print-handlers',
                        dest="printHandlers",
                        action='store_true',
                        default=False,
                        help='print out all handlers')

    parser.add_argument('--print-events',
                        dest="printEvents",
                        action='store_true',
                        default=False,
                        help='print out all events')

    parser.add_argument('--print-all',
                        dest="printAll",
                        action='store_true',
                        default=False,
                        help='print out all code info')

    parser.add_argument('--long-format',
                        dest="longFormat",
                        action='store_true',
                        default=False,
                        help='use long output format')



def ProcessArguments(args):
    # Process the command lines arguments, if necessary

    # If --print-all is specified, it forces all info to be printed
    if args.printAll:
        args.printFunctions=True
        args.printHandlers=True
        args.printEvents=True



def GetImportedCodeList(importList):
    #print importList
    importedCodeList = []

    for path in importList:
        fullname = os.path.basename(path)
        name = os.path.splitext(fullname)[0]

        data = open(path, 'r').read()

        # In the current .api file, the imported types will be referenced using "name.",
        codeTypes.SetImportName(name)

        # Parse the imported file, which implicitly populates the type translation data
        parsedCode = interfaceParser.ParseCode(data, path)
        codeList = parsedCode['codeList']
        importedCodeList += codeList

    return importedCodeList



def RunCommand(args,
               data,
               importList,
               hashValue):

    ProcessArguments(args)

    # Process all the imported files first, and get the corresponding code objects
    importedCodeList = GetImportedCodeList(importList)

    # Reset the import name, since this is the main file.
    codeTypes.SetImportName("")

    # Parse the API file and get all the code objects
    parsedCode = interfaceParser.ParseCode(data, args.interfaceFile)
    allCodeList = importedCodeList + parsedCode['codeList']

    # Dump info on the parsed file.  No need to generate any code.
    if args.dump:
        # Print out info on all the types, functions and handlers
        codeTypes.PrintCode(allCodeList)

        # Print out the type dictionary.
        print '='*40 + '\n'
        codeTypes.PrintDefinedTypes()
        sys.exit(0)

    # Pass 'args' so that the function can determine what needs to be output
    codeGen.WriteAllCode(args, parsedCode, hashValue)


