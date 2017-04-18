/*
 * Copyright (C) Sierra Wireless Inc.
 */

package io.legato.test;

import java.util.logging.Logger;

import io.legato.Ref;
import io.legato.Component;
import io.legato.api.ipcTest;

public class JavaServer implements ipcTest, Component
{
    @Override
    public void EchoSimple
    (
        int InValue,
        Ref<Integer> OutValue
    )
    {
        if (OutValue != null)
        {
            OutValue.setValue(new Integer(InValue));
        }
    }

    @Override
    public void EchoSmallEnum
    (
        ipcTest.SmallEnum InValue,
        Ref<ipcTest.SmallEnum> OutValue
    )
    {
        if (OutValue != null)
        {
            OutValue.setValue(InValue);
        }
    }

    // @Override
    // public void EchoLargeEnum
    // (
    //     ipcTest.LargeEnum InValue
    //     Ref<ipcTest.LargeEnum> OutValue
    // )
    // {
    //     if (OutValue != null)
    //     {
    //         OutValue.setValue(InValue);
    //     }
    // }

    @Override
    public void EchoSmallBitMask
    (
        ipcTest.SmallBitMask InValue,
        Ref<ipcTest.SmallBitMask> OutValue
    )
    {
        if (OutValue != null)
        {
            OutValue.setValue(new ipcTest.SmallBitMask(InValue.getValue()));
        }
    }

    // @Override
    // public void EchoLargeBitMask
    // (
    //     ipcTest.LargeBitMask InValue,
    //     Ref<ipcTest.LargeBitMask> OutValue
    // )
    // {
    //     if (OutValue != null)
    //     {
    //         OutValue.setValue(new ipcTest.LargeBitMask(InValue.getValue()));
    //     }
    // }

    @Override
    public void EchoReference
    (
        ipcTest.SimpleRef InValue,
        Ref<ipcTest.SimpleRef> OutValue
    )
    {
        if (OutValue != null)
        {
            OutValue.setValue(InValue);
        }
    }

    @Override
    public void EchoString
    (
        String InString,
        Ref<String> OutString
    )
    {
        if (OutString != null)
        {
            OutString.setValue(new String(InString));
        }
    }

    @Override
    public void componentInit()
    {
    }

    @Override
    public void setLogger(Logger logger)
    {
    }
}
