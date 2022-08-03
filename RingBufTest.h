#pragma once
#include <vector>
#include <unordered_map>
#include "RingBuf.h"
#include "RingBufMutex.h"

class TestRingBuffer
{
	std::vector<std::vector<std::size_t>> ConsmersStorage;
	std::vector<std::thread> Threads;

	RingBufferWithMutex RingBuf;
	std::size_t RingBufferSize = 0u;

	std::size_t ProducersNum = 1u;
	std::size_t ConsumerNum = 1u;

	std::size_t TotalGeneratedDataSize = 0u;
	std::size_t UserCommand = 1u;

	const std::size_t MaxThreads = 128u;

	// variable to start all thread in once
	std::atomic<bool> Start = false;

public:
	void ManualRealTimeTest();
	void MultipleProducersSingleConsumerTest();
	void SingleProducerMultipleConsumerTest();
	void MultipleProducersMultipleConsumersTest();

private:
	long long AutoTest(const std::size_t Producers, const std::size_t  Consumers, const std::size_t GenerateDataSize = 1'000'00u);

	// Thread functions
	void Producer(const std::size_t  FirstNumInSeq, const std::size_t  SeqSize);
	void Consumer(const std::size_t StorageIndex, const std::size_t StorageSize);

	inline void ThreadWaitForStartFlag()
	{
		while (!Start);
	}
	long long CreateAndRunThreads();

// Main thread helper functions
	void UIClear();
	void Clear();

	void PrintWelcomeMessage();
	void PrintTimePerRun(const std::unordered_map<std::size_t, std::vector<long long>>& Runs, std::size_t MaxThreads = 128);
	void PrintConsumersStorage();
	void AskUserToEnterCommand();
};