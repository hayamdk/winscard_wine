@ extern g_rgSCardT0Pci
@ extern g_rgSCardT1Pci
@ extern g_rgSCardRawPci
@ stdcall SCardEstablishContext(long str str str)
@ stdcall SCardListReadersA(long str str str)
@ stdcall SCardListReadersW(long str str str)
@ stdcall SCardConnectA(long str long long str str)
@ stdcall SCardConnectW(long str long long str str)
@ stdcall SCardTransmit(long str str long str str str)
@ stdcall SCardDisconnect(long long)
@ stdcall SCardReleaseContext(long)
@ stdcall SCardFreeMemory(long str)
@ stdcall SCardGetStatusChangeA(long long str long)
@ stdcall SCardGetStatusChangeW(long long str long)
@ stdcall SCardIsValidContext(long)
@ stdcall SCardStatusA(long str str str str str str)
@ stdcall SCardStatusW(long str str str str str str)
