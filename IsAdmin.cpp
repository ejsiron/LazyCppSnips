// returns <true, ERROR_SUCCESS> if user is admin
// returns <false, ERROR_SUCCESS> if user is not an admin
// returns <false, WinErrorCode> if something breaks
// i don't remember what the "alternative" section is for, probably something i saw on stackoverflow
std::pair<bool, DWORD> IsAdmin()
{
	try
	{
		HANDLE hCurrentProcess{GetCurrentProcess()};
		HANDLE hProcessToken{0};
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
			throw;

		TOKEN_ELEVATION_TYPE ElevationType;
		DWORD TokenSize;
		if (!GetTokenInformation(hProcessToken, TokenElevationType, &ElevationType, sizeof(ElevationType), &TokenSize))
			throw;

		bool IsAdmin{ElevationType == TOKEN_ELEVATION_TYPE::TokenElevationTypeFull};
		return std::make_pair(IsAdmin, ERROR_SUCCESS);

		// alternative
		//TOKEN_ELEVATION Elevation;
		//if (!GetTokenInformation(hProcessToken, TokenElevation, &Elevation, sizeof(Elevation), &TokenSize))
		//	throw;
	}
	catch (...)
	{
		return std::make_pair(false, GetLastError());
	}
}
