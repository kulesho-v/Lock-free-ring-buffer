#pragma once
#include <atomic>
#include <vector>

class RingBuffer
{
	std::vector<std::size_t> Buffer;

	static const std::size_t First = 0;
	std::size_t Last = 0;

	std::atomic<std::size_t> WriteIndex = 0;
	std::atomic<std::size_t> ReadIndex = 0;

	// Should be signed
	std::atomic<int> FreeSpaceNum = 0;
	std::atomic<int> UsedSpaceNum = 0;

	void Init(const std::size_t InitSize);

public:
	// Copy and move constructors and assignement operators are deleted by compiler
	// because class contains std::atomic

	explicit RingBuffer(const std::size_t InSize = 1);

	// This is not the part of classic ring-buffer interface
	// this needs only as testing purpose
	void Resize(const std::size_t NewSize);

	inline bool Empty() const;

	bool Push(const std::size_t Value);
	bool Pop(std::size_t& Value);
};