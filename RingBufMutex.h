#pragma once
#include <vector>
#include <mutex>

class RingBufferWithMutex
{
	std::vector<std::size_t> Buffer;

	std::mutex Mutex;

	static const std::size_t First = 0u;
	std::size_t Last = 0u;

	std::size_t WriteIndex = 0u;
	std::size_t ReadIndex = 0u;

	int FreeSpaceNum = 0;
	int UsedSpaceNum = 0;

	void Init(const std::size_t InitSize)
	{
		Last = InitSize - 1;
		FreeSpaceNum = InitSize;
		UsedSpaceNum = 0;
		WriteIndex = ReadIndex = 0u;
		Buffer.resize(InitSize);
	}

public:
	RingBufferWithMutex(std::size_t InSize = 1u)
	{
		Init(InSize);
	}

	void Resize(const std::size_t NewSize)
	{
		Init(NewSize);
	}

	bool Pop(std::size_t& Value)
	{
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			if (FreeSpaceNum < Last + 1)
			{
				Value = Buffer[ReadIndex];
				ReadIndex = ReadIndex < Last ? Last + 1 : First;
				++FreeSpaceNum;
				return true;
			}
		}
		return false;
	}

	bool Push(const std::size_t Value)
	{
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			if (FreeSpaceNum > 0)
			{
				Buffer[WriteIndex] = Value;
				WriteIndex = WriteIndex < Last ? Last + 1 : First;
				--FreeSpaceNum;
				return true;
			}
		}
		return false;
	}

	inline bool IsEmpty() const { return FreeSpaceNum == Last + 1;}

};