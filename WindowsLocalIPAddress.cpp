#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

#pragma comment(lib, "iphlpapi.lib") // *UnicastIpAddress* and *Mib* functions
#pragma comment (lib, "Ws2_32.lib")	// INetNtop function

constexpr size_t MaxAddressTextLength{ 46 };

static std::mutex consolemutex{};
static std::mutex tablemutex{};
static std::multimap<IF_INDEX, std::wstring> InterfaceAddresses{};

void ModifyAddressTable(const PMIB_UNICASTIPADDRESS_TABLE pAddressTable, const bool Delete = false)
{
	wchar_t szAddressText[46]{ 0 };
	std::wstring AddressText{};
	std::lock_guard TableGuard(tablemutex);
	std::lock_guard ConsoleGuard(consolemutex);
	NET_IFINDEX CurrentIndex{ 0 };
	for (unsigned long i{ 0 }; i < pAddressTable->NumEntries; ++i)
	{
		CurrentIndex =  pAddressTable->Table[i].InterfaceIndex;
		switch (pAddressTable->Table[i].Address.si_family)
		{
		case AF_INET:
			InetNtop(AF_INET, &pAddressTable->Table[i].Address.Ipv4.sin_addr, szAddressText, MaxAddressTextLength);
			break;
		case AF_INET6:
			InetNtop(AF_INET6, &pAddressTable->Table[i].Address.Ipv6.sin6_addr, szAddressText, MaxAddressTextLength);
			break;
		default:
			std::wcout << L"Unknown address format" << std::endl;
			continue;
		}
		AddressText.assign(szAddressText);
		if (Delete)
		{
			for (auto loc{ InterfaceAddresses.lower_bound(CurrentIndex) };
				loc != InterfaceAddresses.upper_bound(CurrentIndex);)
				// duplicates should never happen, but this table is small enough to make certainty affordable
			{
				if (loc->second == AddressText)
				{
					loc = InterfaceAddresses.erase(loc);
					std::wcout << L"Erased " << AddressText << L" from index " << CurrentIndex << std::endl;
				}
				else
				{
					++loc;
				}
			}
		}
		else
		{	// because this is the only insertion point for the multimap, no need to get creative with duplicate prevention
			// practically, duplicates should never happen anyway
			auto loc{ std::find_if(InterfaceAddresses.lower_bound(CurrentIndex), InterfaceAddresses.end(),
				[AddressText](std::pair<const IF_INDEX, std::wstring>& Entry) { return Entry.second == AddressText; }) };
			if (loc == InterfaceAddresses.end())
			{
				InterfaceAddresses.insert({ pAddressTable->Table[i].InterfaceIndex, std::move(std::wstring{ AddressText }) });
				std::wcout << L"Added " << AddressText << L" to index " << CurrentIndex << std::endl;
			}
			else
			{
				std::wcout << L"Did not add duplicate " << AddressText << L" for index " << CurrentIndex << std::endl;
			}
		}
	}
}

void LoadAllIPAddresses()
{
	PMIB_UNICASTIPADDRESS_TABLE pAddressTable{ nullptr };
	auto Result = GetUnicastIpAddressTable(AF_UNSPEC, &pAddressTable);
	if (Result == NO_ERROR)
	{
		ModifyAddressTable(pAddressTable);
		FreeMibTable(pAddressTable);
	}
}

VOID WINAPI IpAddressChanged(PVOID pCallerContext, PMIB_UNICASTIPADDRESS_ROW pChangedIPRow, MIB_NOTIFICATION_TYPE NotificationType)
{
	MIB_UNICASTIPADDRESS_TABLE EntryTable;	// do not run FreeMibTable() on this
	bool DeleteFlag{ false };
	std::wstring ActivityMessage;
	switch (NotificationType)
	{
	case MIB_NOTIFICATION_TYPE::MibDeleteInstance:
		DeleteFlag = true;
		[[fallthrough]];
	case MIB_NOTIFICATION_TYPE::MibAddInstance:
		EntryTable = { 1, *pChangedIPRow };
		ModifyAddressTable(&EntryTable, DeleteFlag);
		ActivityMessage.assign(L"Processed change notification");
		break;
	case MIB_NOTIFICATION_TYPE::MibInitialNotification:
		ActivityMessage.assign(L"Received initial notification");
		break;
	case MIB_NOTIFICATION_TYPE::MibParameterNotification:
		ActivityMessage.assign(L"Ignored parameter update");
		break;
	default:
		ActivityMessage.assign(L"Unknown notification");
		break;
	}
	std::lock_guard ConsoleGuard(consolemutex);
	std::wcout << ActivityMessage;
	if (pChangedIPRow != nullptr) std::wcout << L" for adapter index " << pChangedIPRow->InterfaceIndex;
	std::wcout << std::endl;
}

int main()
{
	LoadAllIPAddresses();
	std::wstring LocalRoutineContext{ L"LocalContext" };
	std::wcout << L"Registered context ID: " << LocalRoutineContext << std::endl;
	HANDLE AddressChangedHandle{};
	auto RegistrationResult{ NotifyUnicastIpAddressChange(AF_UNSPEC, IpAddressChanged, &LocalRoutineContext, TRUE, &AddressChangedHandle) };
	consolemutex.lock();
	std::wcout << std::endl << L"Press [A] to see address table. Press [Q] to end." << std::endl << std::endl;
	consolemutex.unlock();
	wchar_t KeyChar{ 0 };
	do
	{
		KeyChar = _getwch();
		if (KeyChar == L'a' || KeyChar == L'A')
		{
			std::lock_guard TableGuard(tablemutex);
			std::lock_guard ConsoleGuard(consolemutex);
			std::wcout << L"Index\t| Address" << std::endl;
			std::wcout << L"-----\t  -------" << std::endl;
			for (const auto& AddressEntry : InterfaceAddresses)
			{
				std::wcout << AddressEntry.first << L"\t| " << AddressEntry.second << std::endl;
			}
			std::wcout << std::endl;
		}
	} while (KeyChar != L'q' && KeyChar != L'Q');
	if (AddressChangedHandle)
	{
		CancelMibChangeNotify2(AddressChangedHandle);
		std::wcout << L"Deregistered listener";
	}
	else
	{
		std::wcout << L"Windows says that it has no notification handle to unregister" << std::endl;
	}
	return 0;
}
