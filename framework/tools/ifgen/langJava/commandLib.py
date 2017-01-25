
# --------------------------------------------------------------------------------------------------
#
#  Called by the core ifgen code, this will add the language specific command line arguments and
#  kick off the code generation.
#
#  Copyright (C) Sierra Wireless Inc.
#
# --------------------------------------------------------------------------------------------------
import os
import sys

import interfaceParser
import codeTypes
import codeGen




# --------------------------------------------------------------------------------------------------
# Called to add our custom command line arguments to the mix.
# --------------------------------------------------------------------------------------------------
def AddLangArgumentGroup(parentParser):

    parser = parentParser.add_argument_group('Java Language Specific')

    parser.add_argument('--gen-client',
                        dest="genClient",
                        action='store_true',
                        default=False,
                        help='generate client IPC implementation file')

    parser.add_argument('--gen-server',
                        dest="genServer",
                        action='store_true',
                        default=False,
                        help='generate server IPC implementation file')

    parser.add_argument('--name-prefix',
                        dest="namePrefix",
                        default='',
                        help='''optional prefix for generated functions/types; defaults to input
                        filename''')




# --------------------------------------------------------------------------------------------------
# Called by ifgen to generate Java code.
# --------------------------------------------------------------------------------------------------



def GetImportedCodeList(importList):
    #print importList
    importedCodeList = []
    importedNameList = []

    for path in importList:
        fullname = os.path.basename(path)
        name = os.path.splitext(fullname)[0]

        data = open(path, 'r').read()

        # In the current .api file, the imported types will be referenced using "name.",
        importedNameList.append(name)
        codeTypes.SetImportName(name)

        # Parse the imported file, which implicitly populates the type translation data
        parsedCode = interfaceParser.ParseCode(data, path)
        codeList = parsedCode['codeList']
        importedCodeList += codeList

    return ( importedCodeList, importedNameList )



def RunCommand(args, data, importList, hashValue):

    if args.namePrefix:
        args.serviceName = args.namePrefix

    # Process all the imported files first, and get the corresponding code objects
    ( importedCodeList, importedNameList ) = GetImportedCodeList(importList)

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
    codeGen.WriteAllCode(args, importedCodeList, importedNameList, parsedCode, hashValue)
