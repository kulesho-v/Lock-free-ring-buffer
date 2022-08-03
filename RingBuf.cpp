#include "RingBuf.h"
#include <cassert>

void RingBuffer::Init(const std::size_t InitSize)
{
	assert(InitSize && "Error, Init, InitSize == 0!");

	Buffer.resize(InitSize);

	WriteIndex.store(0u);
	ReadIndex.store(0u);

	UsedSpaceNum.store(0);
	FreeSpaceNum.store(static_cast<int>(InitSize));

	Last = InitSize - 1;
}

RingBuffer::RingBuffer(const std::size_t InSize)
{
	Init(InSize);
}

void RingBuffer::Resize(const std::size_t NewSize)
{
	Init(NewSize);
}

inline bool RingBuffer::Empty() const
{
	return FreeSpaceNum.load(std::memory_order_relaxed) == Last + 1;
}

bool RingBuffer::Push(const std::size_t Value)
{
	while (FreeSpaceNum.fetch_sub(1, std::memory_order::memory_order_acq_rel) > 0)
	{
		std::size_t CurWrite = WriteIndex.load(std::memory_order_acquire);
		const std::size_t Next = CurWrite < Last ? CurWrite + 1 : First;

		if (WriteIndex.compare_exchange_strong(CurWrite, Next, std::memory_order_release, std::memory_order_relaxed))
		{
			Buffer[CurWrite] = Value;
			UsedSpaceNum.fetch_add(1, std::memory_order_release);
			return true;
		}
		FreeSpaceNum.fetch_add(1, std::memory_order_relaxed);
	}
	FreeSpaceNum.fetch_add(1, std::memory_order_relaxed);
	return false;
}

bool RingBuffer::Pop(std::size_t& Value)
{
	while (UsedSpaceNum.fetch_sub(1, std::memory_order::memory_order_acq_rel) > 0)
	{
		std::size_t CurRead = ReadIndex.load(std::memory_order_acquire);
		const std::size_t Next = CurRead < Last ? CurRead + 1 : First;

		if (ReadIndex.compare_exchange_strong(CurRead, Next, std::memory_order_release, std::memory_order_relaxed))
		{
			Value = Buffer[CurRead];
			FreeSpaceNum.fetch_add(1, std::memory_order_release);
			return true;
		}
		UsedSpaceNum.fetch_add(1, std::memory_order_relaxed);
	}
	UsedSpaceNum.fetch_add(1, std::memory_order_relaxed);

	return false;
}
