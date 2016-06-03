#
# Implement the C language specific parts of the ifgen command line tool
#
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    parser = parentParser.add_argument_group('C Language Specific')

    parser.add_argument('--gen-all',
                        dest="genAll",
                        action='store_true',
                        default=False,
                        help='generate all files; overrides individual file options')

    parser.add_argument('--gen-interface',
                        dest="genInterface",
                        action='store_true',
                        default=False,
                        help='generate interface header file')

    parser.add_argument('--gen-local',
                        dest="genLocal",
                        action='store_true',
                        default=False,
                        help='generate local header file')

    parser.add_argument('--gen-client',
                        dest="genClient",
                        action='store_true',
                        default=False,
                        help='generate client IPC implementation file')

    parser.add_argument('--gen-server-interface',
                        dest="genServerInterface",
                        action='store_true',
                        default=False,
                        help='generate server interface header file')

    parser.add_argument('--gen-server',
                        dest="genServer",
                        action='store_true',
                        default=False,
                        help='generate server IPC implementation file')

    parser.add_argument('--async-server',
                        dest="async",
                        action='store_true',
                        default=False,
                        help='generate asynchronous-style server functions')

    parser.add_argument('--name-prefix',
                        dest="namePrefix",
                        default='',
                        help='''optional prefix for generated functions/types; defaults to input
                        filename''')

    parser.add_argument('--file-prefix',
                        dest="filePrefix",
                        default='',
                        help='optional prefix for generated files; defaults to input file name')

    parser.add_argument('--no-default-prefix',
                        dest="noDefaultPrefix",
                        action='store_true',
                        default=False,
                        help='do not use default file or name prefix if none is specified')



def ProcessArguments(args):
    # Process the command lines arguments

    # If --gen-all is specified, it forces all files to be generated
    if args.genAll:
        args.genInterface=True
        args.genLocal=True
        args.genClient=True
        args.genServerInterface=True
        args.genServer=True

    # Get the base file name, without any extensions.
    apiFileName = os.path.splitext( os.path.basename(args.interfaceFile) )[0]

    # If appropriate, use the default name or file prefixes
    if not args.noDefaultPrefix:
        if not args.namePrefix:
            args.namePrefix = apiFileName
        if not args.filePrefix:
            args.filePrefix = apiFileName

    # todo: Remove this once all callers are updated.
    #       The usage has changed slightly, so the trailing '_' will be added when necessary.
    #       Until all callers have been updated, strip off the trailing underscore, if given.
    if args.namePrefix and args.namePrefix[-1] == '_':
        args.namePrefix = args.namePrefix[:-1]

    if args.filePrefix and args.filePrefix[-1] == '_':
        args.filePrefix = args.filePrefix[:-1]



def GetImportedCodeList(importList):
    #print importList
    importedCodeList = []

    for path in importList:
        fullname = os.path.basename(path)
        name = os.path.splitext(fullname)[0]

        data = open(path, 'r').read()

        # In the current .api file, the imported types will be referenced using "name.",
        # whereas in the generated C code, the prefix will be "name_".
        codeTypes.SetImportName(name)
        codeTypes.SetNamePrefix(name)

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

    # Set the name prefix first.  This has to be done before the interface is actually
    # parsed, since the resulting parsedCode will use the prefix.  Also, reset the
    # import name, since this is the main file.
    codeTypes.SetNamePrefix(args.namePrefix)
    codeTypes.SetImportName("")

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


