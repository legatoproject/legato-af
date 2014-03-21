
// -------------------------------------------------------------------------------------------------
/*
 *  Implementation of the low level tree DB structure.  This code also handles the persisting of the
 *  tree db to the filesystem.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdbool.h>
#include "legato.h"
#include "interfaces.h"
#include "stringBuffer.h"
#include "internalCfgTypes.h"
#include "treeDb.h"




// Path to the config tree directory in the linux filesystem.
#define CFG_TREE_PATH "/opt/legato/configTree"


// Set the size of each string segment.
#define SEGMENT_SIZE 28
// TODO: Tune SEGMENT_SIZE for efficiency.




// -------------------------------------------------------------------------------------------------
/**
 *  Names and values in the db all use this structure.  This way, these strings do not need to have
 *  a fixed size.  The string values are grown and shrunk as needed.
 */
// -------------------------------------------------------------------------------------------------
typedef struct NodeString_t
{
    char value[SEGMENT_SIZE];      ///< The actual text.  The segment is NULL padded, not NULL
                                   ///<   terminated.
    struct NodeString_t* nextPtr;  ///< A pointer to the next segment in the chain.
}
NodeString_t;




// -------------------------------------------------------------------------------------------------
/**
 *  Flags that can be set on a node to allow the code to keep track of the various changes as
 *  they're made to the nodes.
 */
// -------------------------------------------------------------------------------------------------
typedef enum
{
    NODE_FLAGS_UNSET = 0x0,  ///< No flags have been set.
    NODE_IS_SHADOW   = 0x1,  ///< The node is a shadow for a node in another tree.
    NODE_IS_MODIFIED = 0x2,  ///< This node has been modified.
    NODE_IS_DELETED  = 0x4   ///< This node has been marked as deleted, the actual deletion will
                             ///<   take place later.
}
NodeFlags_t;




// -------------------------------------------------------------------------------------------------
/**
 *  The node structure.
 */
// -------------------------------------------------------------------------------------------------
typedef struct tdb_Node_t
{
    struct tdb_Node_t* parentPtr;         ///< The parent node of this one.

    le_cfg_nodeType_t type;                  ///< What kind of value does this node hold.

    NodeFlags_t flags;                    ///< Various flags set on the node.
    struct tdb_Node_t* shadowPtr;         ///< If this node is shadowing another then the pointer to
                                          ///<   that shadowed node is here.

    NodeString_t name;                    ///< The name of this node.

    struct tdb_Node_t* siblingPtr;        ///< The linked list of node siblings.  All of the nodes
                                          ///<   in this list have the same parent node.

    // TODO: Callback chain...
    // TODO: See if I can convert this back to a union.
    struct
    {
        NodeString_t value;               ///< The value of the node.  This is only valid if the
                                          ///<   node is not a stem.

        struct
        {
            uint32_t count;               ///< Count of children of this node.
            struct tdb_Node_t* childPtr;  ///< The linked list of children belonging to this node.
        }
        children;                         ///< Values in this structure are only valid if this node
                                          ///<   is a stem.
    }
    info;                                 ///< The actual inforation that this node stores.
}
tdb_Node_t;




// The memory pool responsible for tree nodes.
le_mem_PoolRef_t NodePool = NULL;
#define CFG_NODE_POOL_NAME "configTree.nodePool"

// This pool is used to manage the memory used by the node strings.
le_mem_PoolRef_t NodeStringPool = NULL;
#define CFG_NODE_STRING_POOL_NAME "configTree.nodeStringPool"

// The collection of configuration trees managed by the system.
le_hashmap_Ref_t TreeCollectionRef = NULL;
le_mem_PoolRef_t TreePoolRef = NULL;

#define CFG_TREE_COLLECTION_NAME "configTree.treeCollection"
#define CFG_TREE_POOL_NAME       "configTree.treePool"








// ------------------------------------------------------------------------------------------------
/**
 *  Called to determine if a given node string is equivlant to "".
 */
// -------------------------------------------------------------------------------------------------
static bool IsNodeStringEmpty
(
    NodeString_t* nodeStringPtr  ///< The string to check.
)
{
    // If we're dealing with a NULL node string the consider it empty.
    if (nodeStringPtr == NULL)
    {
        return true;
    }

    // Also, consider the string empty if the first character is NULL.
    return nodeStringPtr->value[0] == '\0';
}




// ------------------------------------------------------------------------------------------------
/**
 *  Free up the memory used by the node string.
 */
