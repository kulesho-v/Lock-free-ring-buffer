#pragma once
#include "RingBufTest.h"
#include <assert.h>
#include <thread>
#include <iostream>
#include <algorithm>
#include <chrono>

#define PRINT 1

template<typename T>
static bool IsUnique(std::vector<T>& X)
{
	std::sort(X.begin(), X.end());
	return std::unique(X.begin(), X.end()) == X.end();
}

void TestRingBuffer::ManualRealTimeTest()
{
	while (UserCommand)
	{
		UIClear();
		PrintWelcomeMessage();
		CreateAndRunThreads();
		PrintConsumersStorage();
		AskUserToEnterCommand();
	}
}

void TestRingBuffer::MultipleProducersSingleConsumerTest()
{
	std::unordered_map<std::size_t, std::vector<long long>> TimePerRunPerTryInMs;
	const std::size_t TriesPerTest = 10u;

	for (std::size_t Producers = 2u; Producers <= MaxThreads; Producers *= 2u)
	{
		auto& CurrentRun = TimePerRunPerTryInMs[Producers] = std::vector<long long>(TriesPerTest);

		for (std::size_t Tries = 0u; Tries < TriesPerTest; Tries++)
			CurrentRun.at(Tries) = AutoTest(Producers, 1u);
	}

	PrintTimePerRun(TimePerRunPerTryInMs);
}

void TestRingBuffer::SingleProducerMultipleConsumerTest()
{
	std::unordered_map<std::size_t, std::vector<long long>> TimePerRunPerTryInMs;
	const std::size_t TriesPerTest = 10u;

	for (std::size_t Consumers = 2u; Consumers <= MaxThreads; Consumers *= 2u)
	{
		auto& CurrentRun = TimePerRunPerTryInMs[Consumers] = std::vector<long long>(TriesPerTest);

		for (std::size_t Tries = 0u; Tries < TriesPerTest; Tries++)
			CurrentRun.at(Tries) = AutoTest(1u, Consumers);
	}

	PrintTimePerRun(TimePerRunPerTryInMs);
}

void TestRingBuffer::MultipleProducersMultipleConsumersTest()
{
	std::unordered_map<std::size_t, std::unordered_map<std::size_t, std::vector<long long>>> TimePerRunPerTryInMs;

	for (std::size_t Producers = 2u; Producers <= MaxThreads/2; Producers *= 2u)
	{
		const std::size_t TriesPerTest = 10u;
		auto& TimePerProducer = TimePerRunPerTryInMs[Producers] = {};

		for (std::size_t Consumers = 2u; Consumers <= MaxThreads/2; Consumers *= 2u)
		{
			auto& TimePerConsumer = TimePerProducer[Consumers] = std::vector<long long>(TriesPerTest) ;

			for (std::size_t Tries = 0u; Tries < TriesPerTest; Tries++)
				TimePerConsumer.at(Tries) = AutoTest(Producers, Consumers);
		}
	}

	for (std::size_t Producers = 2u; Producers <= MaxThreads / 2; Producers *= 2u)
	{
		auto& TimePerProducer = TimePerRunPerTryInMs[Producers];

		std::cout << "Producers: " << Producers << " with all consumers (2,4,8,16,32,64).\n";
		PrintTimePerRun(TimePerProducer, MaxThreads / 2);
		std::cout << "\n\n";

	}
}

long long TestRingBuffer::AutoTest(const std::size_t Producers, const std::size_t Consumers, const std::size_t GenerateDataSize)
{
	Clear();

	RingBufferSize = 1u;
	ProducersNum = Producers;
	ConsumerNum = Consumers;
	TotalGeneratedDataSize = GenerateDataSize;

	return CreateAndRunThreads();
}

void TestRingBuffer::Producer(const std::size_t FirstNumInSeq, const std::size_t SeqSize)
{
	const std::size_t  MaxTriesAllowed = 100u;
	std::size_t  TriesCount = 0u;

	ThreadWaitForStartFlag();

	for (std::size_t i = FirstNumInSeq; i < SeqSize;)
	{
		if (TriesCount > MaxTriesAllowed)
		{
			TriesCount = 0;
			std::this_thread::yield();
		}
		else if (RingBuf.Push(i))
		{
			TriesCount = 0;
			++i;
		}
		else
		{
			++TriesCount;
		}
	}
}

void TestRingBuffer::Consumer(const std::size_t StorageIndex, const std::size_t StorageSize)
{
	std::vector<size_t>& ConsumerStorage = ConsmersStorage.at(StorageIndex);
	ConsumerStorage.resize(StorageSize);

	const std::size_t  MaxTriesAllowed = 100u;
	std::size_t  TriesCount = 0u;

	ThreadWaitForStartFlag();

	for (unsigned i = 0; i < ConsumerStorage.size();)
	{
		if (TriesCount > MaxTriesAllowed)
		{
			TriesCount = 0;
			std::this_thread::yield();
		}
		else if (RingBuf.Pop(ConsumerStorage[i]))
		{
			TriesCount = 0;
			++i;
		}
		else
		{
			++TriesCount;
		}
	}
}

