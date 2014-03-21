/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Gilles Cannenterre for Sierra Wireless - initial API and implementation
 *******************************************************************************/

/*
 * test.c
 *
 *  Created on: May 14, 2009
 *      Author: cbugot
 */

#include "smspdu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
// TEST 0: test 7bit <=> 8bits conversions
//  int size;
//  char hello7[] = "E8329BFD4697D9EC37";
//  char hello8[] = "hellohello";
//  char buf[20];
//
//  size = convert_7bit_to_ascii(hello7, 5, 5, buf);
//  buf[size] = 0;
//  printf("7bits size: %d, %s\n", size, buf);
//
//  size = convert_ascii_to_7bit(hello8, 0, 10, buf);
//  printf("8bits size: %d\n", size);
//
//  exit(0);


  // TEST: Test PDU decoding
  //char pdu[] = "07914487200030232408E602710079000490306161131000047071727374";
  //char pdu[] = "07914487200030236408E6027100790004903031415260000F060504080008006162636465666768";
  //char pdu[] = "07917283010010F5040BC87238880900F10000993092516195800AE8329BFD4697D9EC37";

  // Concat SMS: 4 SMS
  //char pdu[] = "07914477581006504408A1027100790004905091312162408C050003010401DEADBEEF31062F1F2DB69181924435354641464639314446443933463538303838333342443643414131454234453034333234343400030B6A00C54601C655018711060357636D4E41504944000187070603496E73674E41504E616D650001871006AB01870806037761702E766F69636573747265616D2E636F6D00018709060341504E0001";
  //char pdu[] = "07914477581006504008A1027100790004905091315155408C050003010402871401C65A01870C06035041500001870D0603757365726E616D650001870E060370776400010101C600015501873606037737000187380603504C54000187070603504C540001870503494E49540001C6560187340603687474703A2F2F3132372E302E302E312F0001C60000530187230603383000010101870503544F2D50524F58590006";
  //char pdu[] = "07914477581006504008A1027100790004905091316163408C050003010403036C6F6750726F78790001C6000157018730068D0187310603504C54000187320603393939390001872F06033930323030000101C6570187300603434C49454E540001873106033432353938333431373732323732360001873306930187320603393939390001872F0603393032303000010101C600005101871506036C6F6750726F787900";
  //char pdu[] = "07914477581006504408A102710079000490509131716140490500030104040187070603426F6F7473747261702050726F78790001C65201872F060370687950726F7879000187200603000187210685018722060357636D4E415049440001010101";

  //char pdu[] = "099188320580200000F0040F9188320500002123F300007080708163148A13D4F29C9E769F4166F9BB0D5286E7F0B21C";
  //char pdu[] = "07913366003000F0040B913366982288F80000903031017462401DF4F71B444FD3D3207A981E06A5D9A076D81DAF9741753788DE00";
  //char pdu[] = "07914477581006500410D0F4BA9C5DA7D7E975000090601171704540087537885EC6D3CB";
  char pdu[] = "07914487200030236408E602710079000490901241440540390605040B840B8455060401C4AF872DE46143016A29441C325FEE961FE50F02F80000009E721357415645434F4D2D52444D532D534552564552";

  char buf[400];
  buf[0] = 0;
  int code;
  SMS* sms;

  code = decode_smspdu(pdu, &sms);
  bintohex(sms->message, sms->message_length, buf, 0);
  buf[sms->message_length*2] = 0;
  printf("retcode %d\n", code);
  printf("SMS:\n\tSender: %s\n\tMessage: length:%d, %s\n", sms->address, sms->message_length, sms->message);
  printf("HEX: %s\n", buf);
  if (sms->concat_maxnb>1)
  {
    printf("CONCAT SMS detected !\n");
    printf("concat_ref:%d concat_maxnb:%d, concat_seqnb:%d\n", sms->concat_ref, sms->concat_maxnb, sms->concat_seqnb);
  }
  if (sms->portbits)
  {
    printf("SMS contains ports\n");
    printf("bits=%d, dst=%d, src=%d\n", sms->portbits, sms->dstport, sms->srcport);
  }


  // TEST 3: test PDU encoding
//  int nb, i;
//  PDU* pdu;
//  char message[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcd0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcd0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcd0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcd0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcd";
//  //char message[] ="hellohello";
//
//  nb = encode_smspdu(message, sizeof(message)-1, "0622468910", &pdu);
//
//  printf("Nb of sms: %d\n", nb);
//  for (i = 0; i < nb; ++i)
//  {
//    printf("PDU %d: size:%d, [%s] [%d]\n", i, pdu[i].size, pdu[i].buffer, strlen(pdu[i].buffer)/2);
//  }



  return 0;
}
