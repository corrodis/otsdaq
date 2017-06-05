universalWrite(0000000100000009,0);
universalWrite(1001,packetCount);
universalRead(1001,data);
universalWrite(1002,400000);
universalRead(1001,data);
universalWrite(0000000100000006,7F000001);
universalWrite(0000000100000008,fa5);