// -------------------------------------------------------------------------------------------------
static void ReleaseNodeStr
(
    NodeString_t* nodeStrPtr  ///< The string to free.
)
{
    // Release the node string, and all of it's child items.
    while (nodeStrPtr != NULL)
    {
        NodeString_t* thisPtr = nodeStrPtr;

        nodeStrPtr = thisPtr->nextPtr;
        le_mem_Release(thisPtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Copy a node strng into a traditional string buffer.  This function also takes care to make sure
 *  that the destionation buffer is not overflowed.
 */
// -------------------------------------------------------------------------------------------------
static void CopyNodeStringToString
(
    char* destPtr,                  ///< The C string to copy into.
    size_t destMax,                 ///< How big is this buffer?
    const NodeString_t* nodeStrPtr  ///< The source node string to copy from.
)
{
    size_t copied = 0;

    // Fire through the segments while making sure that we don't exceed the destination buffer.
    while (   (copied < destMax)
           && (nodeStrPtr != NULL))
    {
        // Compute what we need for the whole segment.
        size_t toCopy = copied + SEGMENT_SIZE;
        size_t diff = 0;

        // Back it off from that ideal if there isn't enough space in the destination buffer.
        if (toCopy >= destMax)
        {
            diff = toCopy - destMax;
        }

        // Copy the string and update our destination pointer to reflect what we had copied.
        strncpy(destPtr, nodeStrPtr->value, SEGMENT_SIZE - diff);

        destPtr += (toCopy - copied);
        copied = toCopy;

        // Move onto the next segment.
        nodeStrPtr = nodeStrPtr->nextPtr;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Take a regualar C style string and copy it into a node string, allocating and freeing as
 *  required.
 */
// -------------------------------------------------------------------------------------------------
static void CopyStringToNodeString
(
    NodeString_t* nodeStrPtr,  ///< The string to update.
    const char* sourcePtr      ///< The string to copy.
)
{
    // Go through the string copying it segment by segment.  Only allocate new blocks if they're
    // needed.
    size_t length = strlen(sourcePtr);
    size_t copied = 0;

    while (copied < length)
    {
        strncpy(nodeStrPtr->value, sourcePtr, SEGMENT_SIZE);

        sourcePtr += SEGMENT_SIZE;
        copied += SEGMENT_SIZE;

        if (copied < length)
        {
            if (nodeStrPtr->nextPtr == NULL)
            {
                nodeStrPtr->nextPtr = le_mem_ForceAlloc(NodeStringPool);
                memset(nodeStrPtr->nextPtr, 0, sizeof(NodeString_t));
            }

            nodeStrPtr = nodeStrPtr->nextPtr;
        }
    }


    // Now that the copy is finished, free up any unused blocks.
    ReleaseNodeStr(nodeStrPtr->nextPtr);
    nodeStrPtr->nextPtr = NULL;
}




// ------------------------------------------------------------------------------------------------
/**
 *  Copy a node string into another node string.
 */
// -------------------------------------------------------------------------------------------------
static void CopyNodeString
(
    NodeString_t* destStrPtr,  ///< The copy target.
    NodeString_t* srcStrPtr    ///< The copy source.
)
{
    // Whip through and copy the segements as needed.  Allocate new ones as we go.
    while (srcStrPtr != NULL)
    {
        memcpy(destStrPtr->value, srcStrPtr->value, SEGMENT_SIZE);

        srcStrPtr = srcStrPtr->nextPtr;

        if (srcStrPtr != NULL)
        {
            if (destStrPtr->nextPtr != NULL)
            {
                destStrPtr = destStrPtr->nextPtr;
            }
            else
            {
                destStrPtr->nextPtr = le_mem_ForceAlloc(NodeStringPool);
                memset(destStrPtr->nextPtr, 0, sizeof(NodeString_t));

                destStrPtr = destStrPtr->nextPtr;
            }
        }
    }


    // If the new copy is smaller that what was previously held, shrink the string down to match.
    ReleaseNodeStr(destStrPtr->nextPtr);
    destStrPtr->nextPtr = NULL;
}




// ------------------------------------------------------------------------------------------------
/**
 *  Compute the length of a node string.
 */
// -------------------------------------------------------------------------------------------------
static size_t GetNodeStringLength
(
    const NodeString_t* nodeStrPtr  ///< The string to read.
)
{
    size_t count = 0;

    // Count whole blocks, then get what's actually used in the last one.
    while (nodeStrPtr->nextPtr != NULL)
    {
        count++;
        nodeStrPtr = nodeStrPtr->nextPtr;
    }

    return strnlen(nodeStrPtr->value, SEGMENT_SIZE) + (count * SEGMENT_SIZE);
}




// ------------------------------------------------------------------------------------------------
/**
 *  Allocate and zero out a new tree node..
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewNode
(
)
{
    // Create a new blank node.
    tdb_NodeRef_t newNode = le_mem_ForceAlloc(NodePool);
    memset(newNode, 0, sizeof(tdb_Node_t));

    return newNode;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Allocate a new node from our pool, and turn it into a shadow of an existing node.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewShadowNode
(
    tdb_NodeRef_t nodePtr  ///< The node to shadow.
)
{
    // It's possible for nodePtr to be NULL.  We could be creating a shadow node for which no
    // original exists.  Which is the case when creating a new path that didn't exist in the
    // original tree.  However, if we do have a valid base node, check the programs sanity and make
    // sure it isn't already a shadow node.
    if (nodePtr != NULL)
    {
        LE_ASSERT((nodePtr->flags & NODE_IS_SHADOW) == 0);
    }

    // Allocate a new blank node.
    tdb_NodeRef_t newPtr = NewNode();

    // Turn it into a shadow of the original node.
    newPtr->type = nodePtr->type;
    newPtr->flags = NODE_IS_SHADOW | nodePtr->flags;
    newPtr->shadowPtr = nodePtr;

    // Now, if the parent node, (if there is a parent node,) is marked as deleted, then do the same
    // with this new node.
    if (   (nodePtr->parentPtr != NULL)
        && (tdb_IsDeleted(nodePtr->parentPtr)))
    {
        newPtr->flags |= NODE_IS_DELETED;
    }

    // Keep track of the child count if this is a shadow of a stem.
    if (   (nodePtr != NULL)
        && (nodePtr->type == LE_CFG_TYPE_STEM))
    {
        newPtr->info.children.count = nodePtr->info.children.count;
    }

    return newPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to get the first shadow child of a node collection..
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetFirstShadowChild
(
    tdb_NodeRef_t shadowParentPtr
)
{
    // Sanity check, if we were given a bad node pointer, or if the given node isn't even a stem,
    // then there are no children to return.
    if (   (shadowParentPtr == NULL)
        || (shadowParentPtr->type != LE_CFG_TYPE_STEM))
    {
        return NULL;
    }

    // This is a valid node pointer.  Does this node have any children currently?  If yes, then
    // return the pointer to the first one.
    if (shadowParentPtr->info.children.childPtr != NULL)
    {
        return shadowParentPtr->info.children.childPtr;
    }


    // This node has no shadow children.  So what we do now is check the original node... Does it
    // have any children?  If it does, we simply recreate the whole collection now.  (We do not
    // recurse into the grandchildren though.)  Doing this now makes life simpler, instead of doing
    // this piecemeal and possibly out of order.
    tdb_NodeRef_t originalPtr = shadowParentPtr->shadowPtr;

    if (originalPtr == NULL)
    {
        return NULL;
    }


    // Simply iterate through the original collection and add a new shadow child to our own
    // collection.
    tdb_NodeRef_t shadowPtr = NULL;
    tdb_NodeRef_t originalChildPtr = tdb_GetFirstChildNode(originalPtr);

    while (originalChildPtr != NULL)
    {
        tdb_NodeRef_t newShadowPtr = NewShadowNode(originalChildPtr);
        newShadowPtr->parentPtr = shadowParentPtr;

        if (shadowPtr != NULL)
        {
            shadowPtr->siblingPtr = newShadowPtr;
        }
        else
        {
            shadowParentPtr->info.children.childPtr = newShadowPtr;
        }

        shadowPtr = newShadowPtr;
        originalChildPtr = originalChildPtr->siblingPtr;
    }

    // Now, finally, return the first of the newly created shadow nodes.
    return shadowParentPtr->info.children.childPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a new node and insert it into the given node's children collection.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t NewChildNode
(
    tdb_NodeRef_t nodePtr  ///< The node to be given with a new child.
)
{
    LE_ASSERT(nodePtr != NULL);

    // If the node is currently empty, then turn it into a stem.
    if (nodePtr->type == LE_CFG_TYPE_EMPTY)
    {
        nodePtr->type = LE_CFG_TYPE_STEM;
    }

    LE_ASSERT(nodePtr->type == LE_CFG_TYPE_STEM);


    // Create a new node.  Then set it's parent to the given node
    tdb_NodeRef_t newPtr = NewNode();

    newPtr->parentPtr = nodePtr;
    newPtr->type = LE_CFG_TYPE_EMPTY;

    // Get the new node to inheret the parent's shadow and deletion flags.
    newPtr->flags = nodePtr->flags & (NODE_IS_SHADOW | NODE_IS_DELETED);

    // Now make sure to add the new child node to the end of the parents collection.
    tdb_NodeRef_t nextPtr = nodePtr->info.children.childPtr;

    if (nextPtr == NULL)
    {
        nodePtr->info.children.childPtr = newPtr;
    }
    else
    {
        while (nextPtr->siblingPtr != NULL)
        {
            nextPtr = nextPtr->siblingPtr;
        }

        nextPtr->siblingPtr = newPtr;
    }

    // Finally return the newly created node to the caller.
    return newPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Free up a given node and the string values it may have.  Note that this will not free up the
 *  node's children.  It is expected that the node's children have already been deleted.
 *
 *  This will also properly free up shadow nodes.
 */
// -------------------------------------------------------------------------------------------------
static void ForceReleaseNode
(
    tdb_NodeRef_t nodePtr  ///< The node to free.
)
{
    if (nodePtr == NULL)
    {
        return;
    }

    // Free up any extra memory, (if any,) associated with the node.
    ReleaseNodeStr(nodePtr->name.nextPtr);

    if (nodePtr->type != LE_CFG_TYPE_STEM)
    {
        ReleaseNodeStr(nodePtr->info.value.nextPtr);
    }

    // Now properly release this node too.
    le_mem_Release(nodePtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Free up a given node and the string values it may have.  Note that this will not free up the
 *  node's children.  It is expected that the node's children have already been deleted.
 *
 *  Shadow nodes are only marked for deletion.  This is so that later these deletions can be
 *  propigated into the master tree.
 */
// -------------------------------------------------------------------------------------------------
static void ReleaseNode
(
    tdb_NodeRef_t nodePtr  ///< The node to free.
)
{
    if (nodePtr == NULL)
    {
        return;
    }

    // Mark the node as having been deleted.
    nodePtr->flags |= NODE_IS_DELETED;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Call this function to make sure that the given node and all of it's parents will be persisted.
 *  Often in shadow trees paths can be created but are only fully comitted when values are written
 *  to them.
 */
// -------------------------------------------------------------------------------------------------
static void EnsureExists
(
    tdb_NodeRef_t nodePtr  ///< Make sure that this node "offically" exists.
)
{
    // Simply go up through the chain and clear any deleted flags.
    while (nodePtr != NULL)
    {
        nodePtr->flags &= ~NODE_IS_DELETED;

        if (   ((nodePtr->flags & NODE_IS_SHADOW) != 0)
            && (nodePtr->shadowPtr == NULL))
        {
            nodePtr->flags |= NODE_IS_MODIFIED;
        }

        nodePtr = nodePtr->parentPtr;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to take a shadow node and merge it's data back into the original tree.
 */
// -------------------------------------------------------------------------------------------------
static void MergeNode
(
    tdb_NodeRef_t nodePtr  ///< The shadow node to merge with it's original.
)
{
    if (nodePtr == NULL)
    {
        return;
    }

    // Sanity check, is this really a shadow node?
    LE_ASSERT((nodePtr->flags & NODE_IS_SHADOW) != 0);

    // If this node as been delted then mark the original as well.
    if ((nodePtr->flags & NODE_IS_DELETED) != 0)
    {
        ReleaseNode(nodePtr->shadowPtr);
        return;
    }

    // Don't bother to do anything if the node has never been changed.
    if ((nodePtr->flags & NODE_IS_MODIFIED) == 0)
    {
        return;
    }


    // Check to see if there is a node that was shadowed.  If there wasn't then we need to crate a
    // new node in the list.
    tdb_NodeRef_t shadowedPtr = nodePtr->shadowPtr;


    if (shadowedPtr == NULL)
    {
        // There is no original node, so create a new node in the parent's collection.  (Which
        // better exist.)
        tdb_NodeRef_t shadowedParentPtr = nodePtr->parentPtr->shadowPtr;

        LE_ASSERT(shadowedParentPtr != NULL);

        // Create the child node and copy our data over to it.  If this will be a new stem node then
        // the new child nodes will be created later in subsequent calls to this funciton.
        shadowedPtr = NewChildNode(shadowedParentPtr);
        nodePtr->shadowPtr = shadowedPtr;

        CopyNodeString(&shadowedPtr->name, &nodePtr->name);
        shadowedPtr->type = nodePtr->type;

        if (shadowedPtr->type != LE_CFG_TYPE_STEM)
        {
            CopyNodeString(&shadowedPtr->info.value, &nodePtr->info.value);
        }
    }
    else
    {
        // There is an existing node.  If the name has been changed, copy it over now.
        if (IsNodeStringEmpty(&nodePtr->name) == false)
        {
            CopyNodeString(&shadowedPtr->name, &nodePtr->name);
        }

        // If this isn't a stem node, then copy the value over now.
        shadowedPtr->type = nodePtr->type;

        if (   (shadowedPtr->type != LE_CFG_TYPE_STEM)
            && (IsNodeStringEmpty(&nodePtr->info.value) == false))
        {
            // Clear out any existing values or child nodes.  Then copy over the new string.
            tdb_ClearNode(shadowedPtr);
            CopyNodeString(&shadowedPtr->info.value, &nodePtr->info.value);
        }
    }

    // Finally clear the modifed flag and make sure that the node now offically exists.
    nodePtr->flags &= ~NODE_IS_MODIFIED;
    nodePtr->shadowPtr->flags &= ~NODE_IS_DELETED;

    EnsureExists(nodePtr->shadowPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  .When traversing the entire tree, this function is used to find the next, last, leftmost node.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t FindTailChild
(
    tdb_NodeRef_t nodePtr  ///< The node to search from.
)
{
    if (nodePtr == NULL)
    {
        return NULL;
    }

    tdb_NodeRef_t foundPtr = NULL;

    while (foundPtr == NULL)
    {
        if (   (nodePtr->type == LE_CFG_TYPE_STEM)
            && (nodePtr->info.children.childPtr != NULL))
        {
            nodePtr = nodePtr->info.children.childPtr;
        }
        else
        {
            foundPtr = nodePtr;
        }
    }

    return foundPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to look for a named child in a collection.  If the given node shadows another and the
 *  child wasn't found in this collection the search then folloows up the shadow chain.
 */
// -------------------------------------------------------------------------------------------------
static tdb_NodeRef_t GetNamedChild
(
    tdb_NodeRef_t nodePtr,  ///< Find the named child of this node.
    const char* namePtr,    ///< The name we're looking for.
    bool forceCreate        ///< Should a node be created, even if we're not on a shadow tree?
)
{
    // If this node doesn't exist, then it's children do not exist either.
    if (nodePtr == NULL)
    {
        return NULL;
    }

    // Looks like we were really just asking about this node anyhow.
    if (strcmp(namePtr, ".") == 0)
    {
        return nodePtr;
    }

    // Grab this node's parent.
    if (strcmp(namePtr, "..") == 0)
    {
        return nodePtr->parentPtr;
    }

    // If we're allowed to manufacture the child nodes, then make sure that makes sense here.
    if (   (((nodePtr->flags & NODE_IS_SHADOW) != 0) || (forceCreate == true))
        && (nodePtr->type == LE_CFG_TYPE_EMPTY))
    {
        nodePtr->type = LE_CFG_TYPE_STEM;
    }

    if (nodePtr->type != LE_CFG_TYPE_STEM)
    {
        return NULL;
    }


    // Search the child list for a node with the given name.
    tdb_NodeRef_t currentPtr = tdb_GetFirstChildNode(nodePtr);
    char* currentNamePtr = sb_Get();

    while (currentPtr != NULL)
    {
        tdb_GetName(currentPtr, currentNamePtr, SB_SIZE);

        if (strncmp(currentNamePtr, namePtr, SB_SIZE) == 0)
        {
            sb_Release(currentNamePtr);
            return currentPtr;
        }

        currentPtr = tdb_GetNextSibling(currentPtr);
    }

    sb_Release(currentNamePtr);


    // At this point the node has not been found.  Check to see if we can create a new node.  If we
    // can, do so now and add it to the parent's list.
    if (   ((nodePtr->flags & NODE_IS_SHADOW) != 0)
        || (forceCreate == true))
    {
        tdb_NodeRef_t childPtr = NewChildNode(nodePtr);

        tdb_SetName(childPtr, namePtr);

        childPtr->flags |= NODE_IS_DELETED;
        return childPtr;
    }

    // Nope, no creation was allowed, so there is no node to return.
    return NULL;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see what kind of character this is.
 *
 *  @return LE_CFG_TYPE_INT is returned if the character looks like an integer value.
 *          LE_CFG_TYPE_FLOAT is returned if this could be a floating point value.
 *          LE_CFG_TYPE_STRING is returned in all other cases.
 */
// -------------------------------------------------------------------------------------------------
static le_cfg_nodeType_t DigitType
(
    char digit  ///< The character to check.
)
{

    if (   ((digit >= '0') && (digit <= '9'))
        || (digit == '-')
        || (digit == '+'))
    {
        return LE_CFG_TYPE_INT;
    }

    if (   (digit == '.')
        || (digit == 'e')
        || (digit == 'E'))
    {
        return LE_CFG_TYPE_FLOAT;
    }

    return LE_CFG_TYPE_STRING;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function is called to do a quick guess on the string type.
 */
// -------------------------------------------------------------------------------------------------
static le_cfg_nodeType_t GuessTypeFromString
(
    const char* stringPtr  ///< The string to check.
)
{
    // If there's no string or an empty string then there isn't much left to do.
    if (   (stringPtr == NULL)
        || (*stringPtr == 0))
    {
        return LE_CFG_TYPE_EMPTY;
    }


    // Now, is this a boolean value?
    if (   (strcmp(stringPtr, "true") == 0)
        || (strcmp(stringPtr, "false") == 0))
    {
        return LE_CFG_TYPE_BOOL;
    }


    // If this starts off with a negative, but there is no string to follow, consider it a string
    // and not an int.
    if (*stringPtr == '-')
    {
        stringPtr++;

        if (*stringPtr == 0)
        {
            return LE_CFG_TYPE_STRING;
        }
    }

    // Ok, now assume this is an integer.
    le_cfg_nodeType_t type = LE_CFG_TYPE_INT;

    while (*stringPtr)
    {
        // Check the digit type, if we run into a string character just give up and call it a
        // string.  If we encounter a floating point value, then promote it to a float.
        le_cfg_nodeType_t newType = DigitType(*stringPtr);

        if (newType == LE_CFG_TYPE_STRING)
        {
            type = newType;
            break;
        }
        else if (newType == LE_CFG_TYPE_FLOAT)
        {
            type = LE_CFG_TYPE_FLOAT;
        }

        stringPtr++;
    }

    // Finally return what we got.
    return type;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Peek into the input stream one character ahead.
 */
// -------------------------------------------------------------------------------------------------
static signed char PeekChar
(
    FILE* filePtr  ///< The file stream to peek into.
)
{
    char next = fgetc(filePtr);
    ungetc(next, filePtr);

    return next;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Skip any whitespace encountered in the input stream.  Stop skipping once we hit a valid token.
 *
 *  @return Returns true if there is more text to be read once this function returns.  A false is
 *          returned if the function reaches the end of the file.
 */
// -------------------------------------------------------------------------------------------------
static bool SkipWhitespace
(
    FILE* filePtr  ///< The file stream to seek through.
)
{
    bool done = false;
    bool isEof = false;

    while (done == false)
    {
        switch (PeekChar(filePtr))
        {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
                // Eat the character.
                fgetc(filePtr);
                break;

            case EOF:
                done = true;
                isEof = true;
                break;

            default:
                done = true;
                break;
        }
    }

    return isEof == false;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a token from the input stream.  A token can be an opening '{' or a closing '}' bracket, or
 *  a token can be a string.
 *
 *  @return Returns true if there is more text to be read once this function returns.  A false is
 *          returned if the function reaches the end of the file.
 */
// -------------------------------------------------------------------------------------------------
static bool ReadToken
(
    FILE* filePtr,    ///<
    char* stringPtr,  ///<
    size_t maxString  ///<
)
{
    memset(stringPtr, 0, maxString);

    if (SkipWhitespace(filePtr) == false)
    {
        return false;
    }

    signed char next = fgetc(filePtr);

    // Consider a { or a } character a whole token.
    if (   (next == '{')
        || (next == '}'))
    {
        *stringPtr = next;
        return true;
    }

    // It's not a bracket, so it'd better be a string.
    if (next != '\"')
    {
        return false;
    }


    // Ok, we have a string, so read on until we hit the end of hte string.
    size_t index = 0;
    bool done = false;

    while (   (done == false)
           && (next != EOF)
           && (index < (maxString - 1)))
    {
        next = PeekChar(filePtr);

        switch (next)
        {
            // We hit the end of the file in the middle of the string.  Return failure.
            case EOF:
                return false;

            // We found the end of the string, so all done.
            case '\"':
                fgetc(filePtr);
                done = true;
                break;

            // We found an escape character, so read in what the user was escaping.
            case '\\':
                fgetc(filePtr);
                next = PeekChar(filePtr);
                if (   (next == '\"')
                    || (next == '\\'))
                {
                    *stringPtr = fgetc(filePtr);
                    stringPtr++;
                    index++;
                }
                else
                {
                    // Looks like we werent escaping a string char or a slash, so error out:
                    return false;
                }
                break;

            // Simply add this character to the string we're building.
            default:
                *stringPtr = fgetc(filePtr);
                stringPtr++;
                index++;
                break;
        }
    }

    return true;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Write a string token to the output stream.
 */
// -------------------------------------------------------------------------------------------------
static void WriteToken
(
    int descriptor,        ///< The file descriptor being written to.
    const char* stringPtr  ///< The token string to write.
)
{
    // This function will filter any illegial characters found in the string and write it to the
    // output file, including the required quotes around the token.
    char* originalPtr = sb_Get();
    strncpy(originalPtr, stringPtr, SB_SIZE);

    char* newString = originalPtr;
    char* position = strchr(newString, '\"');

    write(descriptor, "\"", 1);

    while (position != NULL)
    {
        *position = 0;

        write(descriptor, newString, strlen(newString));
        write(descriptor, "\\\"", 2);

        newString = ++position;
        position = strchr(newString, '\"');
    }

    write(descriptor, newString, strlen(newString));
    write(descriptor, "\" ", 2);

    // TODO: Deal with strings with a \ in them...

    sb_Release(originalPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Create a path to a tree file with the given revision id.
 */
// -------------------------------------------------------------------------------------------------
static char* GetTreePath
(
    const char* treeNamePtr,
    int revisionId
)
{
    // paper    --> rock       1 -> 2
    // rock     --> scissors   2 -> 3
    // scissors --> paper      3 -> 1

    static const char* revNames[] = { "paper", "rock", "scissors" };
    char* fullPathPtr = NULL;

    LE_ASSERT((revisionId >= 1) && (revisionId <= 3));

    fullPathPtr = sb_Get();

    snprintf(fullPathPtr,
             SB_SIZE,
             "%s/%s.%s",
             CFG_TREE_PATH,
             treeNamePtr,
             revNames[revisionId - 1]);

    return fullPathPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a configTree file at the given revision already exists in the filesystem.
 */
// -------------------------------------------------------------------------------------------------
static bool TreeFileExists
(
    const char* treeNamePtr,  ///< Name of the tree to check.
    int revisionId            ///< The revision of the tree to check against.
)
{
    char* fullPathPtr = GetTreePath(treeNamePtr, revisionId);

    // Make sure this part was successful.
    if (fullPathPtr == NULL)
    {
        return false;
    }

    // Now call into the Linux and ask the file system if the file exists.
    bool result = false;

    if (access(fullPathPtr, R_OK) != -1)
    {
        result = true;
    }

    sb_Release(fullPathPtr);

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check the filesystem and get the "valid" version of the file.  That is, if there are two files
 *  for a given tree, we use the older one.  The idea being, if there are two versions of the same
 *  file in the filesystem then it's highly likely there was a system failure during a streaming
 *  operation.  So we abandon the newer file and go with the older more reliable file.
 */
// -------------------------------------------------------------------------------------------------
static int GetRevision
(
    const char* treeName  ///< The tree to look for.
)
{
    if (TreeFileExists(treeName, 1))
    {
        if (TreeFileExists(treeName, 3))
        {
            return 3;
        }

        return 1;
    }

    if (TreeFileExists(treeName, 3))
    {
        if (TreeFileExists(treeName, 2))
        {
            return 2;
        }

        return 3;
    }

    if (TreeFileExists(treeName, 2))
    {
        return 2;
    }

    return 0;
}



// -------------------------------------------------------------------------------------------------
/**
 *  Bump up the version id of this tree.
 */
// -------------------------------------------------------------------------------------------------
static void IncrementRevision
(
    TreeInfo_t* treePtr
)
{
    treePtr->revisionId++;

    if (treePtr->revisionId > 3)
    {
        treePtr->revisionId = 1;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Load a configuration tree from the filesystem.
 */
// -------------------------------------------------------------------------------------------------
static void LoadTree
(
    TreeInfo_t* treePtr,  ///< The configuration tree to load.
    int revisionId        ///< The revision of the tree to try to load.
)
{
    // If we don't know the revision then hunt it out from the filesystem.
    if (treePtr->revisionId == 0)
    {
        treePtr->revisionId = GetRevision(treePtr->name);
    }

    // If this tree has no root, create it now.
    if (treePtr->rootNodeRef == NULL)
    {
        treePtr->rootNodeRef = NewNode();
    }

    // Ok, if we found a valid revision of the tree in the fs, try to load it now.
    if (treePtr->revisionId != 0)
    {
        char* pathPtr = GetTreePath(treePtr->name, treePtr->revisionId);
        int fileRef = open(pathPtr, O_RDONLY);

        EnsureExists(treePtr->rootNodeRef);

        if (tdb_ReadTreeNode(treePtr->rootNodeRef, fileRef) == false)
        {
            LE_ERROR("Could not open configuration tree: %s", pathPtr);
        }

        close(fileRef);

        sb_Release(pathPtr);
    }
}





// -------------------------------------------------------------------------------------------------
/**
 *  Allocate our pools and load the config data from the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_Init
(
)
{
    // Initialize the memory pools.
    NodePool = le_mem_CreatePool(CFG_NODE_POOL_NAME, sizeof(tdb_Node_t));
    NodeStringPool = le_mem_CreatePool(CFG_NODE_STRING_POOL_NAME, sizeof(NodeString_t));

    TreePoolRef = le_mem_CreatePool(CFG_TREE_POOL_NAME, sizeof(TreeInfo_t));
    TreeCollectionRef = le_hashmap_Create(CFG_TREE_COLLECTION_NAME,
                                          31,
                                          le_hashmap_HashString,
                                          le_hashmap_EqualsString);

    // Preload the system tree.
    tdb_GetTree("system");
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the named tree.
 */
// -------------------------------------------------------------------------------------------------
TreeInfo_t* tdb_GetTree
(
    const char* treeNamePtr  ///< The tree to load.
)
{
    // Check to see if we have this tree loaded up in our map.
    TreeInfo_t* treePtr = le_hashmap_Get(TreeCollectionRef, treeNamePtr);

    if (treePtr == NULL)
    {
        // Looks like we don't so create an object for it, and add it to our map.
        treePtr = le_mem_ForceAlloc(TreePoolRef);

        strncpy(treePtr->name, treeNamePtr, MAX_USER_NAME);
        treePtr->revisionId = 0;
        treePtr->rootNodeRef = NewNode();
        treePtr->activeReadCount = 0;
        treePtr->activeWriteIterPtr = NULL;
        treePtr->requestList = LE_SLS_LIST_INIT;

        le_hashmap_Put(TreeCollectionRef, treePtr->name, treePtr);
    }

    // Has the tree been loaded from the filesystem?  If not, try to do so now.
    if (treePtr->revisionId == 0)
    {
        LoadTree(treePtr, GetRevision(treeNamePtr));
    }

    // Finally return the tree we have to the user.
    return treePtr;
}




//--------------------------------------------------------------------------------------------------
/**
 * Gets an interator for step-by-step iteration over the tree collection. In this mode the iteration
 * is controlled by the calling function using the le_ref_NextNode() function.  There is one
 * iterator and calling this function resets the iterator position to the start of the map.  The
 * iterator is not ready for data access until le_ref_NextNode() has been called at least once.
 *
 * @return  Returns A reference to a hashmap iterator which is ready for le_hashmap_NextNode() to be
 *          called on it.
 */
//--------------------------------------------------------------------------------------------------
tdb_IterRef_t tdb_GetTreeIterator
(
    void
)
{
    return (tdb_IterRef_t)le_hashmap_GetIterator(TreeCollectionRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator to the next key/value pair in the map.  If the hashmap is modified during
 * iteration then this function will return an error.
 *
 * @return  Returns LE_OK unless you go past the end of the map, then returns LE_NOT_FOUND.
 *          If the iterator has been invalidated by the map changing or you have previously
 *          received a LE_NOT_FOUND then this returns LE_FAULT.
 */
//--------------------------------------------------------------------------------------------------
le_result_t tdb_NextNode
(
    tdb_IterRef_t iteratorRef  ///< Reference to the iterator.
)
{
    return le_hashmap_NextNode((le_hashmap_It_Ref_t)iteratorRef);
}




//--------------------------------------------------------------------------------------------------
/**
 * Retrieves a pointer to the value which the iterator is currently pointing at.  If the iterator
 * has just been initialized and le_ref_NextNode() has not been called, or if the iterator has been
 * invalidated then this will return NULL.
 *
 * @return  A pointer to the current value, or NULL if the iterator has been invalidated or is not
 *          ready.
 */
//--------------------------------------------------------------------------------------------------
TreeInfo_t* tdb_iter_GetTree
(
    tdb_IterRef_t iteratorRef  ///< Reference to the iterator.
)
{
    return (TreeInfo_t*)le_hashmap_GetKey((le_hashmap_It_Ref_t)iteratorRef);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to commit the changes to a given config tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_CommitTree
(
    TreeInfo_t* treePtr  ///< The tree to be committed to the filesystem.
)
{
    // Attempt to save the tree.  Start by bumping up the version.
    int oldId = treePtr->revisionId;
    IncrementRevision(treePtr);

    // Compute the path and open the new file for this tree.
    char* newFilePath = GetTreePath(treePtr->name, treePtr->revisionId);
    int fileRef = open(newFilePath,
                       O_RDWR | O_CREAT | O_TRUNC,
                       S_IRUSR | S_IWUSR);

    LE_DEBUG("Attempting to serialize the tree to <%s>.", newFilePath);
    sb_Release(newFilePath);

    // Now, write the tree data, and if an old revision still exists, delete it now.
    tdb_WriteTreeNode(treePtr->rootNodeRef, fileRef);
    close(fileRef);

    if (   (oldId != 0)
        && (TreeFileExists(treePtr->name, oldId)))
    {
        // Again, compute a path, and this time simply delete the old file.
        char* newFilePath = GetTreePath(treePtr->name, oldId);
        unlink(newFilePath);
        sb_Release(newFilePath);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Serialize a tree node and it's children to a file in the filesystem.
 */
// -------------------------------------------------------------------------------------------------
void tdb_WriteTreeNode
(
    tdb_NodeRef_t nodePtr,  ///< Write the contents of this node to a file descriptor.
    int descriptor          ///< The file descriptor to write to.
)
{
    LE_ASSERT(nodePtr != NULL);
    LE_ASSERT(descriptor != -1);

    // Now, create a C file stream that wraps the descriptor.
    char* stringBuffer = sb_Get();


    // Get the first child of this node, ignoring the nodes that are marked as deleted.
    tdb_NodeRef_t currentPtr = tdb_GetFirstActiveChildNode(nodePtr);

    while (currentPtr != NULL)
    {
        // Get the name of this node and write it out.
        tdb_GetName(currentPtr, stringBuffer, SB_SIZE);
        WriteToken(descriptor, stringBuffer);

        tdb_NodeRef_t nextPtr = NULL;

        // If this is a stem node, then advance on to the child.
        if (currentPtr->type == LE_CFG_TYPE_STEM)
        {
            write(descriptor, "{ ", 2);
            nextPtr = tdb_GetFirstActiveChildNode(currentPtr);

            if (nextPtr == NULL)
            {
                write(descriptor, "} ", 2);
                nextPtr = tdb_GetNextActiveSibling(currentPtr);
            }
        }
        else
        {
            // Otherwise, write out the value for this node and advance on to the next sibling.
            tdb_GetAsString(currentPtr, stringBuffer, SB_SIZE);
            WriteToken(descriptor, stringBuffer);

            nextPtr = tdb_GetNextActiveSibling(currentPtr);
        }

        // Check to see if we were successful in advancing on.
        if (nextPtr != NULL)
        {
            // We were, so use that new node in our next round of iteration.
            currentPtr = nextPtr;
        }
        else
        {
            // Ok, there are no more siblings.  So, close off this branch and advance back up the
            // tree a level.  If that level has no siblings keep going back up the chain until we
            // get back to the node we started this write off on.  If we get back to the parent
            // node then we're done.
            while (   (currentPtr != nodePtr)
                   && (nextPtr == NULL))
            {
                currentPtr = tdb_GetParentNode(currentPtr);
                nextPtr = tdb_GetNextActiveSibling(currentPtr);

                if (currentPtr != nodePtr)
                {
                    write(descriptor, "} ", 2);
                }
            }

            if (currentPtr == nodePtr)
            {
                currentPtr = NULL;
            }
            else
            {
                currentPtr = nextPtr;
            }
        }
    }

    sb_Release(stringBuffer);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read a configuration tree node's contents from the file system.
 *
 *  @return A true if the read was successful.  A false otherwise.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_ReadTreeNode
(
    tdb_NodeRef_t nodePtr,  ///< The node to write the new data to.
    int descriptor          ///< The file to read from.
)
{
    LE_ASSERT(nodePtr != NULL);


    // First, clear out any existing value or children of this node.  They will be replaced with
    // whatever we find in the file.  Then make sure this node and all of it's parents have their
    // deleted flag cleared.
    tdb_ClearNode(nodePtr);
    EnsureExists(nodePtr);

    // Now, create a C file stream that wraps the descriptor.
    FILE* filePtr = fdopen(descriptor, "r");

    if (filePtr == NULL)
    {
        LE_ERROR("Could not access the input stream for tree import.");
        return false;
    }

    bool wasSuccessful = true;
    char* stringPtr = sb_Get();

    tdb_NodeRef_t currentPtr = nodePtr;

    int inBracketCount = 0;


    while (   (currentPtr != NULL)
           && ReadToken(filePtr, stringPtr, SB_SIZE))
    {
        if (strcmp(stringPtr, "}") == 0)
        {
            currentPtr = currentPtr->parentPtr;
            inBracketCount--;

            if (inBracketCount < 0)
            {
                currentPtr = NULL;
            }
        }
        else if (strcmp(stringPtr, "") == 0)
        {
            // Looks like a node was given with an empty name string.  Fail out.
            currentPtr = NULL;
            wasSuccessful = false;
        }
        else
        {
            // Looks like we were given a valid name.  Try to find the name in currentPtr's
            // collection.
            tdb_NodeRef_t newNodePtr = GetNamedChild(currentPtr, stringPtr, false);

            if (newNodePtr == NULL)
            {
                // Create a new node and add it to the collection.  Set the node's name.
                newNodePtr = NewChildNode(currentPtr);
                tdb_SetName(newNodePtr, stringPtr);
            }
            else
            {
                // Make sure that this node isn't makred for removal.
                newNodePtr->flags &= ~NODE_IS_DELETED;
            }

            // Get the next string for the node's value.  If the string marks a new collection make
            // sure that the new node is a stem and get ready to start populating the node with
            // values.   Otherwise, simply set the nodes value.  If a string wasn't supplied then
            // that's an error.
            if (ReadToken(filePtr, stringPtr, SB_SIZE))
            {
                if (strcmp(stringPtr, "{") == 0)
                {
                    inBracketCount++;

                    // Make the new node a stem, then continue on with this as the base node.
                    newNodePtr->type = LE_CFG_TYPE_STEM;
                    currentPtr = newNodePtr;
                }
                else
                {
                    tdb_SetAsString(newNodePtr, stringPtr);
                }
            }
            else
            {
                currentPtr = NULL;
                wasSuccessful = false;
            }
        }
    }


    sb_Release(stringPtr);

    return wasSuccessful;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called to create a new tree that shadows an existing one.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_ShadowTree
(
    tdb_NodeRef_t originalPtr  ///< The tree to shadow.
)
{
    LE_ASSERT(originalPtr != NULL);
    LE_ASSERT((originalPtr->flags & NODE_IS_SHADOW) == 0);


    // We don't actually shadow the whole tree all at once.  Just this one node.  As the user
    // traverses this shadow tree, new nodes are created as required.
    return NewShadowNode(originalPtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Traverse a shadow tree and merge it's changes back into it's parent.
 */
// -------------------------------------------------------------------------------------------------
void tdb_MergeTree
(
    tdb_NodeRef_t shadowTreeRef  ///< Merge the ndoes from this tree into their base tree.
)
{
    // We basicly start at the root node of the tree and travel up from parent to child until we've
    // covered all of the shadow tree.

    // This way when new nodes need to be created in the base tree we can be garunteed that the
    // parent will already exist.

    tdb_NodeRef_t currentPtr = tdb_GetRootNode(shadowTreeRef);

    while (currentPtr != NULL)
    {
        tdb_NodeRef_t nextPtr = NULL;

        MergeNode(currentPtr);

        if (currentPtr->type == LE_CFG_TYPE_STEM)
        {
            nextPtr = tdb_GetFirstChildNode(currentPtr);

            if (nextPtr == NULL)
            {
                nextPtr = tdb_GetNextSibling(currentPtr);
            }
        }
        else
        {
            nextPtr = tdb_GetNextSibling(currentPtr);
        }

        if (nextPtr != NULL)
        {
            currentPtr = nextPtr;
        }
        else
        {
            while (   (currentPtr != NULL)
                   && (nextPtr == NULL))
            {
                currentPtr = tdb_GetParentNode(currentPtr);
                nextPtr = tdb_GetNextSibling(currentPtr);
            }

            currentPtr = nextPtr;
        }
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Call this to realease a tree.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ReleaseTree
(
    tdb_NodeRef_t treeRef  ///< The tree to free.  Note that this doesn't have to be the root node.
)
{
    // Simply traverse the entire tree leaf back to root deleting nodes as we go.
    tdb_NodeRef_t currentPtr = FindTailChild(tdb_GetRootNode(treeRef));

    while (currentPtr != NULL)
    {
        tdb_NodeRef_t parentPtr = currentPtr->parentPtr;
        tdb_NodeRef_t siblingPtr = currentPtr->siblingPtr;

        ForceReleaseNode(currentPtr);

        if (siblingPtr != NULL)
        {
            currentPtr = FindTailChild(siblingPtr);
        }
        else
        {
            currentPtr = parentPtr;
        }
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Given a base node and a path, find another node in the tree.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNode
(
    tdb_NodeRef_t baseNodePtr,  ///< The base node to start from.
    const char* nodePathPtr,    ///< The path we're searching for in the tree.
    bool forceCreate            ///< Should the nodes on the path be created?  Even if this isn't
                                ///<   a shadow tree?
)
{
    le_path_IteratorRef_t pathIterator = le_path_iter_Create(nodePathPtr, "/");

    // Check to see if we're starting at the given node, or that node's root node.
    tdb_NodeRef_t currentPtr = baseNodePtr;

    if (le_path_iter_IsAbsolute(pathIterator))
    {
        currentPtr = tdb_GetRootNode(currentPtr);
    }

    // Now start moving along the path, moving the current node along as we go.  The called function
    // also deals with . and .. names in the path as well, returning the current and parent nodes
    // respectivly.
    char* namePtr = sb_Get();

    while (le_path_iter_GetNextNode(pathIterator, namePtr, SB_SIZE) != LE_NOT_FOUND)
    {
        currentPtr = GetNamedChild(currentPtr, namePtr, forceCreate);
    }

    sb_Release(namePtr);
    le_path_iter_Delete(pathIterator);

    // Finally return the last node we traversed to.
    return currentPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Delete a given node and all of it's children.
 */
// -------------------------------------------------------------------------------------------------
void tdb_DeleteNode
(
    tdb_NodeRef_t nodePtr  ///< The node to delete.
)
{
    if (nodePtr == NULL)
    {
        return;
    }

    // Clear out any contents of this node, then release the node itself.
    tdb_ClearNode(nodePtr);
    ReleaseNode(nodePtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Find a root node for the given tree node.
 *
 *  @return The root node of the tree that the given node is a part of.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetRootNode
(
    tdb_NodeRef_t nodePtr  ///< Find the parent for this node.
)
{
    if (nodePtr == NULL)
    {
        return NULL;
    }

    tdb_NodeRef_t parentPtr = tdb_GetParentNode(nodePtr);

    while (parentPtr != NULL)
    {
        nodePtr = parentPtr;
        parentPtr = tdb_GetParentNode(nodePtr);
    }

    return nodePtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the parent of the given node.
 *
 *  @return The parent node of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetParentNode
(
    tdb_NodeRef_t nodePtr  ///< Get the parent of this node.
)
{
    if (nodePtr == NULL)
    {
        return NULL;
    }

    return nodePtr->parentPtr;
}





// -------------------------------------------------------------------------------------------------
/**
 *  Called to get the first child node of this node.  If this node has no children, then return
 *  NULL.
 *
 *  @return The first child of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetFirstChildNode
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    if (nodePtr == NULL)
    {
        return NULL;
    }

    if (nodePtr->type != LE_CFG_TYPE_STEM)
    {
        return NULL;
    }

    // If the node is a shadow node, and it doesn't have any children call GetFirstShadowChild to
    // propigate over the original collection of child nodes into this one.  Then return the first
    // new shadow child node.
    if (   ((nodePtr->flags & NODE_IS_SHADOW) != 0)
        && (nodePtr->shadowPtr != NULL))
    {
        return GetFirstShadowChild(nodePtr);
    }

    // Otherwise just return the first child of this node...  Or NULL if it doesn't have one.
    return nodePtr->info.children.childPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the next sibling for a given node.
 *
 *  @return The next sibling node for the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNextSibling
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    if (nodePtr == NULL)
    {
        return NULL;
    }

    return nodePtr->siblingPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Like tdb_GetFirstChildNode this will return a child of the given parent node.  However, this
 *  function will ignore all nodes that are marked as deleted.
 *
 *  @return The first not-deleted child node of the given node.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetFirstActiveChildNode
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    tdb_NodeRef_t childPtr = tdb_GetFirstChildNode(nodePtr);

    if (   (childPtr != NULL)
        && ((childPtr->flags & NODE_IS_DELETED) != 0))
    {
        return tdb_GetNextActiveSibling(childPtr);
    }

    return childPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function will return the first active, that is not deleted, sibling of the given node.
 *
 *  @return The next "live" node in the sibling chain.
 */
// -------------------------------------------------------------------------------------------------
tdb_NodeRef_t tdb_GetNextActiveSibling
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    tdb_NodeRef_t nextPtr = tdb_GetNextSibling(nodePtr);

    while (   (nextPtr != NULL)
           && ((nextPtr->flags & NODE_IS_DELETED) != 0))
    {
        nextPtr = tdb_GetNextSibling(nextPtr);
    }

    return nextPtr;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the count of the children of this node.
 */
// -------------------------------------------------------------------------------------------------
size_t tdb_ChildNodeCount
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    if (nodePtr == NULL)
    {
        return 0;
    }

    if (nodePtr->type != LE_CFG_TYPE_STEM)
    {
        return 0;
    }

    return nodePtr->info.children.count;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Check to see if a given node pointer represents a deleted node.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_IsDeleted
(
    tdb_NodeRef_t nodePtr  ///< The node to check.
)
{
    if (nodePtr == NULL)
    {
        return true;
    }

    return (nodePtr->flags & NODE_IS_DELETED) != 0;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the name of a given node.
 */
// -------------------------------------------------------------------------------------------------
void tdb_GetName
(
    tdb_NodeRef_t nodePtr,  ///< The node to read.
    char* stringPtr,        ///< Destination buffer to hold the name.
    size_t maxSize          ///< Size of this buffer.
)
{
    memset(stringPtr, 0, maxSize);

    if (nodePtr != NULL)
    {
        if ((nodePtr->flags & NODE_IS_SHADOW) != 0)
        {
            if (   (IsNodeStringEmpty(&nodePtr->name))
                && (nodePtr->shadowPtr != NULL))
            {
                CopyNodeStringToString(stringPtr, maxSize, &nodePtr->shadowPtr->name);
                return;
            }
        }

        CopyNodeStringToString(stringPtr, maxSize, &nodePtr->name);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Compute the length of the name.
 */
// -------------------------------------------------------------------------------------------------
size_t tdb_GetNameLength
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    if (nodePtr != NULL)
    {
        if (   ((nodePtr->flags & NODE_IS_SHADOW) != 0)
            && (IsNodeStringEmpty(&nodePtr->name))
            && (nodePtr->shadowPtr != NULL))
        {
            {
                return GetNodeStringLength(&nodePtr->shadowPtr->name);
            }
        }

        return GetNodeStringLength(&nodePtr->name);
    }

    return 0;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set the name of a given node.  If this is a node in a shadow tree, the name is then overwritten.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetName
(
    tdb_NodeRef_t nodePtr,  ///< The node to write to.
    const char* stringPtr   ///< The name to give to the node.
)
{
    if (nodePtr != NULL)
    {
        CopyStringToNodeString(&nodePtr->name, stringPtr);
        nodePtr->flags |= NODE_IS_MODIFIED;
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Get the nodes string value and copy into the destination buffer.
 */
// -------------------------------------------------------------------------------------------------
void tdb_GetAsString
(
    tdb_NodeRef_t nodePtr,  ///< The node object to read.
    char* stringPtr,        ///< Target buffer for the value string.
    size_t maxSize          ///< Maximum size the buffer can hold.
)
{
    memset(stringPtr, 0, maxSize);

    if ((nodePtr->flags & NODE_IS_DELETED) != 0)
    {
        return;
    }

    if (   (nodePtr != NULL)
        && (nodePtr->type != LE_CFG_TYPE_STEM))
    {
        if ((nodePtr->flags & NODE_IS_SHADOW) != 0)
        {
            if (   (IsNodeStringEmpty(&nodePtr->info.value))
                && (nodePtr->shadowPtr != NULL))
            {
                CopyNodeStringToString(stringPtr, maxSize, &nodePtr->shadowPtr->info.value);
                return;
            }
        }

        CopyNodeStringToString(stringPtr, maxSize, &nodePtr->info.value);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set the given node to a string value.  If the given node is a stem then all children will be
 *  lost.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsString
(
    tdb_NodeRef_t nodePtr,  ///< The node to set.
    const char* stringPtr   ///< The value to write to the node.
)
{
    if (nodePtr == NULL)
    {
        return;
    }

    if (nodePtr->type == LE_CFG_TYPE_STEM)
    {
        tdb_ClearNode(nodePtr);
    }

    nodePtr->type = GuessTypeFromString(stringPtr);
    CopyStringToNodeString(&nodePtr->info.value, stringPtr);

    nodePtr->flags |= NODE_IS_MODIFIED;

    EnsureExists(nodePtr);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a boolean value.
 */
// -------------------------------------------------------------------------------------------------
bool tdb_GetAsBool
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    bool result;

    switch (nodePtr->type)
    {
        case LE_CFG_TYPE_STRING:
            result = !IsNodeStringEmpty(&nodePtr->info.value);
            break;

        case LE_CFG_TYPE_BOOL:
            {
                char buffer[7] = { 0 };

                tdb_GetAsString(nodePtr, buffer, 6);

                if (strcmp(buffer, "false") == 0)
                {
                    result = false;
                }
                else
                {
                    result = buffer[0] != 0;
                }
            }
            break;

        case LE_CFG_TYPE_INT:
            result = tdb_GetAsInt(nodePtr) != 0;
            break;

        case LE_CFG_TYPE_FLOAT:
            {
                float value = tdb_GetAsFloat(nodePtr);
                result = (value > 0.0f) || (value < 0.0f);
            }
            break;

        case LE_CFG_TYPE_STEM:
        case LE_CFG_TYPE_EMPTY:
        default:
            result = false;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a node value as a new boolen value..
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsBool
(
    tdb_NodeRef_t nodePtr,  ///< The node to write to.
    bool value              ///< The new value to write to that node.
)
{
    tdb_SetAsString(nodePtr, value ? "true" : "false");
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as an integer value.
 */
// -------------------------------------------------------------------------------------------------
int tdb_GetAsInt
(
    tdb_NodeRef_t nodePtr  ///< The node to read from.
)
{
    int result;

    switch (nodePtr->type)
    {
        case LE_CFG_TYPE_BOOL:
            result = tdb_GetAsBool(nodePtr) ? 1 : 0;
            break;

        case LE_CFG_TYPE_INT:
            {
                char buffer[SEGMENT_SIZE + 1] = { 0 };

                tdb_GetAsString(nodePtr, buffer, SEGMENT_SIZE);
                result = atoi(buffer);
            }
            break;

        case LE_CFG_TYPE_FLOAT:
            result = (int)tdb_GetAsFloat(nodePtr) + 0.5;
            break;

        case LE_CFG_TYPE_STRING:
        case LE_CFG_TYPE_EMPTY:
        case LE_CFG_TYPE_STEM:
        default:
            result = 0;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Set an integer value to a given node, overwriting the previous value.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsInt
(
    tdb_NodeRef_t nodePtr,  ///< The ndoe to write to.
    int value               ///< The value to write.
)
{
    char buffer[SEGMENT_SIZE + 1] = { 0 };

    snprintf(buffer, SEGMENT_SIZE, "%d", value);
    tdb_SetAsString(nodePtr, buffer);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Read the given node and interpret it as a floating point value.
 */
// -------------------------------------------------------------------------------------------------
float tdb_GetAsFloat
(
    tdb_NodeRef_t nodePtr  ///< The ndoe to read.
)
{
    float result;

    switch (nodePtr->type)
    {
        case LE_CFG_TYPE_BOOL:
            result = tdb_GetAsBool(nodePtr) ? 1.0f : 0.0f;
            break;

        case LE_CFG_TYPE_INT:
            result = tdb_GetAsInt(nodePtr);
            break;

        case LE_CFG_TYPE_FLOAT:
            {
                char buffer[SEGMENT_SIZE + 1] = { 0 };

                tdb_GetAsString(nodePtr, buffer, SEGMENT_SIZE);
                result = (float)atof(buffer);
            }
            break;

        case LE_CFG_TYPE_STRING:
        case LE_CFG_TYPE_EMPTY:
        case LE_CFG_TYPE_STEM:
        default:
            result = 0.0f;
            break;
    }

    return result;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Overwite a given ndoes value with a floating point one.
 */
// -------------------------------------------------------------------------------------------------
void tdb_SetAsFloat
(
    tdb_NodeRef_t nodePtr,  ///< The node to write
    float value             ///< The value to write to that node.
)
{
    char buffer[SEGMENT_SIZE + 1] = { 0 };

    snprintf(buffer, SEGMENT_SIZE, "%f", value);
    tdb_SetAsString(nodePtr, buffer);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Reate the current type id out of the node.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_nodeType_t tdb_GetTypeId
(
    tdb_NodeRef_t nodePtr  ///< The node to read.
)
{
    if (nodePtr == NULL)
    {
        return LE_CFG_TYPE_EMPTY;
    }

    if ((nodePtr->flags & NODE_IS_DELETED) != 0)
    {
        return LE_CFG_TYPE_EMPTY;
    }

    if (   (nodePtr->type == LE_CFG_TYPE_STEM)
        && (tdb_GetFirstActiveChildNode(nodePtr) == NULL))
    {
        return LE_CFG_TYPE_EMPTY;
    }

    return nodePtr->type;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Is the node currently empty?
 */
// -------------------------------------------------------------------------------------------------
bool tdb_IsNodeEmpty
(
    tdb_NodeRef_t nodePtr  ///< The ndoe to read.
)
{
    if (nodePtr == NULL)
    {
        return true;
    }

    if ((nodePtr->flags & NODE_IS_DELETED) != 0)
    {
        return true;
    }

    return tdb_GetTypeId(nodePtr) == LE_CFG_TYPE_EMPTY;
}




// -------------------------------------------------------------------------------------------------
/**
 *  Clear out the contents of hte node.
 */
// -------------------------------------------------------------------------------------------------
void tdb_ClearNode
(
    tdb_NodeRef_t nodePtr  ///< The node to clear.
)
{
    tdb_NodeRef_t currentPtr = FindTailChild(nodePtr);

    while (currentPtr != NULL)
    {
        tdb_NodeRef_t parentPtr = currentPtr->parentPtr;
        tdb_NodeRef_t siblingPtr = currentPtr->siblingPtr;

        ReleaseNode(currentPtr);

        if (currentPtr == nodePtr)
        {
            currentPtr = NULL;
        }
        else if (siblingPtr != NULL)
        {
            currentPtr = FindTailChild(siblingPtr);
        }
        else
        {
            currentPtr = parentPtr;
        }
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Append an update watch callback.
 */
// -------------------------------------------------------------------------------------------------
void tdb_AppendCallback
(
    tdb_NodeRef_t nodeRef,                         ///< The node to watch.
    le_cfg_ChangeHandlerFunc_t updateCallbackPtr,  ///< The function to call on update.
    void* contextPtr                               ///< A context to supply the callback with.
)
{
}
