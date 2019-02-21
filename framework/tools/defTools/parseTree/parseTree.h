//--------------------------------------------------------------------------------------------------
/**
 * @file parseTree.h
 *
 * Content_t is the base class for all file content items, including Token_t, and CompoundItem_t.
 *
 * The root of each parse tree is a DefFile_t.  It has a pointer to the first token (Token_t) that
 * was parsed from it.  It also has a list of top-level sections (CompoundItem_t).
 *
 * Each Token_t keeps track of its exact text and the file, line, and column where it was found.
 * As tokens are parsed from the file, they are linked together into a doubly-linked list.
 * The DefFile_t has a pointer to the first Token_t in the file.
 *
 * Each CompoundItem_t has a type indicating what kind of item it is.  It also has pointers to its
 * first and last Token_t and a list of Content_t items that are inside it.  If it is a simple
 * section or named item with only a single name or number inside it, there will only be one
 * Token_t pointer inside this list.  If it is a more complex section or named item, there could
 * be any number of Content_t objects in this content list, and it could even be empty if there's
 * nothing but a pair of curly braces with nothing but whitespace or comments between them.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PARSE_TREE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PARSE_TREE_H_INCLUDE_GUARD

namespace parseTree
{

#include "content.h"
#include "token.h"
#include "compoundItem.h"
#include "defFile.h"
#include "substitution.h"

} // namespace parseTree

#endif // LEGATO_DEFTOOLS_PARSE_TREE_H_INCLUDE_GUARD
