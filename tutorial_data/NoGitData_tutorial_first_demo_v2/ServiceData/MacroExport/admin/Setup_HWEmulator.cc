//Generated Macro Name: Setup_HWEmulator
//Generated Dec-01-2016 09:03:38
//	Paste this whole file into an interface to transfer Macro functionality.
{

	uint8_t addrs[universalAddressSize_];	//create address buffer of interface size
	uint8_t data[universalDataSize_];		//create data buffer of interface size

	// universalWrite(0x0000000100000009,0x0);
	{
		uint8_t macroAddrs[8] = {0x09, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 8)?macroAddrs[i]:0;

		uint8_t macroData[1] = {0x00};	//create macro data buffer
		for(unsigned int i=0;i<universalDataSize_;++i) //fill with macro address and 0 fill
				data[i] = (i < 1)?macroData[i]:0;
		universalWrite((char *)addrs,(char *)data);
	}

	// universalWrite(0x1001,0xpacketCount);
	{
		uint8_t macroAddrs[2] = {0x01, 0x10};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 2)?macroAddrs[i]:0;

		uint64_t macroData = theXDAQContextConfigTree_.getNode(theConfigurationPath_).getNode(
			"packetCount").getValue<uint64_t>();	//create macro data buffer
		for(unsigned int i=0;i<universalDataSize_;++i) //fill with macro address and 0 fill
				data[i] = (i < 8)?((uint8_t *)(&macroData))[i]:0;
		universalWrite((char *)addrs,(char *)data);
	}

	// universalRead(0x1001,data);
	{
		uint8_t macroAddrs[2] = {0x01, 0x10};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 2)?macroAddrs[i]:0;

		universalRead((char *)addrs,(char *)data);
	}

	// universalWrite(0x1002,0x400000);
	{
		uint8_t macroAddrs[2] = {0x02, 0x10};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 2)?macroAddrs[i]:0;

		uint8_t macroData[3] = {0x00, 0x00, 0x40};	//create macro data buffer
		for(unsigned int i=0;i<universalDataSize_;++i) //fill with macro address and 0 fill
				data[i] = (i < 3)?macroData[i]:0;
		universalWrite((char *)addrs,(char *)data);
	}

	// universalRead(0x1001,data);
	{
		uint8_t macroAddrs[2] = {0x01, 0x10};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 2)?macroAddrs[i]:0;

		universalRead((char *)addrs,(char *)data);
	}

	// universalWrite(0x0000000100000006,0x7F000001);
	{
		uint8_t macroAddrs[8] = {0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 8)?macroAddrs[i]:0;

		uint8_t macroData[4] = {0x01, 0x00, 0x00, 0x7F};	//create macro data buffer
		for(unsigned int i=0;i<universalDataSize_;++i) //fill with macro address and 0 fill
				data[i] = (i < 4)?macroData[i]:0;
		universalWrite((char *)addrs,(char *)data);
	}

	// universalWrite(0x0000000100000008,0xfa5);
	{
		uint8_t macroAddrs[8] = {0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};	//create macro address buffer
		for(unsigned int i=0;i<universalAddressSize_;++i) //fill with macro address and 0 fill
				addrs[i] = (i < 8)?macroAddrs[i]:0;

		uint8_t macroData[2] = {0xa5, 0x0f};	//create macro data buffer
		for(unsigned int i=0;i<universalDataSize_;++i) //fill with macro address and 0 fill
				data[i] = (i < 2)?macroData[i]:0;
		universalWrite((char *)addrs,(char *)data);
	}
}