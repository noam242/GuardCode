#define TO_REG_DATA(_type, _entry) (_type)((PUCHAR)((PLOG_ENTRY)_entry + 1) + ((PLOG_ENTRY)_entry)->nFrameChainCounts * sizeof(PVOID))

class CRegEvent : public CLogEvent
{
public:
	CString GetPath()
	{
		return TEXT("TODO");
		//PLOG_ENTRY pEntry2 = reinterpret_cast<PLOG_ENTRY>(getPostLog().GetBuffer());
		PLOG_ENTRY pEntry = reinterpret_cast<PLOG_ENTRY>(getPreLog().GetBuffer());
		printf("\nReglen:%s\n", pEntry); 
		printf("\nentrylen:%u\n", (((PLOG_ENTRY)pEntry)->DataLength + (sizeof(PVOID) * ((PLOG_ENTRY)pEntry)->nFrameChainCounts) + sizeof(LOG_ENTRY)));
		PLOG_REG_OPT pFileOpt = TO_EVENT_DATA(PLOG_REG_OPT, pEntry);
		//PLOG_REG_OPT pFileOpt2 = TO_REG_DATA(PLOG_REG_OPT, pEntry2);
		printf("\nRegPath:%ws\n", pFileOpt->Name);
		//printf("\nReglen:%lu\n", pFileOpt->NameLength);
		//printf("\nRegPath:%ws\n", pFileOpt2->Name);
		CString strFileName;
		strFileName.Append(pFileOpt->Name, pFileOpt->NameLength);

		return strFileName;
	}
	CString GetDetail()
	{
		return TEXT("TODO");
	}
};