long long TestRingBuffer::CreateAndRunThreads()
{
	RingBuf.Resize(RingBufferSize);
	ConsmersStorage.resize(ConsumerNum);

	std::size_t DataSizeGeneratedByProducer = TotalGeneratedDataSize / ProducersNum;
	std::size_t ConsumerStorageSize = TotalGeneratedDataSize / ConsumerNum;
	std::size_t TotalThreads = ProducersNum + ConsumerNum;

	// Create threads
	for (unsigned TotalIdx = 0, ProducerIdx = 0, ConsumerIdx = 0; TotalIdx < TotalThreads; ++TotalIdx, ++ProducerIdx, ++ConsumerIdx)
	{
		if (ProducerIdx == ProducersNum - 1)
			Threads.push_back(std::thread(&TestRingBuffer::Producer, this, ProducerIdx * DataSizeGeneratedByProducer, TotalGeneratedDataSize));
		else if (ProducerIdx < ProducersNum)
			Threads.push_back(std::thread(&TestRingBuffer::Producer, this, ProducerIdx * DataSizeGeneratedByProducer, ProducerIdx * DataSizeGeneratedByProducer + DataSizeGeneratedByProducer));

		if (ConsumerIdx == ConsumerNum - 1)
			Threads.push_back(std::thread(&TestRingBuffer::Consumer, this, ConsumerIdx, TotalGeneratedDataSize - ConsumerStorageSize * ConsumerIdx));
		else if (ConsumerIdx < ConsumerNum)
			Threads.push_back(std::thread(&TestRingBuffer::Consumer, this, ConsumerIdx, ConsumerStorageSize));
	}

	const auto StartTime = std::chrono::steady_clock::now();

	// Run all threads
	Start = true;

	for (auto& Thread : Threads)
		if (Thread.joinable())
			Thread.join();

	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - StartTime).count();
}

void TestRingBuffer::UIClear()
{
	Clear();
	UserCommand = 1u;
	system("cls");
}

void TestRingBuffer::AskUserToEnterCommand()
{
	do
	{
		std::cout << "Available commands: \n 1 - run another test \n 0 - exit \n Enter command: ";
		std::cin >> UserCommand;

		if ((UserCommand > 1))
		{
			std::cout << "You enter wrong command: " << UserCommand << ", please try again.";
		}
		else break;

	} while (true);
}

void TestRingBuffer::Clear()
{
	Start = false;

	ProducersNum = 1u;
	ConsumerNum = 1u;

	TotalGeneratedDataSize = 0u;

	Threads.clear();
	ConsmersStorage.clear();

	RingBufferSize = 1u;
	RingBuf.Resize(RingBufferSize);
}

void TestRingBuffer::PrintWelcomeMessage()
{
	std::cout << "Hello there, this program allow you to test lock-free ring buffer :)" << std::endl;

	std::cout << "Enter ring buffer size: ";
	std::cin >> RingBufferSize;

	std::cout << "Enter the amount of producer threads: ";
	std::cin >> ProducersNum;
	assert(ProducersNum && "Error, amount of producers should be at least 1.");

	std::cout << "Enter the amount of consumer threads: ";
	std::cin >> ConsumerNum;
	assert(ConsumerNum && "Error, amount of consumers should be at least 1.");

	std::cout << "Enter the generated (by producers) data size: ";
	std::cin >> TotalGeneratedDataSize;
}

void TestRingBuffer::PrintTimePerRun(const std::unordered_map<std::size_t, std::vector<long long>>& Runs, std::size_t MaxThreads)
{
	for (std::size_t Threads = 2u; Threads <= MaxThreads; Threads *= 2)
	{
		std::cout << "Tries with " << Threads << " threads\n";

		for (const auto Value : Runs.at(Threads))
		{
			std::cout << Value << ", ";
		}

		std::cout << "\n\n";
	}
}

void TestRingBuffer::PrintConsumersStorage()
{
	std::size_t ConsumerIndex = 0;
	for (std::vector<std::size_t>& ConsumerBuf : ConsmersStorage)
	{
#ifdef PRINT
		std::cout << "\n\nConsumer Buffer #" << ConsumerIndex++ << "\n";
		for (unsigned i = 0; i < ConsumerBuf.size(); ++i)
		{
			if (!(i % 10)) std::cout << "\n";
			std::cout << ConsumerBuf[i] << ',';
		}
		std::cout << "\n\n";
#endif
		std::cout << "Does the consumer buffer contain only unique items? " << IsUnique(ConsumerBuf) << std::endl;
	}
}