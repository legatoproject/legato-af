
// -------------------------------------------------------------------------------------------------
/**
 *  @file Ref.java
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 *
 */
// -------------------------------------------------------------------------------------------------

package io.legato;




// -------------------------------------------------------------------------------------------------
/**
 *  Simple reference type to help support API out parameters.
 */
// -------------------------------------------------------------------------------------------------
public class Ref<RefType>
{
    private RefType value;

    public Ref(RefType newValue)
    {
        value = newValue;
    }

    public RefType getValue()
    {
        return value;
    }

    public void setValue(RefType newValue)
    {
        value = newValue;
    }
}
